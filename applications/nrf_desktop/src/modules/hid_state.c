/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <limits.h>
#include <sys/types.h>

#include <zephyr/types.h>
#include <zephyr/sys/util.h>

#include <caf/events/led_event.h>
#include "hid_report_provider_event.h"
#include "hid_event.h"

#include CONFIG_DESKTOP_HID_STATE_HID_KEYBOARD_LEDS_DEF_PATH
#include "hid_report_desc.h"

#define MODULE hid_state
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_HID_STATE_LOG_LEVEL);


/* Module state. */
enum state {
	STATE_DISCONNECTED,	/* Not connected. */
	STATE_CONNECTED_IDLE,	/* Connected, no data exchange. */
	STATE_CONNECTED_BUSY	/* Connected, report is generated. */
};

#define SUBSCRIBER_COUNT CONFIG_DESKTOP_HID_STATE_SUBSCRIBER_COUNT

#define OUTPUT_REPORT_STATE_COUNT (ARRAY_SIZE(output_reports))
#define INPUT_REPORT_STATE_COUNT  (ARRAY_SIZE(input_reports))

struct report_state {
	enum state state;
	uint8_t cnt;
	uint8_t report_id;
	struct subscriber *subscriber;
	struct provider *provider;
	bool update_needed;
};

struct output_report_state {
	uint8_t data[REPORT_BUFFER_SIZE_OUTPUT_REPORT];
};

struct provider {
	uint8_t report_id;
	const struct hid_report_provider_api *api;
	struct report_state *linked_rs;
};

struct subscriber {
	const void *id;
	uint8_t priority;
	uint8_t pipeline_size;
	uint8_t report_max;
	uint8_t report_cnt;
	struct output_report_state output_reports[OUTPUT_REPORT_STATE_COUNT];
	struct report_state state[INPUT_REPORT_STATE_COUNT];
};

/* HID state structure. */
struct hid_state {
	struct provider provider[INPUT_REPORT_STATE_COUNT];
	struct subscriber subscriber[SUBSCRIBER_COUNT];
	struct subscriber *active_subscriber;
};

static struct hid_state state;

static size_t get_input_report_idx(uint8_t report_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(input_reports); i++) {
		if (input_reports[i] == report_id) {
			return i;
		}
	}

	/* Should not happen. */
	LOG_ERR("No index for input report ID:0x%" PRIx8, report_id);
	__ASSERT_NO_MSG(false);
	return 0;
}

static size_t get_output_report_idx(uint8_t report_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(output_reports); i++) {
		if (output_reports[i] == report_id) {
			return i;
		}
	}

	/* Should not happen. */
	LOG_ERR("No index for output report ID:0x%" PRIx8, report_id);
	__ASSERT_NO_MSG(false);
	return 0;
}

static struct report_state *get_report_state(struct subscriber *subscriber,
					     uint8_t report_id)
{
	BUILD_ASSERT(ARRAY_SIZE(input_reports) == ARRAY_SIZE(subscriber->state));
	__ASSERT_NO_MSG(subscriber);

	return &subscriber->state[get_input_report_idx(report_id)];
}

static struct subscriber *get_subscriber(const void *subscriber_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(state.subscriber); i++) {
		if (subscriber_id == state.subscriber[i].id) {
			return &state.subscriber[i];
		}
	}

	LOG_WRN("No subscriber %p", subscriber_id);
	return NULL;
}

static struct provider *get_provider(uint8_t report_id)
{
	BUILD_ASSERT(ARRAY_SIZE(input_reports) == ARRAY_SIZE(state.provider));

	return &state.provider[get_input_report_idx(report_id)];
}

static bool report_send(struct report_state *rs,
			struct provider *provider,
			bool send_always)
{
	bool report_sent = false;

	if (!rs && provider) {
		rs = provider->linked_rs;
	}
	if (!provider && rs) {
		provider = rs->provider;
	}

	if (!provider->api) {
		LOG_WRN("Provider %p not yet ready", (void *)provider);
		return report_sent;
	}

	if (!rs || !provider) {
		LOG_WRN("No link between report state and provider");
		return report_sent;
	}

	__ASSERT_NO_MSG(provider->api);
	__ASSERT_NO_MSG(rs->state != STATE_DISCONNECTED);

	/* We must update subscriber to refresh state. */
	rs->update_needed |= send_always;

	while (rs->subscriber->report_cnt < rs->subscriber->report_max) {
		bool sent = false;

		if (provider->linked_rs == rs) {
			sent = provider->api->send_report(provider->report_id,
							  rs->update_needed);
		} else {
			if (provider->api->send_empty_report && (rs->update_needed)) {
				provider->api->send_empty_report(provider->report_id,
								 rs->subscriber->id);
				sent = true;
			} else {
				/* Do not retry update if unsupported. */
				rs->update_needed = false;
				sent = false;
			}
		}

		if (sent) {
			__ASSERT_NO_MSG(rs->cnt < UINT8_MAX);
			rs->cnt++;
			rs->subscriber->report_cnt++;
			rs->update_needed = false;
			report_sent = true;
		} else {
			/* Nothing to send. */
			break;
		}
	}

	if (rs->cnt != 0) {
		rs->state = STATE_CONNECTED_BUSY;
	}

	return report_sent;
}

