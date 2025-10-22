/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/settings/settings.h>

#include <dk_buttons_and_leds.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

#if !IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED)
#include <nfc_t2t_lib.h>
#endif /* !IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED) */
#include <nfc/ndef/msg.h>
#include <nfc/ndef/record.h>
#include <nfc/ndef/ch.h>
#include <nfc/ndef/ch_msg.h>
#include <nfc/ndef/le_oob_rec.h>

#include <helpers/nrfx_reset_reason.h>

#include "pwr_service.h"

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED       DK_LED1
#define CON_STATUS_LED       DK_LED2
#define NFC_FIELD_STATUS_LED DK_LED3

#define CONNECTABLE_ADV_BUTTON     DK_BTN1_MSK
#define NON_CONNECTABLE_ADV_BUTTON DK_BTN2_MSK

#define RUN_LED_BLINK_INTERVAL 1000
#define SYSTEM_OFF_DELAY       5

#define NOTIFICATION_INTERVAL       CONFIG_BT_POWER_PROFILING_NOTIFICATION_INTERVAL
#define CONNECTABLE_ADV_TIMEOUT     CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_DURATION
#define NON_CONNECTABLE_ADV_TIMEOUT CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_DURATION
#define NFC_ADV_TIMEOUT             CONFIG_BT_POWER_PROFILING_NFC_ADV_DURATION
#define NOTIFICATION_TIMEOUT        CONFIG_BT_POWER_PROFILING_NOTIFICATION_TIMEOUT

#define CONNECTABLE_ADV_INTERVAL_MIN     CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_INTERVAL_MIN
#define CONNECTABLE_ADV_INTERVAL_MAX     CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_INTERVAL_MAX
#define NON_CONNECTABLE_ADV_INTERVAL_MIN CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_INTERVAL_MIN
#define NON_CONNECTABLE_ADV_INTERVAL_MAX CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_INTERVAL_MAX

#define NOTIFICATION_PIPELINE_SIZE CONFIG_BT_CONN_TX_MAX

#define NFC_BUFFER_SIZE 1024


static struct bt_le_oob oob_local;
static uint8_t tk_local[NFC_NDEF_LE_OOB_REC_TK_LEN];
#if !IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED)
static uint8_t nfc_buffer[NFC_BUFFER_SIZE];
static void adv_work_handler(struct k_work *work);
static K_WORK_DEFINE(adv_work, adv_work_handler);
#endif /* !IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED) */

static struct bt_le_ext_adv *adv_set;

static void system_off_work_handler(struct k_work *work);
static void key_generation_work_handler(struct k_work *work);
static void notify_work_handler(struct k_work *work);
static void notify_timeout_handler(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(system_off_work, system_off_work_handler);
static K_WORK_DEFINE(key_generate_work, key_generation_work_handler);
static K_WORK_DELAYABLE_DEFINE(notify_work, notify_work_handler);
static K_WORK_DELAYABLE_DEFINE(notify_timeout, notify_timeout_handler);

static atomic_t notify_pipeline = ATOMIC_INIT(NOTIFICATION_PIPELINE_SIZE);
static struct bt_conn *device_conn;

static const struct bt_data connectable_ad_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_data non_connectable_ad_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_URI, /* The URI of the https://www.nordicsemi.com website */
		      0x17, /* UTF-8 code point for “https:” */
		      '/', '/', 'w', 'w', 'w', '.',
		      'n', 'o', 'r', 'd', 'i', 'c', 's', 'e', 'm', 'i', '.',
		      'c', 'o', 'm'),
};

static const struct bt_data non_connectable_sd_data[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_le_adv_param *connectable_ad_params =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN,
			CONNECTABLE_ADV_INTERVAL_MIN,
			CONNECTABLE_ADV_INTERVAL_MAX,
			NULL);

static const struct bt_le_adv_param *non_connectable_ad_params =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_NONE,
			NON_CONNECTABLE_ADV_INTERVAL_MIN,
			NON_CONNECTABLE_ADV_INTERVAL_MAX,
			NULL);

static int leds_init(void)
{
	if (IS_ENABLED(CONFIG_BT_POWER_PROFILING_LED_DISABLED)) {
		return 0;
	} else {
		return dk_leds_init();
	}
}

static int set_led(uint8_t led_idx, uint32_t val)
{
	if (IS_ENABLED(CONFIG_BT_POWER_PROFILING_LED_DISABLED)) {
		ARG_UNUSED(led_idx);
		ARG_UNUSED(val);
		return 0;
	} else {
		return dk_set_led(led_idx, val);
	}
}

