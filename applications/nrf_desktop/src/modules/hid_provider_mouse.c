/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <limits.h>
#include <sys/types.h>

#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <caf/events/button_event.h>
#include "motion_event.h"
#include "wheel_event.h"
#include "hid_event.h"
#include "hid_report_provider_event.h"

#include "hid_keymap.h"
#include "hid_report_desc.h"

#define MODULE hid_provider_mouse
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_HID_REPORT_PROVIDER_MOUSE_LOG_LEVEL);

/* Make sure that mouse buttons would fit in button bitmask. */
BUILD_ASSERT(MOUSE_REPORT_BUTTON_COUNT_MAX <= BITS_PER_BYTE);

struct report_data {
	uint8_t button_bm; /* Bitmask of pressed mouse buttons. */
	int16_t axes[MOUSE_REPORT_AXIS_COUNT]; /* Array of axes (motion X, motion Y, wheel). */
	bool update_needed;
	uint8_t pipeline_cnt;
	uint8_t pipeline_size;
};

static const void *active_sub;
static bool boot_mode;

static const struct hid_state_api *hid_state_api;
static struct report_data report_data;


static void clear_report_data(struct report_data *rd)
{
	LOG_INF("Clear report data");

	rd->button_bm = 0;
	memset(&rd->axes, 0x00, sizeof(rd->axes));
	rd->update_needed = false;
	rd->pipeline_cnt = 0;
	rd->pipeline_size = 0;
}

static void send_empty_report(uint8_t report_id, const void *subscriber)
{
	__ASSERT_NO_MSG((report_id == REPORT_ID_MOUSE) ||
			(IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE) &&
			 (report_id == REPORT_ID_BOOT_MOUSE)));
	__ASSERT_NO_MSG(subscriber);

	size_t size = (report_id == REPORT_ID_MOUSE) ?
		      (REPORT_SIZE_MOUSE) : (REPORT_SIZE_MOUSE_BOOT);

	struct hid_report_event *event = new_hid_report_event(sizeof(report_id) + size);

	event->source = &report_data;
	event->subscriber = subscriber;

	event->dyndata.data[0] = report_id;
	memset(&event->dyndata.data[1], 0x00, size);

	APP_EVENT_SUBMIT(event);
}

static bool send_report_mouse(uint8_t report_id, bool force)
{
	__ASSERT_NO_MSG(report_id == REPORT_ID_MOUSE);
	__ASSERT_NO_MSG(active_sub && !boot_mode);

	struct report_data *rd = &report_data;

	if (force) {
		/* Send HID report to refresh state of HID subscriber. */
	} else if (rd->pipeline_cnt >= rd->pipeline_size) {
		/* Buffer HID data internally until previously submitted reports are sent. */
		return false;
	} else if (!rd->update_needed) {
		/* Nothing to send. */
		return false;
	}

	/* X/Y axis */
	int16_t dx = CLAMP(rd->axes[MOUSE_REPORT_AXIS_X],
			   MOUSE_REPORT_XY_MIN, MOUSE_REPORT_XY_MAX);
	int16_t dy = CLAMP(-rd->axes[MOUSE_REPORT_AXIS_Y],
			   MOUSE_REPORT_XY_MIN, MOUSE_REPORT_XY_MAX);

	/* Wheel */
	int16_t wheel = CLAMP(rd->axes[MOUSE_REPORT_AXIS_WHEEL] / 2,
			      MOUSE_REPORT_WHEEL_MIN, MOUSE_REPORT_WHEEL_MAX);

	/* Button bitmask. */
	uint8_t button_bm = rd->button_bm;

	/* Update stored report data. */
	if (dx) {
		rd->axes[MOUSE_REPORT_AXIS_X] -= dx;
	}
	if (dy) {
		rd->axes[MOUSE_REPORT_AXIS_Y] += dy;
	}
	if (wheel) {
		rd->axes[MOUSE_REPORT_AXIS_WHEEL] -= wheel * 2;
	}

	/* Encode report. */
	BUILD_ASSERT(REPORT_SIZE_MOUSE == 5, "Invalid report size");

	struct hid_report_event *event = new_hid_report_event(sizeof(report_id)
							      + REPORT_SIZE_MOUSE);

	event->source = &report_data;
	event->subscriber = active_sub;

	/* Convert to little-endian. */
	uint8_t x_buff[sizeof(dx)];
	uint8_t y_buff[sizeof(dy)];

	sys_put_le16(dx, x_buff);
	sys_put_le16(dy, y_buff);

	event->dyndata.data[0] = report_id;
	event->dyndata.data[1] = button_bm;
	event->dyndata.data[2] = wheel;
	event->dyndata.data[3] = x_buff[0];
	event->dyndata.data[4] = (y_buff[0] << 4) | (x_buff[1] & 0x0f);
	event->dyndata.data[5] = (y_buff[1] << 4) | (y_buff[0] >> 4);

	APP_EVENT_SUBMIT(event);

	rd->pipeline_cnt++;

	if ((rd->axes[MOUSE_REPORT_AXIS_X] != 0) || (rd->axes[MOUSE_REPORT_AXIS_Y] != 0) ||
	    (rd->axes[MOUSE_REPORT_AXIS_WHEEL] < -1) || (rd->axes[MOUSE_REPORT_AXIS_WHEEL] > 1)) {
		/* If there is some axis data to send, request report update. */
		rd->update_needed = true;
	} else {
		/* Keep the update needed flag until HID mouse report pipeline is created. */
		rd->update_needed = (rd->pipeline_cnt < rd->pipeline_size);
	}

	return true;
}

