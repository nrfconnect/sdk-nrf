/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

#include "module_state_event.h"

#define MODULE		dis
#define MODULE_NAME	STRINGIFY(MODULE)

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_DIS_LEVEL
#include <logging/sys_log.h>


static const char *dis_model = CONFIG_SOC;
static const char *dis_manuf = "Nordic Semiconductor";

static ssize_t read_model(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, dis_model,
				 strlen(dis_model));
}

static ssize_t read_manuf(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, dis_manuf,
				 strlen(dis_manuf));
}

/* Device Information Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_model, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME,
			       BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_manuf, NULL, NULL),
};


static int dis_init(void)
{
	static struct bt_gatt_service dis_svc = BT_GATT_SERVICE(attrs);

	return bt_gatt_service_register(&dis_svc);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, "ble_state", "ready")) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (dis_init()) {
				SYS_LOG_ERR("service init failed");

				return false;
			}
			SYS_LOG_INF("service initialized");

			module_set_state("ready");
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
