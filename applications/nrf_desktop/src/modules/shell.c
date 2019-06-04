/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <shell/shell.h>
#include <shell/shell_rtt.h>

#define MODULE shell
#include "module_state_event.h"
#include "power_event.h"

static bool event_handler(const struct event_header *eh)
{
	static bool enabled;

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;
			__ASSERT_NO_MSG(!initialized);
			shell_prompt_change(shell_backend_rtt_get_ptr(),
					    "nrf_desktop>");
			initialized = true;
			enabled = true;
			module_set_state(MODULE_STATE_READY);
		}
		return false;
	}

	if (is_wake_up_event(eh)) {
		if (!enabled) {
			shell_start(shell_backend_rtt_get_ptr());
			enabled = true;
			module_set_state(MODULE_STATE_READY);
		}
		return false;
	}

	if (is_power_down_event(eh)) {
		if (enabled) {
			shell_stop(shell_backend_rtt_get_ptr());
			enabled = false;
			module_set_state(MODULE_STATE_OFF);
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