static int set_led_on(uint8_t led_idx)
{
	if (IS_ENABLED(CONFIG_BT_POWER_PROFILING_LED_DISABLED)) {
		ARG_UNUSED(led_idx);
		return 0;
	} else {
		return dk_set_led_on(led_idx);
	}
}

static int set_led_off(uint8_t led_idx)
{
	if (IS_ENABLED(CONFIG_BT_POWER_PROFILING_LED_DISABLED)) {
		ARG_UNUSED(led_idx);
		return 0;
	} else {
		return dk_set_led_off(led_idx);
	}
}

#if !IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED)
static void nfc_callback(void *context, nfc_t2t_event_t event, const uint8_t *data,
			 size_t data_length)
{
	int err;
	static bool adv_permission;

	switch (event) {
	case NFC_T2T_EVENT_FIELD_ON:
		/* Try to cancel system off. */
		err = k_work_cancel_delayable(&system_off_work);
		if (err) {
			/* Action will be continued on system on. */
			return;
		}

		set_led_on(NFC_FIELD_STATUS_LED);
		adv_permission = true;
		break;

	case NFC_T2T_EVENT_FIELD_OFF:
		set_led_off(NFC_FIELD_STATUS_LED);
		break;

	case NFC_T2T_EVENT_DATA_READ:
		if (adv_permission) {
			k_work_submit(&adv_work);
			adv_permission = false;
		}

		break;

	default:
		break;
	}
}
#endif /* !IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED) */

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn_err) {
		printk("Connection failed, err 0x%02x %s\n", conn_err, bt_hci_err_to_str(conn_err));
		return;
	}

	device_conn = conn;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected %s\n", addr);

	set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));

	set_led_off(CON_STATUS_LED);

	atomic_set(&notify_pipeline, NOTIFICATION_PIPELINE_SIZE);

	/* Called from system workqueue context. */
	k_work_cancel_delayable(&notify_work);

	device_conn = NULL;

	/* If there is not any connection, schedule system off. */
	k_work_schedule(&system_off_work, K_SECONDS(1));
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d %s\n", addr, level, err,
		       bt_security_err_to_str(err));
	}
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
			     uint16_t timeout)
{
	printk("Connection parameter updated\n"
	       "Connection interval: 1.25 * %u ms, latency: %u, timeout: 10 * %u ms\n",
	       interval, latency, timeout);
}

static void le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
	printk("PHY updated, TX: %u RX: %u\n", param->tx_phy, param->rx_phy);
}

BT_CONN_CB_DEFINE(connection_cb) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
	.le_param_updated = le_param_updated,
	.le_phy_updated = le_phy_updated
};

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static void legacy_tk_value_set(struct bt_conn *conn)
{
	int err;

	err = bt_le_oob_set_legacy_tk(conn, tk_local);
	if (err) {
		printk("Failed to set local TK (err %d)\n", err);
	}
}

static void lesc_oob_data_set(struct bt_conn *conn, struct bt_conn_oob_info *info)
{
	int err;
	struct bt_conn_info conn_info;
	char addr[BT_ADDR_LE_STR_LEN];

	if (info->lesc.oob_config != BT_CONN_OOB_LOCAL_ONLY) {
		printk("Unsupported OOB request\n");
		return;
	}

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		printk("Failed to get conn info (err %d)\n", err);
		return;
	}

	if (bt_addr_le_cmp(conn_info.le.local, &oob_local.addr) != 0) {
		bt_addr_le_to_str(conn_info.le.local, addr, sizeof(addr));
		printk("No OOB data available for local %s", addr);
		bt_conn_auth_cancel(conn);
		return;
	}

	err = bt_le_oob_set_sc_data(conn, &oob_local.le_sc_data, NULL);
	if (err) {
		printk("Failed to set OOB SC local data (err %d)\n", err);
	}
}

static void oob_data_request(struct bt_conn *conn, struct bt_conn_oob_info *info)
{
	if (info->type == BT_CONN_OOB_LE_SC) {
		printk("LESC OOB data requested\n");
		lesc_oob_data_set(conn, info);
	}

	if (info->type == BT_CONN_OOB_LE_LEGACY) {
		printk("Legacy TK value requested\n");
		legacy_tk_value_set(conn);
	}
}

static const struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
	.oob_data_request = oob_data_request
};

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);

	/* Generate new pairing keys if pairing procedure succeed. */
	k_work_submit(&key_generate_work);
}

static void pwr_notify_sent_cb(struct bt_conn *conn, void *user_data)
{
	atomic_inc(&notify_pipeline);
}

