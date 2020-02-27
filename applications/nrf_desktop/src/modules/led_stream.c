/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <sys/byteorder.h>

#define MODULE led_stream
#include "module_state_event.h"

#include "led_event.h"
#include "config_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_LED_STREAM_LOG_LEVEL);

#define INCOMING_LED_COLOR_COUNT 3
#define LED_STREAM_DATA_SIZE 8
#define FETCH_CONFIG_SIZE 2
#define LED_ID_POS 7

#define LED_ID(led) ((led) - &leds[0])

#define STEPS_QUEUE_ARRAY_SIZE (CONFIG_DESKTOP_LED_STREAM_QUEUE_SIZE + 1)

struct led {
	const struct led_effect *state_effect;
	struct led_effect led_stream_effect;
	struct led_effect_step steps_queue[STEPS_QUEUE_ARRAY_SIZE];
	u8_t rx_idx;
	u8_t tx_idx;
	bool streaming;
};

static struct led leds[CONFIG_DESKTOP_LED_COUNT];
static bool initialized;


static size_t next_index(size_t index)
{
	return (index + 1) % STEPS_QUEUE_ARRAY_SIZE;
}

static bool queue_data(const struct event_dyndata *dyndata, struct led *led)
{
	static const size_t min_len = CONFIG_DESKTOP_LED_COLOR_COUNT * sizeof(led->steps_queue[led->rx_idx].color.c[0])
			 + sizeof(led->steps_queue[led->rx_idx].substep_count)
			 + sizeof(led->steps_queue[led->rx_idx].substep_time)
			 + sizeof(u8_t); /* LED ID */

	BUILD_ASSERT_MSG(min_len <= LED_STREAM_DATA_SIZE, "");

	if (dyndata->size != LED_STREAM_DATA_SIZE) {
		LOG_WRN("Invalid stream data size (%zu)", dyndata->size);
		return false;
	}

	LOG_DBG("Enqueue effect data");

	size_t pos = 0;

	/* Fill only leds available in the system */
	memcpy(led->steps_queue[led->rx_idx].color.c, &dyndata->data[pos],
	       CONFIG_DESKTOP_LED_COLOR_COUNT);
	pos += INCOMING_LED_COLOR_COUNT;

	led->steps_queue[led->rx_idx].substep_count = sys_get_le16(&dyndata->data[pos]);
	pos += sizeof(led->steps_queue[led->rx_idx].substep_count);

	led->steps_queue[led->rx_idx].substep_time = sys_get_le16(&dyndata->data[pos]);

	if (led->steps_queue[led->rx_idx].substep_count == 0) {
		LOG_WRN("Dropped led_effect with substep count equal 0");
		return false;
	}

	led->rx_idx = next_index(led->rx_idx);

	return true;
}

static void send_effect(const struct led_effect *effect, struct led *led)
{
	LOG_DBG("Send effect to leds");

	struct led_event *led_event = new_led_event();

	led_event->led_id = LED_ID(led);
	led_event->led_effect = effect;

	__ASSERT_NO_MSG(led_event->led_effect->steps);

	EVENT_SUBMIT(led_event);
}

static size_t count_free_places(struct led *led)
{
	size_t len = (STEPS_QUEUE_ARRAY_SIZE + led->rx_idx - led->tx_idx)
		     % STEPS_QUEUE_ARRAY_SIZE;
	return STEPS_QUEUE_ARRAY_SIZE - len - 1;
}

static bool is_queue_empty(struct led *led)
{
	return led->rx_idx == led->tx_idx;
}

static bool store_data(const struct event_dyndata *data, struct led *led)
{
	size_t free_places = count_free_places(led);

	if (free_places > 0) {
		LOG_DBG("Insert data on position %" PRIu8
			", free places %zu", led->rx_idx, free_places);

		if (!queue_data(data, led)) {
			return false;
		}
	} else {
		LOG_WRN("Queue is full - drop incoming step");
	}

	return true;
}

