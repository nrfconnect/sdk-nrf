/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <app_event_manager.h>
#include <dk_buttons_and_leds.h>

#define MODULE ui_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/data_module_event.h"
#include "events/ui_module_event.h"
#include "events/sensor_module_event.h"
#include "events/util_module_event.h"
#include "events/location_module_event.h"
#include "events/modem_module_event.h"
#include "events/cloud_module_event.h"
#include "events/led_state_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_UI_MODULE_LOG_LEVEL);

/* Define a custom STATIC macro that exposes internal variables when unit testing. */
#if defined(CONFIG_UNITY)
#define STATIC
#else
#define STATIC static
#endif

struct ui_msg_data {
	union {
		struct app_module_event app;
		struct modem_module_event modem;
		struct data_module_event data;
		struct location_module_event location;
		struct util_module_event util;
		struct cloud_module_event cloud;
	} module;
};

/* UI module states. */
STATIC enum state_type {
	STATE_INIT,
	STATE_RUNNING,
	STATE_LTE_CONNECTING,
	STATE_CLOUD_CONNECTING,
	STATE_CLOUD_ASSOCIATING,
	STATE_FOTA_UPDATING,
	STATE_SHUTDOWN
} state;

/* UI module sub states. */
STATIC enum sub_state_type {
	SUB_STATE_ACTIVE,
	SUB_STATE_PASSIVE,
} sub_state;

/* UI module sub-sub states. */
STATIC enum sub_sub_state_type {
	SUB_SUB_STATE_LOCATION_INACTIVE,
	SUB_SUB_STATE_LOCATION_ACTIVE
} sub_sub_state;

/* Forward declarations */
static void led_pattern_update_work_fn(struct k_work *work);

/* Definition used to specify LED patterns that should hold forever. */
#define HOLD_FOREVER -1

/* List of LED patterns supported in the UI module. */
STATIC struct led_pattern {
	/* Variable used to construct a linked list of led patterns. */
	sys_snode_t header;
	/* LED state. */
	enum led_state led_state;
	/* Duration of the LED state. */
	int16_t duration_sec;
} led_pattern_list[LED_STATE_COUNT];

/* Linked list used to schedule multiple LED pattern transitions. */
STATIC sys_slist_t pattern_transition_list = SYS_SLIST_STATIC_INIT(&pattern_transition_list);

/* Delayed work that is used to display and transition to the correct LED pattern depending on the
 * internal state of the module.
 */
static K_WORK_DELAYABLE_DEFINE(led_pattern_update_work, led_pattern_update_work_fn);

/* UI module message queue. */
#define UI_QUEUE_ENTRY_COUNT		10
#define UI_QUEUE_BYTE_ALIGNMENT		4

K_MSGQ_DEFINE(msgq_ui, sizeof(struct ui_msg_data),
	      UI_QUEUE_ENTRY_COUNT, UI_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
	.name = "ui",
	.msg_q = NULL,
	.supports_shutdown = true,
};

/* Forward declarations. */
static void message_handler(struct ui_msg_data *msg);

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type new_state)
{
	switch (new_state) {
	case STATE_INIT:
		return "STATE_INIT";
	case STATE_RUNNING:
		return "STATE_RUNNING";
	case STATE_LTE_CONNECTING:
		return "STATE_LTE_CONNECTING";
	case STATE_CLOUD_CONNECTING:
		return "STATE_CLOUD_CONNECTING";
	case STATE_CLOUD_ASSOCIATING:
		return "STATE_CLOUD_ASSOCIATING";
	case STATE_FOTA_UPDATING:
		return "STATE_FOTA_UPDATING";
	case STATE_SHUTDOWN:
		return "STATE_SHUTDOWN";
	default:
		return "Unknown";
	}
}

