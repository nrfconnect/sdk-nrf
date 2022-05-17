/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/zephyr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/settings/settings.h>

#include <dk_buttons_and_leds.h>

#include "bt_adv_helper.h"
#include "hids_helper.h"

#define RUN_STATUS_LED			DK_LED1
#define CON_STATUS_LED			DK_LED2
#define FP_DISCOVERABLE_STATUS_LED	DK_LED3

#define FP_DISCOVERABLE_BUTTON_MASK	DK_BTN1_MSK
#define VOLUME_UP_BUTTON_MASK		DK_BTN2_MSK
#define VOLUME_DOWN_BUTTON_MASK		DK_BTN4_MSK

#define RUN_LED_BLINK_INTERVAL_MS	1000

static bool fp_discoverable = true;
static struct bt_conn *peer;

static struct k_work bt_adv_restart;


static void advertising_start(void)
{
	int err = bt_adv_helper_adv_start(fp_discoverable);

	if (!err) {
		printk("%siscoverable advertising started\n",
		       fp_discoverable ? "D" : "Non-d");
	} else {
		printk("Advertising failed to start (err %d)\n", err);
	}
}

static void bt_adv_restart_fn(struct k_work *w)
{
	advertising_start();
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	/* Multiple simultaneous connections are not supported by the sample. */
	__ASSERT_NO_MSG(!peer);

	if (err) {
		printk("Connection failed (err %u)\n", err);
		k_work_submit(&bt_adv_restart);
		return;
	}

	printk("Connected\n");

	dk_set_led_on(CON_STATUS_LED);
	peer = conn;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	dk_set_led_off(CON_STATUS_LED);
	peer = NULL;

	k_work_submit(&bt_adv_restart);
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

static const char *volume_change_to_str(enum hids_helper_volume_change volume_change)
{
	const char *res = NULL;

	switch (volume_change) {
	case HIDS_HELPER_VOLUME_CHANGE_DOWN:
		res = "Decrease";
		break;

	case HIDS_HELPER_VOLUME_CHANGE_NONE:
		break;

	case HIDS_HELPER_VOLUME_CHANGE_UP:
		res = "Increase";
		break;

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
		break;
	}

	return res;
}

static void hid_volume_control_send(enum hids_helper_volume_change volume_change)
{
	const char *operation_str = volume_change_to_str(volume_change);
	int err = hids_helper_volume_ctrl(volume_change);

	if (!err) {
		if (operation_str) {
			printk("%s audio volume\n", operation_str);
		}
	} else if (err == -ENOTCONN) {
		/* HID host not connected. Silently drop HID data. */
	} else {
		printk("Failed to control audio volume (err %d)\n", err);
	}
}

static void volume_control_btn_handle(uint32_t button_state, uint32_t has_changed)
{
	static enum hids_helper_volume_change volume_change = HIDS_HELPER_VOLUME_CHANGE_NONE;
	enum hids_helper_volume_change new_volume_change = volume_change;

	if (has_changed & VOLUME_UP_BUTTON_MASK) {
		if (button_state & VOLUME_UP_BUTTON_MASK) {
			new_volume_change = HIDS_HELPER_VOLUME_CHANGE_UP;
		} else if (volume_change == HIDS_HELPER_VOLUME_CHANGE_UP) {
			new_volume_change = HIDS_HELPER_VOLUME_CHANGE_NONE;
		}
	}

	if (has_changed & VOLUME_DOWN_BUTTON_MASK) {
		if (button_state & VOLUME_DOWN_BUTTON_MASK) {
			new_volume_change = HIDS_HELPER_VOLUME_CHANGE_DOWN;
		} else if (volume_change == HIDS_HELPER_VOLUME_CHANGE_DOWN) {
			new_volume_change = HIDS_HELPER_VOLUME_CHANGE_NONE;
		}
	}

	if (volume_change != new_volume_change) {
		volume_change = new_volume_change;
		hid_volume_control_send(volume_change);
	}
}

static void fp_discoverable_btn_handle(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & button_state & FP_DISCOVERABLE_BUTTON_MASK) {
		fp_discoverable = !fp_discoverable;
		dk_set_led(FP_DISCOVERABLE_STATUS_LED, fp_discoverable);

		if (!peer) {
			advertising_start();
		}
	}
}

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	fp_discoverable_btn_handle(button_state, has_changed);
	volume_control_btn_handle(button_state, has_changed);
}

void main(void)
{
	bool run_led_on = true;
	int err;

	printk("Starting Bluetooth Fast Pair example\n");

	err = hids_helper_init();
	if (err) {
		printk("HIDS init failed (err %d)\n", err);
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

	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return;
	}

	k_work_init(&bt_adv_restart, bt_adv_restart_fn);

	dk_set_led(FP_DISCOVERABLE_STATUS_LED, fp_discoverable);
	advertising_start();

	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Buttons init failed (err %d)\n", err);
		return;
	}

	for (;;) {
		dk_set_led(RUN_STATUS_LED, run_led_on);
		run_led_on = !run_led_on;
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL_MS));
	}
}
