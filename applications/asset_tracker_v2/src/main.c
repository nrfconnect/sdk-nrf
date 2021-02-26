/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <event_manager.h>
#include <modem/nrf_modem_lib.h>

#if defined(CONFIG_WATCHDOG_APPLICATION)
#include "watchdog.h"
#endif

/* Module name is used by the event manager macros in this file */
#define MODULE app_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/cloud_module_event.h"
#include "events/data_module_event.h"
#include "events/sensor_module_event.h"
#include "events/ui_module_event.h"
#include "events/util_module_event.h"
#include "events/modem_module_event.h"

#include <logging/log.h>
#include <logging/log_ctrl.h>

LOG_MODULE_REGISTER(MODULE, CONFIG_APPLICATION_MODULE_LOG_LEVEL);

/* Message structure. Events from other modules are converted to messages
 * in the event manager handler, and then queued up in the message queue
 * for processing in the main thread.
 */
struct app_msg_data {
	union {
		struct cloud_module_event cloud;
		struct ui_module_event ui;
		struct sensor_module_event sensor;
		struct data_module_event data;
		struct util_module_event util;
		struct modem_module_event modem;
		struct app_module_event app;
	} module;
};

/* Application module super states. */
static enum state_type {
	STATE_INIT,
	STATE_RUNNING,
	STATE_SHUTDOWN
} state;

/* Application sub states. The application can be in either active or passive
 * mode.
 *
 * Active mode: Sensor data and GPS position is acquired at a configured
 *		interval and sent to cloud.
 *
 * Passive mode: Sensor data and GPS position is acquired when movement is
 *		 detected, or after the configured movement timeout occurs.
 */
static enum sub_state_type {
	SUB_STATE_ACTIVE_MODE,
	SUB_STATE_PASSIVE_MODE,
} sub_state;

/* Internal copy of the device configuration. */
static struct cloud_data_cfg app_cfg;

/* Timer callback used to signal when timeout has occurred both in active
 * and passive mode.
 */
static void data_sample_timer_handler(struct k_timer *timer);

/* Application module message queue. */
#define APP_QUEUE_ENTRY_COUNT		10
#define APP_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_app, sizeof(struct app_msg_data), APP_QUEUE_ENTRY_COUNT,
	      APP_QUEUE_BYTE_ALIGNMENT);

/* Data sample timer used in active mode. */
K_TIMER_DEFINE(data_sample_timer, data_sample_timer_handler, NULL);

/* Movement timer used to detect movement timeouts in passive mode. */
K_TIMER_DEFINE(movement_timeout_timer, data_sample_timer_handler, NULL);

/* Movement resolution timer decides the period after movement that consecutive
 * movements are ignored and do not cause data collection. This is used to
 * lower power consumption by limiting how often GPS search is performed and
 * data is sent on air.
 */
K_TIMER_DEFINE(movement_resolution_timer, NULL, NULL);

/* Module data structure to hold information of the application module, which
 * opens up for using convenience functions available for modules.
 */
static struct module_data self = {
	.name = "app",
	.msg_q = &msgq_app,
};

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type new_state)
{
	switch (new_state) {
	case STATE_INIT:
		return "STATE_INIT";
	case STATE_RUNNING:
		return "STATE_RUNNING";
	case STATE_SHUTDOWN:
		return "STATE_SHUTDOWN";
	default:
		return "Unknown";
	}
}

