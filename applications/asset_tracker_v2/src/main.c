/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <app_event_manager.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/sys/reboot.h>
#include <net/nrf_cloud.h>

/* Module name is used by the Application Event Manager macros in this file */
#define MODULE main
#include <caf/events/module_state_event.h>

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/cloud_module_event.h"
#include "events/data_module_event.h"
#include "events/sensor_module_event.h"
#include "events/util_module_event.h"
#include "events/modem_module_event.h"

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

LOG_MODULE_REGISTER(MODULE, CONFIG_APPLICATION_MODULE_LOG_LEVEL);

/* Message structure. Events from other modules are converted to messages
 * in the Application Event Manager handler, and then queued up in the message queue
 * for processing in the main thread.
 */
struct app_msg_data {
	union {
		struct cloud_module_event cloud;
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
 * Active mode: Sensor data and GNSS position is acquired at a configured
 *		interval and sent to cloud.
 *
 * Passive mode: Sensor data and GNSS position is acquired when movement is
 *		 detected, or after the configured movement timeout occurs.
 */
static enum sub_state_type {
	SUB_STATE_ACTIVE_MODE,
	SUB_STATE_PASSIVE_MODE,
} sub_state;

/* Internal copy of the device configuration. */
static struct cloud_data_cfg app_cfg;

/* Variable that keeps track whether modem static data has been successfully sampled by the
 * modem module. Modem static data does not change and only needs to be sampled and sent to cloud
 * once.
 */
static bool modem_static_sampled;

/* Variable that is set high whenever a sample request is ongoing. */
static bool sample_request_ongoing;

/* Variable that is set high whenever the device is considered active (under movement). */
static bool activity;

/* Timer callback used to signal when timeout has occurred both in active
 * and passive mode.
 */
static void data_sample_timer_handler(struct k_timer *timer);

/* Application module message queue. */
#define APP_QUEUE_ENTRY_COUNT		10
#define APP_QUEUE_BYTE_ALIGNMENT	4

/* Data fetching timeouts */
#define DATA_FETCH_TIMEOUT_DEFAULT 2

K_MSGQ_DEFINE(msgq_app, sizeof(struct app_msg_data), APP_QUEUE_ENTRY_COUNT,
	      APP_QUEUE_BYTE_ALIGNMENT);

/* Data sample timer used in active mode. */
K_TIMER_DEFINE(data_sample_timer, data_sample_timer_handler, NULL);

/* Movement timer used to detect movement timeouts in passive mode. */
K_TIMER_DEFINE(movement_timeout_timer, data_sample_timer_handler, NULL);

/* Movement resolution timer decides the period after movement that consecutive
 * movements are ignored and do not cause data collection. This is used to
 * lower power consumption by limiting how often GNSS search is performed and
 * data is sent on air.
 */
K_TIMER_DEFINE(movement_resolution_timer, NULL, NULL);

/* Module data structure to hold information of the application module, which
 * opens up for using convenience functions available for modules.
 */
static struct module_data self = {
	.name = "app",
	.msg_q = &msgq_app,
	.supports_shutdown = true,
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

#if defined(CONFIG_NRF_MODEM_LIB)
/* Check the return code from nRF modem library initialization to ensure that
 * the modem is rebooted if a modem firmware update is ready to be applied or
 * an error condition occurred during firmware update or library initialization.
 */
static void modem_init(void)
{
	int ret = nrf_modem_lib_init();

	/* Handle return values relating to modem firmware update */
	switch (ret) {
	case 0:
		/* Initialization successful, no action required. */
		return;
	case NRF_MODEM_DFU_RESULT_OK:
		LOG_DBG("MODEM UPDATE OK. Will run new modem firmware after reboot");
		break;
	case NRF_MODEM_DFU_RESULT_UUID_ERROR:
	case NRF_MODEM_DFU_RESULT_AUTH_ERROR:
		LOG_ERR("MODEM UPDATE ERROR %d. Will run old firmware", ret);
		break;
	case NRF_MODEM_DFU_RESULT_HARDWARE_ERROR:
	case NRF_MODEM_DFU_RESULT_INTERNAL_ERROR:
		LOG_ERR("MODEM UPDATE FATAL ERROR %d. Modem failure", ret);
		break;
	case NRF_MODEM_DFU_RESULT_VOLTAGE_LOW:
		LOG_ERR("MODEM UPDATE CANCELLED %d.", ret);
		LOG_ERR("Please reboot once you have sufficient power for the DFU");
		break;
	default:
		/* All non-zero return codes other than DFU result codes are
		 * considered irrecoverable and a reboot is needed.
		 */
		LOG_ERR("nRF modem lib initialization failed, error: %d", ret);
		break;
	}

#if defined(CONFIG_NRF_CLOUD_FOTA)
	/* Ignore return value, rebooting below */
	(void)nrf_cloud_fota_pending_job_validate(NULL);
#endif
	LOG_DBG("Rebooting...");
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);
}
#endif /* CONFIG_NRF_MODEM_LIB */

/* Application Event Manager handler. Puts event data into messages and adds them to the
 * application message queue.
 */
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct app_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_cloud_module_event(aeh)) {
		struct cloud_module_event *evt = cast_cloud_module_event(aeh);

		msg.module.cloud = *evt;
		enqueue_msg = true;
	}

