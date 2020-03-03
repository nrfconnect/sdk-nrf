/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <power/reboot.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_vs.h>

#include "ble_event.h"
#include "passkey_event.h"

#ifdef CONFIG_BT_LL_NRFXLIB
#include "ble_controller_hci_vs.h"
#endif

#define MODULE ble_state
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_STATE_LOG_LEVEL);


struct bond_find_data {
	bt_addr_le_t peer_address;
	u8_t peer_id;
	u8_t peer_count;
};

static struct bt_conn *active_conn[CONFIG_BT_MAX_CONN];
static bool passkey_input;


static void bond_find(const struct bt_bond_info *info, void *user_data)
{
	struct bond_find_data *bond_find_data = user_data;

	if (bond_find_data->peer_id == bond_find_data->peer_count) {
		bt_addr_le_copy(&bond_find_data->peer_address, &info->addr);
	}

	__ASSERT_NO_MSG(bond_find_data->peer_count < UCHAR_MAX);
	bond_find_data->peer_count++;
}

static void disconnect_peer(struct bt_conn *conn)
{
	int err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	if (err == -ENOTCONN) {
		err = 0;
	}
	LOG_WRN("Device %s", err ? "failed to disconnect" : "disconnected");

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	}
}

void send_passkey_req(bool active)
{
	__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_BLE_ENABLE_PASSKEY));
	__ASSERT_NO_MSG(!passkey_input || !active);

	if (passkey_input != active) {
		struct passkey_req_event *event = new_passkey_req_event();

		event->active = active;
		EVENT_SUBMIT(event);

		passkey_input = active;
	}
}

static void set_tx_power(struct bt_conn *conn)
{
	u16_t conn_handle;

	int err = bt_hci_get_conn_handle(conn, &conn_handle);

	if (err) {
		LOG_ERR("No connection handle (err %d)", err);
	} else {
		struct bt_hci_cp_vs_write_tx_power_level *cp;
		struct bt_hci_rp_vs_write_tx_power_level *rp;
		struct net_buf *buf;
		struct net_buf *rsp = NULL;

		buf = bt_hci_cmd_create(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL,
					sizeof(*cp));
		if (!buf) {
			LOG_ERR("Cannot allocate buffer to set TX power");
			return;
		}

		cp = net_buf_add(buf, sizeof(*cp));
		cp->handle = sys_cpu_to_le16(conn_handle);
		cp->handle_type = BT_HCI_VS_LL_HANDLE_TYPE_CONN;
		cp->tx_power_level = CONFIG_DESKTOP_BLE_TX_PWR;

		LOG_INF("Setting TX power to: %" PRId8, cp->tx_power_level);

		err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL,
					   buf, &rsp);
		if (err) {
			u8_t reason = rsp ?
			  ((struct bt_hci_rp_vs_write_tx_power_level *)rsp->data)->status : 0;

			LOG_ERR("Cannot set TX power (err: %d reason 0x%02x)",
				err, reason);
		} else {
			rp = (struct bt_hci_rp_vs_write_tx_power_level *)rsp->data;
			LOG_INF("TX power returned by command: %" PRId8,
				rp->selected_tx_power);
		}

		if (rsp) {
			net_buf_unref(rsp);
		}
	}
}

static void connected(struct bt_conn *conn, u8_t error)
{
	/* Make sure that connection will remain valid. */
	bt_conn_ref(conn);

	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	if (error) {
		struct ble_peer_event *event = new_ble_peer_event();

		event->id = conn;
		event->state = PEER_STATE_CONN_FAILED;
		EVENT_SUBMIT(event);

		LOG_WRN("Failed to connect to %s (%u)", log_strdup(addr_str),
			error);
		return;
	}

	/* For nrfxlib LL TX power level has to be set using HCI command.
	 * The default value set in Kconfig has no effect.
	 */
	if (IS_ENABLED(CONFIG_BT_LL_NRFXLIB)) {
		set_tx_power(conn);
	}

	LOG_INF("Connected to %s", log_strdup(addr_str));

	size_t i;

	for (i = 0; i < ARRAY_SIZE(active_conn); i++) {
		if (!active_conn[i]) {
			break;
		}
	}
	if (i >= ARRAY_SIZE(active_conn)) {
		k_panic();
	}
	active_conn[i] = conn;

	struct ble_peer_event *event = new_ble_peer_event();

	event->id = conn;
	event->state = PEER_STATE_CONNECTED;
	EVENT_SUBMIT(event);

	struct bt_conn_info info;

	int err = bt_conn_get_info(conn, &info);

	if (err) {
		LOG_WRN("Cannot get conn info");
		goto disconnect;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    (info.role == BT_CONN_ROLE_SLAVE)) {
		struct bond_find_data bond_find_data = {
			.peer_id = 0,
			.peer_count = 0,
		};
		bt_foreach_bond(info.id, bond_find, &bond_find_data);

		LOG_INF("Identity %u has %u bonds", info.id,
			bond_find_data.peer_count);
		if ((bond_find_data.peer_count > 0) &&
		    bt_addr_le_cmp(bt_conn_get_dst(conn),
				   &bond_find_data.peer_address)) {
			bt_addr_le_to_str(&bond_find_data.peer_address, addr_str,
					sizeof(addr_str));
			LOG_INF("Already bonded to %s", log_strdup(addr_str));
			goto disconnect;
		}
		/* Security must be enabled after peer event is sent.
		 * This is to make sure notification events are propagated
		 * in the right order.
		 */

		LOG_INF("Set security level");
		err = bt_conn_set_security(conn, BT_SECURITY_L2);
		if (err) {
			LOG_ERR("Failed to set security");
			goto disconnect;
		}
	}

	return;

disconnect:
	disconnect_peer(conn);
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected from %s (reason %u)", log_strdup(addr), reason);

	size_t i;

	for (i = 0; i < ARRAY_SIZE(active_conn); i++) {
		if (active_conn[i] == conn) {
			break;
		}
	}

	if (i == ARRAY_SIZE(active_conn)) {
		__ASSERT_NO_MSG(false);
		return;
	}

	active_conn[i] = NULL;

	struct ble_peer_event *event = new_ble_peer_event();

	event->id = conn;
	event->state = PEER_STATE_DISCONNECTED;
	EVENT_SUBMIT(event);

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_ENABLE_PASSKEY)) {
		send_passkey_req(false);
	}
}