static char *sub_state2str(enum sub_state_type new_state)
{
	switch (new_state) {
	case SUB_STATE_ACTIVE_MODE:
		return "SUB_STATE_ACTIVE_MODE";
	case SUB_STATE_PASSIVE_MODE:
		return "SUB_STATE_PASSIVE_MODE";
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

/* Check the return code from nRF modem library initializaton to ensure that
 * the modem is rebooted if a modem firmware update is ready to be applied or
 * an error condition occurred during firmware update or library initialization.
 */
static void handle_nrf_modem_lib_init_ret(void)
{
	int ret = nrf_modem_lib_get_init_ret();

	/* Handle return values relating to modem firmware update */
	switch (ret) {
	case 0:
		/* Initialization successful, no action required. */
		return;
	case MODEM_DFU_RESULT_OK:
		LOG_INF("MODEM UPDATE OK. Will run new firmware after reboot");
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		LOG_ERR("MODEM UPDATE ERROR %d. Will run old firmware", ret);
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		LOG_ERR("MODEM UPDATE FATAL ERROR %d. Modem failure", ret);
		break;
	default:
		/* All non-zero return codes other than DFU result codes are
		 * considered irrecoverable and a reboot is needed.
		 */
		LOG_ERR("nRF modem lib initialization failed, error: %d", ret);
		break;
	}

	LOG_WRN("Rebooting...");
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);
}

/* Event manager handler. Puts event data into messages and adds them to the
 * application message queue.
 */
static bool event_handler(const struct event_header *eh)
{
	struct app_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_cloud_module_event(eh)) {
		struct cloud_module_event *evt = cast_cloud_module_event(eh);

		msg.module.cloud = *evt;
		enqueue_msg = true;
	}

	if (is_app_module_event(eh)) {
		struct app_module_event *evt = cast_app_module_event(eh);

		msg.module.app = *evt;
		enqueue_msg = true;
	}

	if (is_data_module_event(eh)) {
		struct data_module_event *evt = cast_data_module_event(eh);

		msg.module.data = *evt;
		enqueue_msg = true;
	}

	if (is_sensor_module_event(eh)) {
		struct sensor_module_event *evt = cast_sensor_module_event(eh);

		msg.module.sensor = *evt;
		enqueue_msg = true;
	}

	if (is_util_module_event(eh)) {
		struct util_module_event *evt = cast_util_module_event(eh);

		msg.module.util = *evt;
		enqueue_msg = true;
	}

	if (is_modem_module_event(eh)) {
		struct modem_module_event *evt = cast_modem_module_event(eh);

		msg.module.modem = *evt;
		enqueue_msg = true;
	}

	if (is_ui_module_event(eh)) {
		struct ui_module_event *evt = cast_ui_module_event(eh);

		msg.module.ui = *evt;
		enqueue_msg = true;
	}

	if (enqueue_msg) {
		int err = module_enqueue_msg(&self, &msg);

		if (err) {
			LOG_ERR("Message could not be enqueued");
			SEND_ERROR(app, APP_EVT_ERROR, err);
		}
	}

	return false;
}

static void data_sample_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	SEND_EVENT(app, APP_EVT_DATA_GET_ALL);
}

/* Static module functions. */
static void data_get(void)
{
	static bool first = true;
	struct app_module_event *app_module_event = new_app_module_event();
	size_t count = 0;

	/* Specify which data that is to be included in the transmission. */
	app_module_event->data_list[count++] = APP_DATA_MODEM_DYNAMIC;
	app_module_event->data_list[count++] = APP_DATA_BATTERY;
	app_module_event->data_list[count++] = APP_DATA_ENVIRONMENTAL;

	/* Specify a timeout that each module has to fetch data. If data is not
	 * fetched within this timeout, the data that is available is sent.
	 *
	 * The reason for having at least 65 seconds timeout is that the GNSS
	 * module in nRF9160 will always search for at least 60 seconds for the
	 * first position fix after a reboot.
	 *
	 * The addition of 5 seconds to the configured GPS timeout is  done
	 * to let the GPS module run the currently ongoing search until
	 * the end. If the timeout for sending data is exactly the same as for
	 * the GPS search, a fix occurring at the same time as timeout is
	 * triggered will be missed and not sent to cloud before the next
	 * interval has  passed in active mode, or until next movement in
	 * passive mode.
	 */
	app_module_event->timeout = MAX(app_cfg.gps_timeout + 5, 65);

	if (first) {
		if (IS_ENABLED(CONFIG_APP_REQUEST_GPS_ON_INITIAL_SAMPLING)) {
			app_module_event->data_list[count++] = APP_DATA_GNSS;
		} else {
			app_module_event->timeout = 10;
		}

		app_module_event->data_list[count++] = APP_DATA_MODEM_STATIC;
		first = false;
	} else {
		app_module_event->data_list[count++] = APP_DATA_GNSS;
	}

	/* Set list count to number of data types passed in app_module_event. */
	app_module_event->count = count;
	app_module_event->type = APP_EVT_DATA_GET;

	EVENT_SUBMIT(app_module_event);
}

