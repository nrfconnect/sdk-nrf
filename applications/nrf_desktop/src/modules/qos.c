/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

#include "chmap_filter.h"

#include "ble_event.h"

#define MODULE qos
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_QOS_LOG_LEVEL);


static struct bt_uuid_128 custom_desc_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x24470275, 0x4619, 0x177d, 0xd578, 0xb698e6e6654c));

static const struct bt_uuid_128 custom_desc_chrc_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x25470275, 0x4619, 0x177d, 0xd578, 0xb698e6e6654c));

static bool active;
static uint8_t chmap[CHMAP_BLE_BITMASK_SIZE];
static struct k_spinlock lock;

static void chmap_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	active = (value == BT_GATT_CCC_NOTIFY);
}

static ssize_t read_chmap(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  void *buf, uint16_t len, uint16_t offset)
{
	const void *value = attr->user_data;
	__ASSERT_NO_MSG(value == &chmap);

	uint8_t chmap_local[sizeof(chmap)];

	k_spinlock_key_t key = k_spin_lock(&lock);
	memcpy(chmap_local, chmap, sizeof(chmap));
	k_spin_unlock(&lock, key);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, chmap_local,
				 sizeof(chmap_local));
}

BT_GATT_SERVICE_DEFINE(qos_svc,
	BT_GATT_PRIMARY_SERVICE(&custom_desc_uuid),
	BT_GATT_CHARACTERISTIC(&custom_desc_chrc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       read_chmap, NULL, &chmap),
	BT_GATT_CCC(chmap_ccc_cfg_changed,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
);

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_ble_qos_event(aeh)) {
		struct ble_qos_event *event = cast_ble_qos_event(aeh);

		BUILD_ASSERT(sizeof(chmap) == sizeof(event->chmap));

		k_spinlock_key_t key = k_spin_lock(&lock);
		memcpy(chmap, event->chmap, sizeof(chmap));
		k_spin_unlock(&lock, key);

		if (active) {
			int err = bt_gatt_notify(NULL, &qos_svc.attrs[1],
						 &event->chmap,
						 sizeof(event->chmap));

			if (err) {
				LOG_ERR("GATT notify failed (err=%d)", err);
			}
		}

		return false;
	}

	if (is_module_state_event(aeh)) {
		struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
			static bool initialized;

			memset(chmap, 0xFF, sizeof(chmap));

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
APP_EVENT_SUBSCRIBE(MODULE, ble_qos_event);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