static struct bt_gatt_exchange_params exchange_params;

static void exchange_func(struct bt_conn *conn, u8_t err,
			  struct bt_gatt_exchange_params *params)
{
	LOG_INF("MTU exchange done");
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err bt_err)
{
	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		__ASSERT_NO_MSG(active_conn[0] == conn);
	}

	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!bt_err && (level >= BT_SECURITY_L2)) {
		LOG_INF("Security with %s level %u", log_strdup(addr), level);
	} else {
		LOG_WRN("Security with %s failed, level %u err %d",
			log_strdup(addr), level, bt_err);
		if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
			disconnect_peer(conn);
		}

		return;
	}

	struct ble_peer_event *event = new_ble_peer_event();
	event->id = conn;
	event->state = PEER_STATE_SECURED;
	EVENT_SUBMIT(event);

	struct bt_conn_info info;

	err = bt_conn_get_info(conn, &info);
	if (err) {
		LOG_WRN("Cannot get conn info");
	} else {
		if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
		    (info.role == BT_CONN_ROLE_MASTER)) {
			exchange_params.func = exchange_func;
			err = bt_gatt_exchange_mtu(conn, &exchange_params);
			if (err) {
				LOG_ERR("MTU exchange failed");
			}
		}
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	LOG_INF("Conn parameters request:"
		"\n\tinterval (0x%04x, 0x%04x)\n\tsl %d\n\ttimeout %d",
		param->interval_min, param->interval_max,
		param->latency, param->timeout);

	/* Accept the request */
	return true;
}

static void le_param_updated(struct bt_conn *conn, u16_t interval,
			     u16_t latency, u16_t timeout)
{
	LOG_INF("Conn parameters updated:"
		"\n\tinterval 0x%04x\n\tlat %d\n\ttimeout %d\n",
		interval, latency, timeout);
}

static void bt_ready(int err)
{
	if (err) {
		LOG_ERR("Bluetooth initialization failed (err %d)", err);
		sys_reboot(SYS_REBOOT_WARM);
	}

	LOG_INF("Bluetooth initialized");

#ifdef CONFIG_BT_LL_NRFXLIB
	hci_vs_cmd_llpm_mode_set_t *p_cmd_enable;

	struct net_buf *buf = bt_hci_cmd_create(HCI_VS_OPCODE_CMD_LLPM_MODE_SET,
						sizeof(*p_cmd_enable));

	p_cmd_enable = net_buf_add(buf, sizeof(*p_cmd_enable));
	p_cmd_enable->enable = 1;

	err = bt_hci_cmd_send_sync(HCI_VS_OPCODE_CMD_LLPM_MODE_SET, buf, NULL);
	if (err) {
		LOG_ERR("Error enabling LLPM (err: %d)", err);
	} else {
		LOG_INF("LLPM enabled");
	}
#endif /* CONFIG_BT_LL_NRFXLIB */

	module_set_state(MODULE_STATE_READY);
}

void auth_passkey_entry(struct bt_conn *conn)
{
	send_passkey_req(true);
	LOG_INF("Passkey input started");
}

void auth_cancel(struct bt_conn *conn)
{
	send_passkey_req(false);
	LOG_INF("Authentication cancelled");
}

static int ble_state_init(void)
{
	BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_PERIPHERAL) ||
		     (ARRAY_SIZE(active_conn) == 1));

	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
		.security_changed = security_changed,
		.le_param_req = le_param_req,
		.le_param_updated = le_param_updated,
	};
	bt_conn_cb_register(&conn_callbacks);

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_ENABLE_PASSKEY)) {
		static const struct bt_conn_auth_cb conn_auth_callbacks = {
			.passkey_entry = auth_passkey_entry,
			.cancel = auth_cancel,
		};

		bt_conn_auth_cb_register(&conn_auth_callbacks);
	}

	return bt_enable(bt_ready);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (ble_state_init()) {
				LOG_ERR("Cannot initialize");
			}
		}

		return false;
	}

	if (is_ble_peer_event(eh)) {
		const struct ble_peer_event *event = cast_ble_peer_event(eh);

		switch (event->state) {
		case PEER_STATE_CONN_FAILED:
		case PEER_STATE_DISCONNECTED:
			/* Connection object is no longer in use. */
			bt_conn_unref(event->id);
			break;
		default:
			/* Ignore. */
			break;
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_ENABLE_PASSKEY) &&
	    is_passkey_input_event(eh)) {
		const struct passkey_input_event *event =
			cast_passkey_input_event(eh);

		if (passkey_input) {
			int err = bt_conn_auth_passkey_entry(active_conn[0],
							     event->passkey);
			if (err) {
				LOG_ERR("Problem entering passkey (err %d)",
					err);
			}

			passkey_input = false;
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_DESKTOP_BLE_ENABLE_PASSKEY
EVENT_SUBSCRIBE(MODULE, passkey_input_event);
#endif /* CONFIG_DESKTOP_BLE_ENABLE_PASSKEY */
EVENT_SUBSCRIBE_FINAL(MODULE, ble_peer_event);
