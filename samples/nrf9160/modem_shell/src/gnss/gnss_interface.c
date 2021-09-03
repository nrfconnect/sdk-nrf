/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <string.h>

#include <zephyr.h>
#include <init.h>
#include <assert.h>
#include <nrf_modem_gnss.h>
#if defined(CONFIG_SUPL_CLIENT_LIB)
#include <supl_session.h>
#include <supl_os_client.h>
#endif

#include "mosh_print.h"
#include "gnss.h"
#if defined(CONFIG_SUPL_CLIENT_LIB)
#include "gnss_supl_support.h"
#endif

#define GNSS_DATA_HANDLER_THREAD_STACK_SIZE 1536
#define GNSS_DATA_HANDLER_THREAD_PRIORITY   5

#define GNSS_WORKQ_THREAD_STACK_SIZE 2048
#define GNSS_WORKQ_THREAD_PRIORITY   5

enum gnss_operation_mode {
	GNSS_OP_MODE_CONTINUOUS,
	GNSS_OP_MODE_SINGLE_FIX,
	GNSS_OP_MODE_PERIODIC_FIX,
	GNSS_OP_MODE_PERIODIC_FIX_GNSS
};

K_THREAD_STACK_DEFINE(gnss_workq_stack_area, GNSS_WORKQ_THREAD_STACK_SIZE);

static struct k_work_q gnss_work_q;

#if defined(CONFIG_SUPL_CLIENT_LIB)
static struct k_work get_agps_data_work;

static struct nrf_modem_gnss_agps_data_frame agps_data;
#endif

static struct k_work gnss_stop_work;
static struct k_work_delayable gnss_start_work;
static struct k_work_delayable gnss_timeout_work;

static enum gnss_operation_mode operation_mode = GNSS_OP_MODE_CONTINUOUS;
static uint32_t periodic_fix_interval;
static uint32_t periodic_fix_retry;
static bool nmea_mask_set;
#if defined(CONFIG_SUPL_CLIENT_LIB)
static bool agps_automatic;
static bool agps_inject_ephe = true;
static bool agps_inject_alm = true;
static bool agps_inject_utc = true;
static bool agps_inject_klob = true;
static bool agps_inject_neq = true;
static bool agps_inject_time = true;
static bool agps_inject_pos = true;
static bool agps_inject_int = true;
#endif

/* Struct for an event item. The data is read in the event handler and passed as a part of the
 * event item because an NMEA string would get overwritten by the next NMEA string before the
 * consumer thread is able to read it. This is not a problem with PVT and AGPS request data, but
 * all data is handled in the same way for simplicity.
 */
struct event_item {
	uint8_t id;
	void    *data;
};

K_MSGQ_DEFINE(event_msgq, sizeof(struct event_item), 10, 4);

/* Output configuration */
static uint8_t pvt_output_level = 2;
static uint8_t nmea_output_level;
static uint8_t event_output_level;

static int get_event_data(void **dest, uint8_t type, size_t len)
{
	void *data;

	data = k_malloc(len);
	if (data == NULL) {
		return -1;
	}

	if (nrf_modem_gnss_read(data, len, type) != 0) {
		printk("Failed to read event data, type %d\n", type);
		k_free(data);
		return -1;
	}

	*dest = data;

	return 0;
}

/* This handler is called in ISR, so need to avoid heavy processing */
static void gnss_event_handler(int event_id)
{
	int err = 0;
	struct event_item event;

	event.id = event_id;
	event.data = NULL;

	/* Copy data for those events which have associated data */
	switch (event_id) {
	case NRF_MODEM_GNSS_EVT_PVT:
		err = get_event_data(&event.data,
				     NRF_MODEM_GNSS_DATA_PVT,
				     sizeof(struct nrf_modem_gnss_pvt_data_frame));
		break;

	case NRF_MODEM_GNSS_EVT_FIX:
		/* Data is not read for the fix notification because we use the PVT notification
		 * and the data is the same.
		 */
		if (operation_mode == GNSS_OP_MODE_PERIODIC_FIX) {
			k_work_cancel_delayable(&gnss_timeout_work);
			k_work_submit_to_queue(&gnss_work_q, &gnss_stop_work);
		}
		break;

	case NRF_MODEM_GNSS_EVT_NMEA:
		err = get_event_data(&event.data,
				     NRF_MODEM_GNSS_DATA_NMEA,
				     sizeof(struct nrf_modem_gnss_nmea_data_frame));
		break;

	case NRF_MODEM_GNSS_EVT_AGPS_REQ:
		err = get_event_data(&event.data,
				     NRF_MODEM_GNSS_DATA_AGPS_REQ,
				     sizeof(struct nrf_modem_gnss_agps_data_frame));
		break;

	default:
		break;
	}

	if (err) {
		/* Failed to get event data */
		return;
	}

	err = k_msgq_put(&event_msgq, &event, K_NO_WAIT);
	if (err) {
		/* Failed to put event into queue */
		k_free(event.data);
	}
}

