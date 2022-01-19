/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <date_time.h>
#include <event_manager.h>
#include <nrf_modem_at.h>
#include <nrf_modem_gnss.h>
#if defined(CONFIG_NRF_CLOUD_PGPS) && defined(CONFIG_GNSS_MODULE_PGPS_STORE_LOCATION)
#include <net/nrf_cloud_pgps.h>
#endif

#define MODULE gnss_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/gnss_module_event.h"
#include "events/data_module_event.h"
#include "events/util_module_event.h"
#include "events/modem_module_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_GNSS_MODULE_LOG_LEVEL);

#define GNSS_TIMEOUT_DEFAULT	     60
/* Timeout (in seconds) for determining that GNSS search has stopped because of a timeout. Used
 * with pre-v1.3.0 MFWs, which do not support the sleep events.
 */
#define GNSS_INACTIVITY_TIMEOUT	     5
#define GNSS_EVENT_THREAD_STACK_SIZE 768
#define GNSS_EVENT_THREAD_PRIORITY   5

struct gnss_msg_data {
	union {
		struct app_module_event app;
		struct data_module_event data;
		struct util_module_event util;
		struct modem_module_event modem;
		struct gnss_module_event gnss;
	} module;
};

/* GNSS module super states. */
static enum state_type {
	STATE_INIT,
	STATE_RUNNING,
	STATE_SHUTDOWN
} state;

/* GNSS module sub states. */
static enum sub_state_type {
	SUB_STATE_IDLE,
	SUB_STATE_SEARCH
} sub_state;

static uint16_t gnss_timeout = GNSS_TIMEOUT_DEFAULT;

static struct nrf_modem_gnss_pvt_data_frame pvt_data;
static struct nrf_modem_gnss_nmea_data_frame nmea_data;
static struct nrf_modem_gnss_agps_data_frame agps_data;

K_MSGQ_DEFINE(event_msgq, sizeof(int), 10, 4);

static struct module_stats {
	/* Uptime set when GNSS search is started. Used to calculate search time for
	 * GNSS fix and timeout.
	 */
	uint64_t start_uptime;
	/* Number of satellites tracked before getting a GNSS fix or a timeout. */
	uint8_t satellites_tracked;
} stats;

static struct module_data self = {
	.name = "gnss",
	.msg_q = NULL,
	.supports_shutdown = true,
};

/* Forward declarations. */
static void message_handler(struct gnss_msg_data *data);
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
		struct gnss_msg_data msg = {
			.module.app = *event
		};

		message_handler(&msg);
	}

	if (is_data_module_event(eh)) {
		struct data_module_event *event = cast_data_module_event(eh);
		struct gnss_msg_data msg = {
			.module.data = *event
		};

		message_handler(&msg);
	}

	if (is_util_module_event(eh)) {
		struct util_module_event *event = cast_util_module_event(eh);
		struct gnss_msg_data msg = {
			.module.util = *event
		};

		message_handler(&msg);
	}

	if (is_gnss_module_event(eh)) {
		struct gnss_module_event *event = cast_gnss_module_event(eh);
		struct gnss_msg_data msg = {
			.module.gnss = *event
		};

		message_handler(&msg);
	}

	if (is_modem_module_event(eh)) {
		struct modem_module_event *event = cast_modem_module_event(eh);
		struct gnss_msg_data msg = {
			.module.modem = *event
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
		SEND_ERROR(gnss, GNSS_EVT_ERROR_CODE, err);
	}
}

static uint8_t set_satellites_tracked(struct nrf_modem_gnss_sv *sv, uint8_t sv_entries)
{
	uint8_t sv_used = 0;

	/* Check number of tracked Satellites. SV number 0 indicates that the satellite was not
	 * tracked.
	 */
	for (int i = 0; i < sv_entries; i++) {
		if (sv[i].sv) {
			sv_used++;
		}
	}

	return sv_used;
}

