/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <date_time.h>
#include <event_manager.h>
#include <modem/at_cmd.h>
#include <nrf_modem_gnss.h>
#if defined(CONFIG_NRF_CLOUD_PGPS) && defined(CONFIG_GPS_MODULE_PGPS_STORE_LOCATION)
#include <net/nrf_cloud_pgps.h>
#endif

#define MODULE gps_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/gps_module_event.h"
#include "events/data_module_event.h"
#include "events/util_module_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_GPS_MODULE_LOG_LEVEL);

#define GNSS_TIMEOUT_DEFAULT	     60
/* Timeout (in seconds) for determining that GNSS search has stopped because of a timeout. Used
 * with pre-v1.3.0 MFWs, which do not support the sleep events.
 */
#define GNSS_INACTIVITY_TIMEOUT	     5
#define GNSS_EVENT_THREAD_STACK_SIZE 768
#define GNSS_EVENT_THREAD_PRIORITY   5

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

static uint16_t gnss_timeout = GNSS_TIMEOUT_DEFAULT;

static struct nrf_modem_gnss_pvt_data_frame pvt_data;
static struct nrf_modem_gnss_nmea_data_frame nmea_data;
static struct nrf_modem_gnss_agps_data_frame agps_data;

K_MSGQ_DEFINE(event_msgq, sizeof(int), 10, 4);

static struct module_data self = {
	.name = "gps",
	.msg_q = NULL,
	.supports_shutdown = true,
};

/* Forward declarations. */
static void message_handler(struct gps_msg_data *data);
static void search_start(void);
static void inactive_send(void);
static void time_set(void);
static void data_send_pvt(void);
static void data_send_nmea(void);
static void print_pvt(void);

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

static void inactivity_timeout_handler(struct k_timer *timer_id)
{
	/* Simulate a sleep event from GNSS with older MFWs. */
	int event = NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT;

	k_msgq_put(&event_msgq, &event, K_NO_WAIT);
}

static K_TIMER_DEFINE(inactivity_timer, inactivity_timeout_handler, NULL);

static void gnss_event_handler(int event)
{
	int err;

	/* Write the event into a message queue, processing is done in a separate thread. */
	err = k_msgq_put(&event_msgq, &event, K_NO_WAIT);
	if (err) {
		SEND_ERROR(gps, GPS_EVT_ERROR_CODE, err);
	}
}