static void print_pvt_flags(struct nrf_modem_gnss_pvt_data_frame *pvt)
{
	mosh_print("");
	mosh_print(
		"Fix valid:          %s",
		(pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) ==
		NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID ?
		"true" :
		"false");
	mosh_print(
		"Leap second valid:  %s",
		(pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_LEAP_SECOND_VALID) ==
		NRF_MODEM_GNSS_PVT_FLAG_LEAP_SECOND_VALID ?
		"true" :
		"false");
	mosh_print(
		"Sleep between PVT:  %s",
		(pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_SLEEP_BETWEEN_PVT) ==
		NRF_MODEM_GNSS_PVT_FLAG_SLEEP_BETWEEN_PVT ?
		"true" :
		"false");
	mosh_print(
		"Deadline missed:    %s",
		(pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED) ==
		NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED ?
		"true" :
		"false");
	mosh_print(
		"Insuf. time window: %s",
		(pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME) ==
		NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME ?
		"true" :
		"false");
}

static void print_pvt(struct nrf_modem_gnss_pvt_data_frame *pvt)
{
	if (pvt_output_level == 0) {
		return;
	}

	print_pvt_flags(pvt);

	if ((pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) == NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
		mosh_print(
			"Time:            %02d.%02d.%04d %02d:%02d:%02d.%03d",
			pvt->datetime.day,
			pvt->datetime.month,
			pvt->datetime.year,
			pvt->datetime.hour,
			pvt->datetime.minute,
			pvt->datetime.seconds,
			pvt->datetime.ms);

		mosh_print("Latitude:        %f", pvt->latitude);
		mosh_print("Longitude:       %f", pvt->longitude);
		mosh_print("Altitude:        %.1f m", pvt->altitude);
		mosh_print("Accuracy:        %.1f m", pvt->accuracy);
		mosh_print("Speed:           %.1f m/s", pvt->speed);
		mosh_print("Speed accuracy:  %.1f m/s", pvt->speed_accuracy);
		mosh_print("Heading:         %.1f deg", pvt->heading);
		mosh_print("PDOP:            %.1f", pvt->pdop);
		mosh_print("HDOP:            %.1f", pvt->hdop);
		mosh_print("VDOP:            %.1f", pvt->vdop);
		mosh_print("TDOP:            %.1f", pvt->tdop);

		mosh_print(
			"Google maps URL: https://maps.google.com/?q=%f,%f",
			pvt->latitude, pvt->longitude);
	}

	if (pvt_output_level < 2) {
		return;
	}

	/* SV data */
	for (int i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; i++) {
		if (pvt->sv[i].sv == 0) {
			/* SV not valid, skip */
			continue;
		}

		mosh_print(
			"SV: %3d C/N0: %4.1f el: %2d az: %3d signal: %d in fix: %d unhealthy: %d",
			pvt->sv[i].sv,
			pvt->sv[i].cn0 * 0.1,
			pvt->sv[i].elevation,
			pvt->sv[i].azimuth,
			pvt->sv[i].signal,
			(pvt->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX) ==
			NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX ? 1 : 0,
			(pvt->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY) ==
			NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY ? 1 : 0);
	}
}

static void print_nmea(struct nrf_modem_gnss_nmea_data_frame *nmea)
{
	if (nmea_output_level == 0) {
		return;
	}

	for (int i = 0; i < NRF_MODEM_GNSS_NMEA_MAX_LEN; i++) {
		if (nmea->nmea_str[i] == '\r' || nmea->nmea_str[i] == '\n') {
			nmea->nmea_str[i] = '\0';
			break;
		}
	}
	mosh_print("%s", nmea->nmea_str);
}

