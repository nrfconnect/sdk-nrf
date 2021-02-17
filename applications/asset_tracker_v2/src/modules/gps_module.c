/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <drivers/gps.h>
#include <stdio.h>
#include <date_time.h>
#include <event_manager.h>
#include <drivers/gps.h>

#define MODULE gps_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/gps_module_event.h"
#include "events/data_module_event.h"
#include "events/util_module_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_GPS_MODULE_LOG_LEVEL);

/* Maximum GPS interval value. Dummy value, will not be used. Starting
 * and stopping of GPS is done by the application.
 */
#define GPS_INTERVAL_MAX 1800

struct gps_msg_data {
	union {
		struct app_module_event app;
		struct data_module_event data;
		struct util_module_event util;
		struct gps_module_event gps;
	} module;
};

/* GPS module super states. */
static enum state_type {
	STATE_INIT,
	STATE_RUNNING,
	STATE_SHUTDOWN
} state;

/* GPS module sub states. */
static enum sub_state_type {
	SUB_STATE_IDLE,
	SUB_STATE_SEARCH
} sub_state;

/* GPS device. Used to identify the GPS driver in the sensor API. */
static const struct device *gps_dev;

/* nRF9160 GPS driver configuration. */
static struct gps_config gps_cfg = {
	.nav_mode = GPS_NAV_MODE_PERIODIC,
	.power_mode = GPS_POWER_MODE_DISABLED,
	.interval = GPS_INTERVAL_MAX
};

static struct module_data self = {
	.name = "gps",
	.msg_q = NULL,
};

/* Forward declarations. */
static void message_handler(struct gps_msg_data *data);
static void search_start(void);
static void search_stop(void);
static void time_set(struct gps_pvt *gps_data);
static void data_send(struct gps_pvt *gps_data);

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
static bool event_handler(const struct event_header *eh)
{
	if (is_app_module_event(eh)) {
		struct app_module_event *event = cast_app_module_event(eh);
		struct gps_msg_data msg = {
			.module.app = *event
		};

		message_handler(&msg);
	}

	if (is_data_module_event(eh)) {
		struct data_module_event *event = cast_data_module_event(eh);
		struct gps_msg_data msg = {
			.module.data = *event
		};

		message_handler(&msg);
	}

	if (is_util_module_event(eh)) {
		struct util_module_event *event = cast_util_module_event(eh);
		struct gps_msg_data msg = {
			.module.util = *event
		};

		message_handler(&msg);
	}

	if (is_gps_module_event(eh)) {
		struct gps_module_event *event = cast_gps_module_event(eh);
		struct gps_msg_data msg = {
			.module.gps = *event
		};

		message_handler(&msg);
	}

	return false;
}

static void gps_event_handler(const struct device *dev, struct gps_event *evt)
{
	switch (evt->type) {
	case GPS_EVT_SEARCH_STARTED:
		LOG_DBG("GPS_EVT_SEARCH_STARTED");
		break;
	case GPS_EVT_SEARCH_STOPPED:
		LOG_DBG("GPS_EVT_SEARCH_STOPPED");
		break;
	case GPS_EVT_SEARCH_TIMEOUT:
		LOG_DBG("GPS_EVT_SEARCH_TIMEOUT");
		SEND_EVENT(gps, GPS_EVT_TIMEOUT);
		search_stop();
		break;
	case GPS_EVT_PVT:
		/* Don't spam logs */
		break;
	case GPS_EVT_PVT_FIX:
		LOG_DBG("GPS_EVT_PVT_FIX");
		time_set(&evt->pvt);
		data_send(&evt->pvt);
		search_stop();
		break;
	case GPS_EVT_NMEA:
		/* Don't spam logs */
		break;
	case GPS_EVT_NMEA_FIX:
		LOG_DBG("Position fix with NMEA data");
		break;
	case GPS_EVT_OPERATION_BLOCKED:
		LOG_DBG("GPS_EVT_OPERATION_BLOCKED");
		break;
	case GPS_EVT_OPERATION_UNBLOCKED:
		LOG_DBG("GPS_EVT_OPERATION_UNBLOCKED");
		break;
	case GPS_EVT_AGPS_DATA_NEEDED:
		LOG_DBG("GPS_EVT_AGPS_DATA_NEEDED");
		struct gps_module_event *gps_module_event =
				new_gps_module_event();

		gps_module_event->data.agps_request = evt->agps_request;
		gps_module_event->type = GPS_EVT_AGPS_NEEDED;
		EVENT_SUBMIT(gps_module_event);
		break;
	case GPS_EVT_ERROR:
		LOG_DBG("GPS_EVT_ERROR\n");
		break;
	default:
		break;
	}
}

/* Static module functions. */
static void data_send(struct gps_pvt *gps_data)
{
	struct gps_module_event *gps_module_event = new_gps_module_event();

	gps_module_event->data.gps.longitude = gps_data->longitude;
	gps_module_event->data.gps.latitude = gps_data->latitude;
	gps_module_event->data.gps.altitude = gps_data->altitude;
	gps_module_event->data.gps.accuracy = gps_data->accuracy;
	gps_module_event->data.gps.speed = gps_data->speed;
	gps_module_event->data.gps.heading = gps_data->heading;
	gps_module_event->data.gps.timestamp = k_uptime_get();
	gps_module_event->type = GPS_EVT_DATA_READY;

	EVENT_SUBMIT(gps_module_event);
}