static void pwr_notify_status_cb(enum pwr_service_status status, void *user_data)
{
	switch (status) {
	case PWR_SERVICE_STATUS_NOTIFY_ENABLED:
		k_work_schedule(&notify_timeout, K_MSEC(NOTIFICATION_TIMEOUT));
		k_work_schedule(&notify_work, K_NO_WAIT);

		printk("Notification enabled\n");

		break;

	case PWR_SERVICE_STATUS_NOTIFY_DISABLED:
		/* Called from cooperative thread context. */
		k_work_cancel_delayable(&notify_work);
		k_work_cancel_delayable(&notify_timeout);

		printk("Notification disabled\n");

		break;

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
		break;
	}
}

static const struct pwr_service_cb pwr_service_callback = {
	.sent_cb = pwr_notify_sent_cb,
	.notify_status_cb = pwr_notify_status_cb,
};

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed conn: %s, reason %d %s\n", addr, reason,
	       bt_security_err_to_str(reason));

	/* Generate new pairing keys if pairing procedure failed. */
	k_work_submit(&key_generate_work);
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	int err;
	uint32_t buttons = button_state & has_changed;

	if (buttons & CONNECTABLE_ADV_BUTTON) {
		struct bt_le_ext_adv_start_param connectable_start_param = {
			.timeout = CONNECTABLE_ADV_TIMEOUT,
			.num_events = 0
		};

		/* It is called from workqueue context, checking return value is not needed. */
		k_work_cancel_delayable(&system_off_work);

		(void)bt_le_ext_adv_stop(adv_set);
		err = bt_le_ext_adv_update_param(adv_set, connectable_ad_params);
		if (err) {
			printk("Failed to set connectable advertising data (err %d)\n", err);
			return;
		}

		err = bt_le_ext_adv_set_data(adv_set, connectable_ad_data,
				     ARRAY_SIZE(connectable_ad_data), NULL, 0);
		if (err) {
			printk("Failed to set data for connectable advertising (err %d)\n", err);
			return;
		}

		err = bt_le_ext_adv_start(adv_set, &connectable_start_param);
		if (err) {
			printk("Connectable advertising failed to start (err %d)\n", err);
		} else {
			printk("Connectable advertising started\n");
		}

		return;
	}

	if (buttons & NON_CONNECTABLE_ADV_BUTTON) {
		struct bt_le_ext_adv_start_param non_connectable_start_param = {
			.timeout = NON_CONNECTABLE_ADV_TIMEOUT,
			.num_events = 0
		};

		/* It is called from workqueue context, checking return value is not needed. */
		k_work_cancel_delayable(&system_off_work);

		(void)bt_le_ext_adv_stop(adv_set);
		err = bt_le_ext_adv_update_param(adv_set, non_connectable_ad_params);
		if (err) {
			printk("Failed to set non-connectable advertising data (err %d)\n", err);
			return;
		}

		err = bt_le_ext_adv_set_data(adv_set, non_connectable_ad_data,
					     ARRAY_SIZE(non_connectable_ad_data),
					     non_connectable_sd_data,
					     ARRAY_SIZE(non_connectable_sd_data));
		if (err) {
			printk("Failed to set data for non-connectable advertising (err %d)\n",
			       err);
			return;
		}

		err = bt_le_ext_adv_start(adv_set, &non_connectable_start_param);
		if (err) {
			printk("Non-connectable advertising failed to start (err %d)\n", err);
		} else {
			printk("Non-connectable advertising started\n");
		}

		return;
	}
}

static int pairing_key_generate(void)
{
	int err;

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob_local);
	if (err) {
		printk("Failed to get local oob data (err %d)\n", err);
		return err;
	}

	err = bt_rand(tk_local, sizeof(tk_local));
	if (err) {
		printk("Failed to generate random TK value (err %d)\n", err);
	}

	return err;
}

static void key_generation_work_handler(struct k_work *work)
{
	pairing_key_generate();
}

#if !IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED)
static void adv_work_handler(struct k_work *work)
{
	int err;
	struct bt_le_ext_adv_start_param param = {
		.timeout = NFC_ADV_TIMEOUT,
		.num_events = 0
	};

	/* Called from workqueue context. */
	k_work_cancel_delayable(&system_off_work);

	(void)bt_le_ext_adv_stop(adv_set);
	err = bt_le_ext_adv_update_param(adv_set, connectable_ad_params);
	if (err) {
		printk("Failed to set connectable advertising data (err %d)\n", err);
		return;
	}

	err = bt_le_ext_adv_set_data(adv_set, connectable_ad_data,
				     ARRAY_SIZE(connectable_ad_data), NULL, 0);
	if (err) {
		printk("Failed to set data for connectable advertising (err %d)\n", err);
		return;
	}

	err = bt_le_ext_adv_start(adv_set, &param);
	if (err) {
		printk("Connectable advertising failed to start (err %d)\n", err);
	} else {
		printk("Connectable advertising started\n");
	}
}
#endif /* !IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED) */