static bool send_report_boot_mouse(uint8_t report_id, bool force)
{
	__ASSERT_NO_MSG(report_id == REPORT_ID_BOOT_MOUSE);
	__ASSERT_NO_MSG(active_sub && boot_mode);

	struct report_data *rd = &report_data;

	if (force) {
		/* Send HID report to refresh state of HID subscriber. */
	} else if (rd->pipeline_cnt >= rd->pipeline_size) {
		/* Buffer HID data internally until previously submitted reports are sent. */
		return false;
	} else if (!rd->update_needed) {
		/* Nothing to send. */
		return false;
	}

	/* X/Y axis */
	int8_t dx = CLAMP(rd->axes[MOUSE_REPORT_AXIS_X], INT8_MIN, INT8_MAX);
	int8_t dy = CLAMP(-rd->axes[MOUSE_REPORT_AXIS_Y], INT8_MIN, INT8_MAX);

	/* Button bitmask. */
	uint8_t button_bm = rd->button_bm;

	if (dx) {
		rd->axes[MOUSE_REPORT_AXIS_X] -= dx;
	}
	if (dy) {
		rd->axes[MOUSE_REPORT_AXIS_Y] += dy;
	}
	rd->axes[MOUSE_REPORT_AXIS_WHEEL] = 0;

	size_t report_size = sizeof(report_id) + sizeof(button_bm) + sizeof(dx) + sizeof(dy);
	struct hid_report_event *event = new_hid_report_event(report_size);

	event->source = &report_data;
	event->subscriber = active_sub;

	event->dyndata.data[0] = report_id;
	event->dyndata.data[1] = button_bm;
	event->dyndata.data[2] = dx;
	event->dyndata.data[3] = dy;

	APP_EVENT_SUBMIT(event);

	rd->pipeline_cnt++;

	if ((rd->axes[MOUSE_REPORT_AXIS_X] != 0) || (rd->axes[MOUSE_REPORT_AXIS_Y] != 0)) {
		/* If there is some axis data to send, request report update. */
		rd->update_needed = true;
	} else {
		/* Keep the update needed flag until HID mouse report pipeline is created. */
		rd->update_needed = (rd->pipeline_cnt < rd->pipeline_size);
	}

	return true;
}

static void mouse_report_connection_state_changed(uint8_t report_id,
						  const struct subscriber_conn_state *cs)
{
	/* Only one subscriber can be connected at a time */
	__ASSERT_NO_MSG(!(active_sub && cs->subscriber));
	active_sub = cs->subscriber;

	LOG_INF("Report 0x%" PRIx8 " %s",
		report_id, (cs->subscriber) ? "connected" : "disconnected");

	switch (report_id) {
	case REPORT_ID_MOUSE:
		boot_mode = false;
		break;
	case REPORT_ID_BOOT_MOUSE:
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE));
		boot_mode = true;
		break;
	default:
		/* Not supported. */
		__ASSERT_NO_MSG(false);
		break;
	}

	struct report_data *rd = &report_data;

	if (active_sub) {
		/* Set HID subscriber's pipeline size. */
		rd->pipeline_cnt = cs->pipeline_cnt;
		rd->pipeline_size = cs->pipeline_size;
		/* Clear axes. */
		memset(&rd->axes, 0x00, sizeof(rd->axes));
	} else {
		/* Clear whole report data. */
		clear_report_data(rd);
	}
}

static void mouse_report_sent(uint8_t report_id, bool error)
{
	ARG_UNUSED(report_id);
	__ASSERT_NO_MSG(active_sub);
	__ASSERT_NO_MSG(((report_id == REPORT_ID_MOUSE) && !boot_mode) ||
			((report_id == REPORT_ID_BOOT_MOUSE) && boot_mode));

	__ASSERT_NO_MSG(report_data.pipeline_cnt > 0);
	report_data.pipeline_cnt--;

	if (error) {
		LOG_WRN("Error while sending report");
		/* HID state will send subsequent HID mouse report to update state. No need to do
		 * anything.
		 */
		report_data.update_needed = true;
	}
}

static void trigger_report_transmission(void)
{
	/* Mark that update is needed. */
	report_data.update_needed = true;

	if (active_sub) {
		__ASSERT_NO_MSG(hid_state_api);
		(void)hid_state_api->trigger_report_send(boot_mode ?
							 REPORT_ID_BOOT_MOUSE : REPORT_ID_MOUSE);
	} else {
		LOG_DBG("Subscription not enabled");
	}
}

