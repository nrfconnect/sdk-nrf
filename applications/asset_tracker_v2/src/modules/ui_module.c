/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <event_manager.h>
#include <dk_buttons_and_leds.h>

#include "led.h"

#define MODULE ui_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/data_module_event.h"
#include "events/ui_module_event.h"
#include "events/sensor_module_event.h"
#include "events/util_module_event.h"
#include "events/gps_module_event.h"
#include "events/modem_module_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_UI_MODULE_LOG_LEVEL);

struct ui_msg_data {
	union {
		struct app_module_event app;
		struct modem_module_event modem;
		struct data_module_event data;
		struct gps_module_event gps;
		struct util_module_event util;
	} module;
};

/* UI module states. */
static enum state_type {
	STATE_ACTIVE,
	STATE_PASSIVE,
	STATE_SHUTDOWN
} state;

/* UI module sub states. */
static enum sub_state_type {
	SUB_STATE_GPS_INACTIVE,
	SUB_STATE_GPS_ACTIVE
} sub_state;

/* Forward declarations */
static void led_pat_active_work_fn(struct k_work *work);
static void led_pat_passive_work_fn(struct k_work *work);
static void led_pat_gps_work_fn(struct k_work *work);

/* Delayed works that is used to make sure the device always reverts back to the
 * device mode or GPS search LED pattern.
 */
static K_WORK_DELAYABLE_DEFINE(led_pat_active_work, led_pat_active_work_fn);
static K_WORK_DELAYABLE_DEFINE(led_pat_passive_work, led_pat_passive_work_fn);
static K_WORK_DELAYABLE_DEFINE(led_pat_gps_work, led_pat_gps_work_fn);

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
	case STATE_ACTIVE:
		return "STATE_ACTIVE";
	case STATE_PASSIVE:
		return "STATE_PASSIVE";
	case STATE_SHUTDOWN:
		return "STATE_SHUTDOWN";
	default:
		return "Unknown";
	}
}

