/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/conn.h>
#include <bluetooth/hci.h>
#include <bluetooth/gatt_dm.h>

#ifdef CONFIG_BT_LL_SOFTDEVICE
#include "sdc_hci_vs.h"
#endif /* CONFIG_BT_LL_SOFTDEVICE */

#define MODULE ble_conn_params
#include <caf/events/module_state_event.h>
#include <caf/events/ble_common_event.h>
#include "ble_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_CONN_PARAMS_LOG_LEVEL);

#define CONN_INTERVAL_LLPM_US		1000   /* 1 ms */
#define CONN_INTERVAL_LLPM_REG		0x0d01 /* 1 ms */
#if (CONFIG_CAF_BLE_USE_LLPM && (CONFIG_BT_MAX_CONN > 2))
 #define CONN_INTERVAL_BLE_REG		0x0008 /* 10 ms */
#else
 #define CONN_INTERVAL_BLE_REG		0x0006 /* 7.5 ms */
#endif
#define CONN_SUPERVISION_TIMEOUT	400

#define CONN_PARAMS_ERROR_TIMEOUT	K_MSEC(100)

struct connected_peer {
	struct bt_conn *conn;
	bool discovered;
	bool llpm_support;
	uint16_t requested_latency;
};

static struct connected_peer peers[CONFIG_BT_MAX_CONN];
static struct k_delayed_work conn_params_update;


static struct connected_peer *find_connected_peer(const struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(peers); i++) {
		if (peers[i].conn == conn) {
			return &peers[i];
		}
	}

	return NULL;
}

static int set_conn_params(struct bt_conn *conn, uint16_t conn_latency,
			   bool peer_llpm_support)
{
	int err;

#ifdef CONFIG_CAF_BLE_USE_LLPM
	if (peer_llpm_support) {
		struct net_buf *buf;
		sdc_hci_cmd_vs_conn_update_t *cmd_conn_update;
		uint16_t conn_handle;

		err = bt_hci_get_conn_handle(conn, &conn_handle);
		if (err) {
			LOG_ERR("Failed obtaining conn_handle (err:%d)", err);
			return err;
		}

		buf = bt_hci_cmd_create(SDC_HCI_OPCODE_CMD_VS_CONN_UPDATE,
					sizeof(*cmd_conn_update));
		if (!buf) {
			LOG_ERR("Could not allocate command buffer");
			return -ENOBUFS;
		}

		cmd_conn_update = net_buf_add(buf, sizeof(*cmd_conn_update));
		cmd_conn_update->connection_handle   = conn_handle;
		cmd_conn_update->conn_interval_us    = CONN_INTERVAL_LLPM_US;
		cmd_conn_update->conn_latency        = conn_latency;
		cmd_conn_update->supervision_timeout = CONN_SUPERVISION_TIMEOUT;

		err = bt_hci_cmd_send_sync(SDC_HCI_OPCODE_CMD_VS_CONN_UPDATE, buf,
					   NULL);
	} else
#endif /* CONFIG_CAF_BLE_USE_LLPM */
	{
		struct bt_le_conn_param param = {
			.interval_min = CONN_INTERVAL_BLE_REG,
			.interval_max = CONN_INTERVAL_BLE_REG,
			.latency = conn_latency,
			.timeout = CONN_SUPERVISION_TIMEOUT,
		};

		err = bt_conn_le_param_update(conn, &param);

		if (err == -EALREADY) {
			/* Connection parameters are already set. */
			err = 0;
		}
	}

	return err;
}

