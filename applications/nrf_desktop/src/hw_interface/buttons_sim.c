/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <app_event_manager.h>
#include <caf/events/button_event.h>
#include <caf/events/power_event.h>

#define MODULE buttons_sim
#include <caf/events/module_state_event.h>

#include "buttons_sim_def.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BUTTONS_SIM_LOG_LEVEL);

enum state {
	STATE_DISABLED,
	STATE_IDLE,
	STATE_ACTIVE,
	STATE_OFF,
};

static enum state state;
static struct k_work_delayable generate_keys;
static size_t cur_key_idx;


static void start_generating_keys(void)
{
	__ASSERT_NO_MSG((state == STATE_ACTIVE) ||
			(state == STATE_IDLE));

	if (state == STATE_ACTIVE) {
		LOG_INF("Already generating key presses");
	} else {
		LOG_INF("Start generating key presses");
		cur_key_idx = 0;
		k_work_reschedule(&generate_keys, K_NO_WAIT);
		state = STATE_ACTIVE;
	}
}

static void stop_generating_keys(void)
{
	__ASSERT_NO_MSG((state == STATE_ACTIVE) ||
			(state == STATE_IDLE));

	if (state == STATE_IDLE) {
		LOG_INF("Already not generating key presses");
	} else {
		LOG_INF("Stop generating key presses");
		/* Cancel cannot fail if executed from another work's context. */
		(void)k_work_cancel_delayable(&generate_keys);
		state = STATE_IDLE;
	}
}

static void send_button_event(uint16_t key_id, bool pressed)
{
	struct button_event *event = new_button_event();

	event->key_id = key_id;
	event->pressed = pressed;
	APP_EVENT_SUBMIT(event);
}

static bool gen_key_id(uint16_t *generated_key)
{
	if (cur_key_idx >= ARRAY_SIZE(simulated_key_sequence)) {
		cur_key_idx = 0;
		if (!IS_ENABLED(CONFIG_DESKTOP_BUTTONS_SIM_LOOP_FOREVER)) {
			return false;
		}
	}

	*generated_key = simulated_key_sequence[cur_key_idx];
	cur_key_idx++;

	return true;
}

static bool generate_button_event(void)
{
	uint16_t key_id;

	if (!gen_key_id(&key_id)) {
		return false;
	}

	send_button_event(key_id, true);
	send_button_event(key_id, false);

	return true;
}

void generate_keys_fn(struct k_work *w)
{
	if (generate_button_event()) {
		k_work_reschedule(&generate_keys,
				K_MSEC(CONFIG_DESKTOP_BUTTONS_SIM_INTERVAL));
	} else {
		state = STATE_IDLE;
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_button_event(aeh)) {
		const struct button_event *event = cast_button_event(aeh);

		if (event->key_id == CONFIG_DESKTOP_BUTTONS_SIM_TRIGGER_KEY_ID) {
			if (event->pressed) {
				switch (state) {
				case STATE_IDLE:
					start_generating_keys();
					break;

				case STATE_ACTIVE:
					stop_generating_keys();
					break;

				case STATE_DISABLED:
				case STATE_OFF:
					break;

				default:
					__ASSERT_NO_MSG(false);
				}
			}

			return true;
		}

		return false;
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			__ASSERT_NO_MSG(state == STATE_DISABLED);
			k_work_init_delayable(&generate_keys, generate_keys_fn);

			state = STATE_IDLE;
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (is_power_down_event(aeh)) {
		if (state != STATE_OFF) {
			/* Cancel cannot fail if executed from another work's context. */
			(void)k_work_cancel_delayable(&generate_keys);
			state = STATE_OFF;
			module_set_state(MODULE_STATE_OFF);
		}

		return false;
	}

	if (is_wake_up_event(aeh)) {
		if (state == STATE_OFF) {
			state = STATE_IDLE;
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, button_event);
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
