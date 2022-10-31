/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/adv_prov/fast_pair.h>

#define MODULE nrf_desktop_fast_pair
#include <caf/events/module_state_event.h>
#include <caf/events/ble_common_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_FAST_PAIR_LOG_LEVEL);

static uint8_t local_id = BT_ID_DEFAULT;


static void bond_cnt_cb(const struct bt_bond_info *info, void *user_data)
{
	size_t *cnt = user_data;

	(*cnt)++;
}

static inline size_t bond_cnt(uint8_t local_id)
{
	size_t cnt = 0;

	bt_foreach_bond(local_id, bond_cnt_cb, &cnt);

	return cnt;
}

static inline bool can_pair(uint8_t bt_local_id)
{
	return (bond_cnt(bt_local_id) < CONFIG_DESKTOP_FAST_PAIR_MAX_LOCAL_ID_BONDS);
}

static inline void update_fast_pair_ui_indication(void)
{
	bool show_ui_pairing = can_pair(local_id);

	bt_le_adv_prov_fast_pair_show_ui_pairing(show_ui_pairing);
	LOG_INF("%s Fast Pair not discoverable advertising UI indication",
		(show_ui_pairing) ? ("Show") : ("Hide"));
}

static bool handle_ble_peer_operation_event(const struct ble_peer_operation_event *event)
{
	switch (event->op) {
	case PEER_OPERATION_SELECTED:
	case PEER_OPERATION_ERASE_ADV:
	case PEER_OPERATION_ERASE_ADV_CANCEL:
		local_id = event->bt_stack_id;
		update_fast_pair_ui_indication();
		break;

	default:
		break;
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

static bool app_event_handler(const struct app_event_header *aeh)
{
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
APP_EVENT_SUBSCRIBE_EARLY(MODULE, ble_peer_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, ble_peer_operation_event);
