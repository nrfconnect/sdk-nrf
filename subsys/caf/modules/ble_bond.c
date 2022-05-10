/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>

#define MODULE ble_bond
#include <caf/events/module_state_event.h>
#include <caf/events/click_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_BLE_BOND_LOG_LEVEL);

#define BOND_ERASE_CLICK	IS_ENABLED(CONFIG_CAF_BLE_BOND_PEER_ERASE_CLICK)
#if BOND_ERASE_CLICK
	#define BOND_ERASE_KEY_ID	CONFIG_CAF_BLE_BOND_PEER_ERASE_CLICK_KEY_ID
	#define BOND_ERASE_MAX_UPTIME	CONFIG_CAF_BLE_BOND_PEER_ERASE_CLICK_TIMEOUT
	#if CONFIG_CAF_BLE_BOND_PEER_ERASE_CLICK_SHORT
		#define BOND_ERASE_CLICK_TYPE	CLICK_SHORT
	#elif CONFIG_CAF_BLE_BOND_PEER_ERASE_CLICK_LONG
		#define BOND_ERASE_CLICK_TYPE	CLICK_LONG
	#elif CONFIG_CAF_BLE_BOND_PEER_ERASE_CLICK_DOUBLE
		#define BOND_ERASE_CLICK_TYPE	CLICK_DOUBLE
	#else
		#error "Click type is not defined"
	#endif
#else
	#define BOND_ERASE_KEY_ID	0xffff
	#define BOND_ERASE_MAX_UPTIME	0
	#define BOND_ERASE_CLICK_TYPE	CLICK_COUNT
#endif /* BOND_ERASE_CLICK */

static bool initialized;
static bool bond_erase_before_init;


static void bond_erase(void)
{
	int err = bt_unpair(BT_ID_DEFAULT, NULL);

	if (err) {
		LOG_ERR("Cannot unpair for default ID");
		module_set_state(MODULE_STATE_ERROR);
	} else {
		LOG_INF("Removed bond from default identity");
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(settings_loader), MODULE_STATE_READY)) {
			initialized = true;
			module_set_state(MODULE_STATE_READY);

			if (BOND_ERASE_CLICK && bond_erase_before_init) {
				bond_erase();
				bond_erase_before_init = false;
			}
		}

		return false;
	}

	if (BOND_ERASE_CLICK && is_click_event(aeh)) {
		const struct click_event *event = cast_click_event(aeh);

		if ((event->key_id == BOND_ERASE_KEY_ID) &&
		    (event->click == BOND_ERASE_CLICK_TYPE)) {

			if ((BOND_ERASE_MAX_UPTIME < 0) ||
			    (k_uptime_get() < BOND_ERASE_MAX_UPTIME)) {
				if (initialized) {
					bond_erase();
				} else {
					bond_erase_before_init = true;
				}
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#if BOND_ERASE_CLICK
	APP_EVENT_SUBSCRIBE(MODULE, click_event);
#endif /* BOND_ERASE_CLICK */
