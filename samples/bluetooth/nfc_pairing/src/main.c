/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <drivers/entropy.h>
#include <zephyr.h>

#include <nfc_t4t_lib.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/ch.h>
#include <nfc/ndef/ch_msg.h>
#include <nfc/ndef/le_oob_rec.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <dk_buttons_and_leds.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define NDEF_MSG_BUF_SIZE 256
#define AUTH_SC_FLAG 0x08

#define NFC_FIELD_LED DK_LED2
#define CON_STATUS_LED DK_LED1

#define KEY_BOND_REMOVE_MASK DK_BTN4_MSK

/* Buffer used to hold an NFC NDEF message. */
static u8_t ndef_msg_buf[NDEF_MSG_BUF_SIZE];
static struct bt_le_oob oob_local;
static struct k_work adv_work;
static struct k_work pair_msg_work;
static u8_t conn_cnt;
static u8_t tk_value[NFC_NDEF_LE_OOB_REC_TK_LEN];

/* Bonded address queue. */
K_MSGQ_DEFINE(bonds_queue,
	      sizeof(bt_addr_le_t),
	      CONFIG_BT_MAX_PAIRED,
	      4);

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

#if defined(NRF5340_XXAA_APPLICATION)
static int tk_value_generate(void)
{
	int err;

	err = bt_rand(tk_value, sizeof(tk_value));
	if (err) {
		printk("Random TK value generation failed: %d\n", err);
	}

	return err;
}
#else
static int tk_value_generate(void)
{
	int err;
	struct device *dev;

	dev = device_get_binding(DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);
	if (!dev) {
		printk("error: no entropy device\n");
		return -ENXIO;
	}

	err = entropy_get_entropy(dev, tk_value, sizeof(tk_value));
	if (err) {
		printk("entropy_get_entropy failed: %d\n", err);
	}

	return err;
}
#endif /* NRF5340_XXAA_APPLICATION */

static void bond_find(const struct bt_bond_info *info, void *user_data)
{
	int err;

	/* Filter already connected peers. */
	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (bt_conn_lookup_addr_le(BT_ID_DEFAULT, &info->addr)) {
			return;
		}
	}

	err = k_msgq_put(&bonds_queue, (void *) &info->addr, K_NO_WAIT);
	if (err) {
		printk("No space in the queue for the bond.\n");
	}
}

static void advertising_start(void)
{
	k_msgq_purge(&bonds_queue);
	bt_foreach_bond(BT_ID_DEFAULT, bond_find, NULL);

	k_work_submit(&adv_work);
}

/** .. include_startingpoint_pair_msg_rst */
static int pairing_msg_generate(u32_t *len)
{
	int err;
	struct nfc_ndef_le_oob_rec_payload_desc rec_payload;
	struct nfc_ndef_ch_msg_records ch_records;

	NFC_NDEF_MSG_DEF(hs_msg, 2);

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob_local);
	if (err) {
		printk("Error while fetching local OOB data: %d\n", err);
	}

	err = tk_value_generate();
	if (err) {
		return err;
	}

	memset(&rec_payload, 0, sizeof(rec_payload));

	rec_payload.addr = &oob_local.addr;
	rec_payload.le_sc_data = &oob_local.le_sc_data;
	rec_payload.tk_value = tk_value;
	rec_payload.local_name = bt_get_name();
	rec_payload.le_role = NFC_NDEF_LE_OOB_REC_LE_ROLE(
		NFC_NDEF_LE_OOB_REC_LE_ROLE_PERIPH_ONLY);
	rec_payload.appearance = NFC_NDEF_LE_OOB_REC_APPEARANCE(
		CONFIG_BT_DEVICE_APPEARANCE);
	rec_payload.flags = NFC_NDEF_LE_OOB_REC_FLAGS(BT_LE_AD_NO_BREDR);

	NFC_NDEF_LE_OOB_RECORD_DESC_DEF(oob_rec, '0', &rec_payload);
	NFC_NDEF_CH_AC_RECORD_DESC_DEF(oob_ac, NFC_AC_CPS_ACTIVE, 1, "0", 0);
	NFC_NDEF_CH_HS_RECORD_DESC_DEF(hs_rec, NFC_NDEF_CH_MSG_MAJOR_VER,
				       NFC_NDEF_CH_MSG_MINOR_VER, 1);

	ch_records.ac = &NFC_NDEF_CH_AC_RECORD_DESC(oob_ac);
	ch_records.carrier = &NFC_NDEF_LE_OOB_RECORD_DESC(oob_rec);
	ch_records.cnt = 1;

	err = nfc_ndef_ch_msg_hs_create(&NFC_NDEF_MSG(hs_msg),
					&NFC_NDEF_CH_RECORD_DESC(hs_rec),
					&ch_records);

	return nfc_ndef_msg_encode(&NFC_NDEF_MSG(hs_msg), ndef_msg_buf, len);
}
/** .. include_endpoint_pair_msg_rst */

/**
 * @brief Callback function for handling NFC events.
 */
static void nfc_callback(void *context,
			 enum nfc_t4t_event event,
			 const u8_t *data,
			 size_t data_length,
			 u32_t flags)
{
	ARG_UNUSED(context);
	ARG_UNUSED(data);
	ARG_UNUSED(flags);

	switch (event) {
	case NFC_T4T_EVENT_FIELD_ON:
		dk_set_led_on(NFC_FIELD_LED);
		break;

	case NFC_T4T_EVENT_FIELD_OFF:
		dk_set_led_off(NFC_FIELD_LED);
		break;

	case NFC_T4T_EVENT_NDEF_READ:
		advertising_start();

		break;

	default:
		break;
	}
}

