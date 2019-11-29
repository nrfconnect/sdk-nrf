/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <gpio.h>
#include <soc.h>
#include <assert.h>
#include <spinlock.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/bas.h>
#include <bluetooth/services/hids.h>
#include <bluetooth/services/dis.h>
#include <dk_buttons_and_leds.h>

#include "app_nfc.h"

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define BASE_USB_HID_SPEC_VERSION   0x0101

#define OUTPUT_REPORT_MAX_LEN            1
#define OUTPUT_REPORT_BIT_MASK_CAPS_LOCK 0x02
#define INPUT_REP_KEYS_REF_ID            1
#define OUTPUT_REP_KEYS_REF_ID           1
#define MODIFIER_KEY_POS                 0
#define SHIFT_KEY_CODE                   0x02
#define SCAN_CODE_POS                    2
#define KEYS_MAX_LEN                    (INPUT_REPORT_KEYS_MAX_LEN - \
					SCAN_CODE_POS)

#define ADV_LED_BLINK_INTERVAL  1000

#define ADV_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2
#define LED_CAPS_LOCK  DK_LED3
#define NFC_LED	       DK_LED4
#define KEY_TEXT_MASK  DK_BTN1_MSK
#define KEY_SHIFT_MASK DK_BTN2_MSK
#define KEY_ADV_MASK   DK_BTN4_MSK

/* Key used to accept or reject passkey value */
#define KEY_PAIRING_ACCEPT DK_BTN1_MSK
#define KEY_PAIRING_REJECT DK_BTN2_MSK

/* HIDs queue elements. */
#define HIDS_QUEUE_SIZE 10

/* ********************* */
/* Buttons configuration */

/* Note: The configuration below is the same as BOOT mode configuration
 * This simplifies the code as the BOOT mode is the same as REPORT mode.
 * Changing this configuration would require separate implementation of
 * BOOT mode report generation.
 */
#define KEY_CTRL_CODE_MIN 224 /* Control key codes - required 8 of them */
#define KEY_CTRL_CODE_MAX 231 /* Control key codes - required 8 of them */
#define KEY_CODE_MIN      0   /* Normal key codes */
#define KEY_CODE_MAX      101 /* Normal key codes */
#define KEY_PRESS_MAX     6   /* Maximum number of non-control keys
			       * pressed simultaneously
			       */

/* Number of bytes in key report
 *
 * 1B - control keys
 * 1B - reserved
 * rest - non-control keys
 */
#define INPUT_REPORT_KEYS_MAX_LEN (1 + 1 + KEY_PRESS_MAX)

/* Current report map construction requires exactly 8 buttons */
BUILD_ASSERT((KEY_CTRL_CODE_MAX - KEY_CTRL_CODE_MIN) + 1 == 8);

/* OUT report internal indexes.
 *
 * This is a position in internal report table and is not related to
 * report ID.
 */
enum {
	OUTPUT_REP_KEYS_IDX = 0
};

/* INPUT report internal indexes.
 *
 * This is a position in internal report table and is not related to
 * report ID.
 */
enum {
	INPUT_REP_KEYS_IDX = 0
};

/* HIDS instance. */
BT_GATT_HIDS_DEF(hids_obj,
		 OUTPUT_REPORT_MAX_LEN,
		 INPUT_REPORT_KEYS_MAX_LEN);

static volatile bool is_adv;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
		      (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
		      (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      0x12, 0x18,       /* HID Service */
		      0x0f, 0x18),      /* Battery Service */

};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct conn_mode {
	struct bt_conn *conn;
	bool in_boot_mode;
} conn_mode[CONFIG_BT_GATT_HIDS_MAX_CLIENT_COUNT];

static const u8_t hello_world_str[] = {
	0x0b,	/* Key h */
	0x08,	/* Key e */
	0x0f,	/* Key l */
	0x0f,	/* Key l */
	0x12,	/* Key o */
	0x28,	/* Key Return */
};

static const u8_t shift_key[] = { 225 };

/* Current report status
 */