static void link_provider_to_rs(struct provider *provider, struct report_state *rs)
{
	if (!provider->api) {
		LOG_WRN("Provider %p not yet ready", (void *)provider);
		return;
	}

	if (provider->linked_rs == rs) {
		/* Already linked. */
		return;
	}

	__ASSERT_NO_MSG(!rs || rs->subscriber);
	const struct subscriber_conn_state cs = {
		.subscriber = (rs ? rs->subscriber->id : NULL),
		.pipeline_cnt = (rs ? rs->cnt : 0),
		.pipeline_size = (rs ? rs->subscriber->pipeline_size : 0),
	};
	struct report_state *prev_rs = provider->linked_rs;

	/* Previous report state needs to be unlinked before linking new report state. */
	__ASSERT_NO_MSG(!(prev_rs && rs));
	__ASSERT_NO_MSG(!rs || (rs->provider == provider));

	provider->linked_rs = rs;
	provider->api->connection_state_changed(provider->report_id, &cs);

	if (rs) {
		/* Refresh state of the newly linked subscriber. */
		(void)report_send(rs, NULL, true);
	} else if (prev_rs->state != STATE_DISCONNECTED) {
		/* Provide an empty HID report to the unlinked subscriber to clear state. */
		(void)report_send(prev_rs, NULL, true);
	}
}

static void report_issued(const void *subscriber_id, uint8_t report_id, bool error)
{
	struct subscriber *subscriber = get_subscriber(subscriber_id);

	if (!subscriber) {
		LOG_WRN("No subscriber %p", subscriber_id);
		return;
	}

	bool subscriber_unblocked = (subscriber->report_cnt == subscriber->report_max);
	subscriber->report_cnt--;

	struct report_state *rs = get_report_state(subscriber, report_id);

	if (rs->state != STATE_DISCONNECTED) {
		__ASSERT_NO_MSG(rs->cnt > 0);
		rs->cnt--;

		if (rs->cnt == 0) {
			rs->state = STATE_CONNECTED_IDLE;
		}

		if (rs->provider->linked_rs == rs) {
			if (rs->provider->api->report_sent) {
				rs->provider->api->report_sent(report_id, error);
			}
		} else {
			if (error) {
				/* Issue with forwarding empty HID report. Retry. */
				rs->update_needed = true;
			}
		}
	}

	/* Pick next report to send. */
	struct report_state *next_rs;

	if (!subscriber_unblocked) {
		next_rs = rs;
	} else {
		/* Subscriber was blocked. Let's see if there are some other
		 * reports waiting to be sent.
		 */
		next_rs = rs + 1;
		if (next_rs == &subscriber->state[ARRAY_SIZE(subscriber->state)]) {
			next_rs = &subscriber->state[0];
		}
	}

	while (true) {
		if (next_rs->state != STATE_DISCONNECTED) {
			__ASSERT_NO_MSG(next_rs->provider);
			if (report_send(next_rs, NULL, false)) {
				break;
			}
		}

		if (next_rs == rs) {
			/* No report has data to be sent. */
			break;
		}

		next_rs = next_rs + 1;
		if (next_rs == &subscriber->state[ARRAY_SIZE(subscriber->state)]) {
			next_rs = &subscriber->state[0];
		}
	}
}

static void update_led(uint8_t led_id, const struct led_effect *effect)
{
	if (led_id == LED_UNAVAILABLE) {
		return;
	}

	struct led_event *event = new_led_event();

	event->led_id = led_id;
	event->led_effect = effect;
	APP_EVENT_SUBMIT(event);
}

static void broadcast_keyboard_leds(void)
{
	BUILD_ASSERT(HID_KEYBOARD_LEDS_COUNT <= CHAR_BIT);
	BUILD_ASSERT(REPORT_SIZE_KEYBOARD_LEDS == 1);

	const struct subscriber *sub = state.active_subscriber;
	size_t idx = get_output_report_idx(REPORT_ID_KEYBOARD_LEDS);

	static uint8_t displayed_leds_state;
	uint8_t new_leds_state = sub ? sub->output_reports[idx].data[0] : 0;
	uint8_t update_needed = (displayed_leds_state ^ new_leds_state);

	for (size_t i = 0; i < ARRAY_SIZE(keyboard_led_map); i++) {
		if (update_needed & BIT(i)) {
			update_led(keyboard_led_map[i],
				((new_leds_state & BIT(i)) ? &keyboard_led_on : &keyboard_led_off));
		}
	}

	displayed_leds_state = new_leds_state;
}