static void search_start(void)
{
	int err;

	err = gps_start(gps_dev, &gps_cfg);
	if (err) {
		LOG_WRN("Failed to start GPS, error: %d", err);
		return;
	}

	SEND_EVENT(gps, GPS_EVT_ACTIVE);
}

static void search_stop(void)
{
	int err;

	err = gps_stop(gps_dev);
	if (err) {
		LOG_WRN("Failed to stop GPS, error: %d", err);
		return;
	}

	SEND_EVENT(gps, GPS_EVT_INACTIVE);
}

static void time_set(struct gps_pvt *gps_data)
{
	/* Change datetime.year and datetime.month to accommodate the
	 * correct input format.
	 */
	struct tm gps_time = {
		.tm_year = gps_data->datetime.year - 1900,
		.tm_mon = gps_data->datetime.month - 1,
		.tm_mday = gps_data->datetime.day,
		.tm_hour = gps_data->datetime.hour,
		.tm_min = gps_data->datetime.minute,
		.tm_sec = gps_data->datetime.seconds,
	};

	date_time_set(&gps_time);
}

static bool gps_data_requested(enum app_module_data_type *data_list,
			       size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (data_list[i] == APP_DATA_GNSS) {
			return true;
		}
	}

	return false;
}

static int setup(void)
{
	int err;

	gps_dev = device_get_binding(CONFIG_GPS_DEV_NAME);
	if (gps_dev == NULL) {
		LOG_ERR("Could not get %s device", CONFIG_GPS_DEV_NAME);
		return -ENODEV;
	}

	err = gps_init(gps_dev, gps_event_handler);
	if (err) {
		LOG_ERR("Could not initialize GPS, error: %d", err);
		return err;
	}

	return 0;
}

/* Message handler for STATE_INIT. */
static void on_state_init(struct gps_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_INIT)) {
		gps_cfg.timeout = msg->module.data.data.cfg.gps_timeout;
		state_set(STATE_RUNNING);
	}
}

/* Message handler for STATE_RUNNING. */
static void on_state_running(struct gps_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY)) {
		gps_cfg.timeout = msg->module.data.data.cfg.gps_timeout;
	}
}

/* Message handler for SUB_STATE_SEARCH. */
static void on_state_running_gps_search(struct gps_msg_data *msg)
{
	if (IS_EVENT(msg, gps, GPS_EVT_INACTIVE)) {
		sub_state_set(SUB_STATE_IDLE);
	}

	if (IS_EVENT(msg, app, APP_EVT_DATA_GET)) {
		if (!gps_data_requested(msg->module.app.data_list,
					msg->module.app.count)) {
			return;
		}

		LOG_WRN("GPS search already active and will not be restarted");
		LOG_WRN("Try setting a sample/publication interval greater");
		LOG_WRN("than the GPS search timeout.");
	}
}

/* Message handler for SUB_STATE_IDLE. */
static void on_state_running_gps_idle(struct gps_msg_data *msg)
{
	if (IS_EVENT(msg, gps, GPS_EVT_ACTIVE)) {
		sub_state_set(SUB_STATE_SEARCH);
	}

	if (IS_EVENT(msg, app, APP_EVT_DATA_GET)) {
		if (!gps_data_requested(msg->module.app.data_list,
					msg->module.app.count)) {
			return;
		}

		search_start();
	}
}

/* Message handler for all states. */
static void on_all_states(struct gps_msg_data *msg)
{
	if (IS_EVENT(msg, app, APP_EVT_START)) {
		int err;

		module_start(&self);
		state_set(STATE_INIT);

		err = setup();
		if (err) {
			LOG_ERR("setup, error: %d", err);
			SEND_ERROR(gps, GPS_EVT_ERROR_CODE, err);
		}
	}

	if (IS_EVENT(msg, util, UTIL_EVT_SHUTDOWN_REQUEST)) {
		/* The module doesn't have anything to shut down and can
		 * report back immediately.
		 */
		SEND_EVENT(gps, GPS_EVT_SHUTDOWN_READY);
		state_set(STATE_SHUTDOWN);
	}
}

static void message_handler(struct gps_msg_data *msg)
{
	switch (state) {
	case STATE_INIT:
		on_state_init(msg);
		break;
	case STATE_RUNNING:
		switch (sub_state) {
		case SUB_STATE_SEARCH:
			on_state_running_gps_search(msg);
			break;
		case SUB_STATE_IDLE:
			on_state_running_gps_idle(msg);
			break;
		default:
			LOG_ERR("Unknown GPS module sub state.");
			break;
		}

		on_state_running(msg);
		break;
	case STATE_SHUTDOWN:
		/* The shutdown state has no transition. */
		break;
	default:
		LOG_ERR("Unknown GPS module state.");
		break;
	}

	on_all_states(msg);
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, app_module_event);
EVENT_SUBSCRIBE(MODULE, data_module_event);
EVENT_SUBSCRIBE(MODULE, util_module_event);
EVENT_SUBSCRIBE(MODULE, gps_module_event);