static void send_data_from_queue(struct led *led)
{
	if (!is_queue_empty(led)) {
		led->led_stream_effect.steps = &led->steps_queue[led->tx_idx];
		led->led_stream_effect.step_count = 1;

		send_effect(&led->led_stream_effect, led);

		led->tx_idx = next_index(led->tx_idx);
	} else {
		LOG_INF("No steps ready in queue, stop streaming");

		led->streaming = false;

		send_effect(led->state_effect, led);
	}
}

static void handle_incoming_step(const struct config_event *event)
{
	if (!initialized) {
		LOG_WRN("Not initialized");
		return;
	}

	size_t led_id = event->dyndata.data[LED_ID_POS];

	if (led_id >= ARRAY_SIZE(leds)) {
		LOG_WRN("Wrong LED ID, effect ignored");
		return;
	}

	struct led *led = &leds[led_id];

	if (!store_data(&event->dyndata, led)) {
		return;
	}

	if (!led->streaming) {
		LOG_DBG("Sending first led effect for led %zu", led_id);

		led->streaming = true;

		send_data_from_queue(led);
	}
}

static void handle_config_event(const struct config_event *event)
{
	if (GROUP_FIELD_GET(event->id) != EVENT_GROUP_LED_STREAM) {
		/* Only LED STREAM events. */
		return;
	}

	switch (TYPE_FIELD_GET(event->id)) {
	case LED_STREAM_DATA:
		handle_incoming_step(event);
		break;

	default:
		LOG_WRN("Unknown LED STREAM config event");
		break;
	}
}

static void handle_config_fetch_request_event(
	const struct config_fetch_request_event *event)
{
	if (GROUP_FIELD_GET(event->id) != EVENT_GROUP_LED_STREAM) {
		/* Only LED STREAM events. */
		return;
	}

	LOG_DBG("Handle config fetch request event");

	size_t led_id = MOD_FIELD_GET(event->id);

	if (led_id >= ARRAY_SIZE(leds)) {
		LOG_WRN("Wrong LED id");
		return;
	}

	struct led *led = &leds[led_id];

	u8_t free_places = count_free_places(led);
	u8_t is_ready = initialized;

	LOG_DBG("Free places left: %" PRIu8 " for led: %zu",
		free_places, led_id);

	static const size_t size = sizeof(free_places) + sizeof(is_ready);

	BUILD_ASSERT_MSG(size == FETCH_CONFIG_SIZE, "");

	struct config_fetch_event *fetch_event = new_config_fetch_event(size);

	fetch_event->id = event->id;
	fetch_event->recipient = event->recipient;
	fetch_event->channel_id = event->channel_id;

	size_t pos = 0;

	fetch_event->dyndata.data[pos] = free_places;
	pos += sizeof(free_places);
	fetch_event->dyndata.data[pos] = is_ready;

	EVENT_SUBMIT(fetch_event);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_config_event(eh)) {
		handle_config_event(cast_config_event(eh));
		return false;
	}

	if (is_config_fetch_request_event(eh)) {
		handle_config_fetch_request_event(cast_config_fetch_request_event(eh));
		return false;
	}

	if (is_led_event(eh)) {
		const struct led_event *event = cast_led_event(eh);

		struct led *led = &leds[event->led_id];

		if (event->led_effect != &led->led_stream_effect) {
			__ASSERT_NO_MSG(event->led_effect);

			led->state_effect = event->led_effect;

			if (led->streaming) {
				return true;
			}
		}
		return false;
	}

	if (is_led_ready_event(eh)) {
		const struct led_ready_event *event = cast_led_ready_event(eh);

		struct led *led = &leds[event->led_id];

		if (event->led_effect == &led->led_stream_effect) {
			send_data_from_queue(led);
		}

		return true;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(leds), MODULE_STATE_READY)) {
			if (!initialized) {
				initialized = true;
				module_set_state(MODULE_STATE_READY);
			}
		}

		if (check_state(event, MODULE_ID(leds), MODULE_STATE_OFF) ||
		    check_state(event, MODULE_ID(leds), MODULE_STATE_STANDBY)) {
			initialized = false;
			module_set_state(MODULE_STATE_OFF);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE_EARLY(MODULE, led_event);
EVENT_SUBSCRIBE(MODULE, led_ready_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, config_event);
EVENT_SUBSCRIBE(MODULE, config_fetch_request_event);