static void get_agps_data_flags_string(char *flags_string, uint32_t data_flags)
{
	size_t len;

	*flags_string = '\0';

	if ((data_flags & NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST) ==
	    NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST) {
		(void)strcat(flags_string, "utc | ");
	}
	if ((data_flags & NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST) ==
	    NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST) {
		(void)strcat(flags_string, "klob | ");
	}
	if ((data_flags & NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST) ==
	    NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST) {
		(void)strcat(flags_string, "neq | ");
	}
	if ((data_flags & NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST) ==
	    NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST) {
		(void)strcat(flags_string, "time | ");
	}
	if ((data_flags & NRF_MODEM_GNSS_AGPS_POSITION_REQUEST) ==
	    NRF_MODEM_GNSS_AGPS_POSITION_REQUEST) {
		(void)strcat(flags_string, "pos | ");
	}
	if ((data_flags & NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST) ==
	    NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST) {
		(void)strcat(flags_string, "int | ");
	}

	len = strlen(flags_string);
	if (len == 0) {
		(void)strcpy(flags_string, "none");
	} else {
		flags_string[len - 3] = '\0';
	}
}

static void data_handler_thread_fn(void)
{
	struct event_item event;

	while (true) {
		k_msgq_get(&event_msgq, &event, K_FOREVER);

		switch (event.id) {
		case NRF_MODEM_GNSS_EVT_PVT:
			print_pvt((struct nrf_modem_gnss_pvt_data_frame *)event.data);
			break;

		case NRF_MODEM_GNSS_EVT_FIX:
			if (event_output_level > 0) {
				mosh_print("GNSS: Got fix");
			}
			break;

		case NRF_MODEM_GNSS_EVT_NMEA:
			print_nmea((struct nrf_modem_gnss_nmea_data_frame *)event.data);
			break;

		case NRF_MODEM_GNSS_EVT_AGPS_REQ:
			if (event_output_level > 0) {
				char flags_string[48];

				get_agps_data_flags_string(
					flags_string,
					((struct nrf_modem_gnss_agps_data_frame *)
					 event.data)->data_flags);

				mosh_print(
					"GNSS: AGPS data needed (ephe: 0x%08x, alm: 0x%08x, "
					"flags: %s)",
					((struct nrf_modem_gnss_agps_data_frame *)
					 event.data)->sv_mask_ephe,
					((struct nrf_modem_gnss_agps_data_frame *)
					 event.data)->sv_mask_alm,
					flags_string);
			}
#if defined(CONFIG_SUPL_CLIENT_LIB)
			if (agps_automatic) {
				struct nrf_modem_gnss_agps_data_frame *event_agps_data;

				event_agps_data = (struct nrf_modem_gnss_agps_data_frame *)
						  event.data;
				(void)memset(&agps_data, 0, sizeof(agps_data));
				if (agps_inject_ephe) {
					agps_data.sv_mask_ephe = event_agps_data->sv_mask_ephe;
				}
				if (agps_inject_alm) {
					agps_data.sv_mask_alm = event_agps_data->sv_mask_alm;
				}
				if (agps_inject_utc) {
					agps_data.data_flags |=
						event_agps_data->data_flags &
						NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST;
				}
				if (agps_inject_klob) {
					agps_data.data_flags |=
						event_agps_data->data_flags &
						NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST;
				}
				if (agps_inject_neq) {
					agps_data.data_flags |=
						event_agps_data->data_flags &
						NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST;
				}
				if (agps_inject_time) {
					agps_data.data_flags |=
						event_agps_data->data_flags &
						NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST;
				}
				if (agps_inject_pos) {
					agps_data.data_flags |=
						event_agps_data->data_flags &
						NRF_MODEM_GNSS_AGPS_POSITION_REQUEST;
				}
				if (agps_inject_int) {
					agps_data.data_flags |=
						event_agps_data->data_flags &
						NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST;
				}
				k_work_submit_to_queue(&gnss_work_q, &get_agps_data_work);
			}
#endif
			break;

		case NRF_MODEM_GNSS_EVT_BLOCKED:
			if (event_output_level > 0) {
				mosh_print("GNSS: Blocked by LTE");
			}
			break;

		case NRF_MODEM_GNSS_EVT_UNBLOCKED:
			if (event_output_level > 0) {
				mosh_print("GNSS: Unblocked by LTE");
			}
			break;

		case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
			if (event_output_level > 0) {
				mosh_print("GNSS: Wakeup");
			}
			break;

		case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
			if (event_output_level > 0) {
				mosh_print("GNSS: Timeout, entering sleep");
			}
			break;

		case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX:
			if (event_output_level > 0) {
				mosh_print("GNSS: Fix, entering sleep");
			}
			break;

		case NRF_MODEM_GNSS_EVT_REF_ALT_EXPIRED:
			if (event_output_level > 0) {
				mosh_print("GNSS: Reference altitude for 3-satellite fix expired");
			}
			break;

		default:
			if (event_output_level > 0) {
				mosh_warn("GNSS: Unknown event %d received", event.id);
			}
			break;
		}

		k_free(event.data);
	}
}