static void notify_work_handler(struct k_work *work)
{
	int err;
	atomic_val_t pipeline = atomic_get(&notify_pipeline);

	if (pipeline == 0) {
		/* There is no place for notification in Bluetooth stack, skip sending notification,
		 * and schedule the new interval.
		 */
		printk("Notification cannot be passed to Bluetooth stack\n");
		k_work_reschedule(k_work_delayable_from_work(work), K_MSEC(NOTIFICATION_INTERVAL));
		return;
	}

	err = pwr_service_notify(NULL);
	if (err) {
		printk("Failed to send PWR service notification (err %d)\n", err);
	} else {
		atomic_dec(&notify_pipeline);
	}

	k_work_reschedule(k_work_delayable_from_work(work), K_MSEC(NOTIFICATION_INTERVAL));
}

static void notify_timeout_handler(struct k_work *work)
{
	int err;

	if (device_conn) {
		k_work_cancel_delayable(&notify_work);

		printk("Notification sending ended. Disconnecting...\n");

		err = bt_conn_disconnect(device_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			printk("Failed to disconnect Bluetooth link (err %d)\n", err);
		}
	}
}

#if !IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED)
static int nfc_oob_data_setup(size_t *size)
{
	static const uint8_t ndef_record_count = 2;
	static const uint8_t ch_record_count = 1;

	int err;
	struct nfc_ndef_le_oob_rec_payload_desc oob_rec_payload;
	struct nfc_ndef_ch_msg_records ch_msg_records;

	NFC_NDEF_MSG_DEF(ndef_msg, ndef_record_count);

	NFC_NDEF_LE_OOB_RECORD_DESC_DEF(oob_rec, '0', &oob_rec_payload);
	NFC_NDEF_CH_AC_RECORD_DESC_DEF(ac_rec, NFC_AC_CPS_ACTIVE, 1, "0", 0);
	NFC_NDEF_CH_HS_RECORD_DESC_DEF(hs_record, NFC_NDEF_CH_MSG_MAJOR_VER,
				       NFC_NDEF_CH_MSG_MINOR_VER, ch_record_count);

	memset(&oob_rec_payload, 0, sizeof(oob_rec_payload));

	oob_rec_payload.addr = &oob_local.addr;
	oob_rec_payload.appearance = NFC_NDEF_LE_OOB_REC_APPEARANCE(bt_get_appearance());
	oob_rec_payload.flags = NFC_NDEF_LE_OOB_REC_FLAGS(BT_LE_AD_NO_BREDR);
	oob_rec_payload.le_role =
			NFC_NDEF_LE_OOB_REC_LE_ROLE(NFC_NDEF_LE_OOB_REC_LE_ROLE_PERIPH_ONLY);
	oob_rec_payload.le_sc_data = &oob_local.le_sc_data;
	oob_rec_payload.local_name = bt_get_name();
	oob_rec_payload.tk_value = tk_local;

	ch_msg_records.ac = &NFC_NDEF_CH_AC_RECORD_DESC(ac_rec);
	ch_msg_records.carrier = &NFC_NDEF_LE_OOB_RECORD_DESC(oob_rec);
	ch_msg_records.cnt = ch_record_count;

	err = nfc_ndef_ch_msg_hs_create(&NFC_NDEF_MSG(ndef_msg),
					&NFC_NDEF_CH_RECORD_DESC(hs_record), &ch_msg_records);
	if (err) {
		printk("Failed to create Connection Handover NDEF message (err %d)\n", err);
		return err;
	}

	return nfc_ndef_msg_encode(&NFC_NDEF_MSG(ndef_msg), nfc_buffer, size);
}
#endif /* !IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED) */

static int nfc_init(void)
{
#if !IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED)
	int err;
	size_t nfc_buffer_size = sizeof(nfc_buffer);

	err = nfc_t2t_setup(nfc_callback, NULL);
	if (err) {
		printk("Failed to setup NFC T2T library (err %d)\n", err);
		return err;
	}

	err = nfc_oob_data_setup(&nfc_buffer_size);
	if (err) {
		printk("Failed to setup NFC OOB data (err %d)\n", err);
		return err;
	}

	err = nfc_t2t_payload_set(nfc_buffer, nfc_buffer_size);
	if (err) {
		printk("Failed to set NFC T2T payload (err %d)\n", err);
		return err;
	}

	err = nfc_t2t_emulation_start();
	if (err) {
		printk("Failed to start NFC T2T emulation (err %d)\n", err);
	}

	return err;
#else
	return 0;
#endif /* !IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED) */
}

