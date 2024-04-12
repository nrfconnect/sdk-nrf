/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <math.h>
#include <date_time.h>
#include <app_event_manager.h>
#include <modem/location.h>
#include <modem/modem_info.h>
#include <nrf_modem_gnss.h>

#define MODULE location_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/location_module_event.h"
#include "events/data_module_event.h"
#include "events/util_module_event.h"
#include "events/modem_module_event.h"
#include "events/cloud_module_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_LOCATION_MODULE_LOG_LEVEL);

struct location_msg_data {
	union {
		struct app_module_event app;
		struct data_module_event data;
		struct util_module_event util;
		struct modem_module_event modem;
		struct cloud_module_event cloud;
		struct location_module_event location;
	} module;
};

/* Location module super states. */
static enum state_type {
	STATE_INIT,
	STATE_RUNNING,
	STATE_SHUTDOWN
} state;

/* Location module sub states. */
static enum sub_state_type {
	SUB_STATE_IDLE,
	SUB_STATE_SEARCH
} sub_state;

static struct nrf_modem_gnss_pvt_data_frame pvt_data;

/* Local copy of the device configuration. */
static struct cloud_data_cfg copy_cfg;

static struct module_stats {
	/* Uptime set when location search is started. Used to calculate search time for
	 * location request and timeout.
	 */
	uint64_t start_uptime;
	/* Uptime set when GNSS search is started. Used to calculate search time for
	 * GNSS fix and timeout.
	 */
	uint32_t search_time;
	/* Number of satellites tracked at the time of a GNSS fix or a timeout. */
	uint8_t satellites_tracked;
} stats;

/* Indicates whether cloud location request is pending towards cloud service, that is,
 * LOCATION_EVT_CLOUD_LOCATION_EXT_REQUEST has been received but not responded yet.
 * This is specifically needed when cloud connection doesn't exist and there is a
 * new sampling request. We can then cancel previous location request and do a new one.
 */
static bool cloud_location_request_pending;

static struct module_data self = {
	.name = "location",
	.msg_q = NULL,
	.supports_shutdown = true,
};

/* Forward declarations. */
static void message_handler(struct location_msg_data *data);
static void search_start(void);
static void inactive_send(void);
static void time_set(void);
static void data_send_pvt(void);

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
	case SUB_STATE_IDLE:
		return "SUB_STATE_IDLE";
	case SUB_STATE_SEARCH:
		return "SUB_STATE_SEARCH";
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
static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_app_module_event(aeh)) {
		struct app_module_event *event = cast_app_module_event(aeh);
		struct location_msg_data msg = {
			.module.app = *event
		};

		message_handler(&msg);
	}

	if (is_data_module_event(aeh)) {
		struct data_module_event *event = cast_data_module_event(aeh);
		struct location_msg_data msg = {
			.module.data = *event
		};

		message_handler(&msg);
	}

	if (is_util_module_event(aeh)) {
		struct util_module_event *event = cast_util_module_event(aeh);
		struct location_msg_data msg = {
			.module.util = *event
		};

		message_handler(&msg);
	}

	if (is_location_module_event(aeh)) {
		struct location_module_event *event = cast_location_module_event(aeh);
		struct location_msg_data msg = {
			.module.location = *event
		};

		message_handler(&msg);
	}

	if (is_modem_module_event(aeh)) {
		struct modem_module_event *event = cast_modem_module_event(aeh);
		struct location_msg_data msg = {
			.module.modem = *event
		};

		message_handler(&msg);
	}

	if (is_cloud_module_event(aeh)) {
		struct cloud_module_event *event = cast_cloud_module_event(aeh);
		struct location_msg_data msg = {
			.module.cloud = *event
		};

		message_handler(&msg);
	}
	return false;
}

static void timeout_send(void)
{
	struct location_module_event *location_module_event = new_location_module_event();

	location_module_event->data.location.search_time = stats.search_time;
	location_module_event->data.location.satellites_tracked = stats.satellites_tracked;
	location_module_event->type = LOCATION_MODULE_EVT_TIMEOUT;

	APP_EVENT_SUBMIT(location_module_event);
}

