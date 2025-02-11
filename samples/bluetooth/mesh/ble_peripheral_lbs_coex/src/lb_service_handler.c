/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>

#include <bluetooth/services/lbs.h>
#include <dk_buttons_and_leds.h>

#if !DT_NODE_EXISTS(DT_ALIAS(sw0))
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#if !DT_NODE_EXISTS(DT_ALIAS(led1))
#error "Unsupported board: led1 devicetree alias is not defined"
#endif

#define LBS_LED DK_LED2
#define LBS_BUTTON DK_BTN1_MSK

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, (sizeof(CONFIG_BT_DEVICE_NAME) - 1)),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};

static struct bt_le_ext_adv *adv;
static uint8_t lbs_conn_id;
static bool ble_button_state;

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info cinfo;
	int ec;

	ec = bt_conn_get_info(conn, &cinfo);
	if (ec) {
		printk("Unable to get connection info (err %d)\n", ec);
		return;
	}

	if (cinfo.id != lbs_conn_id) {
		return;
	}

	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info cinfo;
	int ec;

	ec = bt_conn_get_info(conn, &cinfo);
	if (ec) {
		printk("Unable to get connection info (err %d)\n", ec);
		return;
	}

	if (cinfo.id != lbs_conn_id) {
		return;
	}

	printk("Disconnected (reason %u)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void app_led_cb(bool led_state)
{
	dk_set_led(LBS_LED, led_state);
}

static bool app_button_cb(void)
{
	return ble_button_state;
}

static struct bt_lbs_cb lbs_callbacs = {
	.led_cb = app_led_cb,
	.button_cb = app_button_cb,
};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & LBS_BUTTON) {
		uint32_t user_button_state = button_state & LBS_BUTTON;

		bt_lbs_send_button_state(user_button_state);
		ble_button_state = user_button_state ? true : false;
	}
}

static void lbs_adv_start(void)
{
	int err;
	size_t id_count = 0xFF;
	struct bt_le_adv_param adv_params = *BT_LE_ADV_CONN_FAST_2;
	/* Use different identity from Bluetooth Mesh to avoid conflicts with Mesh Provisioning
	 * Service and Mesh Proxy Service advertisements.
	 */
	(void)bt_id_get(NULL, &id_count);

	if (id_count < CONFIG_BT_ID_MAX) {
		int id = bt_id_create(NULL, NULL);

		if (id < 0) {
			printk("Unable to create a new identity for LBS (err %d)."
			       " Using the default one.\n", id);
			lbs_conn_id = BT_ID_DEFAULT;
		} else {
			lbs_conn_id = id;
		}

		printk("Created a new identity for LBS: %d\n", lbs_conn_id);
	} else {
		lbs_conn_id = BT_ID_DEFAULT + 1;
		printk("Recovered identity for LBS: %d\n", lbs_conn_id);
	}

	adv_params.id = lbs_conn_id;

	err = bt_le_ext_adv_create(&adv_params, NULL, &adv);
	if (err) {
		printk("Creating LBS service adv instance failed (err %d)\n", err);
		return;
	}

	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Setting LBS service adv data failed (err %d)\n", err);
		return;
	}

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Starting advertising of LBS service failed (err %d)\n", err);
	} else {
		printk("Advertising successfully started\n");
	}
}

static void lbs_init(void)
{
	static struct button_handler button_handler = {
		.cb = button_changed,
	};

	dk_button_handler_add(&button_handler);

	int err = bt_lbs_init(&lbs_callbacs);

	if (err) {
		printk("Failed to init LBS (err:%d)\n", err);
		return;
	}
}

void lbs_handler_init(void)
{
	/** Initiate and add LBS to the device service list. */
	lbs_init();

	/** Start separate advertiser instance. This will allow two simountanious GATT
	 *  connections to the device, one for Mesh and one for the LBS.
	 */
	lbs_adv_start();
}