/* GNSS event handler thread. */
static void gnss_event_thread_fn(void)
{
	int err;
	int event;
	static bool got_fix;

	while (true) {
		k_msgq_get(&event_msgq, &event, K_FOREVER);

		switch (event) {
		case NRF_MODEM_GNSS_EVT_PVT:
			/* Don't spam logs. */
			err = nrf_modem_gnss_read(&pvt_data,
						  sizeof(struct nrf_modem_gnss_pvt_data_frame),
						  NRF_MODEM_GNSS_DATA_PVT);
			if (err) {
				LOG_WRN("Reading PVT data from GNSS failed, error: %d", err);
				break;
			}

			if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
				k_timer_stop(&inactivity_timer);

				got_fix = true;

				inactive_send();
				time_set();

#if defined(CONFIG_NRF_CLOUD_PGPS) && defined(CONFIG_GPS_MODULE_PGPS_STORE_LOCATION)
				nrf_cloud_pgps_set_location(pvt_data.latitude, pvt_data.longitude);
#endif

				if (IS_ENABLED(CONFIG_GPS_MODULE_PVT)) {
					data_send_pvt();
				}
			} else {
				k_timer_start(&inactivity_timer,
					K_SECONDS(GNSS_INACTIVITY_TIMEOUT),
					K_NO_WAIT);

				got_fix = false;
			}

			print_pvt();

			break;
		case NRF_MODEM_GNSS_EVT_FIX:
			LOG_DBG("NRF_MODEM_GNSS_EVT_FIX");
			break;
		case NRF_MODEM_GNSS_EVT_NMEA:
			/* Don't spam logs. */
			if (IS_ENABLED(CONFIG_GPS_MODULE_NMEA) && got_fix) {
				err = nrf_modem_gnss_read(
						nmea_data.nmea_str,
						sizeof(struct nrf_modem_gnss_nmea_data_frame),
						NRF_MODEM_GNSS_DATA_NMEA);
				if (err) {
					LOG_WRN("Reading NMEA data from GNSS failed, error: %d",
						err);
					break;
				}

				data_send_nmea();
			}
			break;
		case NRF_MODEM_GNSS_EVT_AGPS_REQ:
			LOG_DBG("NRF_MODEM_GNSS_EVT_AGPS_REQ");
			err = nrf_modem_gnss_read(&agps_data,
						  sizeof(struct nrf_modem_gnss_agps_data_frame),
						  NRF_MODEM_GNSS_DATA_AGPS_REQ);
			if (err) {
				LOG_WRN("Reading A-GPS req data from GNSS failed, error: %d", err);
				break;
			}

			LOG_DBG("Requested A-GPS data: ephe 0x%08x, alm 0x%08x, data_flags 0x%02x",
				agps_data.sv_mask_ephe,
				agps_data.sv_mask_alm,
				agps_data.data_flags);

			struct gps_module_event *gps_module_event = new_gps_module_event();

			gps_module_event->data.agps_request = agps_data;
			gps_module_event->type = GPS_EVT_AGPS_NEEDED;
			EVENT_SUBMIT(gps_module_event);
			break;
		case NRF_MODEM_GNSS_EVT_BLOCKED:
			LOG_DBG("NRF_MODEM_GNSS_EVT_BLOCKED");
			break;
		case NRF_MODEM_GNSS_EVT_UNBLOCKED:
			LOG_DBG("NRF_MODEM_GNSS_EVT_UNBLOCKED");
			break;
		case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
			LOG_DBG("NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP");
			break;
		case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
			LOG_DBG("NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT");
			k_timer_stop(&inactivity_timer);
			SEND_EVENT(gps, GPS_EVT_TIMEOUT);

			/* Wrap sending of GPS_EVT_INACTIVE to avoid macro redefinition errors. */
			inactive_send();
			break;
		case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX:
			LOG_DBG("NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX");
			break;
		case NRF_MODEM_GNSS_EVT_REF_ALT_EXPIRED:
			LOG_DBG("NRF_MODEM_GNSS_EVT_REF_ALT_EXPIRED");
			break;
		default:
			break;
		}
	}
}

K_THREAD_DEFINE(gnss_event_thread, GNSS_EVENT_THREAD_STACK_SIZE,
		gnss_event_thread_fn, NULL, NULL, NULL,
		K_PRIO_PREEMPT(GNSS_EVENT_THREAD_PRIORITY), 0, 0);

/* Static module functions. */
static void data_send_pvt(void)
{
	struct gps_module_event *gps_module_event = new_gps_module_event();

	gps_module_event->data.gps.pvt.longitude = pvt_data.longitude;
	gps_module_event->data.gps.pvt.latitude = pvt_data.latitude;
	gps_module_event->data.gps.pvt.altitude = pvt_data.altitude;
	gps_module_event->data.gps.pvt.accuracy = pvt_data.accuracy;
	gps_module_event->data.gps.pvt.speed = pvt_data.speed;
	gps_module_event->data.gps.pvt.heading = pvt_data.heading;
	gps_module_event->data.gps.timestamp = k_uptime_get();
	gps_module_event->type = GPS_EVT_DATA_READY;
	gps_module_event->data.gps.format = GPS_MODULE_DATA_FORMAT_PVT;

	EVENT_SUBMIT(gps_module_event);
}

static void data_send_nmea(void)
{
	struct gps_module_event *gps_module_event = new_gps_module_event();

	strncpy(gps_module_event->data.gps.nmea, nmea_data.nmea_str,
		sizeof(gps_module_event->data.gps.nmea) - 1);

	gps_module_event->data.gps.nmea[sizeof(gps_module_event->data.gps.nmea) - 1] = '\0';
	gps_module_event->data.gps.timestamp = k_uptime_get();
	gps_module_event->type = GPS_EVT_DATA_READY;
	gps_module_event->data.gps.format = GPS_MODULE_DATA_FORMAT_NMEA;

	EVENT_SUBMIT(gps_module_event);
}

static void print_pvt(void)
{
	if (pvt_data.sv[0].sv == 0) {
		LOG_DBG("No tracked satellites");
		return;
	}

	LOG_DBG("Tracked satellites:");

	for (int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; i++) {
		if (pvt_data.sv[i].sv == 0) {
			break;
		}

		struct nrf_modem_gnss_sv *sv_data = &pvt_data.sv[i];

		LOG_DBG("PRN: %2d, C/N0: %4.1f, in fix: %d, unhealthy: %d",
			sv_data->sv,
			sv_data->cn0 / 10.0,
			sv_data->flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX ? 1 : 0,
			sv_data->flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY ? 1 : 0);
	}
}