static char *sub_state2str(enum sub_state_type new_state)
{
	switch (new_state) {
	case SUB_STATE_GPS_INACTIVE:
		return "SUB_STATE_GPS_INACTIVE";
	case SUB_STATE_GPS_ACTIVE:
		return "SUB_STATE_GPS_ACTIVE";
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

/* Handlers */
static bool event_handler(const struct event_header *eh)
{
	if (is_app_module_event(eh)) {
		struct app_module_event *event = cast_app_module_event(eh);
		struct ui_msg_data ui_msg = {
			.module.app = *event
		};

		message_handler(&ui_msg);
	}

	if (is_data_module_event(eh)) {
		struct data_module_event *event = cast_data_module_event(eh);
		struct ui_msg_data ui_msg = {
			.module.data = *event
		};

		message_handler(&ui_msg);
	}

	if (is_modem_module_event(eh)) {
		struct modem_module_event *event = cast_modem_module_event(eh);
		struct ui_msg_data ui_msg = {
			.module.modem = *event
		};

		message_handler(&ui_msg);
	}

	if (is_gps_module_event(eh)) {
		struct gps_module_event *event = cast_gps_module_event(eh);
		struct ui_msg_data ui_msg = {
			.module.gps = *event
		};

		message_handler(&ui_msg);
	}

	if (is_util_module_event(eh)) {
		struct util_module_event *event = cast_util_module_event(eh);
		struct ui_msg_data ui_msg = {
			.module.util = *event
		};

		message_handler(&ui_msg);
	}

	return false;
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {

		struct ui_module_event *ui_module_event =
				new_ui_module_event();

		ui_module_event->type = UI_EVT_BUTTON_DATA_READY;
		ui_module_event->data.ui.button_number = 1;
		ui_module_event->data.ui.timestamp = k_uptime_get();

		EVENT_SUBMIT(ui_module_event);
	}

#if defined(CONFIG_BOARD_NRF9160DK_NRF9160NS)
	if (has_changed & button_states & DK_BTN2_MSK) {

		struct ui_module_event *ui_module_event =
				new_ui_module_event();

		ui_module_event->type = UI_EVT_BUTTON_DATA_READY;
		ui_module_event->data.ui.button_number = 2;
		ui_module_event->data.ui.timestamp = k_uptime_get();

		EVENT_SUBMIT(ui_module_event);

	}
#endif
}

/* Static module functions. */
static void update_led_pattern(enum led_pattern pattern)
{
#if defined(CONFIG_LED_CONTROL)
	led_set_pattern(pattern);
#endif
}

static void led_pat_active_work_fn(struct k_work *work)
{
	update_led_pattern(LED_ACTIVE_MODE);
}

static void led_pat_passive_work_fn(struct k_work *work)
{
	update_led_pattern(LED_PASSIVE_MODE);
}

static void led_pat_gps_work_fn(struct k_work *work)
{
	update_led_pattern(LED_GPS_SEARCHING);
}

static int setup(const struct device *dev)
{
	ARG_UNUSED(dev);

	int err;

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
		return err;
	}

#if defined(CONFIG_LED_CONTROL)
	err = led_init();
	if (err) {
		LOG_ERR("led_init, error: %d", err);
		return err;
	}
#endif
	return 0;
}

/* Message handler for SUB_STATE_GPS_ACTIVE in STATE_ACTIVE. */
static void on_state_active_sub_state_gps_active(struct ui_msg_data *msg)
{
	if (IS_EVENT(msg, gps, GPS_EVT_INACTIVE)) {
		update_led_pattern(LED_ACTIVE_MODE);
		sub_state_set(SUB_STATE_GPS_INACTIVE);
	}

	if ((IS_EVENT(msg, data, DATA_EVT_DATA_SEND)) ||
	    (IS_EVENT(msg, data, DATA_EVT_UI_DATA_SEND))) {
		update_led_pattern(LED_CLOUD_PUBLISHING);
		k_work_reschedule(&led_pat_gps_work, K_SECONDS(5));
	}

	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY)) {
		if (!msg->module.data.data.cfg.active_mode) {
			state_set(STATE_PASSIVE);
			k_work_reschedule(&led_pat_gps_work,
					      K_SECONDS(5));
		}
	}
}

/* Message handler for SUB_STATE_GPS_INACTIVE in STATE_ACTIVE. */
static void on_state_active_sub_state_gps_inactive(struct ui_msg_data *msg)
{
	if (IS_EVENT(msg, gps, GPS_EVT_ACTIVE)) {
		update_led_pattern(LED_GPS_SEARCHING);
		sub_state_set(SUB_STATE_GPS_ACTIVE);
	}

	if ((IS_EVENT(msg, data, DATA_EVT_DATA_SEND)) ||
	    (IS_EVENT(msg, data, DATA_EVT_UI_DATA_SEND))) {
		update_led_pattern(LED_CLOUD_PUBLISHING);
		k_work_reschedule(&led_pat_active_work, K_SECONDS(5));
	}

	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY)) {
		if (!msg->module.data.data.cfg.active_mode) {
			state_set(STATE_PASSIVE);
			k_work_reschedule(&led_pat_passive_work,
					      K_SECONDS(5));
		}
	}
}

/* Message handler for SUB_STATE_GPS_ACTIVE in STATE_PASSIVE. */
static void on_state_passive_sub_state_gps_active(struct ui_msg_data *msg)
{
	if (IS_EVENT(msg, gps, GPS_EVT_INACTIVE)) {
		update_led_pattern(LED_PASSIVE_MODE);
		sub_state_set(SUB_STATE_GPS_INACTIVE);
	}

	if ((IS_EVENT(msg, data, DATA_EVT_DATA_SEND)) ||
	    (IS_EVENT(msg, data, DATA_EVT_UI_DATA_SEND))) {
		update_led_pattern(LED_CLOUD_PUBLISHING);
		k_work_reschedule(&led_pat_gps_work, K_SECONDS(5));
	}

	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY)) {
		if (msg->module.data.data.cfg.active_mode) {
			state_set(STATE_ACTIVE);
			k_work_reschedule(&led_pat_gps_work,
					      K_SECONDS(5));
		}
	}
}

