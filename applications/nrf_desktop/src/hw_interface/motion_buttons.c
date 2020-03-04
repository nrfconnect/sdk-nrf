/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include "event_manager.h"
#include "button_event.h"
#include "motion_event.h"
#include "hid_event.h"

#define MODULE motion
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_MOTION_LOG_LEVEL);


enum state {
	STATE_IDLE,
	STATE_CONNECTED,
	STATE_PENDING
};

enum dir {
	DIR_UP,
	DIR_DOWN,
	DIR_LEFT,
	DIR_RIGHT,

	DIR_COUNT
};


static u32_t timestamp[DIR_COUNT];
static enum state state;


static void motion_event_send(s16_t dx, s16_t dy)
{
	struct motion_event *event = new_motion_event();

	event->dx = dx;
	event->dy = dy;

	EVENT_SUBMIT(event);
}

static enum dir key_to_dir(u16_t key_id)
{
	enum dir dir = DIR_COUNT;
	switch (key_id) {
	case CONFIG_DESKTOP_MOTION_UP_KEY_ID:
		dir = DIR_UP;
		break;

	case CONFIG_DESKTOP_MOTION_DOWN_KEY_ID:
		dir = DIR_DOWN;
		break;

	case CONFIG_DESKTOP_MOTION_LEFT_KEY_ID:
		dir = DIR_LEFT;
		break;

	case CONFIG_DESKTOP_MOTION_RIGHT_KEY_ID:
		dir = DIR_RIGHT;
		break;

	default:
		break;
	}

	return dir;
}

static s16_t update_motion_dir(enum dir dir, u32_t ts)
{
	u32_t time_diff = ts - timestamp[dir];

	if (time_diff > UCHAR_MAX) {
		time_diff = UCHAR_MAX;
	}

	if (timestamp[dir] > 0) {
		timestamp[dir] = ts;
		return time_diff;
	}

	return 0;
}

static void send_motion(void)
{
	u32_t ts = k_uptime_get();
	s16_t x = 0;
	s16_t y = 0;

	y += update_motion_dir(DIR_UP, ts);
	y -= update_motion_dir(DIR_DOWN, ts);
	x += update_motion_dir(DIR_RIGHT, ts);
	x -= update_motion_dir(DIR_LEFT, ts);

	motion_event_send(x, y);
}

static bool handle_button_event(const struct button_event *event)
{
	enum dir dir = key_to_dir(event->key_id);

	if (dir == DIR_COUNT) {
		return false;
	}

	u32_t *ts = &timestamp[dir];

	if (!event->pressed) {
		*ts = 0;
	} else {
		*ts = k_uptime_get() - 1;
	}

	if (state == STATE_CONNECTED) {
		send_motion();
		state = STATE_PENDING;
	}

	return true;
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	/* Replicate the state of buttons module */
	if (event->module_id == MODULE_ID(buttons)) {
		module_set_state(event->state);
	}

	return false;
}

static bool is_motion_active(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(timestamp); i++) {
		if (timestamp[i] > 0) {
			return true;
		}
	}

	return false;
}

static bool handle_hid_report_sent_event(const struct hid_report_sent_event *event)
{
	if (event->report_id == REPORT_ID_MOUSE) {
		if (state == STATE_PENDING) {
			if (is_motion_active()) {
				send_motion();
			} else {
				state = STATE_CONNECTED;
			}
		}
	}

	return false;
}

static bool handle_hid_report_subscription_event(const struct hid_report_subscription_event *event)
{
	if (event->report_id == REPORT_ID_MOUSE) {
		static u8_t peer_count;

		if (event->enabled) {
			__ASSERT_NO_MSG(peer_count < UCHAR_MAX);
			peer_count++;
		} else {
			__ASSERT_NO_MSG(peer_count > 0);
			peer_count--;
		}

		bool is_connected = (peer_count != 0);

		if ((state == STATE_IDLE) && is_connected) {
			if (is_motion_active()) {
				send_motion();
				state = STATE_PENDING;
			} else {
				state = STATE_CONNECTED;
			}
			return false;
		}
		if ((state != STATE_IDLE) && !is_connected) {
			state = STATE_IDLE;
			return false;
		}
	}

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_hid_report_sent_event(eh)) {
		return handle_hid_report_sent_event(
				cast_hid_report_sent_event(eh));
	}

	if (is_button_event(eh)) {
		return handle_button_event(cast_button_event(eh));
	}

	if (is_module_state_event(eh)) {
		return handle_module_state_event(cast_module_state_event(eh));
	}

	if (is_hid_report_subscription_event(eh)) {
		return handle_hid_report_subscription_event(
				cast_hid_report_subscription_event(eh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE_EARLY(MODULE, button_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, hid_report_sent_event);
EVENT_SUBSCRIBE(MODULE, hid_report_subscription_event);