/* Convenience functions used in internal state handling. */
static char *sub_state2str(enum sub_state_type new_state)
{
	switch (new_state) {
	case SUB_STATE_ACTIVE:
		return "SUB_STATE_ACTIVE";
	case SUB_STATE_PASSIVE:
		return "SUB_STATE_PASSIVE";
	default:
		return "Unknown";
	}
}

static char *sub_sub_state2str(enum sub_sub_state_type new_state)
{
	switch (new_state) {
	case SUB_SUB_STATE_LOCATION_INACTIVE:
		return "SUB_SUB_STATE_LOCATION_INACTIVE";
	case SUB_SUB_STATE_LOCATION_ACTIVE:
		return "SUB_SUB_STATE_LOCATION_ACTIVE";
	default:
		return "Unknown";
	}
}

static void state_set(enum state_type new_state)
{
	if (new_state == state) {
		LOG_DBG("State: %s", state2str(state));
		return;
	}

	LOG_DBG("State transition %s --> %s",
		state2str(state),
		state2str(new_state));

	state = new_state;
}

static void sub_state_set(enum sub_state_type new_state)
{
	if (new_state == sub_state) {
		LOG_DBG("Sub state: %s", sub_state2str(sub_state));
		return;
	}

	LOG_DBG("Sub state transition %s --> %s",
		sub_state2str(sub_state),
		sub_state2str(new_state));

	sub_state = new_state;
}

static void sub_sub_state_set(enum sub_sub_state_type new_state)
{
	if (new_state == sub_sub_state) {
		LOG_DBG("Sub state: %s", sub_sub_state2str(sub_sub_state));
		return;
	}

	LOG_DBG("Sub state transition %s --> %s",
		sub_sub_state2str(sub_sub_state),
		sub_sub_state2str(new_state));

	sub_sub_state = new_state;
}

/* Handlers */
static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_app_module_event(aeh)) {
		struct app_module_event *event = cast_app_module_event(aeh);
		struct ui_msg_data ui_msg = {
			.module.app = *event
		};

		message_handler(&ui_msg);
	}

	if (is_data_module_event(aeh)) {
		struct data_module_event *event = cast_data_module_event(aeh);
		struct ui_msg_data ui_msg = {
			.module.data = *event
		};

		message_handler(&ui_msg);
	}

	if (is_modem_module_event(aeh)) {
		struct modem_module_event *event = cast_modem_module_event(aeh);
		struct ui_msg_data ui_msg = {
			.module.modem = *event
		};

		message_handler(&ui_msg);
	}

	if (is_location_module_event(aeh)) {
		struct location_module_event *event = cast_location_module_event(aeh);
		struct ui_msg_data ui_msg = {
			.module.location = *event
		};

		message_handler(&ui_msg);
	}

	if (is_util_module_event(aeh)) {
		struct util_module_event *event = cast_util_module_event(aeh);
		struct ui_msg_data ui_msg = {
			.module.util = *event
		};

		message_handler(&ui_msg);
	}

	if (is_cloud_module_event(aeh)) {
		struct cloud_module_event *event = cast_cloud_module_event(aeh);
		struct ui_msg_data ui_msg = {
			.module.cloud = *event
		};

		message_handler(&ui_msg);
	}

	return false;
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {

		struct ui_module_event *ui_module_event = new_ui_module_event();

		__ASSERT(ui_module_event, "Not enough heap left to allocate event");

		ui_module_event->type = UI_EVT_BUTTON_DATA_READY;
		ui_module_event->data.ui.button_number = 1;
		ui_module_event->data.ui.timestamp = k_uptime_get();

		APP_EVENT_SUBMIT(ui_module_event);
	}

#if defined(CONFIG_BOARD_NRF9160DK_NRF9160_NS)
	if (has_changed & button_states & DK_BTN2_MSK) {

		struct ui_module_event *ui_module_event = new_ui_module_event();

		__ASSERT(ui_module_event, "Not enough heap left to allocate event");

		ui_module_event->type = UI_EVT_BUTTON_DATA_READY;
		ui_module_event->data.ui.button_number = 2;
		ui_module_event->data.ui.timestamp = k_uptime_get();

		APP_EVENT_SUBMIT(ui_module_event);

	}