static struct keyboard_state {
	u8_t ctrl_keys_state; /* Current keys state */
	u8_t keys_state[KEY_PRESS_MAX];
} hid_keyboard_state;

#if CONFIG_NFC_OOB_PAIRING
static struct k_work adv_work;
#endif

static struct k_work pairing_work;
struct pairing_data_mitm {
	struct bt_conn *conn;
	unsigned int passkey;
};

K_MSGQ_DEFINE(mitm_queue,
	      sizeof(struct pairing_data_mitm),
	      CONFIG_BT_GATT_HIDS_MAX_CLIENT_COUNT,
	      4);

static void advertising_start(void)
{
	int err;
	struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
						BT_LE_ADV_OPT_CONNECTABLE |
						BT_LE_ADV_OPT_ONE_TIME,
						BT_GAP_ADV_FAST_INT_MIN_2,
						BT_GAP_ADV_FAST_INT_MAX_2);

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
	if (err) {
		if (err == -EALREADY) {
			printk("Advertising continued\n");
		} else {
			printk("Advertising failed to start (err %d)\n", err);
		}

		return;
	}

	is_adv = true;
	printk("Advertising successfully started\n");
}


#if CONFIG_NFC_OOB_PAIRING
static void delayed_advertising_start(struct k_work *work)
{
	advertising_start();
}


void nfc_field_detected(void)
{
	dk_set_led_on(NFC_LED);

	for (int i = 0; i < CONFIG_BT_GATT_HIDS_MAX_CLIENT_COUNT; i++) {
		if (!conn_mode[i].conn) {
			k_work_submit(&adv_work);
			break;
		}
	}
}


void nfc_field_lost(void)
{
	dk_set_led_off(NFC_LED);
}
#endif


static void pairing_process(struct k_work *work)
{
	int err;
	struct pairing_data_mitm pairing_data;

	char addr[BT_ADDR_LE_STR_LEN];

	err = k_msgq_peek(&mitm_queue, &pairing_data);
	if (err) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(pairing_data.conn),
			  addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, pairing_data.passkey);
	printk("Press Button 1 to confirm, Button 2 to reject.\n");
}


static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected %s\n", addr);
	dk_set_led_on(CON_STATUS_LED);

	err = bt_gatt_hids_notify_connected(&hids_obj, conn);

	if (err) {
		printk("Failed to notify HID service about connection\n");
		return;
	}

	for (size_t i = 0; i < CONFIG_BT_GATT_HIDS_MAX_CLIENT_COUNT; i++) {
		if (!conn_mode[i].conn) {
			conn_mode[i].conn = conn;
			conn_mode[i].in_boot_mode = false;
			break;
		}
	}

#if CONFIG_NFC_OOB_PAIRING == 0
	for (size_t i = 0; i < CONFIG_BT_GATT_HIDS_MAX_CLIENT_COUNT; i++) {
		if (!conn_mode[i].conn) {
			advertising_start();
			return;
		}
	}
#endif
	is_adv = false;
}


static void disconnected(struct bt_conn *conn, u8_t reason)
{
	int err;
	bool is_any_dev_connected = false;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected from %s (reason %u)\n", addr, reason);

	err = bt_gatt_hids_notify_disconnected(&hids_obj, conn);

	if (err) {
		printk("Failed to notify HID service about disconnection\n");
	}

	for (size_t i = 0; i < CONFIG_BT_GATT_HIDS_MAX_CLIENT_COUNT; i++) {
		if (conn_mode[i].conn == conn) {
			conn_mode[i].conn = NULL;
		} else {
			if (conn_mode[i].conn) {
				is_any_dev_connected = true;
			}
		}
	}

	if (!is_any_dev_connected) {
		dk_set_led_off(CON_STATUS_LED);
	}

#if CONFIG_NFC_OOB_PAIRING
	if (is_adv) {
		printk("Advertising stopped after disconnect\n");
		bt_le_adv_stop();
		is_adv = false;
	}
#else
	advertising_start();
#endif
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


static void caps_lock_handler(const struct bt_gatt_hids_rep *rep)
{
	u8_t report_val = ((*rep->data) & OUTPUT_REPORT_BIT_MASK_CAPS_LOCK) ?
			  1 : 0;
	dk_set_led(LED_CAPS_LOCK, report_val);
}


static void hids_outp_rep_handler(struct bt_gatt_hids_rep *rep,
				  struct bt_conn *conn,
				  bool write)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (!write) {
		printk("Output report read\n");
		return;
	};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Output report has been received %s\n", addr);
	caps_lock_handler(rep);
}


