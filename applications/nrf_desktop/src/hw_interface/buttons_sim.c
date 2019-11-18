/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include "event_manager.h"
#include "button_event.h"
#include "power_event.h"

#define MODULE buttons_sim
#include "module_state_event.h"

#include "buttons_sim_def.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BUTTONS_LOG_LEVEL);

enum state {
	STATE_DISABLED,
	STATE_IDLE,
	STATE_ACTIVE,
	STATE_OFF,
};

static enum state state;
static struct k_delayed_work generate_keys;
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
		k_delayed_work_submit(&generate_keys, 0);
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
		k_delayed_work_cancel(&generate_keys);
		state = STATE_IDLE;
	}
}

static void send_button_event(u16_t key_id, bool pressed)
{
	struct button_event *event = new_button_event();

	event->key_id = key_id;
	event->pressed = pressed;
	EVENT_SUBMIT(event);
}

static bool gen_key_id(u16_t *generated_key)
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
	u16_t key_id;

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
		k_delayed_work_submit(&generate_keys,
				      CONFIG_DESKTOP_BUTTONS_SIM_INTERVAL);
	} else {
		state = STATE_IDLE;
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_button_event(eh)) {
		const struct button_event *event = cast_button_event(eh);

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

	if (is_module_state_event(eh)) {
		const struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			__ASSERT_NO_MSG(state == STATE_DISABLED);
			k_delayed_work_init(&generate_keys, generate_keys_fn);

			state = STATE_IDLE;
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (state != STATE_OFF) {
			k_delayed_work_cancel(&generate_keys);
			state = STATE_OFF;
			module_set_state(MODULE_STATE_OFF);
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
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

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, button_event);
EVENT_SUBSCRIBE(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