static void timeout_send(void)
{
	struct gnss_module_event *gnss_module_event = new_gnss_module_event();

	gnss_module_event->data.gnss.search_time = (uint32_t)(k_uptime_get() - stats.start_uptime);
	gnss_module_event->data.gnss.satellites_tracked =
				set_satellites_tracked(pvt_data.sv, ARRAY_SIZE(pvt_data.sv));
	gnss_module_event->type = GNSS_EVT_TIMEOUT;

	EVENT_SUBMIT(gnss_module_event);
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

#if defined(CONFIG_NRF_CLOUD_PGPS) && defined(CONFIG_GNSS_MODULE_PGPS_STORE_LOCATION)
				nrf_cloud_pgps_set_location(pvt_data.latitude, pvt_data.longitude);
#endif

				if (IS_ENABLED(CONFIG_GNSS_MODULE_PVT)) {
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
			if (IS_ENABLED(CONFIG_GNSS_MODULE_NMEA) && got_fix) {
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

			struct gnss_module_event *gnss_module_event = new_gnss_module_event();

			gnss_module_event->data.agps_request = agps_data;
			gnss_module_event->type = GNSS_EVT_AGPS_NEEDED;
			EVENT_SUBMIT(gnss_module_event);
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
			timeout_send();
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
	struct gnss_module_event *gnss_module_event = new_gnss_module_event();

	gnss_module_event->data.gnss.pvt.longitude = pvt_data.longitude;
	gnss_module_event->data.gnss.pvt.latitude = pvt_data.latitude;
	gnss_module_event->data.gnss.pvt.altitude = pvt_data.altitude;
	gnss_module_event->data.gnss.pvt.accuracy = pvt_data.accuracy;
	gnss_module_event->data.gnss.pvt.speed = pvt_data.speed;
	gnss_module_event->data.gnss.pvt.heading = pvt_data.heading;
	gnss_module_event->data.gnss.timestamp = k_uptime_get();
	gnss_module_event->type = GNSS_EVT_DATA_READY;
	gnss_module_event->data.gnss.format = GNSS_MODULE_DATA_FORMAT_PVT;
	gnss_module_event->data.gnss.satellites_tracked =
				set_satellites_tracked(pvt_data.sv, ARRAY_SIZE(pvt_data.sv));
	gnss_module_event->data.gnss.search_time = (uint32_t)(k_uptime_get() - stats.start_uptime);

	EVENT_SUBMIT(gnss_module_event);
}

static void data_send_nmea(void)
{
	struct gnss_module_event *gnss_module_event = new_gnss_module_event();

	strncpy(gnss_module_event->data.gnss.nmea, nmea_data.nmea_str,
		sizeof(gnss_module_event->data.gnss.nmea) - 1);

	gnss_module_event->data.gnss.nmea[sizeof(gnss_module_event->data.gnss.nmea) - 1] = '\0';
	gnss_module_event->data.gnss.timestamp = k_uptime_get();
	gnss_module_event->type = GNSS_EVT_DATA_READY;
	gnss_module_event->data.gnss.format = GNSS_MODULE_DATA_FORMAT_NMEA;
	gnss_module_event->data.gnss.satellites_tracked =
				set_satellites_tracked(pvt_data.sv, ARRAY_SIZE(pvt_data.sv));
	gnss_module_event->data.gnss.search_time = (uint32_t)(k_uptime_get() - stats.start_uptime);

	EVENT_SUBMIT(gnss_module_event);
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

	if (IS_ENABLED(CONFIG_GNSS_MODULE_NMEA)) {
		err = nrf_modem_gnss_nmea_mask_set(NRF_MODEM_GNSS_NMEA_GGA_MASK);
		if (err) {
			LOG_ERR("Failed to set GNSS NMEA mask, error %d", err);
			return;
		}
	}

#if defined(CONFIG_GNSS_MODULE_ELEVATION_MASK)
	err = nrf_modem_gnss_elevation_threshold_set(CONFIG_GNSS_MODULE_ELEVATION_MASK);
	if (err) {
		LOG_ERR("Failed to set elevation threshold to %d, error: %d",
			CONFIG_GNSS_MODULE_ELEVATION_MASK, err);
	} else {
		LOG_DBG("Elevation threshold set to %d", CONFIG_GNSS_MODULE_ELEVATION_MASK);
	}
#endif

	err = nrf_modem_gnss_start();
	if (err) {
		LOG_WRN("Failed to start GNSS, error: %d", err);
		return;
	}

	SEND_EVENT(gnss, GNSS_EVT_ACTIVE);
	stats.start_uptime = k_uptime_get();
}

static void inactive_send(void)
{
	SEND_EVENT(gnss, GNSS_EVT_INACTIVE);
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

static bool gnss_data_requested(enum app_module_data_type *data_list,
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
	const char *xmagpio_command = CONFIG_GNSS_MODULE_AT_MAGPIO;
	const char *xcoex0_command = CONFIG_GNSS_MODULE_AT_COEX0;

	LOG_DBG("MAGPIO command: %s", log_strdup(xmagpio_command));
	LOG_DBG("COEX0 command: %s", log_strdup(xcoex0_command));

	/* Make sure the AT command is not empty. */
	if (xmagpio_command[0] != '\0') {
		err = nrf_modem_at_printf("%s", xmagpio_command);
		if (err) {
			return err;
		}
	}

	if (xcoex0_command[0] != '\0') {
		err = nrf_modem_at_printf("%s", xcoex0_command);
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

	err = nrf_modem_gnss_event_handler_set(gnss_event_handler);
	if (err) {
		LOG_ERR("Failed to set GNSS event handler, error %d", err);
		return err;
	}

	return 0;
}

/* Message handler for STATE_INIT. */
static void on_state_init(struct gnss_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_INIT)) {
		gnss_timeout = msg->module.data.data.cfg.gnss_timeout;
	}
}

/* Message handler for STATE_RUNNING. */
static void on_state_running(struct gnss_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY)) {
		gnss_timeout = msg->module.data.data.cfg.gnss_timeout;
	}
}