static void data_send_pvt(void)
{
	struct location_module_event *location_module_event = new_location_module_event();

	location_module_event->data.location.pvt.latitude = pvt_data.latitude;
	location_module_event->data.location.pvt.longitude = pvt_data.longitude;
	location_module_event->data.location.pvt.accuracy = pvt_data.accuracy;
	location_module_event->data.location.pvt.altitude = pvt_data.altitude;
	location_module_event->data.location.pvt.altitude_accuracy = pvt_data.altitude_accuracy;
	location_module_event->data.location.pvt.speed = pvt_data.speed;
	location_module_event->data.location.pvt.speed_accuracy = pvt_data.speed_accuracy;
	location_module_event->data.location.pvt.heading = pvt_data.heading;
	location_module_event->data.location.pvt.heading_accuracy = pvt_data.heading_accuracy;
	location_module_event->data.location.timestamp = k_uptime_get();
	location_module_event->type = LOCATION_MODULE_EVT_GNSS_DATA_READY;

	location_module_event->data.location.satellites_tracked = stats.satellites_tracked;
	location_module_event->data.location.search_time =
		(uint32_t)(location_module_event->data.location.timestamp - stats.start_uptime);

	APP_EVENT_SUBMIT(location_module_event);
}

static void search_start(void)
{
	int err;
	struct location_config config;
	int methods_count = 0;
	struct location_method_config methods_updated[CONFIG_LOCATION_METHODS_LIST_SIZE] = { 0 };

	/* Set default location configuration configured at compile time */
	location_config_defaults_set(&config, 0, NULL);

	config.timeout = copy_cfg.location_timeout * MSEC_PER_SEC;

	/* If any method type has been disabled, update the method list by using those methods
	 * that are enabled
	 */
	if (copy_cfg.no_data.neighbor_cell || copy_cfg.no_data.gnss || copy_cfg.no_data.wifi) {
		for (int i = 0; i < config.methods_count; i++) {
			/* Use methods that are not disabled at run-time in no_data configuration */
			if ((config.methods[i].method == LOCATION_METHOD_GNSS &&
			     !copy_cfg.no_data.gnss) ||
			    (config.methods[i].method == LOCATION_METHOD_WIFI &&
			     !copy_cfg.no_data.wifi) ||
			    (config.methods[i].method == LOCATION_METHOD_CELLULAR &&
			     !copy_cfg.no_data.neighbor_cell)) {
				memcpy(&methods_updated[methods_count],
				       &config.methods[i],
				       sizeof(struct location_method_config));
				methods_count++;
			}
		}

		if (methods_count == 0) {
			SEND_EVENT(location, LOCATION_MODULE_EVT_DATA_NOT_READY);
			LOG_INF("All location methods are disabled at run-time");
			return;
		}

		config.methods_count = methods_count;
		memcpy(config.methods,
		       methods_updated,
		       CONFIG_LOCATION_METHODS_LIST_SIZE * sizeof(struct location_method_config));
	}

	LOG_DBG("Requesting location...");

	err = location_request(&config);
	if (err) {
		SEND_EVENT(location, LOCATION_MODULE_EVT_DATA_NOT_READY);
		LOG_ERR("Location request failed: %d", err);
		return;
	}

	SEND_EVENT(location, LOCATION_MODULE_EVT_ACTIVE);
	stats.start_uptime = k_uptime_get();
}

static void inactive_send(void)
{
	cloud_location_request_pending = false;
	SEND_EVENT(location, LOCATION_MODULE_EVT_INACTIVE);
}

static void time_set(void)
{
	/* Change datetime.year and datetime.month to accommodate the
	 * correct input format.
	 */
	struct tm gnss_time = {
		.tm_year = pvt_data.datetime.year - 1900,
		.tm_mon = pvt_data.datetime.month - 1,
		.tm_mday = pvt_data.datetime.day,
		.tm_hour = pvt_data.datetime.hour,
		.tm_min = pvt_data.datetime.minute,
		.tm_sec = pvt_data.datetime.seconds,
	};

	date_time_set(&gnss_time);
}

static inline int adjust_rsrp(int input)
{
	if (IS_ENABLED(CONFIG_LOCATION_MODULE_NEIGHBOR_CELLS_DATA_CONVERT_RSRP_TO_DBM)) {
		return RSRP_IDX_TO_DBM(input);
	}

	return input;
}

static inline int adjust_rsrq(int input)
{
	if (IS_ENABLED(CONFIG_LOCATION_MODULE_NEIGHBOR_CELLS_DATA_CONVERT_RSRQ_TO_DB)) {
		return round(RSRQ_IDX_TO_DB(input));
	}

	return input;
}

