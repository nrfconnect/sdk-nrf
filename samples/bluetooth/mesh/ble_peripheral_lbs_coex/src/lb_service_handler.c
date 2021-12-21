/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <drivers/gpio.h>
#include <soc.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/lbs.h>
#include <settings/settings.h>
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

static bool ble_button_state;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
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
	int err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
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