#endif
}

/* Static module functions. */
static void update_led_pattern(enum led_state pattern)
{
#if defined(CONFIG_LED_CONTROL)
	struct led_state_event *event = new_led_state_event();

	__ASSERT(event, "Not enough heap left to allocate event");

	event->state = pattern;
	APP_EVENT_SUBMIT(event);
#endif
}

static void led_pattern_update_work_fn(struct k_work *work)
{
	struct led_pattern *next_pattern;
	static enum led_state previous_led_state = LED_STATE_COUNT;
	sys_snode_t *node = sys_slist_get(&pattern_transition_list);

	if (node == NULL) {
		LOG_ERR("Cannot find any more LED pattern transitions");
		return;
	}

	next_pattern = CONTAINER_OF(node, struct led_pattern, header);

	/* Prevent the same LED led_state from being scheduled twice in a row. */
	if (next_pattern->led_state != previous_led_state) {
		update_led_pattern(next_pattern->led_state);
		previous_led_state = next_pattern->led_state;
	}

	/* Even if the LED state is not updated due a match with the previous state a LED pattern
	 * update is scheduled. This will prolong the pattern until the LED pattern transition
	 * list is cleared.
	 */
	if (next_pattern->duration_sec > 0) {
		k_work_reschedule(&led_pattern_update_work, K_SECONDS(next_pattern->duration_sec));
	}
}

static int setup(void)
{

	int err;

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
		return err;
	}

	return 0;
}

/* Function that checks if incoming event causes cloud activity. */
static bool is_cloud_related_event(struct ui_msg_data *msg)
{
	if ((IS_EVENT(msg, data, DATA_EVT_DATA_SEND)) ||
	    (IS_EVENT(msg, cloud, CLOUD_EVT_CONNECTED)) ||
	    (IS_EVENT(msg, data, DATA_EVT_UI_DATA_SEND)) ||
	    (IS_EVENT(msg, data, DATA_EVT_DATA_SEND_BATCH)) ||
	    (IS_EVENT(msg, data, DATA_EVT_CLOUD_LOCATION_DATA_SEND))) {
		return true;
	}

	return false;
}

/* Function that clears LED pattern transition list. */
STATIC void transition_list_clear(void)
{
	struct led_pattern *transition, *next_transition = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&pattern_transition_list,
					  transition,
					  next_transition,
					  header) {
		sys_slist_remove(&pattern_transition_list, NULL, &transition->header);
	};
}

/* Function that appends a LED state and a corresponding duration to the
 * LED pattern transition list.
 */
static void transition_list_append(enum led_state led_state, int16_t duration_sec)
{
	led_pattern_list[led_state].led_state = led_state;
	led_pattern_list[led_state].duration_sec = duration_sec;

	sys_slist_append(&pattern_transition_list, &led_pattern_list[led_state].header);
}