	if (is_app_module_event(aeh)) {
		struct app_module_event *evt = cast_app_module_event(aeh);

		msg.module.app = *evt;
		enqueue_msg = true;
	}

	if (is_data_module_event(aeh)) {
		struct data_module_event *evt = cast_data_module_event(aeh);

		msg.module.data = *evt;
		enqueue_msg = true;
	}

	if (is_sensor_module_event(aeh)) {
		struct sensor_module_event *evt = cast_sensor_module_event(aeh);

		msg.module.sensor = *evt;
		enqueue_msg = true;
	}

	if (is_util_module_event(aeh)) {
		struct util_module_event *evt = cast_util_module_event(aeh);

		msg.module.util = *evt;
		enqueue_msg = true;
	}

	if (is_modem_module_event(aeh)) {
		struct modem_module_event *evt = cast_modem_module_event(aeh);

		msg.module.modem = *evt;
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

	/* Cancel if a previous sample request has not completed or the device is not under
	 * activity in passive mode.
	 */
	if (sample_request_ongoing || ((sub_state == SUB_STATE_PASSIVE_MODE) && !activity)) {
		return;
	}

	SEND_EVENT(app, APP_EVT_DATA_GET_ALL);
}

/* Static module functions. */
static void passive_mode_timers_start_all(void)
{
	LOG_DBG("Device mode: Passive");
	LOG_DBG("Start movement timeout: %d seconds interval", app_cfg.movement_timeout);

	LOG_DBG("%d seconds until movement can trigger a new data sample/publication",
		app_cfg.movement_resolution);

	k_timer_start(&data_sample_timer,
		      K_SECONDS(app_cfg.movement_resolution),
		      K_SECONDS(app_cfg.movement_resolution));

	k_timer_start(&movement_resolution_timer,
		      K_SECONDS(app_cfg.movement_resolution),
		      K_SECONDS(0));

	k_timer_start(&movement_timeout_timer,
		      K_SECONDS(app_cfg.movement_timeout),
		      K_SECONDS(app_cfg.movement_timeout));
}

static void active_mode_timers_start_all(void)
{
	LOG_DBG("Device mode: Active");
	LOG_DBG("Start data sample timer: %d seconds interval", app_cfg.active_wait_timeout);

	k_timer_start(&data_sample_timer,
		      K_SECONDS(app_cfg.active_wait_timeout),
		      K_SECONDS(app_cfg.active_wait_timeout));

	k_timer_stop(&movement_resolution_timer);
	k_timer_stop(&movement_timeout_timer);
}

static void activity_event_handle(enum sensor_module_event_type sensor_event)
{
	__ASSERT(((sensor_event == SENSOR_EVT_MOVEMENT_ACTIVITY_DETECTED) ||
		  (sensor_event == SENSOR_EVT_MOVEMENT_INACTIVITY_DETECTED)),
		  "Unknown event");

	activity = (sensor_event == SENSOR_EVT_MOVEMENT_ACTIVITY_DETECTED) ? true : false;

	if (sample_request_ongoing) {
		LOG_DBG("Sample request ongoing, abort request.");
		return;
	}

	if ((sensor_event == SENSOR_EVT_MOVEMENT_ACTIVITY_DETECTED) &&
	    (k_timer_remaining_get(&movement_resolution_timer) != 0)) {
		LOG_DBG("Movement resolution timer has not expired, abort request.");
		return;
	}

	SEND_EVENT(app, APP_EVT_DATA_GET_ALL);
	passive_mode_timers_start_all();
}

static void data_get(void)
{
	struct app_module_event *app_module_event = new_app_module_event();

	__ASSERT(app_module_event, "Not enough heap left to allocate event");

	size_t count = 0;

	/* Set a low sample timeout. If location is requested, the sample timeout
	 * will be increased to accommodate the location request.
	 */
	app_module_event->timeout = DATA_FETCH_TIMEOUT_DEFAULT;

	/* Specify which data that is to be included in the transmission. */
	app_module_event->data_list[count++] = APP_DATA_MODEM_DYNAMIC;
	app_module_event->data_list[count++] = APP_DATA_BATTERY;
	app_module_event->data_list[count++] = APP_DATA_ENVIRONMENTAL;

	if (!modem_static_sampled) {
		app_module_event->data_list[count++] = APP_DATA_MODEM_STATIC;
	}

	if (!app_cfg.no_data.neighbor_cell || !app_cfg.no_data.gnss || !app_cfg.no_data.wifi) {
		app_module_event->data_list[count++] = APP_DATA_LOCATION;

		/* Set application module timeout when location sampling is requested.
		 * This is selected to be long enough so that most of the GNSS would
		 * have enough time to run to get a fix. We also want it to be smaller than
		 * the sampling interval (120s). So, 110s was selected but we take
		 * minimum of sampling interval minus 5 (just some selected number) and 110.
		 * And mode (active or passive) is taken into account.
		 * If the timeout would become smaller than 5s, we want to ensure some time for
		 * the modules so the minimum value for application module timeout is 5s.
		 */
		app_module_event->timeout = (app_cfg.active_mode) ?
			MIN(app_cfg.active_wait_timeout - 5, 110) :
			MIN(app_cfg.movement_resolution - 5, 110);
		app_module_event->timeout = MAX(app_module_event->timeout, 5);
	}

	/* Set list count to number of data types passed in app_module_event. */
	app_module_event->count = count;
	app_module_event->type = APP_EVT_DATA_GET;

	APP_EVENT_SUBMIT(app_module_event);
}

/* Message handler for STATE_INIT. */
static void on_state_init(struct app_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_INIT)) {
		/* Keep a copy of the new configuration. */
		app_cfg = msg->module.data.data.cfg;

		if (app_cfg.active_mode) {
			active_mode_timers_start_all();
		} else {
			passive_mode_timers_start_all();
		}

		state_set(STATE_RUNNING);
		sub_state_set(app_cfg.active_mode ? SUB_STATE_ACTIVE_MODE :
						    SUB_STATE_PASSIVE_MODE);
	}
}

