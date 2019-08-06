/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <misc/byteorder.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt_dm.h>

#define MODULE ble_discovery
#include "module_state_event.h"

#include "ble_event.h"
#include "ble_discovery_def.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_DISCOVERY_LOG_LEVEL);

static u16_t peer_vid;
static u16_t peer_pid;
static struct bt_conn *discovering_peer_conn;

static struct k_work hids_discovery_start;

#define VID_POS_IN_PNP_ID	sizeof(u8_t)
#define PID_POS_IN_PNP_ID	(VID_POS_IN_PNP_ID + sizeof(u16_t))
#define MIN_LEN_DIS_PNP_ID	(PID_POS_IN_PNP_ID + sizeof(u16_t))

static void peer_unpair(void)
{
	__ASSERT_NO_MSG(discovering_peer_conn != NULL);
	int err = bt_unpair(BT_ID_DEFAULT,
			    bt_conn_get_dst(discovering_peer_conn));

	if (err) {
		LOG_ERR("Error while unpairing peer (err:%d)", err);
		module_set_state(MODULE_STATE_ERROR);
	}
	bt_conn_unref(discovering_peer_conn);
	discovering_peer_conn = NULL;
}

static void hids_discovery_completed(struct bt_gatt_dm *dm, void *context)
{
	__ASSERT_NO_MSG(bt_gatt_dm_conn_get(dm) == discovering_peer_conn);
	BUILD_ASSERT_MSG(PEER_TYPE_COUNT <= __CHAR_BIT__, "");
	LOG_INF("HIDS discovery procedure succeeded");

	bt_gatt_dm_data_print(dm);

	struct ble_discovery_complete_event *event =
		new_ble_discovery_complete_event();

	event->dm = dm;
	event->pid = peer_pid;

	for (size_t i = 0; i < ARRAY_SIZE(bt_peripherals); i++) {
		if (bt_peripherals[i].pid == peer_pid) {
			event->peer_type = bt_peripherals[i].peer_type;
			EVENT_SUBMIT(event);
			/* This module is the last one to process this event
			 * and release the discovery data.
			 */
			return;
		}
	}

	/* Nothing was found. */
	LOG_ERR("Unrecognized peer");
	k_free(event);
	int err = bt_gatt_dm_data_release(dm);

	if (err) {
		LOG_ERR("Discovery data release failed (err:%d)", err);
		module_set_state(MODULE_STATE_ERROR);
	}

	peer_unpair();
}

static void hids_discovery_service_not_found(struct bt_conn *conn,
					     void *context)
{
	LOG_ERR("Cannot find HIDS during the discovery");
	__ASSERT_NO_MSG(discovering_peer_conn == conn);
	peer_unpair();
}

static void hids_discovery_error_found(struct bt_conn *conn, int err,
				       void *context)
{
	LOG_ERR("HIDS discovery failed (err:%d)", err);
	__ASSERT_NO_MSG(discovering_peer_conn == conn);
	peer_unpair();
}

static void start_hids_discovery(struct bt_conn *conn)
{
	static const struct bt_gatt_dm_cb discovery_cb = {
		.completed = hids_discovery_completed,
		.service_not_found = hids_discovery_service_not_found,
		.error_found = hids_discovery_error_found,
	};

	int err = bt_gatt_dm_start(conn, BT_UUID_HIDS, &discovery_cb, NULL);

	if (err == -EALREADY) {
		LOG_ERR("Discovery already in progress");
		module_set_state(MODULE_STATE_ERROR);
	} else if (err) {
		LOG_ERR("Cannot start the discovery (err:%d)", err);
		peer_unpair();
	}
}

static bool verify_peer(void)
{
	if (peer_vid != vendor_vid) {
		LOG_WRN("Invalid peripheral VID");
		return false;
	}

	return true;
}

static void hids_discovery_start_fn(struct k_work *w)
{
	if (!verify_peer()) {
		peer_unpair();
	} else {
		start_hids_discovery(discovering_peer_conn);
	}
}

