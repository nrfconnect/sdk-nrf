/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <settings/settings.h>

#include <dk_buttons_and_leds.h>

#include "bt_adv_helper.h"

#define RUN_STATUS_LED			DK_LED1
#define CON_STATUS_LED			DK_LED2
#define RUN_LED_BLINK_INTERVAL_MS	1000


static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");

	dk_set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	dk_set_led_off(CON_STATUS_LED);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d\n", addr, level, err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected        = connected,
	.disconnected     = disconnected,
	.security_changed = security_changed,
};

void main(void)
{
	bool run_led_on = true;
	int err;

	printk("Starting Bluetooth Fast Pair example\n");

	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return;
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = settings_load();
	if (err) {
		printk("Settings load failed (err: %d\n)", err);
		return;
	}

	printk("Settings loaded\n");

	err = bt_adv_helper_adv_start();
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	for (;;) {
		dk_set_led(RUN_STATUS_LED, run_led_on);
		run_led_on = !run_led_on;
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL_MS));
	}
}