static void search_start(void)
{
	int err;

	/* When GNSS is used in single fix mode, it needs to be stopped before it can be
	 * started again.
	 */
	(void)nrf_modem_gnss_stop();

	/* Configure GNSS for single fix mode. */
	err = nrf_modem_gnss_fix_interval_set(0);
	if (err) {
		LOG_WRN("Failed to set GNSS fix interval, error: %d", err);
		return;
	}

	err = nrf_modem_gnss_fix_retry_set(gnss_timeout);
	if (err) {
		LOG_WRN("Failed to set GNSS timeout, error: %d", err);
		return;
	}

	if (IS_ENABLED(CONFIG_GPS_MODULE_NMEA)) {
		err = nrf_modem_gnss_nmea_mask_set(NRF_MODEM_GNSS_NMEA_GGA_MASK);
		if (err) {
			LOG_ERR("Failed to set GNSS NMEA mask, error %d", err);
			return;
		}
	}

	err = nrf_modem_gnss_start();
	if (err) {
		LOG_WRN("Failed to start GNSS, error: %d", err);
		return;
	}

	SEND_EVENT(gps, GPS_EVT_ACTIVE);
}

static void inactive_send(void)
{
	SEND_EVENT(gps, GPS_EVT_INACTIVE);
}

static void time_set(void)
{
	/* Change datetime.year and datetime.month to accommodate the
	 * correct input format.
	 */
	struct tm gps_time = {
		.tm_year = pvt_data.datetime.year - 1900,
		.tm_mon = pvt_data.datetime.month - 1,
		.tm_mday = pvt_data.datetime.day,
		.tm_hour = pvt_data.datetime.hour,
		.tm_min = pvt_data.datetime.minute,
		.tm_sec = pvt_data.datetime.seconds,
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

static int lna_configure(void)
{
	int err;
	const char *xmagpio_command = CONFIG_GPS_MODULE_AT_MAGPIO;
	const char *xcoex0_command = CONFIG_GPS_MODULE_AT_COEX0;

	LOG_DBG("MAGPIO command: %s", log_strdup(xmagpio_command));
	LOG_DBG("COEX0 command: %s", log_strdup(xcoex0_command));

	/* Make sure the AT command is not empty. */
	if (xmagpio_command[0] != '\0') {
		err = at_cmd_write(xmagpio_command, NULL, 0, NULL);
		if (err) {
			return err;
		}
	}

	if (xcoex0_command[0] != '\0') {
		err = at_cmd_write(xcoex0_command, NULL, 0, NULL);
		if (err) {
			return err;
		}
	}

	return 0;
}

static int setup(void)
{
	int err;

	err = lna_configure();
	if (err) {
		LOG_ERR("Failed to configure LNA, error %d", err);
		return err;
	}

	err = nrf_modem_gnss_init();
	if (err) {
		LOG_ERR("Failed to initialize GNSS, error %d", err);
		return err;
	}

	err = nrf_modem_gnss_event_handler_set(gnss_event_handler);
	if (err) {
		LOG_ERR("Failed to set GNSS event handler, error %d", err);
		return err;
	}

	return 0;
}

/* Message handler for STATE_INIT. */
static void on_state_init(struct gps_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_INIT)) {
		gnss_timeout = msg->module.data.data.cfg.gps_timeout;
		state_set(STATE_RUNNING);
	}
}

/* Message handler for STATE_RUNNING. */
static void on_state_running(struct gps_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY)) {
		gnss_timeout = msg->module.data.data.cfg.gps_timeout;
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

		err = module_start(&self);
		if (err) {
			LOG_ERR("Failed starting module, error: %d", err);
			SEND_ERROR(gps, GPS_EVT_ERROR_CODE, err);
		}

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
		SEND_SHUTDOWN_ACK(gps, GPS_EVT_SHUTDOWN_READY, self.id);
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
EVENT_SUBSCRIBE_EARLY(MODULE, app_module_event);
EVENT_SUBSCRIBE(MODULE, data_module_event);
EVENT_SUBSCRIBE(MODULE, util_module_event);
EVENT_SUBSCRIBE(MODULE, gps_module_event);
