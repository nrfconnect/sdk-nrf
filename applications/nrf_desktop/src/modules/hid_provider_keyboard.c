/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <limits.h>
#include <sys/types.h>

#include <zephyr/types.h>
#include <zephyr/sys/util.h>

#include <caf/events/button_event.h>
#include "hid_event.h"
#include "hid_report_provider_event.h"

#include "hid_keymap.h"
#include "hid_eventq.h"
#include "keys_state.h"
#include "hid_report_desc.h"

#define MODULE hid_provider_keyboard
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_HID_REPORT_PROVIDER_KEYBOARD_LOG_LEVEL);

struct report_data {
	struct hid_eventq eventq;
	struct keys_state keys_state;
	bool update_needed;
};

static const struct hid_state_api *hid_state_api;
static const void *active_sub;
static bool boot_mode;
static struct report_data report_data;


static void clear_report_data(struct report_data *rd)
{
	LOG_INF("Clear report data");

	keys_state_clear(&rd->keys_state);
	hid_eventq_reset(&rd->eventq);
	rd->update_needed = false;
}

static void send_empty_report(uint8_t report_id, const void *subscriber)
{
	__ASSERT_NO_MSG((report_id == REPORT_ID_KEYBOARD_KEYS) ||
			(IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD) &&
			 (report_id == REPORT_ID_BOOT_KEYBOARD)));
	__ASSERT_NO_MSG(subscriber);

	BUILD_ASSERT(REPORT_SIZE_KEYBOARD_BOOT == REPORT_SIZE_KEYBOARD_KEYS);
	size_t size = REPORT_SIZE_KEYBOARD_KEYS;
	struct hid_report_event *event = new_hid_report_event(sizeof(report_id) + size);

	event->source = &report_data;
	event->subscriber = subscriber;

	event->dyndata.data[0] = report_id;
	memset(&event->dyndata.data[1], 0x00, size);

	APP_EVENT_SUBMIT(event);
}

static bool key_update(struct keys_state *ks, uint16_t usage_id, bool pressed)
{
	bool update_needed = false;
	int err = keys_state_key_update(ks, usage_id, pressed, &update_needed);

	if (err == -ENOENT) {
		/* Press of the released key was not recorded by the utility. Ignore. */
	} else if (err == -ENOBUFS) {
		/* Number of pressed keys exceeds the limit. Ignore. */
		LOG_WRN("Number of pressed keys exceeds the limit. Keypress dropped");
	} else if (err) {
		/* Other error codes should not happen. */
		__ASSERT_NO_MSG(false);
	}

	return !err && update_needed;
}

static void process_queued_keypress(struct report_data *rd)
{
	while (!rd->update_needed) {
		uint16_t usage_id;
		bool pressed;
		int err = hid_eventq_keypress_dequeue(&rd->eventq, &usage_id, &pressed);

		if (err) {
			/* No keypress enqueued. */
			break;
		}

		rd->update_needed = key_update(&rd->keys_state, usage_id, pressed);
		/* If no item was changed, try next event. */
	}
}