static void update_peer_conn_params(const struct connected_peer *peer)
{
	__ASSERT_NO_MSG(peer);
	/* Do not update peripheral's connection parameters before the discovery
	 * is completed.
	 */
	if (!peer->discovered) {
		return;
	}

	struct bt_conn *conn = peer->conn;
	uint16_t latency = peer->requested_latency;

	__ASSERT_NO_MSG(conn);
	int err = set_conn_params(conn, latency, peer->llpm_support);

	if (err) {
		LOG_WRN("Cannot update conn parameters for peer %p (err:%d)",
			peer, err);
		/* Retry to update the connection parameters after an error. */
		k_delayed_work_submit(&conn_params_update,
				      CONN_PARAMS_ERROR_TIMEOUT);
	} else {
		LOG_INF("Conn params for peer: %p set: %s, latency: %"PRIu16,
		  (void *)peer->conn,
		  (IS_ENABLED(CONFIG_CAF_BLE_USE_LLPM) && peer->llpm_support) ?
		  "LLPM" : "BLE", latency);
	}
}

static bool conn_params_update_required(struct connected_peer *peer)
{
	if (!peer->conn) {
		return false;
	}

	struct bt_conn_info info;
	int err = bt_conn_get_info(peer->conn, &info);

	if (err) {
		LOG_ERR("Cannot get conn info (%d)", err);
		return true;
	}

	__ASSERT_NO_MSG(info.role == BT_CONN_ROLE_MASTER);

	if ((peer->llpm_support && (info.le.interval != CONN_INTERVAL_LLPM_REG)) ||
	    (!peer->llpm_support && (info.le.interval != CONN_INTERVAL_BLE_REG)) ||
	    (info.le.latency != peer->requested_latency)) {
		return true;
	}

	return false;
}

static void conn_params_update_fn(struct k_work *work)
{
	__ASSERT_NO_MSG(work != NULL);

	for (size_t i = 0; i < ARRAY_SIZE(peers); i++) {
		if (conn_params_update_required(&peers[i])) {
			update_peer_conn_params(&peers[i]);
		}
	}
}

static void ble_peer_conn_params_event_handler(const struct ble_peer_conn_params_event *event)
{
	if (event->updated) {
		/* Ignore information that connection parameters were already
		 * updated.
		 */
		return;
	}

	struct connected_peer *peer = find_connected_peer(event->id);

	__ASSERT_NO_MSG(peer);
	peer->requested_latency = event->latency;
	k_delayed_work_submit(&conn_params_update, K_NO_WAIT);

	LOG_INF("Request to update conn: %p latency to: %"PRIu16,
		event->id, event->latency);
}

static void init(void)
{
	k_delayed_work_init(&conn_params_update, conn_params_update_fn);
}

static void peer_connected(struct bt_conn *conn)
{
	struct connected_peer *new_peer = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(peers); i++) {
		if (!peers[i].conn) {
			new_peer = &peers[i];
			break;
		}
	}
	__ASSERT_NO_MSG(new_peer);

	new_peer->conn = conn;
}

static void peer_disconnected(struct bt_conn *conn)
{
	struct connected_peer *peer = find_connected_peer(conn);

	if (peer) {
		peer->conn = NULL;
		peer->llpm_support = false;
		peer->discovered = false;
		peer->requested_latency = 0;
	}
}

static void peer_discovered(struct bt_conn *conn, bool peer_llpm_support)
{
	struct connected_peer *peer = find_connected_peer(conn);

	if (peer) {
		peer->llpm_support = peer_llpm_support;
		peer->discovered = true;
		update_peer_conn_params(peer);
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(ble_state),
				MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			init();
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (is_ble_discovery_complete_event(eh)) {
		const struct ble_discovery_complete_event *event =
			cast_ble_discovery_complete_event(eh);

		peer_discovered(bt_gatt_dm_conn_get(event->dm),
				event->peer_llpm_support);

		return false;
	}

	if (is_ble_peer_event(eh)) {
		const struct ble_peer_event *event =
			cast_ble_peer_event(eh);

		if (event->state == PEER_STATE_CONNECTED) {
			peer_connected(event->id);
		} else if (event->state == PEER_STATE_DISCONNECTED) {
			peer_disconnected(event->id);
		}

		return false;
	}

	if (is_ble_peer_conn_params_event(eh)) {
		ble_peer_conn_params_event_handler(
			cast_ble_peer_conn_params_event(eh));

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, ble_discovery_complete_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_conn_params_event);
