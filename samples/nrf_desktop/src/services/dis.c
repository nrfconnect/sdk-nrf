/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <zephyr/types.h>

#include <bluetooth/services/dis.h>

#define MODULE dis
#include "module_state_event.h"

#include <logging/log.h>
#define LOG_LEVEL CONFIG_DESKTOP_LOG_DIS_LEVEL
LOG_MODULE_REGISTER(MODULE);


static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			dis_init();

			LOG_INF("service initialized");

			module_set_state(MODULE_STATE_READY);
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