static void update_output_report_state(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT)) {
		broadcast_keyboard_leds();
	}
}

static void connect(const void *subscriber_id, uint8_t report_id)
{
	__ASSERT_NO_MSG(subscriber_id);

	struct subscriber *subscriber = get_subscriber(subscriber_id);

	if (!subscriber) {
		LOG_WRN("No subscriber %p", subscriber_id);
		return;
	}

	struct report_state *rs = get_report_state(subscriber, report_id);

	rs->subscriber = subscriber;
	rs->state = STATE_CONNECTED_IDLE;
	rs->report_id = report_id;

	struct provider *provider = get_provider(report_id);

	rs->provider = provider;

	if (subscriber == state.active_subscriber) {
		link_provider_to_rs(provider, rs);
	}
}

static void disconnect(const void *subscriber_id, uint8_t report_id)
{
	__ASSERT_NO_MSG(subscriber_id);

	struct subscriber *subscriber = get_subscriber(subscriber_id);

	if (!subscriber) {
		LOG_WRN("No subscriber %p", subscriber_id);
		return;
	}

	struct report_state *rs = get_report_state(subscriber, report_id);

	rs->subscriber = NULL;
	rs->state = STATE_DISCONNECTED;
	rs->cnt = 0;

	struct provider *provider = rs->provider;

	rs->provider = NULL;

	__ASSERT_NO_MSG(!provider->api || provider->linked_rs);
	if (provider->linked_rs == rs) {
		__ASSERT_NO_MSG(subscriber == state.active_subscriber);
		link_provider_to_rs(provider, NULL);
	}
}

static struct subscriber *find_empty_subscriber_slot(void)
{
	return get_subscriber(NULL);
}

static void link_providers(struct subscriber *subscriber)
{
	for (size_t i = 0; i < ARRAY_SIZE(subscriber->state); i++) {
		struct report_state *rs = &subscriber->state[i];

		if (rs->provider) {
			link_provider_to_rs(rs->provider, rs);
		}
	}
}

static void unlink_providers(struct subscriber *subscriber)
{
	for (size_t i = 0; i < ARRAY_SIZE(subscriber->state); i++) {
		struct report_state *rs = &subscriber->state[i];

		if (rs->provider) {
			__ASSERT_NO_MSG(!rs->provider->api || (rs->provider->linked_rs == rs));
			link_provider_to_rs(rs->provider, NULL);
		}
	}
}

static void connect_subscriber(const void *subscriber_id, uint8_t priority,
			       uint8_t pipeline_size, uint8_t report_max)
{
	__ASSERT_NO_MSG(subscriber_id);
	__ASSERT_NO_MSG(priority > 0);
	__ASSERT_NO_MSG(pipeline_size > 0);
	__ASSERT_NO_MSG(report_max > 0);

	struct subscriber *sub = find_empty_subscriber_slot();
	struct subscriber *prev_sub = state.active_subscriber;

	sub->id = subscriber_id;
	sub->priority = priority;
	sub->pipeline_size = pipeline_size;
	sub->report_max = report_max;
	sub->report_cnt = 0;

	/* Select new active subscriber if needed. */
	if (!state.active_subscriber) {
		state.active_subscriber = sub;
	} else if (state.active_subscriber->priority <= sub->priority) {
		__ASSERT_NO_MSG(state.active_subscriber->priority !=
				sub->priority);
		unlink_providers(state.active_subscriber);
		state.active_subscriber = sub;
	}

	if (prev_sub != state.active_subscriber) {
		update_output_report_state();
	}

	LOG_INF("Subscriber %p connected", subscriber_id);
}

static struct subscriber *find_next_subscriber(struct subscriber *prev_sub)
{
	struct subscriber *next_sub = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(state.subscriber); i++) {
		struct subscriber *sub = &state.subscriber[i];

		if (!sub->id || (sub == prev_sub)) {
			continue;
		}

		if (!next_sub || (next_sub->priority <= sub->priority)) {
			__ASSERT_NO_MSG(!next_sub || (next_sub->priority != sub->priority));
			next_sub = sub;
		}
	}

	return next_sub;
}

static void disconnect_subscriber(const void *subscriber_id)
{
	__ASSERT_NO_MSG(subscriber_id);

	struct subscriber *sub = get_subscriber(subscriber_id);

	if (sub == NULL) {
		return;
	}

	if (sub == state.active_subscriber) {
		unlink_providers(sub);

		state.active_subscriber = find_next_subscriber(sub);

		if (state.active_subscriber) {
			link_providers(state.active_subscriber);
		}

		update_output_report_state();
	}

	memset(sub, 0, sizeof(*sub));

	LOG_INF("Subscriber %p disconnected", subscriber_id);
}