static void send_cloud_location_update(const struct location_data_cloud *cloud_location_info)
{
	struct location_module_event *evt = new_location_module_event();
	struct location_module_neighbor_cells *evt_ncells =
		&evt->data.cloud_location.neighbor_cells;
	evt->data.cloud_location.neighbor_cells_valid = false;

	if (cloud_location_info->cell_data != NULL) {
		BUILD_ASSERT(sizeof(evt_ncells->cell_data) == sizeof(struct lte_lc_cells_info));
		BUILD_ASSERT(sizeof(evt_ncells->neighbor_cells) >=
			     sizeof(struct lte_lc_ncell) * CONFIG_LTE_NEIGHBOR_CELLS_MAX);

		evt->data.cloud_location.neighbor_cells_valid = true;
		memcpy(&evt_ncells->cell_data,
		       cloud_location_info->cell_data,
		       sizeof(struct lte_lc_cells_info));
		memcpy(&evt_ncells->neighbor_cells,
		       cloud_location_info->cell_data->neighbor_cells,
		       sizeof(struct lte_lc_ncell) * evt_ncells->cell_data.ncells_count);

		/* Convert RSRP to dBm and RSRQ to dB per "nRF91 AT Commands" v1.7. */
		evt_ncells->cell_data.current_cell.rsrp =
			adjust_rsrp(evt_ncells->cell_data.current_cell.rsrp);
		evt_ncells->cell_data.current_cell.rsrq =
			adjust_rsrq(evt_ncells->cell_data.current_cell.rsrq);

		for (size_t i = 0; i < evt_ncells->cell_data.ncells_count; i++) {
			evt_ncells->neighbor_cells[i].rsrp =
				adjust_rsrp(evt_ncells->neighbor_cells[i].rsrp);
			evt_ncells->neighbor_cells[i].rsrq =
				adjust_rsrq(evt_ncells->neighbor_cells[i].rsrq);
		}
	}

#if defined(CONFIG_LOCATION_METHOD_WIFI)
	evt->data.cloud_location.wifi_access_points_valid = false;
	if (cloud_location_info->wifi_data != NULL) {
		BUILD_ASSERT(sizeof(evt->data.cloud_location.wifi_access_points.ap_info) >=
			     sizeof(struct wifi_scan_result) *
			     CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT);

		evt->data.cloud_location.wifi_access_points_valid = true;
		evt->data.cloud_location.wifi_access_points.cnt =
			cloud_location_info->wifi_data->cnt;
		memcpy(&evt->data.cloud_location.wifi_access_points.ap_info,
		       cloud_location_info->wifi_data->ap_info,
		       sizeof(struct wifi_scan_result) *
				evt->data.cloud_location.wifi_access_points.cnt);
	}
#endif
	evt->type = LOCATION_MODULE_EVT_CLOUD_LOCATION_DATA_READY;
	evt->data.cloud_location.timestamp = k_uptime_get();

	APP_EVENT_SUBMIT(evt);
}

