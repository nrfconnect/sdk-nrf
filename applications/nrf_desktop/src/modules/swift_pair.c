/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/adv_prov/swift_pair.h>

#define MODULE swift_pair_app
#include <caf/events/ble_common_event.h>
#include "ble_dongle_peer_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_SWIFT_PAIR_LOG_LEVEL);

#define DEFAULT_APP_ID	0
#define INVALID_APP_ID	UINT8_MAX

static uint8_t dongle_app_id = INVALID_APP_ID;


static void update_swift_pair_payload(uint8_t selected_app_id)
{
	bool enable;

	if (selected_app_id == dongle_app_id) {
		enable = IS_ENABLED(CONFIG_DESKTOP_SWIFT_PAIR_ADV_DONGLE_PEER);
	} else {
		enable = IS_ENABLED(CONFIG_DESKTOP_SWIFT_PAIR_ADV_GENERAL_PEER);
	}

	bt_le_adv_prov_swift_pair_enable(enable);
	LOG_DBG("Swift Pair payload %sabled", enable ? "en" : "dis");
}

static bool handle_ble_dongle_peer_event(const struct ble_dongle_peer_event *event)
{
	__ASSERT_NO_MSG(event->bt_app_id != INVALID_APP_ID);

	dongle_app_id = event->bt_app_id;
	update_swift_pair_payload(DEFAULT_APP_ID);

	return false;
}

static bool handle_ble_peer_operation_event(const struct ble_peer_operation_event *event)
{
	/* Module must know the dongle peer's application local identity before peer operations. */
	__ASSERT_NO_MSG(dongle_app_id != INVALID_APP_ID);

	switch (event->op) {
	case PEER_OPERATION_SELECTED:
	case PEER_OPERATION_ERASE_ADV:
	case PEER_OPERATION_ERASE_ADV_CANCEL:
		update_swift_pair_payload(event->bt_app_id);
		break;

	default:
		break;
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_ble_dongle_peer_event(aeh)) {
		return handle_ble_dongle_peer_event(cast_ble_dongle_peer_event(aeh));
	}

	if (is_ble_peer_operation_event(aeh)) {
		return handle_ble_peer_operation_event(cast_ble_peer_operation_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, ble_dongle_peer_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, ble_peer_operation_event);
