/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include "led_state_def.h"

#include <caf/events/led_event.h>
#include "ml_state_event.h"

#define MODULE led_state
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_LED_STATE_LOG_LEVEL);


static bool handle_ml_state_event(const struct ml_state_event *event)
{
	__ASSERT_NO_MSG(event->state < ARRAY_SIZE(ml_state_led_effect));

	struct led_event *led_event = new_led_event();

	led_event->led_id = ml_state_led_id;
	led_event->led_effect = &ml_state_led_effect[event->state];
	EVENT_SUBMIT(led_event);

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_ml_state_event(eh)) {
		return handle_ml_state_event(cast_ml_state_event(eh));
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			module_set_state(MODULE_STATE_READY);
			initialized = true;
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, ml_state_event);