static void hids_boot_kb_outp_rep_handler(struct bt_gatt_hids_rep *rep,
					  struct bt_conn *conn,
					  bool write)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (!write) {
		printk("Output report read\n");
		return;
	};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Boot Keyboard Output report has been received %s\n", addr);
	caps_lock_handler(rep);
}


static void hids_pm_evt_handler(enum bt_gatt_hids_pm_evt evt,
				struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];
	size_t i;

	for (i = 0; i < CONFIG_BT_GATT_HIDS_MAX_CLIENT_COUNT; i++) {
		if (conn_mode[i].conn == conn) {
			break;
		}
	}

	if (i >= CONFIG_BT_GATT_HIDS_MAX_CLIENT_COUNT) {
		printk("Cannot find connection handle when processing PM");
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	switch (evt) {
	case BT_GATT_HIDS_PM_EVT_BOOT_MODE_ENTERED:
		printk("Boot mode entered %s\n", addr);
		conn_mode[i].in_boot_mode = true;
		break;

	case BT_GATT_HIDS_PM_EVT_REPORT_MODE_ENTERED:
		printk("Report mode entered %s\n", addr);
		conn_mode[i].in_boot_mode = false;
		break;

	default:
		break;
	}
}


static void hid_init(void)
{
	int err;
	struct bt_gatt_hids_init_param    hids_init_obj = { 0 };
	struct bt_gatt_hids_inp_rep       *hids_inp_rep;
	struct bt_gatt_hids_outp_feat_rep *hids_outp_rep;

	static const u8_t report_map[] = {
		0x05, 0x01,       /* Usage Page (Generic Desktop) */
		0x09, 0x06,       /* Usage (Keyboard) */
		0xA1, 0x01,       /* Collection (Application) */

		/* Keys */
#if INPUT_REP_KEYS_REF_ID
		0x85, INPUT_REP_KEYS_REF_ID,
#endif
		0x05, 0x07,       /* Usage Page (Key Codes) */
		0x19, 0xe0,       /* Usage Minimum (224) */
		0x29, 0xe7,       /* Usage Maximum (231) */
		0x15, 0x00,       /* Logical Minimum (0) */
		0x25, 0x01,       /* Logical Maximum (1) */
		0x75, 0x01,       /* Report Size (1) */
		0x95, 0x08,       /* Report Count (8) */
		0x81, 0x02,       /* Input (Data, Variable, Absolute) */

		0x95, 0x01,       /* Report Count (1) */
		0x75, 0x08,       /* Report Size (8) */
		0x81, 0x01,       /* Input (Constant) reserved byte(1) */

		0x95, 0x06,       /* Report Count (6) */
		0x75, 0x08,       /* Report Size (8) */
		0x15, 0x00,       /* Logical Minimum (0) */
		0x25, 0x65,       /* Logical Maximum (101) */
		0x05, 0x07,       /* Usage Page (Key codes) */
		0x19, 0x00,       /* Usage Minimum (0) */
		0x29, 0x65,       /* Usage Maximum (101) */
		0x81, 0x00,       /* Input (Data, Array) Key array(6 bytes) */

		/* LED */
#if OUTPUT_REP_KEYS_REF_ID
		0x85, OUTPUT_REP_KEYS_REF_ID,
#endif
		0x95, 0x05,       /* Report Count (5) */
		0x75, 0x01,       /* Report Size (1) */
		0x05, 0x08,       /* Usage Page (Page# for LEDs) */
		0x19, 0x01,       /* Usage Minimum (1) */
		0x29, 0x05,       /* Usage Maximum (5) */
		0x91, 0x02,       /* Output (Data, Variable, Absolute), */
				  /* Led report */
		0x95, 0x01,       /* Report Count (1) */
		0x75, 0x03,       /* Report Size (3) */
		0x91, 0x01,       /* Output (Data, Variable, Absolute), */
				  /* Led report padding */

		0xC0              /* End Collection (Application) */
	};

	hids_init_obj.rep_map.data = report_map;
	hids_init_obj.rep_map.size = sizeof(report_map);

	hids_init_obj.info.bcd_hid = BASE_USB_HID_SPEC_VERSION;
	hids_init_obj.info.b_country_code = 0x00;
	hids_init_obj.info.flags = (BT_GATT_HIDS_REMOTE_WAKE |
				    BT_GATT_HIDS_NORMALLY_CONNECTABLE);

	hids_inp_rep =
		&hids_init_obj.inp_rep_group_init.reports[INPUT_REP_KEYS_IDX];
	hids_inp_rep->size = INPUT_REPORT_KEYS_MAX_LEN;
	hids_inp_rep->id = INPUT_REP_KEYS_REF_ID;
	hids_init_obj.inp_rep_group_init.cnt++;

	hids_outp_rep =
		&hids_init_obj.outp_rep_group_init.reports[OUTPUT_REP_KEYS_IDX];
	hids_outp_rep->size = OUTPUT_REPORT_MAX_LEN;
	hids_outp_rep->id = OUTPUT_REP_KEYS_REF_ID;
	hids_outp_rep->handler = hids_outp_rep_handler;
	hids_init_obj.outp_rep_group_init.cnt++;

	hids_init_obj.is_kb = true;
	hids_init_obj.boot_kb_outp_rep_handler = hids_boot_kb_outp_rep_handler;
	hids_init_obj.pm_evt_handler = hids_pm_evt_handler;

	err = bt_gatt_hids_init(&hids_obj, &hids_init_obj);
	__ASSERT(err == 0, "HIDS initialization failed\n");
}