K_THREAD_DEFINE(gnss_data_handler, GNSS_DATA_HANDLER_THREAD_STACK_SIZE,
		data_handler_thread_fn, NULL, NULL, NULL,
		K_PRIO_PREEMPT(GNSS_DATA_HANDLER_THREAD_PRIORITY), 0, 0);

static int gnss_enable_all_nmeas(void)
{
	return nrf_modem_gnss_nmea_mask_set(
		NRF_MODEM_GNSS_NMEA_GGA_MASK |
		NRF_MODEM_GNSS_NMEA_GLL_MASK |
		NRF_MODEM_GNSS_NMEA_GSA_MASK |
		NRF_MODEM_GNSS_NMEA_GSV_MASK |
		NRF_MODEM_GNSS_NMEA_RMC_MASK);
}

#if defined(CONFIG_SUPL_CLIENT_LIB)

static void get_agps_data(struct k_work *item)
{
	ARG_UNUSED(item);

	char flags_string[48];

	get_agps_data_flags_string(flags_string, agps_data.data_flags);

	mosh_print(
		"GNSS: Getting AGPS data (ephe: 0x%08x, alm: 0x%08x, flags: %s)...",
		agps_data.sv_mask_ephe,
		agps_data.sv_mask_alm,
		flags_string);

	if (open_supl_socket() == 0) {
		supl_session((void *)&agps_data);
		close_supl_socket();
	}
}

static void get_agps_data_type_string(char *type_string, uint16_t type)
{
	switch (type) {
	case NRF_MODEM_GNSS_AGPS_UTC_PARAMETERS:
		(void)strcpy(type_string, "utc");
		break;
	case NRF_MODEM_GNSS_AGPS_EPHEMERIDES:
		(void)strcpy(type_string, "ephe");
		break;
	case NRF_MODEM_GNSS_AGPS_ALMANAC:
		(void)strcpy(type_string, "alm");
		break;
	case NRF_MODEM_GNSS_AGPS_KLOBUCHAR_IONOSPHERIC_CORRECTION:
		(void)strcpy(type_string, "klob");
		break;
	case NRF_MODEM_GNSS_AGPS_NEQUICK_IONOSPHERIC_CORRECTION:
		(void)strcpy(type_string, "neq");
		break;
	case NRF_MODEM_GNSS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS:
		(void)strcpy(type_string, "time");
		break;
	case NRF_MODEM_GNSS_AGPS_LOCATION:
		(void)strcpy(type_string, "pos");
		break;
	case NRF_MODEM_GNSS_AGPS_INTEGRITY:
		(void)strcpy(type_string, "int");
		break;
	default:
		(void)strcpy(type_string, "UNKNOWN");
		break;
	}
}

static int inject_agps_data(void *agps,
			    size_t agps_size,
			    uint16_t type,
			    void *user_data)
{
	ARG_UNUSED(user_data);

	int err;
	char type_string[16];

	err = nrf_modem_gnss_agps_write(agps, agps_size, type);

	get_agps_data_type_string(type_string, type);

	if (err) {
		mosh_error(
			"GNSS: Failed to send AGPS data, type: %s (err: %d)",
			type_string,
			errno);
		return err;
	}

	mosh_print(
		"GNSS: Injected AGPS data, type: %s, size: %d",
		type_string,
		agps_size);

	return 0;
}

#endif /* CONFIG_SUPL_CLIENT_LIB */

