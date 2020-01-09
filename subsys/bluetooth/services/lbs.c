/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 *  @brief LED Button Service (LBS) sample
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/lbs.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(bt_gatt_lbs, CONFIG_BT_GATT_LBS_LOG_LEVEL);

static bool                   notify_enabled;
static bool                   button_state;
static struct bt_gatt_lbs_cb       lbs_cb;

#define BT_UUID_LBS           BT_UUID_DECLARE_128(LBS_UUID_SERVICE)
#define BT_UUID_LBS_BUTTON    BT_UUID_DECLARE_128(LBS_UUID_BUTTON_CHAR)
#define BT_UUID_LBS_LED       BT_UUID_DECLARE_128(LBS_UUID_LED_CHAR)

static void lbslc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				  u16_t value)
{
	notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}

static ssize_t write_led(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 const void *buf,
			 u16_t len, u16_t offset, u8_t flags)
{
	LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle, conn);

	if (lbs_cb.led_cb) {
		lbs_cb.led_cb(*(bool *)buf);
	}

	return len;
}

#ifdef CONFIG_BT_GATT_LBS_POLL_BUTTON
static ssize_t read_button(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  void *buf,
			  u16_t len,
			  u16_t offset)
{
	const char *value = attr->user_data;

	LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, conn);

	if (lbs_cb.button_cb) {
		button_state = lbs_cb.button_cb();
		return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
					 sizeof(*value));
	}

	return 0;
}
#endif

/* LED Button Service Declaration */
BT_GATT_SERVICE_DEFINE(lbs_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_LBS),
#ifdef CONFIG_BT_GATT_LBS_POLL_BUTTON
	BT_GATT_CHARACTERISTIC(BT_UUID_LBS_BUTTON,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_button, NULL,
			       &button_state),
#else
	BT_GATT_CHARACTERISTIC(BT_UUID_LBS_BUTTON,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, NULL, NULL, NULL),
#endif
	BT_GATT_CCC(lbslc_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_LBS_LED,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE,
			       NULL, write_led, NULL),
);

int bt_gatt_lbs_init(struct bt_gatt_lbs_cb *callbacks)
{
	if (callbacks) {
		lbs_cb.led_cb    = callbacks->led_cb;
		lbs_cb.button_cb = callbacks->button_cb;
	}

	return 0;
}

int bt_gatt_lbs_send_button_state(bool button_state)
{
	if (!notify_enabled) {
		return -EACCES;
	}

	return bt_gatt_notify(NULL, &lbs_svc.attrs[2],
			      &button_state,
			      sizeof(button_state));
}