/* Message handler for SUB_SUB_STATE_LOCATION_ACTIVE in SUB_STATE_ACTIVE. */
static void on_active_location_active(struct ui_msg_data *msg)
{
	if (is_cloud_related_event(msg)) {
		transition_list_clear();
		transition_list_append(LED_STATE_CLOUD_PUBLISHING, 5);
		transition_list_append(LED_STATE_ACTIVE_MODE, 5);
		transition_list_append(LED_STATE_LOCATION_SEARCHING, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
	}
}

/* Message handler for SUB_SUB_STATE_LOCATION_INACTIVE in SUB_STATE_ACTIVE. */
static void on_active_location_inactive(struct ui_msg_data *msg)
{
	if (is_cloud_related_event(msg)) {
		transition_list_clear();
		transition_list_append(LED_STATE_CLOUD_PUBLISHING, 5);
		transition_list_append(LED_STATE_ACTIVE_MODE, 5);
		transition_list_append(LED_STATE_TURN_OFF, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
	}
}

/* Message handler for SUB_SUB_STATE_LOCATION_ACTIVE in SUB_STATE_PASSIVE. */
static void on_passive_location_active(struct ui_msg_data *msg)
{
	if (is_cloud_related_event(msg)) {
		transition_list_clear();
		transition_list_append(LED_STATE_CLOUD_PUBLISHING, 5);
		transition_list_append(LED_STATE_PASSIVE_MODE, 5);
		transition_list_append(LED_STATE_LOCATION_SEARCHING, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
	}
}

/* Message handler for SUB_SUB_STATE_LOCATION_INACTIVE in SUB_STATE_PASSIVE. */
static void on_passive_location_inactive(struct ui_msg_data *msg)
{
	if (is_cloud_related_event(msg)) {
		transition_list_clear();
		transition_list_append(LED_STATE_CLOUD_PUBLISHING, 5);
		transition_list_append(LED_STATE_PASSIVE_MODE, 5);
		transition_list_append(LED_STATE_TURN_OFF, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
	}
}

/* Message handler for STATE_INIT. */
static void on_state_init(struct ui_msg_data *msg)
{
	if (IS_EVENT(msg, app, APP_EVT_START)) {
		int err = module_start(&self);

		if (err) {
			LOG_ERR("Failed starting module, error: %d", err);
			SEND_ERROR(ui, UI_EVT_ERROR, err);
		}

		state_set(STATE_RUNNING);
		sub_state_set(SUB_STATE_ACTIVE);
		sub_sub_state_set(SUB_SUB_STATE_LOCATION_INACTIVE);
	}
}

/* Message handler for STATE_RUNNING. */
static void on_state_running(struct ui_msg_data *msg)
{
	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_ACTIVE)) {
		transition_list_clear();
		transition_list_append(LED_STATE_LOCATION_SEARCHING, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
	}

	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_INACTIVE)) {
		transition_list_clear();
		transition_list_append(LED_STATE_TURN_OFF, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
	}

}

/* Message handler for STATE_LTE_CONNECTING. */
static void on_state_lte_connecting(struct ui_msg_data *msg)
{
	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_CONNECTED)) {
		transition_list_clear();
		transition_list_append(LED_STATE_TURN_OFF, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
		state_set(STATE_RUNNING);
	}
}

/* Message handler for STATE_CLOUD_CONNECTING. */
static void on_state_cloud_connecting(struct ui_msg_data *msg)
{
	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONNECTED)) {
		transition_list_clear();
		transition_list_append(LED_STATE_TURN_OFF, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
		state_set(STATE_RUNNING);
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_USER_ASSOCIATED)) {
		transition_list_clear();
		transition_list_append(LED_STATE_CLOUD_ASSOCIATED, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
		state_set(STATE_RUNNING);
	}
}