/* Non-static so that this can be used in tests to mock location library API. */
void location_event_handler(const struct location_event_data *event_data)
{
	switch (event_data->id) {
	case LOCATION_EVT_LOCATION:
		LOG_DBG("Got location:");
		LOG_DBG("  method: %s", location_method_str(event_data->method));
		LOG_DBG("  latitude: %.06f", event_data->location.latitude);
		LOG_DBG("  longitude: %.06f", event_data->location.longitude);
		LOG_DBG("  accuracy: %.01f m", (double)event_data->location.accuracy);
		LOG_DBG("  altitude: %.01f m",
			(double)event_data->location.details.gnss.pvt_data.altitude);
		LOG_DBG("  speed: %.01f m",
			(double)event_data->location.details.gnss.pvt_data.speed);
		LOG_DBG("  heading: %.01f deg",
			(double)event_data->location.details.gnss.pvt_data.heading);

		if (event_data->location.datetime.valid) {
			LOG_DBG("  date: %04d-%02d-%02d",
				event_data->location.datetime.year,
				event_data->location.datetime.month,
				event_data->location.datetime.day);
			LOG_DBG("  time: %02d:%02d:%02d.%03d UTC",
				event_data->location.datetime.hour,
				event_data->location.datetime.minute,
				event_data->location.datetime.second,
				event_data->location.datetime.ms);
		}

		stats.search_time = (uint32_t)(k_uptime_get() - stats.start_uptime);
		LOG_DBG("  search time: %d ms", stats.search_time);

		/* Only GNSS result is handled as cellular and Wi-Fi are handled
		 * as part of LOCATION_EVT_CLOUD_LOCATION_EXT_REQUEST
		 */
		stats.satellites_tracked = 0;
		if (event_data->method == LOCATION_METHOD_GNSS) {
			pvt_data = event_data->location.details.gnss.pvt_data;
			stats.satellites_tracked =
				event_data->location.details.gnss.satellites_tracked;
			LOG_DBG("  satellites tracked: %d", stats.satellites_tracked);

			if (event_data->location.datetime.valid) {
				/* Date and time is in pvt_data that is set above */
				time_set();
			}
			data_send_pvt();
		}
		LOG_DBG("  Google maps URL: https://maps.google.com/?q=%.06f,%.06f",
			event_data->location.latitude, event_data->location.longitude);

		inactive_send();
		break;

	case LOCATION_EVT_RESULT_UNKNOWN:
		LOG_DBG("Getting location completed with undefined result");
		stats.search_time = (uint32_t)(k_uptime_get() - stats.start_uptime);
		LOG_DBG("  search time: %d", stats.search_time);
		inactive_send();
		/* No events are sent because LOCATION_MODULE_EVT_CLOUD_LOCATION_DATA_READY
		 * have already been sent earlier and hence APP_LOCATION has been set already.
		 */
		break;

	case LOCATION_EVT_TIMEOUT:
		LOG_DBG("Getting location timed out");
		stats.search_time = (uint32_t)(k_uptime_get() - stats.start_uptime);
		LOG_DBG("  search time: %d", stats.search_time);

		stats.satellites_tracked = 0;
		if (event_data->method == LOCATION_METHOD_GNSS) {
			stats.satellites_tracked =
				event_data->error.details.gnss.satellites_tracked;
			LOG_DBG("  satellites tracked: %d", stats.satellites_tracked);
		}

		timeout_send();
		inactive_send();
		break;

	case LOCATION_EVT_ERROR:
		LOG_WRN("Getting location failed");
		SEND_EVENT(location, LOCATION_MODULE_EVT_DATA_NOT_READY);
		inactive_send();
		break;

	case LOCATION_EVT_GNSS_ASSISTANCE_REQUEST: {
		LOG_DBG("Requested A-GNSS data");
#if defined(CONFIG_NRF_CLOUD_AGNSS)
		struct location_module_event *location_module_event = new_location_module_event();

		location_module_event->data.agnss_request = event_data->agnss_request;
		location_module_event->type = LOCATION_MODULE_EVT_AGNSS_NEEDED;
		APP_EVENT_SUBMIT(location_module_event);
#endif
		break;
	}

	case LOCATION_EVT_GNSS_PREDICTION_REQUEST: {
		LOG_DBG("Requested P-GPS data");
#if defined(CONFIG_NRF_CLOUD_PGPS)
		struct location_module_event *location_module_event = new_location_module_event();

		location_module_event->data.pgps_request = event_data->pgps_request;
		location_module_event->type = LOCATION_MODULE_EVT_PGPS_NEEDED;
		APP_EVENT_SUBMIT(location_module_event);
#endif
		break;
	}

#if defined(CONFIG_LOCATION_METHOD_CELLULAR) || defined(CONFIG_LOCATION_METHOD_WIFI)
	case LOCATION_EVT_CLOUD_LOCATION_EXT_REQUEST:
		LOG_DBG("Getting cloud location request");
		send_cloud_location_update(&event_data->cloud_location_request);
		cloud_location_request_pending = true;
		break;
#endif

	case LOCATION_EVT_STARTED:
		LOG_DBG("Location request has been started with '%s' method",
			location_method_str(event_data->method));
		break;

	case LOCATION_EVT_FALLBACK:
		LOG_DBG("Location fallback has occurred from '%s' to '%s'",
			location_method_str(event_data->method),
			location_method_str(event_data->fallback.next_method));
		break;

	default:
		LOG_DBG("Getting location: Unknown event %d", event_data->id);
		break;
	}
}

static bool location_data_requested(enum app_module_data_type *data_list, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (data_list[i] == APP_DATA_LOCATION) {
			return true;
		}
	}

	return false;
}

static int setup(void)
{
	int err;

	err = location_init(location_event_handler);
	if (err) {
		LOG_ERR("Initializing the Location library failed, error: %d", err);
		return -1;
	}

	return 0;
}