/* Message handler for SUB_STATE_SEARCH. */
static void on_state_running_gnss_search(struct gnss_msg_data *msg)
{
	if (IS_EVENT(msg, gnss, GNSS_EVT_INACTIVE)) {
		sub_state_set(SUB_STATE_IDLE);
	}

	if (IS_EVENT(msg, app, APP_EVT_DATA_GET)) {
		if (!gnss_data_requested(msg->module.app.data_list,
					msg->module.app.count)) {
			return;
		}

		LOG_WRN("GNSS search already active and will not be restarted");
		LOG_WRN("Try setting a sample/publication interval greater");
		LOG_WRN("than the GNSS search timeout.");
	}
}

/* Message handler for SUB_STATE_IDLE. */
static void on_state_running_gnss_idle(struct gnss_msg_data *msg)
{
	if (IS_EVENT(msg, gnss, GNSS_EVT_ACTIVE)) {
		sub_state_set(SUB_STATE_SEARCH);
	}

	if (IS_EVENT(msg, app, APP_EVT_DATA_GET)) {
		if (!gnss_data_requested(msg->module.app.data_list,
					msg->module.app.count)) {
			return;
		}

		search_start();
	}
}

/* Message handler for all states. */
static void on_all_states(struct gnss_msg_data *msg)
{
	if (IS_EVENT(msg, app, APP_EVT_START)) {
		int err;

		err = module_start(&self);
		if (err) {
			LOG_ERR("Failed starting module, error: %d", err);
			SEND_ERROR(gnss, GNSS_EVT_ERROR_CODE, err);
		}

		state_set(STATE_INIT);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_INITIALIZED)) {
		int err;

		err = setup();
		if (err) {
			LOG_ERR("setup, error: %d", err);
			SEND_ERROR(gnss, GNSS_EVT_ERROR_CODE, err);
		}

		state_set(STATE_RUNNING);
	}

	if (IS_EVENT(msg, util, UTIL_EVT_SHUTDOWN_REQUEST)) {
		/* The module doesn't have anything to shut down and can
		 * report back immediately.
		 */
		SEND_SHUTDOWN_ACK(gnss, GNSS_EVT_SHUTDOWN_READY, self.id);
		state_set(STATE_SHUTDOWN);
	}
}

static void message_handler(struct gnss_msg_data *msg)
{
	switch (state) {
	case STATE_INIT:
		on_state_init(msg);
		break;
	case STATE_RUNNING:
		switch (sub_state) {
		case SUB_STATE_SEARCH:
			on_state_running_gnss_search(msg);
			break;
		case SUB_STATE_IDLE:
			on_state_running_gnss_idle(msg);
			break;
		default:
			LOG_ERR("Unknown GNSS module sub state.");
			break;
		}

		on_state_running(msg);
		break;
	case STATE_SHUTDOWN:
		/* The shutdown state has no transition. */
		break;
	default:
		LOG_ERR("Unknown GNSS module state.");
		break;
	}

	on_all_states(msg);
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE_EARLY(MODULE, app_module_event);
EVENT_SUBSCRIBE(MODULE, data_module_event);
EVENT_SUBSCRIBE(MODULE, util_module_event);
EVENT_SUBSCRIBE(MODULE, modem_module_event);
EVENT_SUBSCRIBE(MODULE, gnss_module_event);
