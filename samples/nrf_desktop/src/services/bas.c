/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

#include "battery_event.h"

#define MODULE bas
#include "module_state_event.h"

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_BAS_LEVEL
#include <logging/sys_log.h>


static struct bt_gatt_ccc_cfg blvl_ccc_cfg[BT_GATT_CCC_MAX];

static bool active;
static u8_t battery = 100; /* TODO use atomic */

static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	active = (value == BT_GATT_CCC_NOTIFY);
}

static ssize_t read_blvl(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(*value));
}

/* Battery Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_blvl, NULL, &battery),
	BT_GATT_CCC(blvl_ccc_cfg, blvl_ccc_cfg_changed),
};

static int bas_init(void)
{
	static struct bt_gatt_service bas_svc = BT_GATT_SERVICE(attrs);

	return bt_gatt_service_register(&bas_svc);
}

static bool event_handler(const struct event_header *eh)
{
	/* TODO subscribe to connection events and handle them. */

	if (is_battery_level_event(eh)) {
		struct battery_level_event *event = cast_battery_level_event(eh);

		battery = event->level;

		if (active) {
			bt_gatt_notify(NULL, &attrs[1], &battery,
					sizeof(battery));
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (bas_init()) {
				SYS_LOG_ERR("service init failed");

				return false;
			}
			SYS_LOG_INF("service initialized");

			module_set_state(MODULE_STATE_READY);
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, battery_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