/* Message handler for STATE_RUNNING. */
static void on_state_running(struct app_msg_data *msg)
{
	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONNECTED)) {
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
			active_mode_timers_start_all();
			sub_state_set(SUB_STATE_ACTIVE_MODE);
			return;
		}

		passive_mode_timers_start_all();
	}

	if ((IS_EVENT(msg, sensor, SENSOR_EVT_MOVEMENT_ACTIVITY_DETECTED)) ||
	    (IS_EVENT(msg, sensor, SENSOR_EVT_MOVEMENT_INACTIVITY_DETECTED))) {
		activity_event_handle(msg->module.sensor.type);
	}
}

/* Message handler for SUB_STATE_ACTIVE_MODE. */
static void on_sub_state_active(struct app_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY)) {
		/* Keep a copy of the new configuration. */
		app_cfg = msg->module.data.data.cfg;

		if (!app_cfg.active_mode) {
			passive_mode_timers_start_all();
			sub_state_set(SUB_STATE_PASSIVE_MODE);
			return;
		}

		active_mode_timers_start_all();
	}
}

/* Message handler for all states. */
static void on_all_events(struct app_msg_data *msg)
{
	if (IS_EVENT(msg, util, UTIL_EVT_SHUTDOWN_REQUEST)) {
		k_timer_stop(&data_sample_timer);
		k_timer_stop(&movement_timeout_timer);
		k_timer_stop(&movement_resolution_timer);

		SEND_SHUTDOWN_ACK(app, APP_EVT_SHUTDOWN_READY, self.id);
		state_set(STATE_SHUTDOWN);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_MODEM_STATIC_DATA_READY)) {
		modem_static_sampled = true;
	}

	if (IS_EVENT(msg, app, APP_EVT_DATA_GET)) {
		sample_request_ongoing = true;
	}

	if (IS_EVENT(msg, data, DATA_EVT_DATA_READY)) {
		sample_request_ongoing = false;
	}

	if (IS_EVENT(msg, sensor, SENSOR_EVT_MOVEMENT_IMPACT_DETECTED)) {
		SEND_EVENT(app, APP_EVT_DATA_GET_ALL);
	}
}

int main(void)
{
	int err;
	struct app_msg_data msg = { 0 };

	if (app_event_manager_init()) {
		/* Without the Application Event Manager, the application will not work
		 * as intended. A reboot is required in an attempt to recover.
		 */
		LOG_ERR("Application Event Manager could not be initialized, rebooting...");
		k_sleep(K_SECONDS(5));
		sys_reboot(SYS_REBOOT_COLD);
	} else {
		module_set_state(MODULE_STATE_READY);
		SEND_EVENT(app, APP_EVT_START);

#if defined(CONFIG_NRF_MODEM_LIB)
		/* The carrier library will initialize the modem if enabled, if not,
		 * we initialize the modem in here, before the rest of the application is started.
		 */
		if (!IS_ENABLED(CONFIG_LWM2M_CARRIER)) {
			modem_init();
		}
#endif
	}

	self.thread_id = k_current_get();

	err = module_start(&self);
	if (err) {
		LOG_ERR("Failed starting module, error: %d", err);
		SEND_ERROR(app, APP_EVT_ERROR, err);
	}

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
				LOG_ERR("Unknown sub state");
				break;
			}

			on_state_running(&msg);
			break;
		case STATE_SHUTDOWN:
			/* The shutdown state has no transition. */
			break;
		default:
			LOG_ERR("Unknown state");
			break;
		}

		on_all_events(&msg);
	}
	return 0;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, cloud_module_event);
APP_EVENT_SUBSCRIBE(MODULE, app_module_event);
APP_EVENT_SUBSCRIBE(MODULE, data_module_event);
APP_EVENT_SUBSCRIBE(MODULE, util_module_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, sensor_module_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, modem_module_event);