static void start_gnss_work_fn(struct k_work *item)
{
	ARG_UNUSED(item);

	int err;

	err = nrf_modem_gnss_start();

	if (err) {
		mosh_error("GNSS: Failed to start GNSS");
	} else {
		if (event_output_level > 0) {
			mosh_print("GNSS: Search started");
		}
	}

	k_work_schedule_for_queue(&gnss_work_q,
				  &gnss_start_work,
				  K_SECONDS(periodic_fix_interval));

	if (periodic_fix_retry > 0 && periodic_fix_retry < periodic_fix_interval) {
		k_work_schedule_for_queue(&gnss_work_q,
					  &gnss_timeout_work,
					  K_SECONDS(periodic_fix_retry));
	}
}

static void stop_gnss_work_fn(struct k_work *item)
{
	ARG_UNUSED(item);

	int err;

	err = nrf_modem_gnss_stop();
	if (err) {
		mosh_error("GNSS: Failed to stop GNSS");
	}
}

static void handle_timeout_work_fn(struct k_work *item)
{
	ARG_UNUSED(item);

	int err;

	if (event_output_level > 0) {
		mosh_print("GNSS: Search timeout");
	}

	err = nrf_modem_gnss_stop();
	if (err) {
		mosh_error("GNSS: Failed to stop GNSS");
	}
}

static void gnss_api_init(void)
{
	int err;
	static bool gnss_api_initialized;

	if (gnss_api_initialized) {
		return;
	}

	/* Activate GNSS API v2 */
	nrf_modem_gnss_init();

	err = nrf_modem_gnss_event_handler_set(gnss_event_handler);

	if (!err) {
		gnss_api_initialized = true;

		k_work_queue_start(
			&gnss_work_q,
			gnss_workq_stack_area,
			K_THREAD_STACK_SIZEOF(gnss_workq_stack_area),
			GNSS_WORKQ_THREAD_PRIORITY,
			NULL);

		k_work_init(&gnss_stop_work, stop_gnss_work_fn);
		k_work_init_delayable(&gnss_start_work, start_gnss_work_fn);
		k_work_init_delayable(&gnss_timeout_work, handle_timeout_work_fn);

#if defined(CONFIG_SUPL_CLIENT_LIB)
		k_work_init(&get_agps_data_work, get_agps_data);

		static struct supl_api supl_api = {
			.read = supl_read,
			.write = supl_write,
			.handler = inject_agps_data,
			.logger = NULL, /* set to "supl_logger" to enable logging */
			.counter_ms = k_uptime_get
		};

		(void)supl_init(&supl_api);
#endif
	} else {
		mosh_error("GNSS: Failed to initialize GNSS API");
	}
}

int gnss_start(void)
{
	int err;

	gnss_api_init();

	if (!nmea_mask_set) {
		/* Enable all NMEAs by default */
		if (gnss_enable_all_nmeas() == 0) {
			nmea_mask_set = true;
		}
	}

	if (operation_mode == GNSS_OP_MODE_PERIODIC_FIX) {
		k_work_schedule_for_queue(&gnss_work_q, &gnss_start_work, K_NO_WAIT);
		return 0;
	}

	err = nrf_modem_gnss_start();
	if (err) {
		mosh_error("GNSS: Failed to start GNSS");
	}

	return err;
}

int gnss_stop(void)
{
	int err;

	gnss_api_init();

	if (operation_mode == GNSS_OP_MODE_PERIODIC_FIX) {
		k_work_cancel_delayable(&gnss_timeout_work);
		k_work_cancel_delayable(&gnss_start_work);
		nrf_modem_gnss_stop();
		return 0;
	}

	err = nrf_modem_gnss_stop();
	if (err) {
		mosh_error("GNSS: Failed to stop GNSS");
	}

	return err;
}