static void bt_ready(void)
{
	printk("Bluetooth initialized\n");

	hid_init();

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

#if CONFIG_NFC_OOB_PAIRING
	k_work_init(&adv_work, delayed_advertising_start);
	app_nfc_init();
#else
	advertising_start();
#endif

	k_work_init(&pairing_work, pairing_process);
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	int err;

	struct pairing_data_mitm pairing_data;

	pairing_data.conn    = bt_conn_ref(conn);
	pairing_data.passkey = passkey;

	err = k_msgq_put(&mitm_queue, &pairing_data, K_NO_WAIT);
	if (err) {
		printk("Pairing queue is full. Purge previous data.\n");
	}

	/* In the case of multiple pairing requests, trigger
	 * pairing confirmation which needed user interaction only
	 * once to avoid display information about all devices at
	 * the same time. Passkey confirmation for next devices will
	 * be proccess from queue after handling the earlier ones.
	 */
	if (k_msgq_num_used_get(&mitm_queue) == 1) {
		k_work_submit(&pairing_work);
	}
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


#if CONFIG_NFC_OOB_PAIRING
static void auth_oob_data_request(struct bt_conn *conn,
				  struct bt_conn_oob_info *info)
{
	int err;
	struct bt_le_oob *oob_local = app_nfc_oob_data_get();

	printk("LESC OOB data requested\n");

	if (info->type != BT_CONN_OOB_LE_SC) {
		printk("Only LESC pairing supported\n");
		return;
	}

	if (info->lesc.oob_config != BT_CONN_OOB_LOCAL_ONLY) {
		printk("LESC OOB config not supported\n");
		return;
	}

	/* Pass only local OOB data. */
	err = bt_le_oob_set_sc_data(conn, &oob_local->le_sc_data, NULL);
	if (err) {
		printk("Error while setting OOB data: %d\n", err);
	} else {
		printk("Successfully provided LESC OOB data\n");
	}
}
#endif


static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed conn: %s, reason %d\n", addr, reason);
}


