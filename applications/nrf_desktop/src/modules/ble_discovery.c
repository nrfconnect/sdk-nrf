/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <sys/byteorder.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt_dm.h>

#define MODULE ble_discovery
#include "module_state_event.h"

#include "ble_event.h"
#include "ble_discovery_def.h"
#include "dev_descr.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_DISCOVERY_LOG_LEVEL);

enum discovery_state {
	DISCOVERY_STATE_START,
	DISCOVERY_STATE_DEV_DESCR,
	DISCOVERY_STATE_DIS,
	DISCOVERY_STATE_HIDS,

	DISCOVERY_STATE_COUNT,
};

static enum discovery_state state;

static u16_t peer_vid;
static u16_t peer_pid;
static bool peer_llpm_support;
static struct bt_conn *discovering_peer_conn;

static struct k_work next_discovery_step;

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
	state = DISCOVERY_STATE_START;
}

static void hids_discovery_completed(struct bt_gatt_dm *dm, void *context)
{
	__ASSERT_NO_MSG(dm != NULL);
	__ASSERT_NO_MSG(bt_gatt_dm_conn_get(dm) == discovering_peer_conn);
	BUILD_ASSERT_MSG(PEER_TYPE_COUNT <= __CHAR_BIT__, "");
	LOG_INF("HIDS discovery procedure succeeded");

	bt_gatt_dm_data_print(dm);

	struct ble_discovery_complete_event *event =
		new_ble_discovery_complete_event();

	event->dm = dm;
	event->pid = peer_pid;
	event->peer_llpm_support = peer_llpm_support;

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

static void service_not_found(struct bt_conn *conn, void *context)
{
	LOG_ERR("Service not found");
	__ASSERT_NO_MSG(discovering_peer_conn == conn);
	peer_unpair();
}

static void discovery_error(struct bt_conn *conn, int err, void *context)
{
	LOG_ERR("Error discovering peer (err:%d)", err);
	__ASSERT_NO_MSG(discovering_peer_conn == conn);
	peer_unpair();
}

static void read_dev_descr(const u8_t *ptr, u16_t len)
{
	__ASSERT_NO_MSG(len >= DEV_DESCR_LEN);
	peer_llpm_support = ptr[DEV_DESCR_LLPM_SUPPORT_POS];
	LOG_INF("LLPM %ssupported", peer_llpm_support ? ("") : ("not "));
}

static void read_dis(const u8_t *ptr, u16_t len)
{
	__ASSERT_NO_MSG(len >= MIN_LEN_DIS_PNP_ID);

	peer_vid = sys_get_le16(&ptr[VID_POS_IN_PNP_ID]);
	peer_pid = sys_get_le16(&ptr[PID_POS_IN_PNP_ID]);

	LOG_INF("VID: %02x PID: %02x", peer_vid, peer_pid);
}

static u8_t read_attr(struct bt_conn *conn, u8_t err,
		      struct bt_gatt_read_params *params,
		      const void *data, u16_t length)
{
	if (err) {
		LOG_ERR("Problem reading GATT (err:%" PRIu8 ")", err);
		peer_unpair();
		return BT_GATT_ITER_STOP;
	}

	__ASSERT_NO_MSG(data != NULL);
	const u8_t *data_ptr = data;

	switch (state) {
	case DISCOVERY_STATE_DEV_DESCR:
		read_dev_descr(data_ptr, length);
		break;

	case DISCOVERY_STATE_DIS:
		read_dis(data_ptr, length);
		break;

	default:
		__ASSERT_NO_MSG(false);
	}

	k_work_submit(&next_discovery_step);

	return BT_GATT_ITER_STOP;
}

static void discovery_completed(struct bt_gatt_dm *dm, void *context)
{
	__ASSERT_NO_MSG(dm != NULL);
	__ASSERT_NO_MSG(discovering_peer_conn == bt_gatt_dm_conn_get(dm));
	const struct bt_uuid *uuid = NULL;
	const struct bt_gatt_dm_attr *attr;
	int err;

	switch (state) {
	case DISCOVERY_STATE_DEV_DESCR:
		uuid = &custom_desc_chrc_uuid.uuid;
		break;

	case DISCOVERY_STATE_DIS:
		uuid = BT_UUID_DIS_PNP_ID;
		break;

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
	}

	attr = bt_gatt_dm_char_by_uuid(dm, uuid);

	if (!attr) {
		LOG_ERR("Characteristic not found - disconnecting");
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

	err = bt_gatt_read(bt_gatt_dm_conn_get(dm), &rp);
	if (err) {
		LOG_ERR("GATT read problem (err:%d)", err);
		peer_unpair();
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		goto error;
	}

	return;
error:
	LOG_ERR("Discovery data release failed (err:%d)", err);
	module_set_state(MODULE_STATE_ERROR);
}

static void start_discovery(struct bt_conn *conn,
			    const struct bt_uuid *svc_uuid,
			    bool discovery_hids)
{
	static const struct bt_gatt_dm_cb discovery_cb = {
		.completed = discovery_completed,
		.service_not_found = service_not_found,
		.error_found = discovery_error,
	};

	static const struct bt_gatt_dm_cb hids_discovery_cb = {
		.completed = hids_discovery_completed,
		.service_not_found = service_not_found,
		.error_found = discovery_error,
	};

	int err = bt_gatt_dm_start(conn, svc_uuid,
			discovery_hids ? &hids_discovery_cb : &discovery_cb,
			NULL);

	__ASSERT_NO_MSG(err != -EALREADY);
	__ASSERT_NO_MSG(err != -EINVAL);
	if (err) {
		LOG_ERR("Cannot start discovery (err:%d)", err);
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

static void next_discovery_step_fn(struct k_work *w)
{
	BUILD_ASSERT_MSG((DISCOVERY_STATE_HIDS + 1) == DISCOVERY_STATE_COUNT,
		"HIDs must be discovered last - after device is verified");

	state++;

	switch (state) {
	case DISCOVERY_STATE_DEV_DESCR:
		start_discovery(discovering_peer_conn, &custom_desc_uuid.uuid, false);
		break;

	case DISCOVERY_STATE_DIS:
		start_discovery(discovering_peer_conn, BT_UUID_DIS, false);
		break;

	case DISCOVERY_STATE_HIDS:
		if (!verify_peer()) {
			peer_unpair();
		} else {
			start_discovery(discovering_peer_conn, BT_UUID_HIDS, true);
		}
		break;

	case DISCOVERY_STATE_START:
	case DISCOVERY_STATE_COUNT:
	default:
		__ASSERT_NO_MSG(false);
	}
}

static void init(void)
{
	k_work_init(&next_discovery_step, next_discovery_step_fn);
	state = DISCOVERY_STATE_START;
	module_set_state(MODULE_STATE_READY);
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
			k_work_submit(&next_discovery_step);
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
		state = DISCOVERY_STATE_START;

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			init();
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