/* Message handler for STATE_INIT. */
static void on_state_init(struct location_msg_data *msg)
{
	if (IS_EVENT(msg, modem, MODEM_EVT_INITIALIZED)) {
		int err;

		err = setup();
		if (err) {
			LOG_ERR("setup, error: %d", err);
			SEND_ERROR(location, LOCATION_MODULE_EVT_ERROR_CODE, err);
		}

		state_set(STATE_RUNNING);
	}
}

/* Message handler for SUB_STATE_SEARCH. */
static void on_state_running_location_search(struct location_msg_data *msg)
{
	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_INACTIVE)) {
		sub_state_set(SUB_STATE_IDLE);
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_CLOUD_LOCATION_RECEIVED)) {
#if defined(CONFIG_LOCATION)
		location_cloud_location_ext_result_set(
			LOCATION_EXT_RESULT_SUCCESS,
			&msg->module.cloud.data.cloud_location);
#endif
	}
	if (IS_EVENT(msg, cloud, CLOUD_EVT_CLOUD_LOCATION_ERROR)) {
		location_cloud_location_ext_result_set(LOCATION_EXT_RESULT_ERROR, NULL);
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_CLOUD_LOCATION_UNKNOWN)) {
		location_cloud_location_ext_result_set(LOCATION_EXT_RESULT_UNKNOWN, NULL);
	}

	if (IS_EVENT(msg, app, APP_EVT_DATA_GET)) {
		if (!location_data_requested(msg->module.app.data_list, msg->module.app.count)) {
			return;
		}

		/* If cloud location request is pending data and cloud modules,
		 * we'll cancel current location request and start a new one
		 */
		if (cloud_location_request_pending) {
			location_request_cancel();
			cloud_location_request_pending = false;
			search_start();
			return;
		}

		LOG_DBG("Location request already active and will not be restarted");
		LOG_DBG("Seeing this message sometimes is normal, especially, "
			"when trying to acquire the first GNSS fix.");
		LOG_DBG("If you see this often for other than the first location search, "
			"try setting the sample/publication interval a bit greater.");
	}
}

/* Message handler for SUB_STATE_IDLE. */
static void on_state_running_location_idle(struct location_msg_data *msg)
{
	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_ACTIVE)) {
		sub_state_set(SUB_STATE_SEARCH);
	}

	if (IS_EVENT(msg, app, APP_EVT_DATA_GET)) {
		if (!location_data_requested(msg->module.app.data_list, msg->module.app.count)) {
			return;
		}

		search_start();
	}
}

/* Message handler for all states. */
static void on_all_states(struct location_msg_data *msg)
{
	if (IS_EVENT(msg, app, APP_EVT_START)) {
		int err;

		err = module_start(&self);
		if (err) {
			LOG_ERR("Failed starting module, error: %d", err);
			SEND_ERROR(location, LOCATION_MODULE_EVT_ERROR_CODE, err);
		}

		state_set(STATE_INIT);
	}

	if (IS_EVENT(msg, util, UTIL_EVT_SHUTDOWN_REQUEST)) {
		/* The module doesn't have anything to shut down and can
		 * report back immediately.
		 */
		SEND_SHUTDOWN_ACK(location, LOCATION_MODULE_EVT_SHUTDOWN_READY, self.id);
		state_set(STATE_SHUTDOWN);
	}

	if ((IS_EVENT(msg, data, DATA_EVT_CONFIG_INIT)) ||
	    (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY))) {
		copy_cfg = msg->module.data.data.cfg;
	}
}

static void message_handler(struct location_msg_data *msg)
{
	switch (state) {
	case STATE_INIT:
		on_state_init(msg);
		break;
	case STATE_RUNNING:
		switch (sub_state) {
		case SUB_STATE_SEARCH:
			on_state_running_location_search(msg);
			break;
		case SUB_STATE_IDLE:
			on_state_running_location_idle(msg);
			break;
		default:
			LOG_ERR("Unknown sub state.");
			break;
		}
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
APP_EVENT_SUBSCRIBE(MODULE, data_module_event);
APP_EVENT_SUBSCRIBE(MODULE, util_module_event);
APP_EVENT_SUBSCRIBE(MODULE, modem_module_event);
APP_EVENT_SUBSCRIBE(MODULE, cloud_module_event);
APP_EVENT_SUBSCRIBE(MODULE, location_module_event);
