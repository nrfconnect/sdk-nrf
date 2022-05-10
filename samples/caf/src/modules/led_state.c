/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#define MODULE led_state
#include <caf/events/module_state_event.h>
#include <caf/events/led_event.h>
#include <caf/events/button_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_SAMPLE_LED_STATE_LOG_LEVEL);

#include "led_state_def.h"


enum button_id {
	BUTTON_ID_NEXT_EFFECT,
	BUTTON_ID_NEXT_LED,

	BUTTON_ID_COUNT
};

static enum led_effect_id curr_led_effect_id[LED_ID_COUNT];
static enum led_id curr_led_id;


static void send_led_event(enum led_id led_id, const struct led_effect *led_effect)
{
	__ASSERT_NO_MSG(led_effect);
	__ASSERT_NO_MSG(led_id < LED_ID_COUNT);

	struct led_event *event = new_led_event();

	event->led_id = led_id;
	event->led_effect = led_effect;
	APP_EVENT_SUBMIT(event);
}

static bool handle_button_event(const struct button_event *evt)
{
	if (evt->pressed) {
		switch (evt->key_id) {
		case BUTTON_ID_NEXT_LED:
			/* Send led effect for current LED. */
			send_led_event(curr_led_id, &led_effect[curr_led_effect_id[curr_led_id]]);
			/* Control next LED, signalize by turning on. */
			if (++curr_led_id == LED_ID_COUNT) {
				curr_led_id = LED_ID_0;
			}
			send_led_event(curr_led_id, &led_effect_on);
			break;
		case BUTTON_ID_NEXT_EFFECT:
			/* Change and display new effect. */
			if (++curr_led_effect_id[curr_led_id] == LED_EFFECT_ID_COUNT) {
				BUILD_ASSERT(LED_EFFECT_ID_OFF == 0);
				curr_led_effect_id[curr_led_id] = LED_EFFECT_ID_OFF;
			}
			send_led_event(curr_led_id, &led_effect[curr_led_effect_id[curr_led_id]]);
			break;
		default:
			break;
		}
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_button_event(aeh)) {
		return handle_button_event(cast_button_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(leds), MODULE_STATE_READY)) {
			/* Turn on the first LED */
			send_led_event(LED_ID_0, &led_effect_on);
		}

		return false;
	}

	/* Event not handled but subscribed. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, button_event);