static bool send_report_keyboard(uint8_t report_id, bool force)
{
	/* Both report and boot protocol reports use the same format. */
	__ASSERT_NO_MSG(((report_id == REPORT_ID_KEYBOARD_KEYS) && !boot_mode) ||
			((report_id == REPORT_ID_BOOT_KEYBOARD) && boot_mode));
	__ASSERT_NO_MSG(active_sub);

	struct report_data *rd = &report_data;

	process_queued_keypress(rd);
	if (!force && !rd->update_needed) {
		/* Nothing to send. */
		return false;
	}

	/* Keyboard report should contain keys plus one byte for modifier
	 * and one reserved byte.
	 */
	BUILD_ASSERT(REPORT_SIZE_KEYBOARD_KEYS == KEYBOARD_REPORT_KEY_COUNT_MAX + 2,
		     "Incorrect keyboard report size");

	/* Encode report. */
	struct hid_report_event *event = new_hid_report_event(sizeof(report_id)
							      + REPORT_SIZE_KEYBOARD_KEYS);

	event->source = &report_data;
	event->subscriber = active_sub;

	event->dyndata.data[0] = report_id;
	event->dyndata.data[2] = 0; /* Reserved byte */

	uint16_t keys[KEYBOARD_REPORT_KEY_COUNT_MAX];
	size_t key_cnt = keys_state_keys_get(&rd->keys_state, keys, ARRAY_SIZE(keys));
	uint8_t modifier_bm = 0;
	uint8_t *key_data_ptr = &event->dyndata.data[3];
	size_t reported_keys_cnt = 0;

	for (int i = key_cnt - 1; i >= 0; i--) {
		uint16_t usage_id = keys[i];

		if (usage_id <= KEYBOARD_REPORT_LAST_KEY) {
			BUILD_ASSERT(KEYBOARD_REPORT_LAST_KEY <= UINT8_MAX);
			key_data_ptr[reported_keys_cnt] = (uint8_t)usage_id;
			reported_keys_cnt++;
		} else if ((usage_id >= KEYBOARD_REPORT_FIRST_MODIFIER) &&
			   (usage_id <= KEYBOARD_REPORT_LAST_MODIFIER)) {
			/* Make sure key bitmask will fit into modifiers. */
			BUILD_ASSERT(KEYBOARD_REPORT_LAST_MODIFIER - KEYBOARD_REPORT_FIRST_MODIFIER
				     < 8);
			modifier_bm |= BIT(usage_id - KEYBOARD_REPORT_FIRST_MODIFIER);
		} else {
			LOG_ERR("Unhandled usage ID 0x%" PRIx16, usage_id);
		}
	}

	/* Fill the rest of report with zeros. */
	memset(&key_data_ptr[reported_keys_cnt], 0x00,
	       KEYBOARD_REPORT_KEY_COUNT_MAX - reported_keys_cnt);

	event->dyndata.data[1] = modifier_bm;

	APP_EVENT_SUBMIT(event);

	rd->update_needed = false;

	return true;
}

static void drop_stale_keypresses(struct hid_eventq *q)
{
	__ASSERT_NO_MSG(!active_sub);

	if (!hid_eventq_is_empty(q)) {
		/* Remove all stale events from the queue. */
		int64_t uptime = k_uptime_get();
		int64_t expire = CONFIG_DESKTOP_HID_REPORT_PROVIDER_KEYBOARD_KEYPRESS_EXPIRATION;

		if (uptime > expire) {
			int64_t min_ts = uptime - expire;

			hid_eventq_cleanup(q, min_ts);
		}
	}
}

static void keyboard_report_connection_state_changed(uint8_t report_id,
						     const struct subscriber_conn_state *cs)
{
	/* Only one subscriber can be connected at a time */
	__ASSERT_NO_MSG(!(active_sub && cs->subscriber));

	LOG_INF("Report 0x%" PRIx8 " %s",
		report_id, (cs->subscriber) ? "connected" : "disconnected");

	struct report_data *rd = &report_data;

	if (cs->subscriber) {
		drop_stale_keypresses(&rd->eventq);
	} else {
		/* Clear whole report data. */
		clear_report_data(rd);
	}

	active_sub = cs->subscriber;

	switch (report_id) {
	case REPORT_ID_KEYBOARD_KEYS:
		boot_mode = false;
		break;
	case REPORT_ID_BOOT_KEYBOARD:
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD));
		boot_mode = true;
		break;
	default:
		/* Not supported. */
		__ASSERT_NO_MSG(false);
		break;
	}
}

static void keyboard_report_sent(uint8_t report_id, bool error)
{
	ARG_UNUSED(report_id);

	__ASSERT_NO_MSG(active_sub);
	__ASSERT_NO_MSG(((report_id == REPORT_ID_KEYBOARD_KEYS) && !boot_mode) ||
			((report_id == REPORT_ID_BOOT_KEYBOARD) && boot_mode));

	if (error) {
		LOG_WRN("Error while sending report");
		/* HID state will try to send next HID keyboard report to refresh state. */
		report_data.update_needed = true;
	}
}

static void trigger_report_transmission(void)
{
	/* Mark that update is needed. */
	report_data.update_needed = true;

	if (active_sub) {
		(void)hid_state_api->trigger_report_send(boot_mode ?
							 REPORT_ID_BOOT_KEYBOARD :
							  REPORT_ID_KEYBOARD_KEYS);
	} else {
		LOG_DBG("Subscription not enabled");
	}
}

