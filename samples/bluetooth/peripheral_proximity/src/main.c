/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
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

#include <bluetooth/services/ias.h>
#include <bluetooth/services/lls.h>
#include <settings/settings.h>

#include <dk_buttons_and_leds.h>

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)


#define RUN_STATUS_LED          DK_LED1
#define CON_STATUS_LED          DK_LED2
#define RUN_LED_BLINK_MILD_ALERT_INTERVAL  1200

#define USER_LED                DK_LED3

#define USER_BUTTON             DK_BTN1_MSK

#define BLE_CHAR_ALERT_LEVEL_NO_ALERT 0
#define BLE_CHAR_ALERT_LEVEL_MILD_ALERT 1
#define BLE_CHAR_ALERT_LEVEL_HIGH_ALERT 2

static volatile bool run_led_blink_off;
static uint16_t run_led_blink_interal = 0;
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LLS_VAL),
};
static void alert_signal(uint8_t alert_level);
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");

	dk_set_led_on(CON_STATUS_LED);
	uint8_t alert_lv = bt_lls_alert_level_get();
	alert_signal(alert_lv);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	dk_set_led_off(CON_STATUS_LED);
	uint8_t alert_lv = bt_lls_alert_level_get();
	alert_signal(alert_lv);
}



static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};


static struct bt_conn_auth_cb conn_auth_callbacks;


/**@brief Function for the Signals alert event from Immediate Alert or Link Loss services.
 *
 * @param[in] alert_level  Requested alert level.
 */
static void alert_signal(uint8_t alert_level)
{


	switch (alert_level) {
	case BLE_CHAR_ALERT_LEVEL_NO_ALERT:

		run_led_blink_off = true;
		dk_set_led_off(RUN_STATUS_LED);
		break; // BLE_CHAR_ALERT_LEVEL_NO_ALERT

	case BLE_CHAR_ALERT_LEVEL_MILD_ALERT:

		run_led_blink_off = false;
		run_led_blink_interal = RUN_LED_BLINK_MILD_ALERT_INTERVAL;

		break; // BLE_CHAR_ALERT_LEVEL_MILD_ALERT

	case BLE_CHAR_ALERT_LEVEL_HIGH_ALERT:

		run_led_blink_off = true;
		dk_set_led_on(RUN_STATUS_LED);
		break; // BLE_CHAR_ALERT_LEVEL_HIGH_ALERT

	default:
		// No implementation needed.
		break;
	}
}

void main(void)
{
	int blink_status = 0;
	int err;

	run_led_blink_interal = RUN_LED_BLINK_MILD_ALERT_INTERVAL;
	run_led_blink_off = true;
	printk("Starting Bluetooth Peripheral proximity example\n");

	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_lls_init();
	if (err) {
		printk("Failed to init LLS (err:%d)\n", err);
		return;
	}
	err = bt_ias_init(alert_signal);
	if (err) {
		printk("Failed to init IAS (err:%d)\n", err);
		return;
	}
	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	for (;;) {
		if (run_led_blink_off == false) {
			dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
			k_sleep(K_MSEC(run_led_blink_interal));
		}
	}

}