/* Message handler for STATE_INIT. */
static void on_state_init(struct app_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_INIT)) {
		/* Keep a copy of the new configuration. */
		app_cfg = msg->module.data.data.cfg;

		if (app_cfg.active_mode) {
			LOG_INF("Device mode: Active");
			LOG_INF("Start data sample timer: %d seconds interval",
				app_cfg.active_wait_timeout);
			k_timer_start(&data_sample_timer,
				      K_SECONDS(app_cfg.active_wait_timeout),
				      K_SECONDS(app_cfg.active_wait_timeout));
		} else {
			LOG_INF("Device mode: Passive");
			LOG_INF("Start movement timeout: %d seconds interval",
				app_cfg.movement_timeout);

			k_timer_start(&movement_timeout_timer,
				K_SECONDS(app_cfg.movement_timeout),
				K_SECONDS(app_cfg.movement_timeout));
		}

		state_set(STATE_RUNNING);
		sub_state_set(app_cfg.active_mode ? SUB_STATE_ACTIVE_MODE :
						    SUB_STATE_PASSIVE_MODE);
	}
}

/* Message handler for STATE_RUNNING. */
static void on_state_running(struct app_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_DATE_TIME_OBTAINED)) {
		data_get();
	}

	if (IS_EVENT(msg, app, APP_EVT_DATA_GET_ALL)) {
		data_get();
	}
}

/* Message handler for SUB_STATE_PASSIVE_MODE. */
void on_sub_state_passive(struct app_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY)) {
		/* Keep a copy of the new configuration. */
		app_cfg = msg->module.data.data.cfg;

		if (app_cfg.active_mode) {
			LOG_INF("Device mode: Active");
			LOG_INF("Start data sample timer: %d seconds interval",
				app_cfg.active_wait_timeout);
			k_timer_start(&data_sample_timer,
				      K_SECONDS(app_cfg.active_wait_timeout),
				      K_SECONDS(app_cfg.active_wait_timeout));
			k_timer_stop(&movement_timeout_timer);
			sub_state_set(SUB_STATE_ACTIVE_MODE);
			return;
		}

		LOG_INF("Device mode: Passive");
		LOG_INF("Start movement timeout: %d seconds interval",
			app_cfg.movement_timeout);

		k_timer_start(&movement_timeout_timer,
			      K_SECONDS(app_cfg.movement_timeout),
			      K_SECONDS(app_cfg.movement_timeout));
		k_timer_stop(&data_sample_timer);
	}

	if ((IS_EVENT(msg, sensor, SENSOR_EVT_MOVEMENT_DATA_READY)) ||
	    (IS_EVENT(msg, ui, UI_EVT_BUTTON_DATA_READY))) {

		if (IS_EVENT(msg, ui, UI_EVT_BUTTON_DATA_READY) &&
		    msg->module.ui.data.ui.button_number != 2) {
			return;
		}

		/* Trigger sample/publication cycle if there has been movement
		 * or button 2 has been pushed on the DK.
		 */

		if (k_timer_remaining_get(&movement_resolution_timer) == 0) {
			/* Do an initial data sample. */
			data_sample_timer_handler(NULL);

			LOG_INF("%d seconds until movement can trigger",
				app_cfg.movement_resolution);
			LOG_INF("a new data sample/publication");

			/* Start one shot timer. After the timer has expired,
			 * movement is the only event that triggers a new
			 * one shot timer.
			 */
			k_timer_start(&movement_resolution_timer,
				      K_SECONDS(app_cfg.movement_resolution),
				      K_SECONDS(0));
		}
	}
}