int gnss_delete_data(enum gnss_data_delete data)
{
	int err;
	uint32_t delete_mask;

	gnss_api_init();

	switch (data) {
	case GNSS_DATA_DELETE_EPHEMERIDES:
		delete_mask = NRF_MODEM_GNSS_DELETE_EPHEMERIDES;
		break;

	case GNSS_DATA_DELETE_ALL:
		/* Delete everything else but TCXO frequency offset data */
		delete_mask = NRF_MODEM_GNSS_DELETE_EPHEMERIDES |
			      NRF_MODEM_GNSS_DELETE_ALMANACS |
			      NRF_MODEM_GNSS_DELETE_IONO_CORRECTION_DATA |
			      NRF_MODEM_GNSS_DELETE_LAST_GOOD_FIX |
			      NRF_MODEM_GNSS_DELETE_GPS_TOW |
			      NRF_MODEM_GNSS_DELETE_GPS_WEEK |
			      NRF_MODEM_GNSS_DELETE_UTC_DATA;
		break;

	default:
		mosh_error("GNSS: Invalid erase data value");
		return -1;
	}

	err = nrf_modem_gnss_nv_data_delete(delete_mask);
	if (err) {
		mosh_error("GNSS: Failed to delete NV data");
	}

	return err;
}

int gnss_set_continuous_mode(void)
{
	int err;

	gnss_api_init();

	operation_mode = GNSS_OP_MODE_CONTINUOUS;

	err = nrf_modem_gnss_fix_interval_set(1);
	if (err) {
		mosh_error("GNSS: Failed to set fix interval");
		return err;
	}

	err = nrf_modem_gnss_fix_retry_set(0);
	if (err) {
		mosh_error("GNSS: Failed to set fix retry");
	}

	return err;
}

int gnss_set_single_fix_mode(uint16_t fix_retry)
{
	int err;

	gnss_api_init();

	operation_mode = GNSS_OP_MODE_SINGLE_FIX;

	err = nrf_modem_gnss_fix_interval_set(0);
	if (err) {
		mosh_error("GNSS: Failed to set fix interval");
		return err;
	}

	err = nrf_modem_gnss_fix_retry_set(fix_retry);
	if (err) {
		mosh_error("GNSS: Failed to set fix retry");
	}

	return err;
}

int gnss_set_periodic_fix_mode(uint32_t fix_interval, uint16_t fix_retry)
{
	int err;

	gnss_api_init();

	operation_mode = GNSS_OP_MODE_PERIODIC_FIX;
	periodic_fix_interval = fix_interval;
	periodic_fix_retry = fix_retry;

	/* Periodic fix mode controlled by the application, GNSS is used in continuous tracking
	 * mode and it's started and stopped as needed.
	 */
	err = nrf_modem_gnss_fix_interval_set(1);
	if (err) {
		mosh_error("GNSS: Failed to set fix interval");
		return err;
	}

	err = nrf_modem_gnss_fix_retry_set(0);
	if (err) {
		mosh_error("GNSS: Failed to set fix retry");
	}

	return err;
}

int gnss_set_periodic_fix_mode_gnss(uint16_t fix_interval, uint16_t fix_retry)
{
	int err;

	gnss_api_init();

	operation_mode = GNSS_OP_MODE_PERIODIC_FIX_GNSS;

	err = nrf_modem_gnss_fix_interval_set(fix_interval);
	if (err) {
		mosh_error("GNSS: Failed to set fix interval");
		return err;
	}

	err = nrf_modem_gnss_fix_retry_set(fix_retry);
	if (err) {
		mosh_error("GNSS: Failed to set fix retry");
	}

	return err;
}

int gnss_set_system_mask(uint8_t system_mask)
{
	int err;

	err = nrf_modem_gnss_system_mask_set(system_mask);
	if (err) {
		mosh_error("GNSS: Failed to set system mask");
	}

	return err;
}

int gnss_set_duty_cycling_policy(enum gnss_duty_cycling_policy policy)
{
	int err;
	uint8_t power_mode;

	gnss_api_init();

	switch (policy) {
	case GNSS_DUTY_CYCLING_DISABLED:
		power_mode = NRF_MODEM_GNSS_PSM_DISABLED;
		break;

	case GNSS_DUTY_CYCLING_PERFORMANCE:
		power_mode = NRF_MODEM_GNSS_PSM_DUTY_CYCLING_PERFORMANCE;
		break;

	case GNSS_DUTY_CYCLING_POWER:
		power_mode = NRF_MODEM_GNSS_PSM_DUTY_CYCLING_POWER;
		break;

	default:
		mosh_error("GNSS: Invalid duty cycling policy");
		return -1;
	}

	err = nrf_modem_gnss_power_mode_set(power_mode);
	if (err) {
		mosh_error("GNSS: Failed to set duty cycling policy");
	}

	return err;
}