/* Message handler for STATE_CLOUD_ASSOCIATING. */
static void on_state_cloud_associating(struct ui_msg_data *msg)
{
	if (IS_EVENT(msg, cloud, CLOUD_EVT_USER_ASSOCIATED)) {
		transition_list_clear();
		transition_list_append(LED_STATE_CLOUD_ASSOCIATED, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
		state_set(STATE_RUNNING);
	}
}

/* Message handler for STATE_FOTA_UPDATING. */
static void on_state_fota_update(struct ui_msg_data *msg)
{
	if ((IS_EVENT(msg, cloud, CLOUD_EVT_FOTA_DONE)) ||
	    (IS_EVENT(msg, cloud, CLOUD_EVT_FOTA_ERROR))) {
		transition_list_clear();
		transition_list_append(LED_STATE_TURN_OFF, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
		state_set(STATE_RUNNING);
	}
}

/* Message handler for all states. */
static void on_all_states(struct ui_msg_data *msg)
{
	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_CONNECTING)) {
		transition_list_clear();
		transition_list_append(LED_STATE_LTE_CONNECTING, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
		state_set(STATE_LTE_CONNECTING);
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONNECTING)) {
		transition_list_clear();
		transition_list_append(LED_STATE_CLOUD_CONNECTING, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
		state_set(STATE_CLOUD_CONNECTING);
	}

	if (IS_EVENT(msg, util, UTIL_EVT_SHUTDOWN_REQUEST)) {

		transition_list_clear();

		switch (msg->module.util.reason) {
		case REASON_FOTA_UPDATE:
			transition_list_append(LED_STATE_FOTA_UPDATE_REBOOT, HOLD_FOREVER);
			break;
		case REASON_GENERIC:
			transition_list_append(LED_STATE_ERROR_SYSTEM_FAULT, HOLD_FOREVER);
			break;
		default:
			LOG_ERR("Unknown shutdown reason");
			break;
		}

		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);

		SEND_SHUTDOWN_ACK(ui, UI_EVT_SHUTDOWN_READY, self.id);
		state_set(STATE_SHUTDOWN);
	}

	if ((IS_EVENT(msg, data, DATA_EVT_CONFIG_INIT)) ||
	    (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY))) {
		sub_state_set(msg->module.data.data.cfg.active_mode ?
			      SUB_STATE_ACTIVE :
			      SUB_STATE_PASSIVE);
	}

	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_ACTIVE)) {
		sub_sub_state_set(SUB_SUB_STATE_LOCATION_ACTIVE);
	}

	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_INACTIVE)) {
		sub_sub_state_set(SUB_SUB_STATE_LOCATION_INACTIVE);
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_FOTA_START)) {
		transition_list_clear();
		transition_list_append(LED_STATE_FOTA_UPDATING, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
		state_set(STATE_FOTA_UPDATING);
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_USER_ASSOCIATION_REQUEST)) {
		transition_list_clear();
		transition_list_append(LED_STATE_CLOUD_ASSOCIATING, HOLD_FOREVER);
		k_work_reschedule(&led_pattern_update_work, K_NO_WAIT);
		state_set(STATE_CLOUD_ASSOCIATING);
	}
}

static void message_handler(struct ui_msg_data *msg)
{
	switch (state) {
	case STATE_INIT:
		on_state_init(msg);
		break;
	case STATE_RUNNING:
		switch (sub_state) {
		case SUB_STATE_ACTIVE:
			switch (sub_sub_state) {
			case SUB_SUB_STATE_LOCATION_ACTIVE:
				on_active_location_active(msg);
				break;
			case SUB_SUB_STATE_LOCATION_INACTIVE:
				on_active_location_inactive(msg);
				break;
			default:
				LOG_ERR("Unknown sub-sub state.");
				break;
			}
			break;
		case SUB_STATE_PASSIVE:
			switch (sub_sub_state) {
			case SUB_SUB_STATE_LOCATION_ACTIVE:
				on_passive_location_active(msg);
				break;
			case SUB_SUB_STATE_LOCATION_INACTIVE:
				on_passive_location_inactive(msg);
				break;
			default:
				LOG_ERR("Unknown sub-sub state.");
				break;
			}
			break;
		default:
			LOG_ERR("Unknown sub state.");
			break;
		}
		on_state_running(msg);
		break;
	case STATE_LTE_CONNECTING:
		on_state_lte_connecting(msg);
		break;
	case STATE_CLOUD_CONNECTING:
		on_state_cloud_connecting(msg);
		break;
	case STATE_CLOUD_ASSOCIATING:
		on_state_cloud_associating(msg);
		break;
	case STATE_FOTA_UPDATING:
		on_state_fota_update(msg);
		break;
	case STATE_SHUTDOWN:
		/* The shutdown state has no transition. */
		break;
	default:
		LOG_ERR("Unknown state.");
		break;
	}

	on_all_states(msg);
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, app_module_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, data_module_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, location_module_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, modem_module_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, util_module_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, cloud_module_event);

SYS_INIT(setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