static void handle_button(uint16_t usage_id, bool pressed)
{
	struct report_data *rd = &report_data;

	if (!active_sub || !hid_eventq_is_empty(&rd->eventq)) {
		/* Keypress needs to go through the queue. */
		int err = hid_eventq_keypress_enqueue(&rd->eventq, usage_id, pressed, false);

		if ((err == -ENOBUFS) && !active_sub) {
			/* Queue is full. If not yet connected, try to drop old events and retry. */
			__ASSERT_NO_MSG(hid_eventq_is_full(&rd->eventq));
			drop_stale_keypresses(&rd->eventq);

			err = hid_eventq_keypress_enqueue(&rd->eventq, usage_id, pressed, true);
		}

		if (err) {
			if (err == -ENOBUFS) {
				LOG_WRN("Queue is full, all events are dropped!");
			} else {
				LOG_ERR("hid_eventq_keypress_enqueue failed (err: %d)", err);
			}
			clear_report_data(rd);
			/* Make sure to forward updated keys state if already connected. */
			rd->update_needed = (active_sub != NULL);
		}
	} else {
		/* Instantly update keys state and trigger sending HID report if needed. */
		if (key_update(&rd->keys_state, usage_id, pressed)) {
			trigger_report_transmission();
		}
	}
}

static bool handle_button_event(const struct button_event *event)
{
	/* Get usage ID and target report from HID Keymap */
	const struct hid_keymap *map = hid_keymap_get(event->key_id);

	/* Boot keyboard report uses keyboard keys report ID in the key map. */
	if (map && (map->report_id == REPORT_ID_KEYBOARD_KEYS)) {
		handle_button(map->usage_id, event->pressed);
	}

	return false;
}

static void init(void)
{
	hid_eventq_init(&report_data.eventq,
			CONFIG_DESKTOP_HID_REPORT_PROVIDER_KEYBOARD_EVENT_QUEUE_SIZE);
	keys_state_init(&report_data.keys_state, KEYBOARD_REPORT_KEY_COUNT_MAX);

	static const struct hid_report_provider_api provider_api_keyboard = {
		.send_report = send_report_keyboard,
		.send_empty_report = send_empty_report,
		.connection_state_changed = keyboard_report_connection_state_changed,
		.report_sent = keyboard_report_sent,
	};

	struct hid_report_provider_event *rp_event;

	rp_event = new_hid_report_provider_event();
	rp_event->report_id = REPORT_ID_KEYBOARD_KEYS;
	rp_event->provider_api = &provider_api_keyboard;
	rp_event->hid_state_api = NULL;
	APP_EVENT_SUBMIT(rp_event);

	if (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD)) {
		static const struct hid_report_provider_api provider_api_boot_keyboard = {
			.send_report = send_report_keyboard,
			.send_empty_report = send_empty_report,
			.connection_state_changed = keyboard_report_connection_state_changed,
			.report_sent = keyboard_report_sent,
		};

		rp_event = new_hid_report_provider_event();
		rp_event->report_id = REPORT_ID_BOOT_KEYBOARD;
		rp_event->provider_api = &provider_api_boot_keyboard;
		rp_event->hid_state_api = NULL;
		APP_EVENT_SUBMIT(rp_event);
	}
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
		LOG_INF("Init keyboard report provider");
		init();
	}

	return false;
}

static bool handle_hid_report_provider_event(const struct hid_report_provider_event *event)
{
	if (event->report_id == REPORT_ID_KEYBOARD_KEYS) {
		__ASSERT_NO_MSG(event->provider_api->connection_state_changed ==
				keyboard_report_connection_state_changed);
		__ASSERT_NO_MSG(!hid_state_api);
		__ASSERT_NO_MSG(event->hid_state_api);
		hid_state_api = event->hid_state_api;
	} else if (event->report_id == REPORT_ID_BOOT_KEYBOARD) {
		__ASSERT_NO_MSG(event->provider_api->connection_state_changed ==
				keyboard_report_connection_state_changed);
		__ASSERT_NO_MSG(hid_state_api == event->hid_state_api);
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
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
#ifdef CONFIG_CAF_BUTTON_EVENTS
APP_EVENT_SUBSCRIBE(MODULE, button_event);
#endif /* CONFIG_CAF_BUTTON_EVENTS */
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_provider_event);