int gnss_set_elevation_threshold(uint8_t elevation)
{
	int err;

	gnss_api_init();

	err = nrf_modem_gnss_elevation_threshold_set(elevation);
	if (err) {
		mosh_error("GNSS: Failed to set elevation threshold");
	}

	return err;
}

int gnss_set_use_case(bool low_accuracy_enabled, bool scheduled_downloads_disabled)
{
	int err;
	uint8_t use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;

	gnss_api_init();

	if (low_accuracy_enabled) {
		use_case |= NRF_MODEM_GNSS_USE_CASE_LOW_ACCURACY;
	}
	if (scheduled_downloads_disabled) {
		use_case |= NRF_MODEM_GNSS_USE_CASE_SCHED_DOWNLOAD_DISABLE;
	}

	err = nrf_modem_gnss_use_case_set(use_case);
	if (err) {
		mosh_error("GNSS: Failed to set use case, check modem FW version");
	}

	return err;
}

int gnss_set_nmea_mask(uint16_t nmea_mask)
{
	int err;

	gnss_api_init();

	err = nrf_modem_gnss_nmea_mask_set(nmea_mask);
	if (err) {
		mosh_error("GNSS: Failed to set NMEA mask");
	} else {
		nmea_mask_set = true;
	}

	return err;
}

int gnss_set_priority_time_windows(bool value)
{
	int err;

	gnss_api_init();

	if (value) {
		err = nrf_modem_gnss_prio_mode_enable();
	} else {
		err = nrf_modem_gnss_prio_mode_disable();
	}

	if (err) {
		mosh_error("GNSS: Failed to set priority time windows");
	}

	return err;
}

int gnss_set_dynamics_mode(enum gnss_dynamics_mode mode)
{
	int err;
	uint32_t dynamics_mode;

	gnss_api_init();

	switch (mode) {
	case GNSS_DYNAMICS_MODE_GENERAL:
		dynamics_mode = NRF_MODEM_GNSS_DYNAMICS_GENERAL_PURPOSE;
		break;
	case GNSS_DYNAMICS_MODE_STATIONARY:
		dynamics_mode = NRF_MODEM_GNSS_DYNAMICS_STATIONARY;
		break;
	case GNSS_DYNAMICS_MODE_PEDESTRIAN:
		dynamics_mode = NRF_MODEM_GNSS_DYNAMICS_PEDESTRIAN;
		break;
	case GNSS_DYNAMICS_MODE_AUTOMOTIVE:
		dynamics_mode = NRF_MODEM_GNSS_DYNAMICS_AUTOMOTIVE;
		break;
	default:
		mosh_error("GNSS: Invalid dynamics mode value %d", mode);
		return -EINVAL;
	}

	err = nrf_modem_gnss_dyn_mode_change(dynamics_mode);
	if (err) {
		mosh_error("GNSS: Failed to change dynamics mode");
	}

	return err;
}

int gnss_set_qzss_nmea_mode(enum gnss_qzss_nmea_mode mode)
{
	int err;
	uint8_t nmea_mode;

	gnss_api_init();

	switch (mode) {
	case GNSS_QZSS_NMEA_MODE_STANDARD:
		nmea_mode = NRF_MODEM_GNSS_QZSS_NMEA_MODE_STANDARD;
		break;

	case GNSS_QZSS_NMEA_MODE_CUSTOM:
		nmea_mode = NRF_MODEM_GNSS_QZSS_NMEA_MODE_CUSTOM;
		break;

	default:
		mosh_error("GNSS: Invalid QZSS NMEA mode value %d", mode);
		return -EINVAL;
	}

	err = nrf_modem_gnss_qzss_nmea_mode_set(nmea_mode);
	if (err) {
		mosh_error("GNSS: Failed to set QZSS NMEA mode");
	}

	return err;
}

int gnss_set_qzss_mask(uint16_t mask)
{
	int err;

	gnss_api_init();

	err = nrf_modem_gnss_qzss_prn_mask_set(mask);
	if (err) {
		mosh_error("GNSS: Failed to set QZSS PRN mask");
	}

	return err;
}

