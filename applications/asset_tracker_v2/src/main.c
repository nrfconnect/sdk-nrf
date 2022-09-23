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
#if defined(CONFIG_NRF_MODEM_LIB)
#include <modem/nrf_modem_lib.h>
#endif /* CONFIG_NRF_MODEM_LIB */
#include <zephyr/sys/reboot.h>
#if defined(CONFIG_LWM2M_INTEGRATION)
#include <net/lwm2m_client_utils.h>
#endif /* CONFIG_LWM2M_INTEGRATION */
#include <net/nrf_cloud.h>

#if defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_agps.h>
#endif

/* Module name is used by the Application Event Manager macros in this file */
#define MODULE main
#include <caf/events/module_state_event.h>

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/cloud_module_event.h"
#include "events/data_module_event.h"
#include "events/sensor_module_event.h"
#include "events/ui_module_event.h"
#include "events/util_module_event.h"
#include "events/modem_module_event.h"
#include "events/led_state_event.h"

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

/* Timer callback used to signal when timeout has occurred both in active
 * and passive mode.
 */
static void data_sample_timer_handler(struct k_timer *timer);

/* Timer callback used to reset the activity trigger flag */
static void movement_resolution_timer_handler(struct k_timer *timer);

/* Application module message queue. */
#define APP_QUEUE_ENTRY_COUNT		10
#define APP_QUEUE_BYTE_ALIGNMENT	4

/* Data fetching timeouts */
#define DATA_FETCH_TIMEOUT_DEFAULT 2
/* Use a timeout of 11 seconds to accommodate for neighbour cell measurements
 * that can take up to 10.24 seconds.
 */
#define DATA_FETCH_TIMEOUT_NEIGHBORHOOD_SEARCH 11

/* Flag to prevent multiple activity events within one movement resolution cycle */
static bool activity_triggered = true;
static bool inactivity_triggered = true;

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
K_TIMER_DEFINE(movement_resolution_timer, movement_resolution_timer_handler, NULL);

/* Module data structure to hold information of the application module, which
 * opens up for using convenience functions available for modules.
 */
static struct module_data self = {
	.name = "app",
	.msg_q = &msgq_app,
	.supports_shutdown = true,
};

#if defined(CONFIG_NRF_MODEM_LIB)
NRF_MODEM_LIB_ON_INIT(asset_tracker_init_hook, on_modem_lib_init, NULL);

/* Initialized to value different than success (0) */
static int modem_lib_init_result = -1;

static void on_modem_lib_init(int ret, void *ctx)
{
	modem_lib_init_result = ret;
}
#endif /* CONFIG_NRF_MODEM_LIB */

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

/* Check the return code from nRF modem library initialization to ensure that
 * the modem is rebooted if a modem firmware update is ready to be applied or
 * an error condition occurred during firmware update or library initialization.
 */
static void handle_nrf_modem_lib_init_ret(void)
{
#if defined(CONFIG_NRF_MODEM_LIB)
	int ret = modem_lib_init_result;

	/* Handle return values relating to modem firmware update */
	switch (ret) {
	case 0:
		/* Initialization successful, no action required. */
		return;
	case MODEM_DFU_RESULT_OK:
		LOG_WRN("MODEM UPDATE OK. Will run new modem firmware after reboot");
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

#if defined(CONFIG_NRF_CLOUD_FOTA)
	/* Ignore return value, rebooting below */
	(void)nrf_cloud_fota_pending_job_validate(NULL);
#elif defined(CONFIG_LWM2M_INTEGRATION)
	lwm2m_verify_modem_fw_update();
#endif
	LOG_WRN("Rebooting...");
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);
#endif /* CONFIG_NRF_MODEM_LIB */
}

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

	if (is_ui_module_event(aeh)) {
		struct ui_module_event *evt = cast_ui_module_event(aeh);

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

static bool is_agps_processed(void)
{
#if defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS)
	struct nrf_modem_gnss_agps_data_frame processed;

	nrf_cloud_agps_processed(&processed);

	if (!processed.sv_mask_ephe) {
		return false;
	}

#endif /* CONFIG_NRF_CLOUD_AGPS || CONFIG_NRF_CLOUD_PGPS */

	return true;
}

static bool request_gnss(void)
{
	uint32_t uptime_current_ms = k_uptime_get();
	int agps_wait_threshold_ms = CONFIG_APP_REQUEST_GNSS_WAIT_FOR_AGPS_THRESHOLD_SEC *
				     MSEC_PER_SEC;

	if (!IS_ENABLED(CONFIG_GNSS_MODULE)) {
		return false;
	}

	if (!IS_ENABLED(CONFIG_APP_REQUEST_GNSS_WAIT_FOR_AGPS)) {
		return true;
	} else if (agps_wait_threshold_ms < 0) {
		/* If CONFIG_APP_REQUEST_GNSS_WAIT_FOR_AGPS_THRESHOLD_SEC is set to -1,
		 * we need to notify the data module that the application module is awaiting
		 * A-GPS data in order to request GNSS. If not, GNSS will never be
		 * requested if the initial A-GPS data request fails.
		 */
		if (is_agps_processed()) {
			return true;
		}

		SEND_EVENT(app, APP_EVT_AGPS_NEEDED);
		return false;

	} else if ((agps_wait_threshold_ms < uptime_current_ms) || is_agps_processed()) {
		return true;
	}

	return false;
}

static void data_sample_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	SEND_EVENT(app, APP_EVT_DATA_GET_ALL);
}