static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.passkey_confirm = auth_passkey_confirm,
	.cancel = auth_cancel,
	.pairing_confirm = pairing_confirm,
#if CONFIG_NFC_OOB_PAIRING
	.oob_data_request = auth_oob_data_request,
#endif
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};


/** @brief Function process keyboard state and sends it
 *
 *  @param pstate     The state to be sent
 *  @param boot_mode  Information if boot mode protocol is selected.
 *  @param conn       Connection handler
 *
 *  @return 0 on success or negative error code.
 */
static int key_report_con_send(const struct keyboard_state *state,
			bool boot_mode,
			struct bt_conn *conn)
{
	int err = 0;
	u8_t  data[INPUT_REPORT_KEYS_MAX_LEN];
	u8_t *key_data;
	const u8_t *key_state;
	size_t n;

	data[0] = state->ctrl_keys_state;
	data[1] = 0;
	key_data = &data[2];
	key_state = state->keys_state;

	for (n = 0; n < KEY_PRESS_MAX; ++n) {
		*key_data++ = *key_state++;
	}
	if (boot_mode) {
		err = bt_gatt_hids_boot_kb_inp_rep_send(&hids_obj, conn, data,
							sizeof(data), NULL);
	} else {
		err = bt_gatt_hids_inp_rep_send(&hids_obj, conn,
						INPUT_REP_KEYS_IDX, data,
						sizeof(data), NULL);
	}
	return err;
}

/** @brief Function process and send keyboard state to all active connections
 *
 * Function process global keyboard state and send it to all connected
 * clients.
 *
 * @return 0 on success or negative error code.
 */
static int key_report_send(void)
{
	for (size_t i = 0; i < CONFIG_BT_GATT_HIDS_MAX_CLIENT_COUNT; i++) {
		if (conn_mode[i].conn) {
			int err;

			err = key_report_con_send(&hid_keyboard_state,
						  conn_mode[i].in_boot_mode,
						  conn_mode[i].conn);
			if (err) {
				printk("Key report send error: %d\n", err);
				return err;
			}
		}
	}
	return 0;
}

/** @brief Change key code to ctrl code mask
 *
 *  Function changes the key code to the mask in the control code
 *  field inside the raport.
 *  Returns 0 if key code is not a control key.
 *
 *  @param key Key code
 *
 *  @return Mask of the control key or 0.
 */
static u8_t button_ctrl_code(u8_t key)
{
	if (KEY_CTRL_CODE_MIN <= key && key <= KEY_CTRL_CODE_MAX) {
		return (u8_t)(1U << (key - KEY_CTRL_CODE_MIN));
	}
	return 0;
}


static int hid_kbd_state_key_set(u8_t key)
{
	u8_t ctrl_mask = button_ctrl_code(key);

	if (ctrl_mask) {
		hid_keyboard_state.ctrl_keys_state |= ctrl_mask;
		return 0;
	}
	for (size_t i = 0; i < KEY_CTRL_CODE_MAX; ++i) {
		if (hid_keyboard_state.keys_state[i] == 0) {
			hid_keyboard_state.keys_state[i] = key;
			return 0;
		}
	}
	/* All slots busy */
	return -EBUSY;
}


static int hid_kbd_state_key_clear(u8_t key)
{
	u8_t ctrl_mask = button_ctrl_code(key);

	if (ctrl_mask) {
		hid_keyboard_state.ctrl_keys_state &= ~ctrl_mask;
		return 0;
	}
	for (size_t i = 0; i < KEY_CTRL_CODE_MAX; ++i) {
		if (hid_keyboard_state.keys_state[i] == key) {
			hid_keyboard_state.keys_state[i] = 0;
			return 0;
		}
	}
	/* Key not found */
	return -EINVAL;
}

/** @brief Press a button and send report
 *
 *  @note Functions to manipulate hid state are not reentrant
 *  @param keys
 *  @param cnt
 *
 *  @return 0 on success or negative error code.
 */
