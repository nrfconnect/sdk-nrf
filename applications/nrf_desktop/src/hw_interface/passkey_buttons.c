/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#define MODULE passkey
#include <caf/events/module_state_event.h>

#include <caf/events/button_event.h>
#include "passkey_event.h"

#include "passkey_buttons_def.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_PASSKEY_LOG_LEVEL);

#define CLEAR_INPUT_ON_HOLD_TIMEOUT (2 * MSEC_PER_SEC) /* ms */

enum state {
	STATE_DISABLED,
	STATE_IDLE,
	STATE_ACTIVE,
};

struct input_buf {
	uint8_t buf[CONFIG_DESKTOP_PASSKEY_MAX_LEN];
	uint8_t cnt;
};

static enum state state;
static struct input_buf digits;


static uint32_t encode_input(struct input_buf *buffer)
{
	uint32_t res = 0;
	uint32_t mult = 1;

	/* Translate input_buf { buf=[5,4], cnt=2} into the uint32_t value 54. */
	for (size_t i = buffer->cnt; i > 0; i--) {
		res += buffer->buf[i - 1] * mult;
		mult *= DIGIT_CNT;
	}

	return res;
}

static void input_confirm(struct input_buf *buffer)
{
	struct passkey_input_event *event = new_passkey_input_event();

	__ASSERT_NO_MSG(sizeof(event->passkey) == sizeof(uint32_t));

	event->passkey = encode_input(buffer);
	APP_EVENT_SUBMIT(event);

	state = STATE_IDLE;
}

static void clear_elems(struct input_buf *buffer)
{
	buffer->cnt = 0;
}

static void remove_elem(struct input_buf *buffer)
{
	if (buffer->cnt > 0) {
		buffer->cnt--;
	}
}

static void add_elem(struct input_buf *buffer, uint8_t new_elem)
{
	if (buffer->cnt < ARRAY_SIZE(buffer->buf)) {
		buffer->buf[buffer->cnt] = new_elem;
		buffer->cnt++;
	} else {
		LOG_WRN("Too long passkey input. Key dropped.");
	}
}

static bool update_digit_input_key(struct input_buf *input,
				   uint16_t in_key_id, bool pressed)
{
	if (!pressed) {
		return false;
	}

	for (size_t i = 0; i < ARRAY_SIZE(input_configs); i++) {
		const struct passkey_input_config *cfg = &input_configs[i];

		for (size_t j = 0; j < ARRAY_SIZE(cfg->key_ids); j++) {
			if (in_key_id == cfg->key_ids[j]) {
				add_elem(input, j);
				return true;
			}
		}
	}

	return false;
}

static bool update_delete_key(struct input_buf *input,
			      uint16_t in_key_id, bool pressed)
{
	static int64_t delete_press_times[ARRAY_SIZE(delete_keys)];

	for (size_t i = 0; i < ARRAY_SIZE(delete_keys); i++) {
		if (in_key_id == delete_keys[i]) {
			if (pressed) {
				remove_elem(input);
				__ASSERT_NO_MSG(delete_press_times[i] == 0);
				delete_press_times[i] = k_uptime_get();

			} else {
				int64_t hold_time =
				    k_uptime_delta(&delete_press_times[i]);

				if (hold_time > CLEAR_INPUT_ON_HOLD_TIMEOUT) {
					clear_elems(input);
				}

				delete_press_times[i] = 0;
			}

			return true;
		}
	}

	return false;
}

static bool update_confirm_key(struct input_buf *input,
			       uint16_t in_key_id, bool pressed)
{
	if (!pressed) {
		return false;
	}

	for (size_t i = 0; i < ARRAY_SIZE(confirm_keys); i++) {
		if (in_key_id == confirm_keys[i]) {
			input_confirm(input);
			return true;
		}
	}

	return false;
}

static bool button_event_handler(const struct button_event *event)
{
	__ASSERT_NO_MSG(state != STATE_DISABLED);

	if (state != STATE_ACTIVE) {
		return false;
	}

	static bool (*update_key_func[])(struct input_buf *input,
					 uint16_t in_key_id, bool pressed) = {
		update_digit_input_key,
		update_confirm_key,
		update_delete_key
	};

	for (int i = 0; i < ARRAY_SIZE(update_key_func); i++) {
		if ((*update_key_func[i])(&digits,
					  event->key_id, event->pressed)) {
			break;
		}
	}

	return true;
}

static void init(void)
{
	__ASSERT_NO_MSG(state == STATE_DISABLED);

	BUILD_ASSERT(CONFIG_DESKTOP_PASSKEY_MAX_LEN != 0,
			 "Passkey length not set in Kconfig");
	BUILD_ASSERT(sizeof(confirm_keys) > 0,
			 "No confirm key defined");

	state = STATE_IDLE;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_button_event(aeh)) {
		return button_event_handler(cast_button_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			init();
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (is_passkey_req_event(aeh)) {
		const struct passkey_req_event *event =
			cast_passkey_req_event(aeh);

		if (event->active) {
			__ASSERT_NO_MSG(state != STATE_ACTIVE);
			state = STATE_ACTIVE;
			clear_elems(&digits);
		} else {
			state = STATE_IDLE;
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
APP_EVENT_SUBSCRIBE(MODULE, passkey_req_event);