static bool handle_hid_report_sent_event(const struct hid_report_sent_event *event)
{
	report_issued(event->subscriber, event->report_id, event->error);

	return false;
}

static void handle_keyboard_leds_report(struct subscriber *sub, const uint8_t *data, size_t len)
{
	BUILD_ASSERT(REPORT_SIZE_KEYBOARD_LEDS == 1);

	if (len != REPORT_SIZE_KEYBOARD_LEDS) {
		LOG_WRN("Improper keyboard LED report size");
		return;
	}

	size_t idx = get_output_report_idx(REPORT_ID_KEYBOARD_LEDS);

	memcpy(sub->output_reports[idx].data, data, len);
	broadcast_keyboard_leds();
}

static void handle_hid_output_report(struct subscriber *sub, uint8_t report_id,
				     const uint8_t *data, size_t len)
{
	__ASSERT_NO_MSG(sub);

	switch (report_id) {
	case REPORT_ID_KEYBOARD_LEDS:
		handle_keyboard_leds_report(sub, data, len);
		break;
	default:
		/* Ignore unknown report. */
		break;
	}
}

static bool handle_hid_report_event(const struct hid_report_event *event)
{
	/* Ignore HID input reports. */
	if (event->subscriber) {
		return false;
	}

	__ASSERT_NO_MSG(event->dyndata.size > 0);

	handle_hid_output_report(get_subscriber(event->source), event->dyndata.data[0],
				 &event->dyndata.data[1], event->dyndata.size - 1);

	return false;
}

static bool handle_hid_report_subscriber_event(const struct hid_report_subscriber_event *event)
{
	if (event->connected) {
		connect_subscriber(event->subscriber, event->params.priority,
				   event->params.pipeline_size, event->params.report_max);
	} else {
		disconnect_subscriber(event->subscriber);
	}

	return false;
}

static bool handle_hid_report_subscription_event(const struct hid_report_subscription_event *event)
{
	if (event->enabled) {
		connect(event->subscriber, event->report_id);
	} else {
		disconnect(event->subscriber, event->report_id);
	}

	return false;
}

static int hid_state_report_trigger(uint8_t report_id)
{
	int err = 0;
	struct provider *provider = get_provider(report_id);

	__ASSERT_NO_MSG(provider->api);

	if (provider->linked_rs) {
		if (!report_send(NULL, provider, false)) {
			err = -EBUSY;
		}
	} else {
		LOG_WRN("No subscriber for report ID:0x%" PRIx8, report_id);
		err = -ENOENT;
	}

	return err;
}

static bool handle_hid_report_provider_event(struct hid_report_provider_event *event)
{
	static const struct hid_state_api hid_state_api = {
		.trigger_report_send = hid_state_report_trigger,
	};
	struct provider *provider = get_provider(event->report_id);

	__ASSERT_NO_MSG(!provider->api);

	__ASSERT_NO_MSG(event->provider_api);
	__ASSERT_NO_MSG(event->provider_api->send_report);
	__ASSERT_NO_MSG(event->provider_api->connection_state_changed);

	provider->report_id = event->report_id;
	provider->api = event->provider_api;

	/* Provide API to let provider trigger sending a HID report. */
	__ASSERT_NO_MSG(!event->hid_state_api);
	event->hid_state_api = &hid_state_api;

	if (state.active_subscriber) {
		struct report_state *rs = get_report_state(state.active_subscriber,
							   event->report_id);

		if (rs->provider == provider) {
			link_provider_to_rs(provider, rs);
		}
	}

	return false;
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
		static bool initialized;

		__ASSERT_NO_MSG(!initialized);
		initialized = true;

		LOG_INF("Init HID state!");
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_hid_report_sent_event(aeh)) {
		return handle_hid_report_sent_event(cast_hid_report_sent_event(aeh));
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT) &&
	    is_hid_report_event(aeh)) {
		return handle_hid_report_event(cast_hid_report_event(aeh));
	}

	if (is_hid_report_subscriber_event(aeh)) {
		return handle_hid_report_subscriber_event(cast_hid_report_subscriber_event(aeh));
	}

	if (is_hid_report_subscription_event(aeh)) {
		return handle_hid_report_subscription_event(
				cast_hid_report_subscription_event(aeh));
	}

	if (is_hid_report_provider_event(aeh)) {
		return handle_hid_report_provider_event(cast_hid_report_provider_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		return handle_module_state_event(cast_module_state_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
#ifdef CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT
APP_EVENT_SUBSCRIBE(MODULE, hid_report_event);
#endif /* CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT */
APP_EVENT_SUBSCRIBE(MODULE, hid_report_sent_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_subscription_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_subscriber_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, hid_report_provider_event);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