static int hid_buttons_press(const u8_t *keys, size_t cnt)
{
	while (cnt--) {
		int err;

		err = hid_kbd_state_key_set(*keys++);
		if (err) {
			printk("Cannot set selected key.\n");
			return err;
		}
	}

	return key_report_send();
}

/** @brief Release the button and send report
 *
 *  @note Functions to manipulate hid state are not reentrant
 *  @param keys
 *  @param cnt
 *
 *  @return 0 on success or negative error code.
 */
static int hid_buttons_release(const u8_t *keys, size_t cnt)
{
	while (cnt--) {
		int err;

		err = hid_kbd_state_key_clear(*keys++);
		if (err) {
			printk("Cannot clear selected key.\n");
			return err;
		}
	}

	return key_report_send();
}


static void button_text_changed(bool down)
{
	static const u8_t *chr = hello_world_str;

	if (down) {
		hid_buttons_press(chr, 1);
	} else {
		hid_buttons_release(chr, 1);
		if (++chr == (hello_world_str + sizeof(hello_world_str))) {
			chr = hello_world_str;
		}
	}
}


static void button_shift_changed(bool down)
{
	if (down) {
		hid_buttons_press(shift_key, 1);
	} else {
		hid_buttons_release(shift_key, 1);
	}
}


static void num_comp_reply(bool accept)
{
	struct pairing_data_mitm pairing_data;
	struct bt_conn *conn;

	if (k_msgq_get(&mitm_queue, &pairing_data, K_NO_WAIT) != 0) {
		return;
	}

	conn = pairing_data.conn;

	if (accept) {
		bt_conn_auth_passkey_confirm(conn);
		printk("Numeric Match, conn %p\n", conn);
	} else {
		bt_conn_auth_cancel(conn);
		printk("Numeric Reject, conn %p\n", conn);
	}

	bt_conn_unref(pairing_data.conn);

	if (k_msgq_num_used_get(&mitm_queue)) {
		k_work_submit(&pairing_work);
	}
}


static void button_changed(u32_t button_state, u32_t has_changed)
{

	u32_t buttons = button_state & has_changed;

	if (k_msgq_num_used_get(&mitm_queue)) {
		if (buttons & KEY_PAIRING_ACCEPT) {
			num_comp_reply(true);

			return;
		}

		if (buttons & KEY_PAIRING_REJECT) {
			num_comp_reply(false);

			return;
		}
	}

	if (has_changed & KEY_TEXT_MASK) {
		button_text_changed((button_state & KEY_TEXT_MASK) != 0);
	}
	if (has_changed & KEY_SHIFT_MASK) {
		button_shift_changed((button_state & KEY_SHIFT_MASK) != 0);
	}
#if CONFIG_NFC_OOB_PAIRING
	if (has_changed & KEY_ADV_MASK) {
		size_t i;

		for (i = 0; i < CONFIG_BT_GATT_HIDS_MAX_CLIENT_COUNT; i++) {
			if (!conn_mode[i].conn) {
				advertising_start();
				return;
			}
		}

		printk("Cannot start advertising, all connections slots are"
		       " taken\n");
	}
#endif
}


static void configure_gpio(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Cannot init buttons (err: %d)\n", err);
	}

	err = dk_leds_init();
	if (err) {
		printk("Cannot init LEDs (err: %d)\n", err);
	}
}


static void bas_notify(void)
{
	u8_t battery_level = bt_gatt_bas_get_battery_level();

	battery_level--;

	if (!battery_level) {
		battery_level = 100U;
	}

	bt_gatt_bas_set_battery_level(battery_level);
}


void main(void)
{
	int err;
	int blink_status = 0;

	printk("Starting Bluetooth Peripheral HIDS keyboard example\n");

	configure_gpio();

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&conn_auth_callbacks);

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_ready();

	for (;;) {
		if (is_adv) {
			dk_set_led(ADV_STATUS_LED, (++blink_status) % 2);
		} else {
			dk_set_led_off(ADV_STATUS_LED);
		}
		k_sleep(ADV_LED_BLINK_INTERVAL);
		/* Battery level simulation */
		bas_notify();
	}
}
