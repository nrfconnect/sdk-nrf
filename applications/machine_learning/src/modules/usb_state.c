/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <usb/usb_device.h>

#define MODULE usb_state
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_USB_STATE_LOG_LEVEL);


static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
		int err = usb_enable(NULL);

		if (!err) {
			module_set_state(MODULE_STATE_READY);
		} else {
			module_set_state(MODULE_STATE_ERROR);
		}
	}

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		return handle_module_state_event(cast_module_state_event(eh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
