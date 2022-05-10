/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

#include "battery_event.h"

#define MODULE bas
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BAS_LOG_LEVEL);


static bool active;
static uint8_t battery = 100;

static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	active = (value == BT_GATT_CCC_NOTIFY);
}

static ssize_t read_blvl(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(*value));
}

/* Battery Service Declaration */
BT_GATT_SERVICE_DEFINE(bas_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       read_blvl, NULL, &battery),
	BT_GATT_CCC(blvl_ccc_cfg_changed,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
);

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_battery_level_event(aeh)) {
		struct battery_level_event *event = cast_battery_level_event(aeh);

		battery = event->level;

		if (active) {
			int err = bt_gatt_notify(NULL, &bas_svc.attrs[1],
						 &battery, sizeof(battery));

			if (err == -ENOTCONN) {
				LOG_WRN("Cannot notify. Peer disconnecting.");
			} else if (err) {
				LOG_ERR("GATT notify failed (err=%d)", err);
			}
		}

		return false;
	}

	if (is_module_state_event(aeh)) {
		struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			module_set_state(MODULE_STATE_READY);
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, battery_level_event);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
