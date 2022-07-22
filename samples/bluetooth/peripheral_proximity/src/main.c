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


#define IAS_STATUS_LED          DK_LED1
#define LLS_STATUS_LED          DK_LED2
#define RUN_LED_BLINK_MILD_ALERT_INTERVAL  1200


static volatile bool ias_led_blink_off, lls_led_blink_off;
static uint16_t led_blink_interval;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_LLS_VAL))
};

static void no_alert_ias(void)
{
	ias_led_blink_off = true;

	dk_set_led_off(IAS_STATUS_LED);

}

static void mild_alert_ias(void)
{
	ias_led_blink_off = false;

	led_blink_interval = RUN_LED_BLINK_MILD_ALERT_INTERVAL;

}
static void high_alert_ias(void)
{
	ias_led_blink_off = true;

	dk_set_led_on(IAS_STATUS_LED);

}

static void no_alert_lls(void)
{
	lls_led_blink_off = true;

	dk_set_led_off(LLS_STATUS_LED);

}

static void mild_alert_lls(void)
{
	lls_led_blink_off = false;

	led_blink_interval = RUN_LED_BLINK_MILD_ALERT_INTERVAL;

}
static void high_alert_lls(void)
{
	lls_led_blink_off = true;

	dk_set_led_on(LLS_STATUS_LED);

}

static struct bt_lls_cb lls_callbacks = {
	.no_alert = no_alert_lls,
	.mild_alert = mild_alert_lls,
	.high_alert = high_alert_lls,
};



STRUCT_SECTION_ITERABLE(bt_ias_cb, ias_callbacks) = {
	.no_alert = no_alert_ias,
	.mild_alert = mild_alert_ias,
	.high_alert = high_alert_ias,
};


void main(void)
{
	int blink_status_ias, blink_status_lls;
	int err;

	blink_status_ias = 0;
	blink_status_lls = 0;
	led_blink_interval = RUN_LED_BLINK_MILD_ALERT_INTERVAL;
	ias_led_blink_off = true;
	lls_led_blink_off = true;

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

	bt_lls_init(&lls_callbacks);

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	for (;;) {
		if (ias_led_blink_off == false) {
			dk_set_led(IAS_STATUS_LED, (++blink_status_ias) % 2);
		}

		if (lls_led_blink_off == false) {
			dk_set_led(LLS_STATUS_LED, (++blink_status_lls) % 2);
		}
		k_sleep(K_MSEC(led_blink_interval));
	}

}
