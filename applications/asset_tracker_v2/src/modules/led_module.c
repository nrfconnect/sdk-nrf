/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/events/led_event.h>
#include "events/led_state_event.h"
#include CONFIG_LED_STATE_DEF_PATH

#define MODULE led_state
#include <caf/events/module_state_event.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led, CONFIG_LED_CONTROL_LOG_LEVEL);


static void send_led_event(size_t led_id, const struct led_effect *led_effect)
{
	__ASSERT_NO_MSG(led_effect);
	__ASSERT_NO_MSG(led_id < LED_ID_COUNT);

	struct led_event *event = new_led_event();

	__ASSERT(event, "Not enough heap left to allocate event");

	event->led_id = led_id;
	event->led_effect = led_effect;
	APP_EVENT_SUBMIT(event);
}

static void update_led(enum led_state state)
{
	uint8_t led_bm = 0;

	switch (state)	{
	case LED_STATE_LTE_CONNECTING:
		send_led_event(LED_ID_CONNECTING,
				&asset_tracker_led_effect[LED_STATE_LTE_CONNECTING]);
		led_bm |= BIT(LED_ID_CONNECTING);
		break;
	case LED_STATE_LOCATION_SEARCHING:
		send_led_event(LED_ID_SEARCHING,
				&asset_tracker_led_effect[LED_STATE_LOCATION_SEARCHING]);
		led_bm |= BIT(LED_ID_SEARCHING);
		break;
	case LED_STATE_CLOUD_PUBLISHING:
		send_led_event(LED_ID_PUBLISHING,
				&asset_tracker_led_effect[LED_STATE_CLOUD_PUBLISHING]);
		led_bm |= BIT(LED_ID_PUBLISHING);
		break;
	case LED_STATE_CLOUD_CONNECTING:
		send_led_event(LED_ID_CLOUD_CONNECTING,
			       &asset_tracker_led_effect[LED_STATE_CLOUD_CONNECTING]);
		led_bm |= BIT(LED_ID_CLOUD_CONNECTING);
		break;
	case LED_STATE_CLOUD_ASSOCIATING:
		send_led_event(LED_ID_ASSOCIATING,
			       &asset_tracker_led_effect[LED_STATE_CLOUD_ASSOCIATING]);
		led_bm |= BIT(LED_ID_ASSOCIATING);
		break;
	case LED_STATE_CLOUD_ASSOCIATED:
		send_led_event(LED_ID_ASSOCIATED,
			       &asset_tracker_led_effect[LED_STATE_CLOUD_ASSOCIATED]);
		led_bm |= BIT(LED_ID_ASSOCIATED);
		break;
	case LED_STATE_ACTIVE_MODE:
		send_led_event(LED_ID_ACTIVE_MODE,
				&asset_tracker_led_effect[LED_STATE_ACTIVE_MODE]);
		led_bm |= BIT(LED_ID_ACTIVE_MODE);
		break;
	case LED_STATE_PASSIVE_MODE:
		send_led_event(LED_ID_PASSIVE_MODE_1,
			       &asset_tracker_led_effect[LED_STATE_PASSIVE_MODE]);
		send_led_event(LED_ID_PASSIVE_MODE_2,
			       &asset_tracker_led_effect[LED_STATE_PASSIVE_MODE]);
		led_bm |= (BIT(LED_ID_PASSIVE_MODE_1) | BIT(LED_ID_PASSIVE_MODE_2));
		break;
	case LED_STATE_ERROR_SYSTEM_FAULT:
		for (size_t i = 0; i < LED_ID_COUNT; i++) {
			send_led_event(i,
					&asset_tracker_led_effect[LED_STATE_ERROR_SYSTEM_FAULT]);
			led_bm |= BIT(i);
		}
		break;
	case LED_STATE_FOTA_UPDATING:
		send_led_event(LED_ID_FOTA_1,
			       &asset_tracker_led_effect[LED_STATE_FOTA_UPDATING]);
		send_led_event(LED_ID_FOTA_2,
			       &asset_tracker_led_effect[LED_STATE_FOTA_UPDATING]);
		led_bm |= (BIT(LED_ID_FOTA_1) | BIT(LED_ID_FOTA_2));
		break;
	case LED_STATE_FOTA_UPDATE_REBOOT:
		send_led_event(LED_ID_FOTA_1,
			       &asset_tracker_led_effect[LED_STATE_FOTA_UPDATE_REBOOT]);
		send_led_event(LED_ID_FOTA_2,
			       &asset_tracker_led_effect[LED_STATE_FOTA_UPDATE_REBOOT]);
		led_bm |= (BIT(LED_ID_FOTA_1) | BIT(LED_ID_FOTA_2));
		break;
	case LED_STATE_TURN_OFF:
		/* Do nothing. */
		break;
	default:
		LOG_ERR("Unrecognized LED state event send");
		break;
	}

	for (size_t i = 0; i < LED_ID_COUNT; i++) {
		if (!(led_bm & BIT(i))) {
			send_led_event(i, &asset_tracker_led_effect[LED_STATE_TURN_OFF]);
		}
	}
}

static bool handle_led_state_event(const struct led_state_event *event)
{
	update_led(event->state);
	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_led_state_event(aeh)) {
		return handle_led_state_event(cast_led_state_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, led_state_event);
