/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/gatt_dm.h>

#ifdef CONFIG_BT_LL_SOFTDEVICE
#include "sdc_hci_vs.h"
#endif /* CONFIG_BT_LL_SOFTDEVICE */

#define MODULE ble_conn_params
#include <caf/events/module_state_event.h>
#include <caf/events/ble_common_event.h>
#include "ble_event.h"

#include "usb_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_CONN_PARAMS_LOG_LEVEL);

#define CONN_INTERVAL_LLPM_US		1000   /* 1 ms */
#define CONN_INTERVAL_LLPM_REG		0x0d01 /* 1 ms */
#if (CONFIG_CAF_BLE_USE_LLPM && (CONFIG_BT_MAX_CONN >= 2))
 #define CONN_INTERVAL_BLE_REG		0x0008 /* 10 ms */
#else
 #define CONN_INTERVAL_BLE_REG		0x0006 /* 7.5 ms */
#endif
#define CONN_SUPERVISION_TIMEOUT	400

#define CONN_PARAMS_ERROR_TIMEOUT	K_MSEC(100)

#define CONN_INTERVAL_PRE_LLPM_MAX_US  10000U /* SoftDevice Controller Limitation DRGN-11297 */

#define CONN_INTERVAL_MS_TO_REG(_x)    (((_x) * USEC_PER_MSEC) / 1250U) /* REG = MS / 1,25 ms */
#define CONN_INTERVAL_USB_SUSPEND CONN_INTERVAL_MS_TO_REG(CONFIG_DESKTOP_BLE_USB_MANAGED_CI_VALUE)

struct connected_peer {
	struct bt_conn *conn;
	bool discovered;
	bool llpm_support;
	uint16_t requested_latency;
	bool conn_param_update_pending;
};

static struct connected_peer peers[CONFIG_BT_MAX_CONN];
static bool usb_suspended;

static void conn_params_update_fn(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(conn_params_update, conn_params_update_fn);


static struct connected_peer *find_connected_peer(const struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(peers); i++) {
		if (peers[i].conn == conn) {
			return &peers[i];
		}
	}

	return NULL;
}

static int set_le_conn_param(struct bt_conn *conn, uint16_t interval, uint16_t latency)
{
	int err;

	struct bt_le_conn_param param = {
		.interval_min = interval,
		.interval_max = interval,
		.latency = latency,
		.timeout = CONN_SUPERVISION_TIMEOUT,
	};

	err = bt_conn_le_param_update(conn, &param);

	if (err == -EALREADY) {
		/* Connection parameters are already set. */
		err = 0;
	}

	return err;
}

static int interval_reg_to_us(uint16_t reg)
{
	bool is_llpm = ((reg & 0x0d00) == 0x0d00) ? true : false;

	return (reg & BIT_MASK(8)) * ((is_llpm) ? 1000 : 1250);
}

static int set_llpm_conn_param(struct bt_conn *conn, uint16_t latency)
{
#ifdef CONFIG_BT_CTLR_SDC_LLPM
	struct net_buf *buf;
	sdc_hci_cmd_vs_conn_update_t *cmd_conn_update;
	uint16_t conn_handle;

	int err = bt_hci_get_conn_handle(conn, &conn_handle);

	if (err) {
		LOG_ERR("Failed obtaining conn_handle (err:%d)", err);
		return err;
	}

	buf = bt_hci_cmd_create(SDC_HCI_OPCODE_CMD_VS_CONN_UPDATE, sizeof(*cmd_conn_update));
	if (!buf) {
		LOG_ERR("Could not allocate command buffer");
		return -ENOBUFS;
	}

	cmd_conn_update = net_buf_add(buf, sizeof(*cmd_conn_update));
	cmd_conn_update->conn_handle = conn_handle;
	cmd_conn_update->conn_interval_us = CONN_INTERVAL_LLPM_US;
	cmd_conn_update->conn_latency = latency;
	cmd_conn_update->supervision_timeout = CONN_SUPERVISION_TIMEOUT;

	return bt_hci_cmd_send_sync(SDC_HCI_OPCODE_CMD_VS_CONN_UPDATE, buf, NULL);
#else
	__ASSERT_NO_MSG(false);
	return -ENOTSUP;
#endif /* CONFIG_BT_CTLR_SDC_LLPM */
}