static void pair_msg_handler(struct k_work *work)
{
	int err;
	u32_t len = sizeof(ndef_msg_buf);

	err = pairing_msg_generate(&len);
	if (err) {
		printk("Paring message encode error\n");
	}
}

static void advertising_continue(void)
{
	struct bt_le_adv_param adv_param;

	bt_addr_le_t addr;

	if (!k_msgq_get(&bonds_queue, &addr, K_NO_WAIT)) {
		char addr_buf[BT_ADDR_LE_STR_LEN];

		adv_param = *BT_LE_ADV_CONN_DIR(&addr);
		adv_param.options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;

		int err = bt_le_adv_start(&adv_param, NULL, 0, NULL, 0);

		if (err) {
			printk("Directed advertising failed to start\n");
			return;
		}

		bt_addr_le_to_str(&addr, addr_buf, BT_ADDR_LE_STR_LEN);
		printk("Direct advertising to %s started\n", addr_buf);
	} else {
		int err;

		adv_param = *BT_LE_ADV_CONN;
		adv_param.options |= BT_LE_ADV_OPT_ONE_TIME;
		err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad),
				      NULL, 0);
		if (err) {
			printk("Advertising failed to start (err %d)\n", err);
			return;
		}

		printk("Regular advertising started\n");
	}
}

static void adv_handler(struct k_work *work)
{
	advertising_continue();
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static void pairing_confirm(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	bt_conn_auth_pairing_confirm(conn);

	printk("Pairing confirmed: %s\n", addr);
}

static void lesc_obb_data_set(struct bt_conn *conn,
			      struct bt_conn_oob_info *info)
{
	int err;

	if (info->lesc.oob_config != BT_CONN_OOB_LOCAL_ONLY) {
		printk("LESC OOB config not supported\n");
		return;
	}

	/* Pass only local OOB data. */
	err = bt_le_oob_set_sc_data(conn, &oob_local.le_sc_data, NULL);
	if (err) {
		printk("Error while setting OOB data: %d\n", err);
	}
}

static void legacy_tk_value_set(struct bt_conn *conn)
{
	int err;

	err = bt_le_oob_set_legacy_tk(conn, tk_value);
	if (err) {
		printk("TK value set error: %d\n", err);
	}
}

static void auth_oob_data_request(struct bt_conn *conn,
				  struct bt_conn_oob_info *info)
{
	if (info->type == BT_CONN_OOB_LE_SC) {
		printk("LESC OOB data requested\n");
		lesc_obb_data_set(conn, info);
	}

	if (info->type == BT_CONN_OOB_LE_LEGACY) {
		printk("Legacy TK value requested\n");
		legacy_tk_value_set(conn);
	}
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);

	k_work_submit(&pair_msg_work);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed conn: %s, reason %d\n", addr, reason);

	k_work_submit(&pair_msg_work);
}

static enum bt_security_err pairing_accept(struct bt_conn *conn,
				const struct bt_conn_pairing_feat *const feat)
{
	if (feat->oob_data_flag && (!(feat->auth_req & AUTH_SC_FLAG))) {
		bt_set_oob_data_flag(true);
	}

	return BT_SECURITY_ERR_SUCCESS;

}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
	.pairing_confirm = pairing_confirm,
	.oob_data_request = auth_oob_data_request,
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
	.pairing_accept = pairing_accept,
};

static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		if (err == BT_HCI_ERR_ADV_TIMEOUT) {
			printk("Direct advertising to %s timed out\n", addr);
			k_work_submit(&adv_work);
		} else {
			printk("Failed to connect to %s (%u)\n", addr, err);
		}

		return;
	}

	conn_cnt++;

	printk("Connected %s\n", addr);
	dk_set_led_on(CON_STATUS_LED);

	if (conn_cnt < CONFIG_BT_MAX_CONN) {
		advertising_start();
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	conn_cnt--;

	if (!conn_cnt) {
		dk_set_led_off(CON_STATUS_LED);
	}

	printk("Disconnected from %s (reason %u)\n", addr, reason);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d\n", addr, level,
			err);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void nfc_init(void)
{
	int err;
	u32_t len = sizeof(ndef_msg_buf);

	/* Set up NFC */
	err = nfc_t4t_setup(nfc_callback, NULL);
	if (err) {
		printk("Cannot setup NFC T4T library!\n");
		return;
	}

	err = pairing_msg_generate(&len);
	if (err) {
		printk("Paring message encode error\n");
		return;
	}

	/* Set created message as the NFC payload */
	err = nfc_t4t_ndef_rwpayload_set(ndef_msg_buf, sizeof(ndef_msg_buf));
	if (err) {
		printk("Cannot set payload!\n");
		return;
	}

	/* Start sensing NFC field */
	err = nfc_t4t_emulation_start();
	if (err) {
		printk("Cannot start emulation!\n");
		return;
	}

	printk("NFC configuration done\n");
}

void button_changed(u32_t button_state, u32_t has_changed)
{
	int err;
	u32_t buttons = button_state & has_changed;

	if (buttons & KEY_BOND_REMOVE_MASK) {
		err = bt_unpair(BT_ID_DEFAULT, NULL);
		if (err) {
			printk("Bond remove failed err: %d\n", err);
		} else {
			printk("All bond removed\n");
		}
	}
}

void main(void)
{
	int err;

	printk("Starting Bluetooth NFC Pairing Reference example\n");

	/* Configure LED-pins as outputs */
	err = dk_leds_init();
	if (err) {
		printk("Cannot init LEDs!\n");
		return;
	}

	/* Configure buttons */
	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Cannot init buttons (err %d\n", err);
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&conn_auth_callbacks);

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	k_work_init(&adv_work, adv_handler);
	k_work_init(&pair_msg_work, pair_msg_handler);
	nfc_init();
}
