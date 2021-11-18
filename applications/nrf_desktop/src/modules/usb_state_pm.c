/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr.h>
#include <zephyr/types.h>

#include "usb_event.h"

#define MODULE usb_state_pm
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_USB_STATE_LOG_LEVEL);

#include <caf/events/power_manager_event.h>
#include <caf/events/force_power_down_event.h>


static bool event_handler(const struct event_header *eh)
{
	if (is_usb_state_event(eh)) {
		const struct usb_state_event *event = cast_usb_state_event(eh);

		LOG_INF("USB state change detected");

		switch (event->state) {
		case USB_STATE_POWERED:
		case USB_STATE_ACTIVE:
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_ALIVE);
			break;
		case USB_STATE_DISCONNECTED:
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_MAX);
			break;
		case USB_STATE_SUSPENDED:
			LOG_INF("USB suspended");
			power_manager_restrict(MODULE_IDX(MODULE), POWER_MANAGER_LEVEL_SUSPENDED);
			force_power_down();
			break;
		default:
			/* Ignore */
			break;
		}
		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event = cast_module_state_event(eh);

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

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, usb_state_event);
