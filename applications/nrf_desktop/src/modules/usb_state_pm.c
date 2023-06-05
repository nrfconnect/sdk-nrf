/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include "usb_event.h"

#define MODULE usb_state_pm
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_USB_STATE_LOG_LEVEL);

#include <caf/events/power_manager_event.h>
#include <caf/events/force_power_down_event.h>


static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_usb_state_event(aeh)) {
		const struct usb_state_event *event = cast_usb_state_event(aeh);

		LOG_DBG("USB state change detected");

		switch (event->state) {
		case USB_STATE_POWERED:
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_SUSPENDED);
			break;
		case USB_STATE_ACTIVE:
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_ALIVE);
			break;
		case USB_STATE_DISCONNECTED:
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_MAX);
			break;
		case USB_STATE_SUSPENDED:
			LOG_DBG("USB suspended");
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_SUSPENDED);
			force_power_down();
			break;
		default:
			/* Ignore */
			break;
		}
		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;
		}
		return false;
	}

	__ASSERT_NO_MSG(false);
	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, usb_state_event);