static void update_key(uint16_t usage_id, bool pressed)
{
	if (!active_sub) {
		/* Ignore keypresses while not connected. */
		return;
	}

	__ASSERT_NO_MSG((usage_id >= 1) && (usage_id <= 8));
	uint8_t bit_pos = usage_id - 1;

	/* Module does not support multiple HW buttons mapped to the same HID usage ID. */
	__ASSERT_NO_MSG(!(pressed && IS_BIT_SET(report_data.button_bm, bit_pos)));
	WRITE_BIT(report_data.button_bm, bit_pos, pressed);

	trigger_report_transmission();
}

static void init(void)
{
	static const struct hid_report_provider_api provider_api_mouse = {
		.send_report = send_report_mouse,
		.send_empty_report = send_empty_report,
		.connection_state_changed = mouse_report_connection_state_changed,
		.report_sent = mouse_report_sent,
	};

	struct hid_report_provider_event *rp_event;

	rp_event = new_hid_report_provider_event();
	rp_event->report_id = REPORT_ID_MOUSE;
	rp_event->provider_api = &provider_api_mouse;
	rp_event->hid_state_api = NULL;
	APP_EVENT_SUBMIT(rp_event);

	if (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE)) {
		static const struct hid_report_provider_api provider_api_boot_mouse = {
			.send_report = send_report_boot_mouse,
			.send_empty_report = send_empty_report,
			.connection_state_changed = mouse_report_connection_state_changed,
			.report_sent = mouse_report_sent,
		};

		rp_event = new_hid_report_provider_event();
		rp_event->report_id = REPORT_ID_BOOT_MOUSE;
		rp_event->provider_api = &provider_api_boot_mouse;
		rp_event->hid_state_api = NULL;
		APP_EVENT_SUBMIT(rp_event);
	}
}

static bool handle_motion_event(const struct motion_event *event)
{
	report_data.axes[MOUSE_REPORT_AXIS_X] += event->dx;
	report_data.axes[MOUSE_REPORT_AXIS_Y] += event->dy;

	trigger_report_transmission();

	return false;
}

static bool handle_wheel_event(const struct wheel_event *event)
{
	report_data.axes[MOUSE_REPORT_AXIS_WHEEL] += event->wheel;

	trigger_report_transmission();

	return false;
}

static bool handle_button_event(const struct button_event *event)
{
	/* Get usage ID and target report from HID Keymap */
	const struct hid_keymap *map = hid_keymap_get(event->key_id);

	/* Boot mouse report uses mouse report ID in the key map. */
	if (map && (map->report_id == REPORT_ID_MOUSE)) {
		update_key(map->usage_id, event->pressed);
	}

	return false;
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
		LOG_INF("Init mouse report provider");
		init();
	}

	return false;
}

static bool handle_hid_report_provider_event(const struct hid_report_provider_event *event)
{
	if (event->report_id == REPORT_ID_MOUSE) {
		__ASSERT_NO_MSG(event->provider_api->connection_state_changed ==
				mouse_report_connection_state_changed);
		__ASSERT_NO_MSG(!hid_state_api);
		__ASSERT_NO_MSG(event->hid_state_api);
		__ASSERT_NO_MSG(event->hid_state_api->trigger_report_send);
		hid_state_api = event->hid_state_api;
	} else if (event->report_id == REPORT_ID_BOOT_MOUSE) {
		__ASSERT_NO_MSG(event->provider_api->connection_state_changed ==
				mouse_report_connection_state_changed);
		__ASSERT_NO_MSG(hid_state_api == event->hid_state_api);
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (!IS_ENABLED(CONFIG_DESKTOP_MOTION_NONE) &&
	    is_motion_event(aeh)) {
		return handle_motion_event(cast_motion_event(aeh));
	}

	if (IS_ENABLED(CONFIG_DESKTOP_WHEEL_ENABLE) &&
	    is_wheel_event(aeh)) {
		return handle_wheel_event(cast_wheel_event(aeh));
	}

	if (IS_ENABLED(CONFIG_CAF_BUTTON_EVENTS) &&
	    is_button_event(aeh)) {
		return handle_button_event(cast_button_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		return handle_module_state_event(cast_module_state_event(aeh));
	}

	if (is_hid_report_provider_event(aeh)) {
		return handle_hid_report_provider_event(cast_hid_report_provider_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
#ifndef CONFIG_DESKTOP_MOTION_NONE
APP_EVENT_SUBSCRIBE(MODULE, motion_event);
#endif /* !CONFIG_DESKTOP_MOTION_NONE */
#ifdef CONFIG_DESKTOP_WHEEL_ENABLE
APP_EVENT_SUBSCRIBE(MODULE, wheel_event);
#endif /* CONFIG_DESKTOP_WHEEL_ENABLE */
#ifdef CONFIG_CAF_BUTTON_EVENTS
APP_EVENT_SUBSCRIBE(MODULE, button_event);
#endif /* CONFIG_CAF_BUTTON_EVENTS */
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_provider_event);