static int set_conn_params(struct connected_peer *peer)
{
	int err;

	__ASSERT_NO_MSG(peer->conn);

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_USB_MANAGED_CI) && usb_suspended) {
		err = set_le_conn_param(peer->conn, CONN_INTERVAL_USB_SUSPEND, 0);
	} else if (IS_ENABLED(CONFIG_CAF_BLE_USE_LLPM) && peer->llpm_support) {
		struct bt_conn_info info;

		err = bt_conn_get_info(peer->conn, &info);
		if (err) {
			LOG_ERR("Cannot get conn info (%d)", err);
			return err;
		}
		uint32_t curr_ci_us = interval_reg_to_us(info.le.interval);

		if (curr_ci_us > CONN_INTERVAL_PRE_LLPM_MAX_US) {
			err =  set_le_conn_param(peer->conn, CONN_INTERVAL_BLE_REG,
						 peer->requested_latency);
			peer->conn_param_update_pending = true;
		} else {
			err = set_llpm_conn_param(peer->conn, peer->requested_latency);
		}
	} else {
		err = set_le_conn_param(peer->conn, CONN_INTERVAL_BLE_REG, peer->requested_latency);
	}

	return err;
}

static void update_peer_conn_params(struct connected_peer *peer)
{
	__ASSERT_NO_MSG(peer);

	/* Do not update peripheral's connection parameters before the discovery
	 * is completed or when conn param update is already pending.
	 */
	if (!peer->discovered || peer->conn_param_update_pending) {
		return;
	}

	int err = set_conn_params(peer);

	if (err) {
		LOG_WRN("Cannot update conn parameters for peer %p (err:%d)",
			(void *)peer->conn, err);
		/* Retry to update the connection parameters after an error. */
		k_work_reschedule(&conn_params_update, CONN_PARAMS_ERROR_TIMEOUT);
	} else {
		LOG_INF("Conn params for peer: %p set: %s, latency: %"PRIu16,
			(void *)peer->conn,
			(IS_ENABLED(CONFIG_CAF_BLE_USE_LLPM) && peer->llpm_support) ?
			"LLPM" : "BLE", peer->requested_latency);
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

	__ASSERT_NO_MSG(info.role == BT_CONN_ROLE_CENTRAL);

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_USB_MANAGED_CI) && usb_suspended) {
		if (info.le.interval != CONN_INTERVAL_USB_SUSPEND) {
			return true;
		}
	} else if ((peer->llpm_support && (info.le.interval != CONN_INTERVAL_LLPM_REG)) ||
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
	struct connected_peer *peer = find_connected_peer(event->id);

	__ASSERT_NO_MSG(peer);

	if (event->updated) {
		peer->conn_param_update_pending = false;
		LOG_INF("Conn params for peer: %p updated.", (void *)peer->conn);
	} else {
		peer->requested_latency = event->latency;
		LOG_INF("Request to update conn: %p latency to: %"PRIu16, event->id,
			event->latency);
	}

	k_work_reschedule(&conn_params_update, K_NO_WAIT);
}

static void usb_state_event_handler(enum usb_state new_state)
{
	switch (new_state) {
	case USB_STATE_SUSPENDED:
		usb_suspended = true;
		break;

	case USB_STATE_ACTIVE:
		usb_suspended = false;
		break;

	default:
		/* Ignore other USB state events */
		return;
	}

	k_work_reschedule(&conn_params_update, K_NO_WAIT);
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
		peer->conn_param_update_pending = false;
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

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (is_ble_discovery_complete_event(aeh)) {
		const struct ble_discovery_complete_event *event =
			cast_ble_discovery_complete_event(aeh);

		peer_discovered(bt_gatt_dm_conn_get(event->dm),
				event->peer_llpm_support);

		return false;
	}

	if (is_ble_peer_event(aeh)) {
		const struct ble_peer_event *event =
			cast_ble_peer_event(aeh);

		if (event->state == PEER_STATE_CONNECTED) {
			peer_connected(event->id);
		} else if (event->state == PEER_STATE_DISCONNECTED) {
			peer_disconnected(event->id);
		}

		return false;
	}

	if (is_ble_peer_conn_params_event(aeh)) {
		ble_peer_conn_params_event_handler(
			cast_ble_peer_conn_params_event(aeh));

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_USB_MANAGED_CI) &&
	    is_usb_state_event(aeh)) {
		struct usb_state_event *event = cast_usb_state_event(aeh);

		usb_state_event_handler(event->state);

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_discovery_complete_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_conn_params_event);
#ifdef CONFIG_DESKTOP_BLE_USB_MANAGED_CI
APP_EVENT_SUBSCRIBE(MODULE, usb_state_event);
#endif