/* Message handler for SUB_STATE_ACTIVE_MODE. */
static void on_sub_state_active(struct app_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY)) {
		/* Keep a copy of the new configuration. */
		app_cfg = msg->module.data.data.cfg;

		if (!app_cfg.active_mode) {
			LOG_INF("Device mode: Passive");
			LOG_INF("Start movement timeout: %d seconds interval",
				app_cfg.movement_timeout);
			k_timer_start(&movement_timeout_timer,
				      K_SECONDS(app_cfg.movement_timeout),
				      K_SECONDS(app_cfg.movement_timeout));
			k_timer_stop(&data_sample_timer);
			sub_state_set(SUB_STATE_PASSIVE_MODE);
			return;
		}

		LOG_INF("Device mode: Active");
		LOG_INF("Start data sample timer: %d seconds interval",
			app_cfg.active_wait_timeout);

		k_timer_start(&data_sample_timer,
			      K_SECONDS(app_cfg.active_wait_timeout),
			      K_SECONDS(app_cfg.active_wait_timeout));
		k_timer_stop(&movement_timeout_timer);
	}
}

/* Message handler for all states. */
static void on_all_events(struct app_msg_data *msg)
{
	if (IS_EVENT(msg, util, UTIL_EVT_SHUTDOWN_REQUEST)) {
		k_timer_stop(&data_sample_timer);
		k_timer_stop(&movement_timeout_timer);
		k_timer_stop(&movement_resolution_timer);

		SEND_EVENT(app, APP_EVT_SHUTDOWN_READY);
		state_set(STATE_SHUTDOWN);
	}
}

void main(void)
{
	struct app_msg_data msg;

	self.thread_id = k_current_get();

	module_start(&self);
	handle_nrf_modem_lib_init_ret();

	if (event_manager_init()) {
		/* Without the event manager, the application will not work
		 * as intended. A reboot is required in an attempt to recover.
		 */
		LOG_ERR("Event manager could not be initialized, rebooting...");
		k_sleep(K_SECONDS(5));
		sys_reboot(SYS_REBOOT_COLD);
	} else {
		SEND_EVENT(app, APP_EVT_START);
	}

#if defined(CONFIG_WATCHDOG_APPLICATION)
	int err = watchdog_init_and_start();

	if (err) {
		LOG_DBG("watchdog_init_and_start, error: %d", err);
		SEND_ERROR(app, APP_EVT_ERROR, err);
	}
#endif

	while (true) {
		module_get_next_msg(&self, &msg);

		switch (state) {
		case STATE_INIT:
			on_state_init(&msg);
			break;
		case STATE_RUNNING:
			switch (sub_state) {
			case SUB_STATE_ACTIVE_MODE:
				on_sub_state_active(&msg);
				break;
			case SUB_STATE_PASSIVE_MODE:
				on_sub_state_passive(&msg);
				break;
			default:
				LOG_WRN("Unknown application sub state");
				break;
			}

			on_state_running(&msg);
			break;
		case STATE_SHUTDOWN:
			/* The shutdown state has no transition. */
			break;
		default:
			LOG_WRN("Unknown application state");
			break;
		}

		on_all_events(&msg);
	}
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE_EARLY(MODULE, cloud_module_event);
EVENT_SUBSCRIBE(MODULE, app_module_event);
EVENT_SUBSCRIBE(MODULE, data_module_event);
EVENT_SUBSCRIBE(MODULE, util_module_event);
EVENT_SUBSCRIBE_FINAL(MODULE, ui_module_event);
EVENT_SUBSCRIBE_FINAL(MODULE, sensor_module_event);
EVENT_SUBSCRIBE_FINAL(MODULE, modem_module_event);