static void reset_reason_print(void)
{
	uint32_t reason;

	reason = nrfx_reset_reason_get();

	if (reason & NRFX_RESET_REASON_OFF_MASK) {
		printk("Wake up by the advertising start buttons\n");
#if !IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED)
	} else if (reason & NRFX_RESET_REASON_NFC_MASK) {
		printk("Wake up by NFC field detected\n");
#endif /* !IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED) */
#if defined(NRF_RESETINFO)
	} else if (reason & NRFX_RESET_REASON_LOCAL_SREQ_MASK) {
		printk("Application soft reset detected\n");
#else
	} else if (reason & NRFX_RESET_REASON_SREQ_MASK) {
		printk("Application soft reset detected\n");
#endif /* defined(NRF_RESETINFO) */
	} else if (reason & NRFX_RESET_REASON_RESETPIN_MASK) {
		printk("Reset from pin-reset\n");
	} else if (reason) {
		printk("System reset from different reason 0x%08X\n", reason);
	} else {
		printk("Power-on reset\n");
	}
}

static void system_off(void)
{
#if !IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX)
	printk("Powering off\n");

	/* Clear the reset reason if it didn't do previously. */
	nrfx_reset_reason_clear(nrfx_reset_reason_get());

	set_led_off(RUN_STATUS_LED);

	if (IS_ENABLED(CONFIG_PM_DEVICE) && IS_ENABLED(CONFIG_SERIAL)) {
		static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
		int err;
		enum pm_device_state state;

		if (dev) {
			do {
				err = pm_device_state_get(dev, &state);
			} while ((err == 0) && (state == PM_DEVICE_STATE_ACTIVE));
		}
	}

	sys_poweroff();
#endif /* !IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX) */
}

static void system_off_work_handler(struct k_work *work)
{
	system_off();
}

static void advertising_terminated(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_sent_info *info)
{
	if (!device_conn) {
		printk("Adverting set %p, terminated.\n", (void *)adv);
#if !IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX)
		printk("Scheduling system off\n");

		k_work_schedule(&system_off_work, K_SECONDS(1));
#endif /* !IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX) */
	}
}

static const struct bt_le_ext_adv_cb adv_callbacks = {
	.sent = advertising_terminated
};

int main(void)
{
	int err;
	uint32_t blink_status = 0;
	uint32_t button_state = 0;
	uint32_t has_changed = 0;

	printk("Starting Bluetooth Power Profiling sample\n");

	err = dk_buttons_init(button_handler);
	if (err) {
		printk("Failed to initialize buttons (err %d)\n", err);
		return 0;
	}

	/* Read the button state after booting to check if advertising start is needed. */
	dk_read_buttons(&button_state, &has_changed);

	err = leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return 0;
	}

	reset_reason_print();

	err = pwr_service_cb_register(&pwr_service_callback, NULL);
	if (err) {
		printk("Failed to register PWR service callback (err %d)\n", err);
		return 0;
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		err = settings_load();
		if (err) {
			printk("Failed to load settings (err %d)\n", err);
			return 0;
		}
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		printk("Failed to register authorization info callbacks (err %d)\n", err);
		return 0;
	}

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		printk("Failed to register authorization callbacks (err %d)\n", err);
		return 0;
	}

	err = bt_le_ext_adv_create(connectable_ad_params, &adv_callbacks, &adv_set);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return 0;
	}

	err = pairing_key_generate();
	if (err) {
		printk("Failed to generate pairing keys (err %d)\n", err);
		return 0;
	}

	if (!IS_ENABLED(CONFIG_BT_POWER_PROFILING_NFC_DISABLED)) {
		err = nfc_init();
		if (err) {
			printk("Failed to initialize NFC (err %d)\n", err);
			return 0;
		}
	}

	button_handler(button_state, has_changed);

	if (!(button_state & (CONNECTABLE_ADV_BUTTON | NON_CONNECTABLE_ADV_BUTTON))) {
		k_work_schedule(&system_off_work, K_SECONDS(SYSTEM_OFF_DELAY));
	}

	if (!IS_ENABLED(CONFIG_BT_POWER_PROFILING_LED_DISABLED)) {
		for (;;) {
			set_led(RUN_STATUS_LED, (++blink_status) % 2);
			k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
		}
	} else {
		return 0;
	}
}