int gnss_set_1pps_mode(const struct gnss_1pps_mode *config)
{
	int err;
	struct nrf_modem_gnss_1pps_config pps_config;

	gnss_api_init();

	if (config->enable) {
		pps_config.pulse_interval = config->pulse_interval;
		pps_config.pulse_width = config->pulse_width;
		pps_config.apply_start_time = config->apply_start_time;
		pps_config.year = config->year;
		pps_config.month = config->month;
		pps_config.day = config->day;
		pps_config.hour = config->hour;
		pps_config.minute = config->minute;
		pps_config.second = config->second;

		err = nrf_modem_gnss_1pps_enable(&pps_config);
	} else {
		err = nrf_modem_gnss_1pps_disable();
	}

	if (err) {
		mosh_error("GNSS: Failed to set 1PPS mode");
	}

	return err;
}

int gnss_set_timing_source(enum gnss_timing_source source)
{
	int err;
	uint8_t timing_source;

	gnss_api_init();

	switch (source) {
	case GNSS_TIMING_SOURCE_RTC:
		timing_source = NRF_MODEM_GNSS_TIMING_SOURCE_RTC;
		break;

	case GNSS_TIMING_SOURCE_TCXO:
		timing_source = NRF_MODEM_GNSS_TIMING_SOURCE_TCXO;
		break;

	default:
		mosh_error("GNSS: Invalid timing source");
		return -EINVAL;
	}

	err = nrf_modem_gnss_timing_source_set(timing_source);
	if (err) {
		mosh_error("GNSS: Failed to set timing source");
	}

	return err;
}

int gnss_set_agps_data_enabled(bool ephe, bool alm, bool utc, bool klob,
			       bool neq, bool time, bool pos, bool integrity)
{
#if defined(CONFIG_SUPL_CLIENT_LIB)
	agps_inject_ephe = ephe;
	agps_inject_alm = alm;
	agps_inject_utc = utc;
	agps_inject_klob = klob;
	agps_inject_neq = neq;
	agps_inject_time = time;
	agps_inject_pos = pos;
	agps_inject_int = integrity;

	return 0;
#else
	mosh_error("GNSS: Enable CONFIG_SUPL_CLIENT_LIB for AGPS support");
	return -EOPNOTSUPP;
#endif
}

int gnss_set_agps_automatic(bool value)
{
#if defined(CONFIG_SUPL_CLIENT_LIB)
	agps_automatic = value;

	return 0;
#else
	mosh_error("GNSS: Enable CONFIG_SUPL_CLIENT_LIB for AGPS support");
	return -EOPNOTSUPP;
#endif
}

int gnss_inject_agps_data(void)
{
#if defined(CONFIG_SUPL_CLIENT_LIB)
	gnss_api_init();

	(void)memset(&agps_data, 0, sizeof(agps_data));
	if (agps_inject_ephe) {
		agps_data.sv_mask_ephe = 0xffffffff;
	}
	if (agps_inject_alm) {
		agps_data.sv_mask_alm = 0xffffffff;
	}
	if (agps_inject_utc) {
		agps_data.data_flags |= NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST;
	}
	if (agps_inject_klob) {
		agps_data.data_flags |= NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST;
	}
	if (agps_inject_neq) {
		agps_data.data_flags |= NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST;
	}
	if (agps_inject_time) {
		agps_data.data_flags |= NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST;
	}
	if (agps_inject_pos) {
		agps_data.data_flags |= NRF_MODEM_GNSS_AGPS_POSITION_REQUEST;
	}
	if (agps_inject_int) {
		agps_data.data_flags |= NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST;
	}

	k_work_submit_to_queue(&gnss_work_q, &get_agps_data_work);

	return 0;
#else
	mosh_error("GNSS: Enable CONFIG_SUPL_CLIENT_LIB for AGPS support");
	return -EOPNOTSUPP;
#endif
}

int gnss_set_pvt_output_level(uint8_t level)
{
	if (level < 0 || level > 2) {
		return -EINVAL;
	}

	pvt_output_level = level;

	return 0;
}

int gnss_set_nmea_output_level(uint8_t level)
{
	if (level < 0 || level > 1) {
		return -EINVAL;
	}

	nmea_output_level = level;

	return 0;
}

int gnss_set_event_output_level(uint8_t level)
{
	if (level < 0 || level > 1) {
		return -EINVAL;
	}

	event_output_level = level;

	return 0;
}
