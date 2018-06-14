/** @file
 *  @brief LED Button Service (LBS) sample
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <gatt/lbs.h>

static struct bt_gatt_ccc_cfg lbslc_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t                   notify_enabled;
static void (*lbs_write_cb)(u8_t);

#define BT_UUID_LBS           BT_UUID_DECLARE_128(LBS_UUID_SERVICE)
#define BT_UUID_LBS_BUTTON    BT_UUID_DECLARE_128(LBS_UUID_BUTTON_CHAR)
#define BT_UUID_LBS_LED       BT_UUID_DECLARE_128(LBS_UUID_LED_CHAR)

static void lbslc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t write_led(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset,
			u8_t flags)
{
	printk("LED write (value %i)\n", *(u8_t*)buf);
	lbs_write_cb(*(u8_t*)buf);

	return len;
}

/* Heart Rate Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_LBS),
	BT_GATT_CHARACTERISTIC(BT_UUID_LBS_BUTTON,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, NULL, NULL, NULL),
	BT_GATT_CCC(lbslc_ccc_cfg, lbslc_ccc_cfg_changed),
	BT_GATT_CHARACTERISTIC(BT_UUID_LBS_LED,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       NULL, write_led, NULL),
};

static struct bt_gatt_service lbs_svc = BT_GATT_SERVICE(attrs);

void lbs_init(void *led_cb)
{
	lbs_write_cb = led_cb;
	bt_gatt_service_register(&lbs_svc);
}

void lbs_notify(u8_t button_state)
{
	if (!notify_enabled) {
		return;
	}

	bt_gatt_notify(NULL, &attrs[2], &button_state, sizeof(button_state));
}