static u8_t read_attr(struct bt_conn *conn, u8_t err,
	       struct bt_gatt_read_params *params,
	       const void *data, u16_t length)
{
	if (err) {
		LOG_ERR("Problem with reading GATT (err:%u)", err);
		peer_unpair();
		return BT_GATT_ITER_STOP;
	}

	__ASSERT_NO_MSG(length >= MIN_LEN_DIS_PNP_ID);
	const u8_t *data_ptr = data;

	peer_vid = sys_get_le16(&data_ptr[VID_POS_IN_PNP_ID]);
	peer_pid = sys_get_le16(&data_ptr[PID_POS_IN_PNP_ID]);

	LOG_INF("VID: %02x PID: %02x", peer_vid, peer_pid);

	k_work_submit(&hids_discovery_start);

	return BT_GATT_ITER_STOP;
}

static void dis_read_completed(struct bt_gatt_dm *dm, void *context)
{
	__ASSERT_NO_MSG(discovering_peer_conn == bt_gatt_dm_conn_get(dm));
	const struct bt_gatt_attr *attr;
	int err;

	attr = bt_gatt_dm_char_by_uuid(dm, BT_UUID_DIS_PNP_ID);

	if (!attr) {
		LOG_ERR("PnP ID characteristic not found - disconnecting");
		err = bt_gatt_dm_data_release(dm);
		if (err) {
			goto error;
		}
		peer_unpair();
		return;
	}

	static struct bt_gatt_read_params rp =  {
		.func = read_attr,
		.handle_count = 1,
		.single.offset = 0,
	};

	rp.single.handle = attr->handle + 1;

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		goto error;
	}

	bt_gatt_read(discovering_peer_conn, &rp);
	return;
error:
	LOG_ERR("Discovery data release failed (err:%d)", err);
	module_set_state(MODULE_STATE_ERROR);
}

static void dis_not_found(struct bt_conn *conn, void *context)
{
	LOG_ERR("DIS not found");
	__ASSERT_NO_MSG(discovering_peer_conn == conn);
	peer_unpair();
}

static void dis_read_error(struct bt_conn *conn, int err, void *context)
{
	LOG_ERR("Error discovering DIS (err:%d)", err);
	__ASSERT_NO_MSG(discovering_peer_conn == conn);
	peer_unpair();
}

static void start_dis_read(struct bt_conn *conn)
{
	static const struct bt_gatt_dm_cb dis_discovery_cb = {
		.completed = dis_read_completed,
		.service_not_found = dis_not_found,
		.error_found = dis_read_error,
	};

	int err = bt_gatt_dm_start(conn, BT_UUID_DIS, &dis_discovery_cb, NULL);

	if (err) {
		LOG_ERR("Cannot start DIS discovery (err:%d)", err);
		peer_unpair();
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_ble_peer_event(eh)) {
		const struct ble_peer_event *event =
			cast_ble_peer_event(eh);

		switch (event->state) {
		case PEER_STATE_SECURED:
			discovering_peer_conn = event->id;
			bt_conn_ref(discovering_peer_conn);

			start_dis_read(event->id);
			break;

		default:
			break;
		}
		return false;
	}

	if (is_ble_discovery_complete_event(eh)) {
		const struct ble_discovery_complete_event *event =
			cast_ble_discovery_complete_event(eh);

		int err = bt_gatt_dm_data_release(event->dm);

		if (err) {
			LOG_ERR("Discovery data release failed (err:%d)", err);
			module_set_state(MODULE_STATE_ERROR);
		}
		bt_conn_unref(discovering_peer_conn);
		discovering_peer_conn = NULL;

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			k_work_init(&hids_discovery_start,
				    hids_discovery_start_fn);
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);
	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
EVENT_SUBSCRIBE_FINAL(MODULE, ble_discovery_complete_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