/* Message handler for SUB_STATE_GPS_INACTIVE in STATE_PASSIVE. */
static void on_state_passive_sub_state_gps_inactive(struct ui_msg_data *msg)
{
	if (IS_EVENT(msg, gps, GPS_EVT_ACTIVE)) {
		update_led_pattern(LED_GPS_SEARCHING);
		sub_state_set(SUB_STATE_GPS_ACTIVE);
	}

	if ((IS_EVENT(msg, data, DATA_EVT_DATA_SEND)) ||
	    (IS_EVENT(msg, data, DATA_EVT_UI_DATA_SEND))) {
		update_led_pattern(LED_CLOUD_PUBLISHING);
		k_work_reschedule(&led_pat_passive_work, K_SECONDS(5));
	}

	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY)) {
		if (msg->module.data.data.cfg.active_mode) {
			state_set(STATE_ACTIVE);
			k_work_reschedule(&led_pat_active_work,
					      K_SECONDS(5));
		}
	}
}

/* Message handler for all states. */
static void on_all_states(struct ui_msg_data *msg)
{
	if (IS_EVENT(msg, app, APP_EVT_START)) {
		int err = module_start(&self);

		if (err) {
			LOG_ERR("Failed starting module, error: %d", err);
			SEND_ERROR(ui, UI_EVT_ERROR, err);
		}

		state_set(STATE_ACTIVE);
		sub_state_set(SUB_STATE_GPS_INACTIVE);
	}

	if (IS_EVENT(msg, util, UTIL_EVT_SHUTDOWN_REQUEST)) {
		switch (msg->module.util.reason) {
		case REASON_FOTA_UPDATE:
			update_led_pattern(LED_FOTA_UPDATE_REBOOT);
			break;
		case REASON_GENERIC:
			update_led_pattern(LED_ERROR_SYSTEM_FAULT);
			break;
		default:
			LOG_WRN("Unknown shutdown reason");
			break;
		}

		SEND_SHUTDOWN_ACK(ui, UI_EVT_SHUTDOWN_READY, self.id);
		state_set(STATE_SHUTDOWN);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_CONNECTING)) {
		update_led_pattern(LED_LTE_CONNECTING);
	}

	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_INIT)) {
		state_set(msg->module.data.data.cfg.active_mode ?
			 STATE_ACTIVE :
			 STATE_PASSIVE);
	}
}

static void message_handler(struct ui_msg_data *msg)
{
	switch (state) {
	case STATE_ACTIVE:
		switch (sub_state) {
		case SUB_STATE_GPS_ACTIVE:
			on_state_active_sub_state_gps_active(msg);
			break;
		case SUB_STATE_GPS_INACTIVE:
			on_state_active_sub_state_gps_inactive(msg);
			break;
		default:
			LOG_WRN("Unknown ui module sub state.");
			break;
		}
		break;
	case STATE_PASSIVE:
		switch (sub_state) {
		case SUB_STATE_GPS_ACTIVE:
			on_state_passive_sub_state_gps_active(msg);
			break;
		case SUB_STATE_GPS_INACTIVE:
			on_state_passive_sub_state_gps_inactive(msg);
			break;
		default:
			LOG_WRN("Unknown ui module sub state.");
			break;
		}
		break;
	case STATE_SHUTDOWN:
		/* The shutdown state has no transition. */
		break;
	default:
		LOG_WRN("Unknown ui module state.");
		break;
	}

	on_all_states(msg);
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE_EARLY(MODULE, app_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, data_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, gps_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, modem_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, util_module_event);

SYS_INIT(setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
