/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/conn.h>
#include <bluetooth/adv_prov/fast_pair.h>

#define MODULE fast_pair_app
#include <caf/events/module_state_event.h>
#include <caf/events/ble_common_event.h>
#include "ble_dongle_peer_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_FAST_PAIR_LOG_LEVEL);

static uint8_t selected_stack_id = BT_ID_DEFAULT;
static uint8_t dongle_app_id = UINT8_MAX;
static bool fp_provider_enabled = true;


static void bond_cnt_cb(const struct bt_bond_info *info, void *user_data)
{
	size_t *cnt = user_data;

	(*cnt)++;
}

static size_t bond_cnt(uint8_t local_id)
{
	size_t cnt = 0;

	bt_foreach_bond(local_id, bond_cnt_cb, &cnt);

	return cnt;
}

static bool can_pair(uint8_t bt_local_id)
{
	return (bond_cnt(bt_local_id) < CONFIG_DESKTOP_FAST_PAIR_MAX_LOCAL_ID_BONDS);
}

static bool in_pairing_mode(uint8_t bt_local_id)
{
	/* Implementation assumes that given local identity advertises in pairing mode until
	 * it has a bonded peer.
	 */
	return (bond_cnt(bt_local_id) == 0);
}

static void update_fast_pair_ui_indication(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_BLE_DONGLE_PEER_ID_INFO) && !fp_provider_enabled) {
		return;
	}

	bool show_ui_pairing = can_pair(selected_stack_id);
	bt_le_adv_prov_fast_pair_show_ui_pairing(show_ui_pairing);

	if (in_pairing_mode(selected_stack_id)) {
		LOG_INF("Fast Pair discoverable advertising");
	} else {
		LOG_INF("%s Fast Pair not discoverable advertising UI indication",
			(show_ui_pairing) ? ("Show") : ("Hide"));
	}
}

static void enable_fast_pair_provider(bool enable)
{
	fp_provider_enabled = enable;
	bt_le_adv_prov_fast_pair_enable(enable);
}

static void control_fast_pair_provider(uint8_t app_id, uint8_t stack_id)
{
	if (IS_ENABLED(CONFIG_DESKTOP_BLE_DONGLE_PEER_ID_INFO)) {
		__ASSERT_NO_MSG(dongle_app_id != UINT8_MAX);

		enable_fast_pair_provider(app_id != dongle_app_id);
	}

	selected_stack_id = stack_id;
	update_fast_pair_ui_indication();
}

static enum bt_security_err pairing_accept(struct bt_conn *conn,
					   const struct bt_conn_pairing_feat *const feat)
{
	ARG_UNUSED(feat);

	enum bt_security_err ret;
	struct bt_conn_info info;
	int err = bt_conn_get_info(conn, &info);

	if (err) {
		LOG_ERR("Cannot get conn info (err %d)", err);
		return BT_SECURITY_ERR_UNSPECIFIED;
	}

	if (in_pairing_mode(info.id)) {
		LOG_INF("Accept normal Bluetooth pairing");
		ret = BT_SECURITY_ERR_SUCCESS;
	} else {
		LOG_WRN("Normal Bluetooth pairing not allowed outside of pairing mode");
		ret = BT_SECURITY_ERR_PAIR_NOT_ALLOWED;
	}

	return ret;
}

static int register_app_auth_cbs(void)
{
	if (!IS_ENABLED(CONFIG_DESKTOP_FAST_PAIR_LIMIT_NORMAL_PAIRING)) {
		return 0;
	}

	/* If the peer follows the Fast Pair pairing flow, the Fast Pair service overlays the
	 * Bluetooth authentication callbacks and takes over Bluetooth pairing. This allows the
	 * service to perform all of the required operations without interacting with the
	 * application.
	 */
	static const struct bt_conn_auth_cb conn_auth_callbacks = {
		.pairing_accept = pairing_accept,
	};

	return bt_conn_auth_cb_register(&conn_auth_callbacks);
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
		int err = register_app_auth_cbs();

		if (!err) {
			module_set_state(MODULE_STATE_READY);
		} else {
			LOG_ERR("Cannot register authentication callbacks (err %d)", err);
			module_set_state(MODULE_STATE_ERROR);
		}
	}

	return false;
}

static bool handle_ble_dongle_peer_event(const struct ble_dongle_peer_event *event)
{
	dongle_app_id = event->bt_app_id;

	if (dongle_app_id == 0) {
		enable_fast_pair_provider(false);
	}

	return false;
}

static bool handle_ble_peer_event(const struct ble_peer_event *event)
{
	if (event->state == PEER_STATE_DISCONNECTED) {
		update_fast_pair_ui_indication();
	}

	return false;
}

static bool handle_ble_peer_operation_event(const struct ble_peer_operation_event *event)
{
	switch (event->op) {
	case PEER_OPERATION_SELECTED:
	case PEER_OPERATION_ERASE_ADV:
	case PEER_OPERATION_ERASE_ADV_CANCEL:
		control_fast_pair_provider(event->bt_app_id, event->bt_stack_id);
		break;

	default:
		break;
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		return handle_module_state_event(cast_module_state_event(aeh));
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_DONGLE_PEER_ID_INFO) && is_ble_dongle_peer_event(aeh)) {
		return handle_ble_dongle_peer_event(cast_ble_dongle_peer_event(aeh));
	}

	if (is_ble_peer_event(aeh)) {
		return handle_ble_peer_event(cast_ble_peer_event(aeh));
	}

	if (is_ble_peer_operation_event(aeh)) {
		return handle_ble_peer_operation_event(cast_ble_peer_operation_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
#if IS_ENABLED(CONFIG_DESKTOP_BLE_DONGLE_PEER_ID_INFO)
APP_EVENT_SUBSCRIBE(MODULE, ble_dongle_peer_event);
#endif
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, ble_peer_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, ble_peer_operation_event);
