/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <string.h>

#include <bluetooth/services/nsms.h>
#include "ml_result_event.h"

#define MODULE ei_state_nsms

#include <zephyr/logging/log.h>

BT_NSMS_DEF(nsms_gesture, "Gesture", CONFIG_ML_APP_EI_RESULT_SECURITY_LEVEL, "Unknown", 16);

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

#if IS_ENABLED(CONFIG_BT_STATUS_SECURITY_ENABLED)
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
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected	= connected,
	.disconnected	= disconnected,
#if IS_ENABLED(CONFIG_BT_STATUS_SECURITY_ENABLED)
	.security_changed = security_changed,
#endif
};

static void guesture_changed(const char *label)
{
	static const char *last_label;

	if (label != last_label) {
		/* Contents under `label` pointer are copied
		 * by bt_nsms_set_status().
		 */
		last_label = label;
		bt_nsms_set_status(&nsms_gesture, label);
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_ml_result_event(aeh)) {
		/* Accessing event data. */
		struct ml_result_event *event = cast_ml_result_event(aeh);

		guesture_changed(event->label);

		return false;
	}

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, ml_result_event);