static void movement_resolution_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	activity_triggered = false;
	inactivity_triggered = false;
}

/* Static module functions. */
static void passive_mode_timers_start_all(void)
{
	LOG_DBG("Device mode: Passive");
	LOG_DBG("Start movement timeout: %d seconds interval", app_cfg.movement_timeout);

	LOG_DBG("%d seconds until movement can trigger a new data sample/publication",
		app_cfg.movement_resolution);

	k_timer_start(&movement_resolution_timer,
		      K_SECONDS(app_cfg.movement_resolution),
		      K_SECONDS(0));

	k_timer_start(&movement_timeout_timer,
		      K_SECONDS(app_cfg.movement_timeout),
		      K_SECONDS(app_cfg.movement_timeout));

	k_timer_stop(&data_sample_timer);
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

static void data_get(void)
{
	struct app_module_event *app_module_event = new_app_module_event();

	__ASSERT(app_module_event, "Not enough heap left to allocate event");

	size_t count = 0;

	/* Set a low sample timeout. If GNSS is requested, the sample timeout will be increased to
	 * accommodate the GNSS timeout.
	 */
	app_module_event->timeout = DATA_FETCH_TIMEOUT_DEFAULT;

	/* Specify which data that is to be included in the transmission. */
	app_module_event->data_list[count++] = APP_DATA_MODEM_DYNAMIC;
	app_module_event->data_list[count++] = APP_DATA_BATTERY;
	app_module_event->data_list[count++] = APP_DATA_ENVIRONMENTAL;

	if (!modem_static_sampled) {
		app_module_event->data_list[count++] = APP_DATA_MODEM_STATIC;
	}

	if (!app_cfg.no_data.neighbor_cell) {
		app_module_event->data_list[count++] = APP_DATA_NEIGHBOR_CELLS;
		app_module_event->timeout = DATA_FETCH_TIMEOUT_NEIGHBORHOOD_SEARCH;
	}

	/* The reason for having at least 75 seconds sample timeout when
	 * requesting GNSS data is that the GNSS module on the nRF9160 modem will always
	 * search for at least 60 seconds for the first position fix after a reboot. This limit
	 * is enforced in order to give the modem time to perform scheduled downloads of
	 * A-GPS data from the GNSS satellites.
	 *
	 * However, if A-GPS data has been downloaded via the cloud connection and processed
	 * before the initial GNSS search, the actual GNSS timeout set by the application is used.
	 *
	 * Processing A-GPS before requesting GNSS data is enabled by default,
	 * and the time that the application will wait for A-GPS data before including GNSS
	 * in sample requests can be adjusted via the
	 * CONFIG_APP_REQUEST_GNSS_WAIT_FOR_AGPS_THRESHOLD_SEC Kconfig option. If this option is
	 * set to -1, the application module will not request GNSS unless A-GPS data has been
	 * processed.
	 *
	 * If A-GPS data has been processed the sample timeout can be ignored as the
	 * GNSS will most likely time out before the sample timeout expires.
	 *
	 * When GNSS is requested, set the sample timeout to (GNSS timeout + 15 seconds)
	 * to let the GNSS module finish ongoing searches before data is sent to cloud.
	 */

	if (request_gnss() && !app_cfg.no_data.gnss) {
		app_module_event->data_list[count++] = APP_DATA_GNSS;
		app_module_event->timeout = MAX(app_cfg.gnss_timeout + 15, 75);
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

	if ((IS_EVENT(msg, ui, UI_EVT_BUTTON_DATA_READY)) ||
	    (IS_EVENT(msg, sensor, SENSOR_EVT_MOVEMENT_ACTIVITY_DETECTED)) ||
	    (IS_EVENT(msg, sensor, SENSOR_EVT_MOVEMENT_IMPACT_DETECTED))) {
		if (IS_EVENT(msg, ui, UI_EVT_BUTTON_DATA_READY) &&
		    msg->module.ui.data.ui.button_number != 2) {
			return;
		}

		if (IS_EVENT(msg, sensor, SENSOR_EVT_MOVEMENT_ACTIVITY_DETECTED)) {
			if (activity_triggered) {
				return;
			}
			activity_triggered = true;
		}

		/* Trigger a sample request if button 2 has been pushed on the DK or activity has
		 * been detected. The data request can only be triggered if the movement
		 * resolution timer has timed out.
		 */

		if (k_timer_remaining_get(&movement_resolution_timer) == 0) {
			data_sample_timer_handler(NULL);
			passive_mode_timers_start_all();
		}
	}
	if (IS_EVENT(msg, sensor, SENSOR_EVT_MOVEMENT_INACTIVITY_DETECTED)
	    && k_timer_remaining_get(&movement_resolution_timer) != 0) {
		/* Trigger a sample request if there has been inactivity after
		 * activity was triggered.
		 */
		if (!inactivity_triggered) {
			data_sample_timer_handler(NULL);
			inactivity_triggered = true;
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
}

void main(void)
{
	int err;
	struct app_msg_data msg = { 0 };

	if (!IS_ENABLED(CONFIG_LWM2M_CARRIER)) {
		handle_nrf_modem_lib_init_ret();
	}

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

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, cloud_module_event);
APP_EVENT_SUBSCRIBE(MODULE, app_module_event);
APP_EVENT_SUBSCRIBE(MODULE, data_module_event);
APP_EVENT_SUBSCRIBE(MODULE, util_module_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, ui_module_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, sensor_module_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, modem_module_event);
