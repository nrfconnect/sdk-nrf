/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <assert.h>
#include <nrf_errno.h>
#include <nrf_modem_at.h>
#include <nrf_modem_gnss.h>

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS) || \
	defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
#include <net/lwm2m_client_utils_location.h>
#include "cloud_lwm2m.h"
#endif

#if defined(CONFIG_LWM2M_CARRIER)
#include <time.h>
#include <lwm2m_carrier.h>
#endif

#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS)
#include <stdlib.h>
#include <modem/modem_jwt.h>
#include <net/nrf_cloud_rest.h>
#if defined(CONFIG_NRF_CLOUD_COAP)
#include <net/nrf_cloud_coap.h>
#endif
#if defined(CONFIG_NRF_CLOUD_AGNSS)
#include <net/nrf_cloud_agnss.h>
#endif /* CONFIG_NRF_CLOUD_AGNSS */
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif /* CONFIG_NRF_CLOUD_PGPS */
#endif /* CONFIG_NRF_CLOUD_AGNSS || CONFIG_NRF_CLOUD_PGPS */

#if defined(CONFIG_SUPL_CLIENT_LIB)
#include <supl_session.h>
#include <supl_os_client.h>
#include "gnss_supl_support.h"
#endif /* CONFIG_SUPL_CLIENT_LIB */

#include "mosh_print.h"
#include "str_utils.h"
#include "gnss.h"

#if (defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS)) && \
	defined(CONFIG_SUPL_CLIENT_LIB)
BUILD_ASSERT(false, "nRF Cloud assistance and SUPL library cannot be enabled at the same time");
#endif

#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_NRF_CLOUD_PGPS)
/* Verify that MQTT, REST or COAP is enabled */
BUILD_ASSERT(IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) ||
	     IS_ENABLED(CONFIG_NRF_CLOUD_REST) ||
	     IS_ENABLED(CONFIG_NRF_CLOUD_COAP),
	     "CONFIG_NRF_CLOUD_MQTT, CONFIG_NRF_CLOUD_REST or CONFIG_NRF_CLOUD_COAP "
	     "transport must be enabled");
#endif

#define GNSS_DATA_HANDLER_THREAD_STACK_SIZE 1536
#define GNSS_DATA_HANDLER_THREAD_PRIORITY   5

enum gnss_operation_mode {
	GNSS_OP_MODE_CONTINUOUS,
	GNSS_OP_MODE_SINGLE_FIX,
	GNSS_OP_MODE_PERIODIC_FIX,
	GNSS_OP_MODE_PERIODIC_FIX_GNSS
};

extern struct k_work_q mosh_common_work_q;

static struct k_work gnss_stop_work;
static struct k_work_delayable gnss_start_work;
static struct k_work_delayable gnss_timeout_work;

static enum gnss_operation_mode operation_mode = GNSS_OP_MODE_CONTINUOUS;
static uint32_t periodic_fix_interval;
static uint32_t periodic_fix_retry;
static bool nmea_mask_set;
static struct nrf_modem_gnss_agnss_data_frame agnss_need;
static uint8_t gnss_elevation = 5; /* init to modem default threshold angle */

#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_SUPL_CLIENT_LIB)
static struct k_work get_agnss_data_work;
static bool agnss_automatic;
static bool agnss_inject_ephe = true;
static bool agnss_inject_alm = true;
static bool agnss_inject_utc = true;
static bool agnss_inject_klob = true;
static bool agnss_inject_neq = true;
static bool agnss_inject_time = true;
static bool agnss_inject_pos = true;
static bool agnss_inject_int = true;
#endif /* CONFIG_NRF_CLOUD_AGNSS || CONFIG_SUPL_CLIENT_LIB */

#if defined(CONFIG_NRF_CLOUD_AGNSS) && !defined(CONFIG_NRF_CLOUD_MQTT) && \
	!defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
static char agnss_data_buf[3500];
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
static bool pgps_enabled;
static struct nrf_cloud_pgps_prediction *prediction;
static struct k_work inject_pgps_data_work;
static struct k_work notify_pgps_prediction_work;
#if !defined(CONFIG_NRF_CLOUD_MQTT)
static struct gps_pgps_request pgps_request;
static struct k_work get_pgps_data_work;
#endif /* !CONFIG_NRF_CLOUD_MQTT */
#endif /* CONFIG_NRF_CLOUD_PGPS */

#if defined(CONFIG_NRF_CLOUD_AGNSS) && defined(CONFIG_NRF_CLOUD_REST) && \
	!defined(CONFIG_NRF_CLOUD_MQTT) && !defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
#define AGNSS_USES_REST
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS) && defined(CONFIG_NRF_CLOUD_REST) && \
	!defined(CONFIG_NRF_CLOUD_MQTT) && !defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
#define PGPS_USES_REST
#endif

#if defined(AGNSS_USES_REST) || defined(PGPS_USES_REST)
/* JWT and receive buffers will be needed if either A-GNSS or P-GPS uses REST. */
static char jwt_buf[600];
static char rx_buf[2048];
#endif

#if defined(CONFIG_NRF_CLOUD_AGNSS_FILTERED_RUNTIME)
static bool gnss_filtered_ephemerides_enabled;
#endif /* CONFIG_NRF_CLOUD_AGNSS_FILTERED_RUNTIME */

/* Struct for an event item. The data is read in the event handler and passed as a part of the
 * event item because an NMEA string would get overwritten by the next NMEA string before the
 * consumer thread is able to read it. This is not a problem with PVT and A-GNSS request data, but
 * all data is handled in the same way for simplicity.
 */
struct event_item {
	uint8_t id;
	void    *data;
};

K_MSGQ_DEFINE(gnss_event_msgq, sizeof(struct event_item), 10, 4);

/* Output configuration */
static uint8_t pvt_output_level = 2;
static uint8_t nmea_output_level;
static uint8_t event_output_level;

static const char *gnss_err_to_str(int err)
{
	switch (err) {
	case -NRF_EPERM:
		return "Modem library not initialized";
	case -NRF_ENOMEM:
		return "not enough shared memory";
	case -NRF_EACCES:
		return "GNSS not enabled in system or functional mode";
	case -NRF_EINVAL:
		return "wrong state or invalid data";
	case -NRF_ENOMSG:
		return "no data to read for selected data type";
	case -NRF_EOPNOTSUPP:
		return "operation not supported by modem firmware";
	case -NRF_ESHUTDOWN:
		return "modem was shut down";
	case -NRF_EMSGSIZE:
		return "supplied buffer too small";
	default:
		return "unknown err value";
	}
}

static int get_event_data(void **dest, uint8_t type, size_t len)
{
	int err;
	void *data;

	data = k_malloc(len);
	if (data == NULL) {
		return -1;
	}

	err = nrf_modem_gnss_read(data, len, type);
	if (err) {
		printk("Failed to read event data, type: %d, error: %d (%s)\n",
		       type, err, gnss_err_to_str(err));
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
			k_work_submit(&gnss_stop_work);
		}
		break;

	case NRF_MODEM_GNSS_EVT_NMEA:
		err = get_event_data(&event.data,
				     NRF_MODEM_GNSS_DATA_NMEA,
				     sizeof(struct nrf_modem_gnss_nmea_data_frame));
		break;

	case NRF_MODEM_GNSS_EVT_AGNSS_REQ:
		err = get_event_data(&event.data,
				     NRF_MODEM_GNSS_DATA_AGNSS_REQ,
				     sizeof(struct nrf_modem_gnss_agnss_data_frame));
		break;

	default:
		break;
	}

	if (err) {
		/* Failed to get event data */
		return;
	}

	err = k_msgq_put(&gnss_event_msgq, &event, K_NO_WAIT);
	if (err) {
		/* Failed to put event into queue */
		k_free(event.data);
	}
}

#if defined(CONFIG_LWM2M_CARRIER)
static time_t gnss_mktime(struct nrf_modem_gnss_datetime *date)
{
	struct tm tm = {
		.tm_sec = date->seconds,
		.tm_min = date->minute,
		.tm_hour = date->hour,
		.tm_mday = date->day,
		.tm_mon = date->month - 1,
		.tm_year = date->year - 1900
	};

	return mktime(&tm);
}

static void gnss_carrier_location(struct nrf_modem_gnss_pvt_data_frame *pvt)
{
	if (pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
		lwm2m_carrier_location_set(pvt->latitude,
					   pvt->longitude,
					   pvt->altitude,
					   (uint32_t)gnss_mktime(&pvt->datetime),
					   pvt->accuracy);
		lwm2m_carrier_velocity_set(pvt->heading,
					   pvt->speed,
					   pvt->vertical_speed,
					   pvt->speed_accuracy,
					   pvt->vertical_speed_accuracy);
	}
}
#endif

static void print_pvt_flags(struct nrf_modem_gnss_pvt_data_frame *pvt)
{
	mosh_print("");
	mosh_print(
		"Fix valid:          %s",
		pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID ? "true" : "false");
	mosh_print(
		"Leap second valid:  %s",
		pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_LEAP_SECOND_VALID ? "true" : "false");
	mosh_print(
		"Sleep between PVT:  %s",
		pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_SLEEP_BETWEEN_PVT ? "true" : "false");
	mosh_print(
		"Deadline missed:    %s",
		pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED ? "true" : "false");
	mosh_print(
		"Insuf. time window: %s",
		pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME ? "true" : "false");
	mosh_print(
		"Velocity valid:     %s",
		pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_VELOCITY_VALID ? "true" : "false");
	mosh_print(
		"Scheduled download: %s",
		pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_SCHED_DOWNLOAD ? "true" : "false");
}

static void print_pvt(struct nrf_modem_gnss_pvt_data_frame *pvt)
{
	if (pvt_output_level == 0) {
		return;
	}

	print_pvt_flags(pvt);

	mosh_print("Execution time:     %u ms", pvt->execution_time);

	if (pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
		mosh_print(
			"Time:              %02d.%02d.%04d %02d:%02d:%02d.%03d",
			pvt->datetime.day,
			pvt->datetime.month,
			pvt->datetime.year,
			pvt->datetime.hour,
			pvt->datetime.minute,
			pvt->datetime.seconds,
			pvt->datetime.ms);

		mosh_print("Latitude:          %f", pvt->latitude);
		mosh_print("Longitude:         %f", pvt->longitude);
		mosh_print("Accuracy:          %.1f m", (double)pvt->accuracy);
		mosh_print("Altitude:          %.1f m", (double)pvt->altitude);
		mosh_print("Altitude accuracy: %.1f m", (double)pvt->altitude_accuracy);
		mosh_print("Speed:             %.1f m/s", (double)pvt->speed);
		mosh_print("Speed accuracy:    %.1f m/s", (double)pvt->speed_accuracy);
		mosh_print("V. speed:          %.1f m/s", (double)pvt->vertical_speed);
		mosh_print("V. speed accuracy: %.1f m/s", (double)pvt->vertical_speed_accuracy);
		mosh_print("Heading:           %.1f deg", (double)pvt->heading);
		mosh_print("Heading accuracy:  %.1f deg", (double)pvt->heading_accuracy);
		mosh_print("PDOP:              %.1f", (double)pvt->pdop);
		mosh_print("HDOP:              %.1f", (double)pvt->hdop);
		mosh_print("VDOP:              %.1f", (double)pvt->vdop);
		mosh_print("TDOP:              %.1f", (double)pvt->tdop);

		mosh_print(
			"Google maps URL:   https://maps.google.com/?q=%f,%f",
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
			pvt->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX ? 1 : 0,
			pvt->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY ? 1 : 0);
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

static void data_handler_thread_fn(void)
{
	struct event_item event;
	struct nrf_modem_gnss_agnss_data_frame *agnss_data_frame;

	while (true) {
		k_msgq_get(&gnss_event_msgq, &event, K_FOREVER);

		switch (event.id) {
		case NRF_MODEM_GNSS_EVT_PVT:
			print_pvt((struct nrf_modem_gnss_pvt_data_frame *)event.data);
#if defined(CONFIG_LWM2M_CARRIER)
			gnss_carrier_location((struct nrf_modem_gnss_pvt_data_frame *)event.data);
#endif
			break;

		case NRF_MODEM_GNSS_EVT_FIX:
			if (event_output_level > 0) {
				mosh_print("GNSS: Got fix");
			}
			break;

		case NRF_MODEM_GNSS_EVT_NMEA:
			print_nmea((struct nrf_modem_gnss_nmea_data_frame *)event.data);
			break;

		case NRF_MODEM_GNSS_EVT_AGNSS_REQ:
			agnss_data_frame = (struct nrf_modem_gnss_agnss_data_frame *)event.data;

			/* GPS data need is always expected to be present and first in list. */
			__ASSERT(agnss_data_frame->system_count > 0,
				 "GNSS system data need not found");
			__ASSERT(agnss_data_frame->system[0].system_id == NRF_MODEM_GNSS_SYSTEM_GPS,
				 "GPS data need not found");

			if (event_output_level > 0) {
				char flags_string[48];

				agnss_data_flags_str_get(
					flags_string,
					agnss_data_frame->data_flags);

				mosh_print("GNSS: A-GNSS data needed: Flags: %s", flags_string);
				for (int i = 0; i < agnss_data_frame->system_count; i++) {
					mosh_print(
						"GNSS: A-GNSS data needed: "
						"%s ephe: 0x%llx, alm: 0x%llx",
						gnss_system_str_get(
							agnss_data_frame->system[i].system_id),
						agnss_data_frame->system[i].sv_mask_ephe,
						agnss_data_frame->system[i].sv_mask_alm);
				}
			}

			memcpy(&agnss_need, event.data, sizeof(agnss_need));

#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_SUPL_CLIENT_LIB)
			if (agnss_automatic) {
				k_work_submit_to_queue(&mosh_common_work_q, &get_agnss_data_work);
			}
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
			if (pgps_enabled && agnss_data_frame->system[0].sv_mask_ephe != 0x0) {
				k_work_submit_to_queue(&mosh_common_work_q,
						       &notify_pgps_prediction_work);
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

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS) || \
	defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
static void assistance_result_cb(uint16_t object_id, int32_t result_code)
{
	if (object_id != GNSS_ASSIST_OBJECT_ID) {
		return;
	}

	switch (result_code) {
	case LOCATION_ASSIST_RESULT_CODE_OK:
		break;

	case LOCATION_ASSIST_RESULT_CODE_PERMANENT_ERR:
	case LOCATION_ASSIST_RESULT_CODE_TEMP_ERR:
	case LOCATION_ASSIST_RESULT_CODE_NO_RESP_ERR:
	default:
		mosh_error("GNSS: Getting assistance data using LwM2M failed, result: %d",
			   result_code);
		break;
	}
}
#endif

#if defined(CONFIG_NRF_CLOUD_AGNSS) && !defined(CONFIG_NRF_CLOUD_MQTT) && \
	!defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
static int serving_cell_info_get(struct lte_lc_cell *serving_cell)
{
	int err;
	char resp_buf[MODEM_INFO_MAX_RESPONSE_SIZE];

	err = modem_info_string_get(MODEM_INFO_CELLID,
				    resp_buf,
				    MODEM_INFO_MAX_RESPONSE_SIZE);
	if (err < 0) {
		return err;
	}

	serving_cell->id = strtol(resp_buf, NULL, 16);

	err = modem_info_string_get(MODEM_INFO_AREA_CODE,
				    resp_buf,
				    MODEM_INFO_MAX_RESPONSE_SIZE);
	if (err < 0) {
		return err;
	}

	serving_cell->tac = strtol(resp_buf, NULL, 16);

	err = modem_info_short_get(MODEM_INFO_RSRP, &serving_cell->rsrp);
	if (err < 0) {
		return err;
	}

	if (serving_cell->rsrp == 255) {
		/* No valid RSRP, modem is probably in PSM and the cell information might not be
		 * valid anymore. %NCELLMEAS could be used to wake up the modem and perform a cell
		 * search instead, but this is good enough for modem_shell.
		 */
		serving_cell->rsrp = 0; /* -140 dBm */
	}

	/* Request for MODEM_INFO_MNC returns both MNC and MCC in the same string. */
	err = modem_info_string_get(MODEM_INFO_OPERATOR,
				    resp_buf,
				    MODEM_INFO_MAX_RESPONSE_SIZE);
	if (err < 0) {
		return err;
	}

	serving_cell->mnc = strtol(&resp_buf[3], NULL, 10);
	/* Null-terminate MCC, read and store it. */
	resp_buf[3] = '\0';
	serving_cell->mcc = strtol(resp_buf, NULL, 10);

	return 0;
}
#endif

#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_SUPL_CLIENT_LIB)
static void get_filtered_agnss_request(struct nrf_modem_gnss_agnss_data_frame *agnss_request)
{
	memset(agnss_request, 0, sizeof(*agnss_request));

	agnss_request->system_count = agnss_need.system_count;

	for (int i = 0; i < agnss_need.system_count; i++) {
		const struct nrf_modem_gnss_agnss_system_data_need *src = &agnss_need.system[i];
		struct nrf_modem_gnss_agnss_system_data_need *dst = &agnss_request->system[i];

		dst->system_id = src->system_id;

		if (agnss_inject_ephe) {
			dst->sv_mask_ephe = src->sv_mask_ephe;
		}
		if (agnss_inject_alm) {
			dst->sv_mask_alm = src->sv_mask_alm;
		}
	}

	if (agnss_inject_utc) {
		agnss_request->data_flags |=
			agnss_need.data_flags & NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST;
	}
	if (agnss_inject_klob) {
		agnss_request->data_flags |=
			agnss_need.data_flags & NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_REQUEST;
	}
	if (agnss_inject_neq) {
		agnss_request->data_flags |=
			agnss_need.data_flags & NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST;
	}
	if (agnss_inject_time) {
		agnss_request->data_flags |=
			agnss_need.data_flags &
			NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST;
	}
	if (agnss_inject_pos) {
		agnss_request->data_flags |=
			agnss_need.data_flags & NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST;
	}
	if (agnss_inject_int) {
		agnss_request->data_flags |=
			agnss_need.data_flags & NRF_MODEM_GNSS_AGNSS_INTEGRITY_REQUEST;
	}
}

static void get_agnss_data(struct k_work *item)
{
	ARG_UNUSED(item);

	int err;
	char flags_string[48];
	struct nrf_modem_gnss_agnss_data_frame agnss_request;

	get_filtered_agnss_request(&agnss_request);

	agnss_data_flags_str_get(flags_string, agnss_request.data_flags);

	mosh_print(
		"GNSS: Getting A-GNSS data from %s...",
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
		"LwM2M"
#elif defined(CONFIG_NRF_CLOUD_AGNSS)
		"nRF Cloud"
#else
		"SUPL"
#endif
		);
	mosh_print("GNSS: A-GNSS request: flags: %s", flags_string);
	for (int i = 0; i < agnss_request.system_count; i++) {
		mosh_print("GNSS: A-GNSS request: %s sv_mask_ephe: 0x%llx, sv_mask_alm: 0x%llx",
			gnss_system_str_get(agnss_request.system[i].system_id),
			agnss_request.system[i].sv_mask_ephe,
			agnss_request.system[i].sv_mask_alm);
	}

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS)
	if (!cloud_lwm2m_is_connected()) {
		mosh_error("GNSS: LwM2M not connected, can't request A-GNSS data");
		return;
	}
	location_assistance_set_result_code_cb(assistance_result_cb);
	location_assistance_agnss_set_mask(&agnss_request);
	err = location_assistance_agnss_request_send(cloud_lwm2m_client_ctx_get());
	if (err) {
		mosh_error("GNSS: Failed to request A-GNSS data, error: %d", err);
	}
#elif defined(CONFIG_NRF_CLOUD_AGNSS)
#if defined(CONFIG_NRF_CLOUD_MQTT)
	err = nrf_cloud_agnss_request(&agnss_request);
	if (err == -EACCES) {
		mosh_error("GNSS: Not connected to nRF Cloud");
	} else if (err) {
		mosh_error("GNSS: Failed to request A-GNSS data, error: %d", err);
	}
#else /* REST and CoAP */
#if defined(CONFIG_NRF_CLOUD_REST)
	err = nrf_cloud_jwt_generate(0, jwt_buf, sizeof(jwt_buf));
	if (err) {
		mosh_error("GNSS: Failed to generate JWT, error: %d", err);
		return;
	}

	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = false,
		.timeout_ms = NRF_CLOUD_REST_TIMEOUT_NONE,
		.auth = jwt_buf,
		.rx_buf = rx_buf,
		.rx_buf_len = sizeof(rx_buf),
	};
#endif

	struct nrf_cloud_rest_agnss_request request = {
		.type = NRF_CLOUD_REST_AGNSS_REQ_CUSTOM,
		.agnss_req = &agnss_request,
	};

	struct nrf_cloud_rest_agnss_result result = {
		.buf = agnss_data_buf,
		.buf_sz = sizeof(agnss_data_buf),
	};

	struct lte_lc_cells_info net_info = { 0 };

#if defined(CONFIG_NRF_CLOUD_AGNSS_FILTERED_RUNTIME)
	if (gnss_filtered_ephemerides_enabled) {
		request.mask_angle = gnss_elevation;
		mosh_print("GNSS: Requesting filtered ephemeris; elevation = %u", gnss_elevation);
	}
	request.filtered = gnss_filtered_ephemerides_enabled;
#endif /* CONFIG_NRF_CLOUD_AGNSS_FILTERED_RUNTIME */

	err = serving_cell_info_get(&net_info.current_cell);
	if (err) {
		mosh_warn("GNSS: Could not get cell info, error: %d", err);
	} else {
		/* Network info for the location request. */
		request.net_info = &net_info;
	}

#if defined(CONFIG_NRF_CLOUD_REST)
	err = nrf_cloud_rest_agnss_data_get(&rest_ctx, &request, &result);
#elif defined(CONFIG_NRF_CLOUD_COAP)
	err = nrf_cloud_coap_agnss_data_get(&request, &result);
#endif
	if (err) {
		mosh_error("GNSS: Failed to get A-GNSS data, error: %d", err);
		return;
	}

	mosh_print("GNSS: Processing A-GNSS data");

	err = nrf_cloud_agnss_process(result.buf, result.agnss_sz);
	if (err) {
		mosh_error("GNSS: Failed to process A-GNSS data, error: %d", err);
		return;
	}

	mosh_print("GNSS: A-GNSS data injected to the modem");
#endif
#else /* CONFIG_SUPL_CLIENT_LIB */
	if (open_supl_socket() == 0) {
		err = supl_session((void *)&agnss_request);
		if (err) {
			mosh_error("GNSS: Failed to get A-GNSS data, error: %d", err);
		}
		close_supl_socket();
	}
#endif
}
#endif /* CONFIG_NRF_CLOUD_AGNSS || CONFIG_SUPL_CLIENT_LIB */

#if defined(CONFIG_SUPL_CLIENT_LIB)
static void get_agnss_data_type_string(char *type_string, uint16_t type)
{
	switch (type) {
	case NRF_MODEM_GNSS_AGNSS_GPS_UTC_PARAMETERS:
		(void)strcpy(type_string, "utc");
		break;
	case NRF_MODEM_GNSS_AGNSS_GPS_EPHEMERIDES:
		(void)strcpy(type_string, "ephe");
		break;
	case NRF_MODEM_GNSS_AGNSS_GPS_ALMANAC:
		(void)strcpy(type_string, "alm");
		break;
	case NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_IONOSPHERIC_CORRECTION:
		(void)strcpy(type_string, "klob");
		break;
	case NRF_MODEM_GNSS_AGNSS_NEQUICK_IONOSPHERIC_CORRECTION:
		(void)strcpy(type_string, "neq");
		break;
	case NRF_MODEM_GNSS_AGNSS_GPS_SYSTEM_CLOCK_AND_TOWS:
		(void)strcpy(type_string, "time");
		break;
	case NRF_MODEM_GNSS_AGNSS_LOCATION:
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

static int inject_agnss_data(void *agnss,
			     size_t agnss_size,
			     uint16_t type,
			     void *user_data)
{
	ARG_UNUSED(user_data);

	int err;
	char type_string[16];

	err = nrf_modem_gnss_agnss_write(agnss, agnss_size, type);

	get_agnss_data_type_string(type_string, type);

	if (err) {
		mosh_error("GNSS: Failed to inject A-GNSS data, type: %s, error: %d (%s)",
			   type_string, err, gnss_err_to_str(err));
		return err;
	}

	mosh_print("GNSS: Injected A-GNSS data, type: %s, size: %d", type_string, agnss_size);

	return 0;
}
#endif /* CONFIG_SUPL_CLIENT_LIB */

static void start_gnss_work_fn(struct k_work *item)
{
	ARG_UNUSED(item);

	int err;

	err = nrf_modem_gnss_start();

	if (err) {
		mosh_error("GNSS: Failed to start GNSS, error: %d (%s)", err, gnss_err_to_str(err));
	} else {
		if (event_output_level > 0) {
			mosh_print("GNSS: Search started");
		}
	}

	k_work_schedule(&gnss_start_work, K_SECONDS(periodic_fix_interval));

	if (periodic_fix_retry > 0 && periodic_fix_retry < periodic_fix_interval) {
		k_work_schedule(&gnss_timeout_work, K_SECONDS(periodic_fix_retry));
	}
}

static void stop_gnss_work_fn(struct k_work *item)
{
	ARG_UNUSED(item);

	int err;

	err = nrf_modem_gnss_stop();
	if (err) {
		mosh_error("GNSS: Failed to stop GNSS, error: %d (%s)", err, gnss_err_to_str(err));
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
		mosh_error("GNSS: Failed to stop GNSS, error: %d (%s)", err, gnss_err_to_str(err));
	}
}

#if defined(CONFIG_NRF_CLOUD_PGPS)
#if !defined(CONFIG_NRF_CLOUD_MQTT)
static void get_pgps_data_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	int err;

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
	mosh_print("GNSS: Getting P-GPS predictions from LwM2M...");

	location_assistance_set_result_code_cb(assistance_result_cb);
	err = location_assist_pgps_set_prediction_count(pgps_request.prediction_count);
	err |= location_assist_pgps_set_prediction_interval(pgps_request.prediction_period_min);
	location_assist_pgps_set_start_gps_day(pgps_request.gps_day);
	err |= location_assist_pgps_set_start_time(pgps_request.gps_time_of_day);
	if (err) {
		mosh_error("GNSS: Failed to set P-GPS request parameters");
		return;
	}

	err = location_assistance_pgps_request_send(cloud_lwm2m_client_ctx_get());
	if (err) {
		mosh_error("GNSS: Failed to request P-GPS data, err: %d", err);
	} else {
		mosh_print("GNSS: P-GPS predictions requested");
	}
#else /* !CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS */
	mosh_print("GNSS: Getting P-GPS predictions from nRF Cloud...");

	struct nrf_cloud_rest_pgps_request request = {
		.pgps_req = &pgps_request
	};

#if defined(CONFIG_NRF_CLOUD_REST)
	err = nrf_cloud_jwt_generate(0, jwt_buf, sizeof(jwt_buf));
	if (err) {
		mosh_error("GNSS: Failed to generate JWT, error: %d", err);
		return;
	}

	struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = false,
		.timeout_ms = NRF_CLOUD_REST_TIMEOUT_NONE,
		.auth = jwt_buf,
		.rx_buf = rx_buf,
		.rx_buf_len = sizeof(rx_buf),
	};

	err = nrf_cloud_rest_pgps_data_get(&rest_ctx, &request);
#elif defined(CONFIG_NRF_CLOUD_COAP)
	struct nrf_cloud_pgps_result file_location = {0};
	static char host[64];
	static char path[128];

	memset(host, 0, sizeof(host));
	memset(path, 0, sizeof(path));
	file_location.host = host;
	file_location.host_sz = sizeof(host);
	file_location.path = path;
	file_location.path_sz = sizeof(path);

	err = nrf_cloud_coap_pgps_url_get(&request, &file_location);
#endif
	if (err) {
		mosh_error("GNSS: Failed to get P-GPS data, error: %d", err);

		nrf_cloud_pgps_request_reset();

		return;
	}

	mosh_print("GNSS: Processing P-GPS response");

#if defined(CONFIG_NRF_CLOUD_REST)
	err = nrf_cloud_pgps_process(rest_ctx.response, rest_ctx.response_len);
#elif defined(CONFIG_NRF_CLOUD_COAP)
	err = nrf_cloud_pgps_update(&file_location);
#endif
	if (err) {
		mosh_error("GNSS: Failed to process P-GPS response, error: %d", err);

		nrf_cloud_pgps_request_reset();

		return;
	}

	mosh_print("GNSS: P-GPS response processed");

	err = nrf_cloud_pgps_notify_prediction();
	if (err) {
		mosh_error("GNSS: Failed to request current prediction, error: %d", err);

		return;
	}

	mosh_print("GNSS: P-GPS predictions requested");
#endif /* CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS */
}
#endif /* !CONFIG_NRF_CLOUD_MQTT */

static void inject_pgps_data_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	int err;

	mosh_print("GNSS: Injecting ephemerides to modem...");

	err = nrf_cloud_pgps_inject(prediction, &agnss_need);
	if (err) {
		mosh_error("GNSS: Failed to inject ephemerides to modem");
	}

	err = nrf_cloud_pgps_preemptive_updates();
	if (err) {
		mosh_error("GNSS: Failed to request preduction updates");
	}
}

static void notify_pgps_prediction_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	int err;

	err = nrf_cloud_pgps_notify_prediction();
	if (err) {
		mosh_error("GNSS: Failed to notify P-GPS predictions");
	}
}

static void pgps_event_handler(struct nrf_cloud_pgps_event *event)
{
	switch (event->type) {
	case PGPS_EVT_READY:
	case PGPS_EVT_AVAILABLE:
		if (event->prediction != NULL) {
			prediction = event->prediction;

			k_work_submit_to_queue(&mosh_common_work_q, &inject_pgps_data_work);
		}
		break;

#if !defined(CONFIG_NRF_CLOUD_MQTT)
	case PGPS_EVT_REQUEST:
		memcpy(&pgps_request, event->request, sizeof(pgps_request));

		k_work_submit_to_queue(&mosh_common_work_q, &get_pgps_data_work);
		break;
#endif

	default:
		/* No action needed */
		break;
	}
}
#endif /* CONFIG_NRF_CLOUD_PGPS */

static void gnss_api_init(void)
{
	int err;
	static bool gnss_api_initialized;

	/* Reset event handler in case some other handler was set in the meantime. */
	(void)nrf_modem_gnss_event_handler_set(gnss_event_handler);

	if (gnss_api_initialized) {
		return;
	}

	/* Make QZSS satellites visible in the NMEA output. */
	err = nrf_modem_gnss_qzss_nmea_mode_set(NRF_MODEM_GNSS_QZSS_NMEA_MODE_CUSTOM);
	if (err) {
		mosh_warn("GNSS: Failed to enable custom QZSS NMEA mode, error: %d (%s)",
			  err, gnss_err_to_str(err));
	}

	gnss_api_initialized = true;

	k_work_init(&gnss_stop_work, stop_gnss_work_fn);
	k_work_init_delayable(&gnss_start_work, start_gnss_work_fn);
	k_work_init_delayable(&gnss_timeout_work, handle_timeout_work_fn);

#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_SUPL_CLIENT_LIB)
	k_work_init(&get_agnss_data_work, get_agnss_data);
#endif /* CONFIG_NRF_CLOUD_AGNSS || CONFIG_SUPL_CLIENT_LIB */

#if defined(CONFIG_SUPL_CLIENT_LIB)
	static struct supl_api supl_api = {
		.read = supl_read,
		.write = supl_write,
		.handler = inject_agnss_data,
		.logger = supl_logger,
		.counter_ms = k_uptime_get
	};

	(void)supl_init(&supl_api);
#endif /* CONFIG_SUPL_CLIENT_LIB */
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
		k_work_schedule(&gnss_start_work, K_NO_WAIT);
		return 0;
	}

	err = nrf_modem_gnss_start();
	if (err) {
		mosh_error("GNSS: Failed to start GNSS, error: %d (%s)", err, gnss_err_to_str(err));
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
		mosh_error("GNSS: Failed to stop GNSS, error: %d (%s)", err, gnss_err_to_str(err));
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

	case GNSS_DATA_DELETE_EKF:
		delete_mask = NRF_MODEM_GNSS_DELETE_EKF;
		break;

	case GNSS_DATA_DELETE_ALL:
		/* Delete everything else but TCXO frequency offset data. Deleting EKF state is not
		 * supported by older MFWs and returns an error. Because of this EKF state is
		 * deleted separately and the return value is ignored.
		 */
		(void)nrf_modem_gnss_nv_data_delete(NRF_MODEM_GNSS_DELETE_EKF);

		delete_mask = NRF_MODEM_GNSS_DELETE_EPHEMERIDES |
			      NRF_MODEM_GNSS_DELETE_ALMANACS |
			      NRF_MODEM_GNSS_DELETE_IONO_CORRECTION_DATA |
			      NRF_MODEM_GNSS_DELETE_LAST_GOOD_FIX |
			      NRF_MODEM_GNSS_DELETE_GPS_TOW |
			      NRF_MODEM_GNSS_DELETE_GPS_WEEK |
			      NRF_MODEM_GNSS_DELETE_UTC_DATA |
			      NRF_MODEM_GNSS_DELETE_GPS_TOW_PRECISION;
		break;

	case GNSS_DATA_DELETE_TCXO:
		delete_mask = NRF_MODEM_GNSS_DELETE_TCXO_OFFSET;
		break;

	default:
		mosh_error("GNSS: Invalid erase data value");
		return -1;
	}

	err = nrf_modem_gnss_nv_data_delete(delete_mask);
	if (err) {
		mosh_error("GNSS: Failed to delete NV data, error: %d (%s)",
			   err, gnss_err_to_str(err));
	}

	return err;
}

int gnss_delete_data_custom(uint32_t mask)
{
	int err;

	err = nrf_modem_gnss_nv_data_delete(mask);
	if (err) {
		mosh_error("GNSS: Failed to delete NV data, error: %d (%s)",
			   err, gnss_err_to_str(err));
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
		mosh_error("GNSS: Failed to set fix interval, error: %d (%s)",
			   err, gnss_err_to_str(err));
		return err;
	}

	err = nrf_modem_gnss_fix_retry_set(0);
	if (err) {
		mosh_error("GNSS: Failed to set fix retry, error: %d (%s)",
			   err, gnss_err_to_str(err));
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
		mosh_error("GNSS: Failed to set fix interval, error: %d (%s)",
			   err, gnss_err_to_str(err));
		return err;
	}

	err = nrf_modem_gnss_fix_retry_set(fix_retry);
	if (err) {
		mosh_error("GNSS: Failed to set fix retry, error: %d (%s)",
			   err, gnss_err_to_str(err));
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
		mosh_error("GNSS: Failed to set fix interval, error: %d (%s)",
			   err, gnss_err_to_str(err));
		return err;
	}

	err = nrf_modem_gnss_fix_retry_set(0);
	if (err) {
		mosh_error("GNSS: Failed to set fix retry, error: %d (%s)",
			   err, gnss_err_to_str(err));
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
		mosh_error("GNSS: Failed to set fix interval, error: %d (%s)",
			   err, gnss_err_to_str(err));
		return err;
	}

	err = nrf_modem_gnss_fix_retry_set(fix_retry);
	if (err) {
		mosh_error("GNSS: Failed to set fix retry, error: %d (%s)",
			   err, gnss_err_to_str(err));
	}

	return err;
}

int gnss_set_system_mask(uint8_t system_mask)
{
	int err;

	err = nrf_modem_gnss_signal_mask_set(system_mask);
	if (err) {
		mosh_error("GNSS: Failed to set system mask, error: %d (%s)",
			   err, gnss_err_to_str(err));
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
		mosh_error("GNSS: Invalid duty cycling policy value %d", policy);
		return -1;
	}

	err = nrf_modem_gnss_power_mode_set(power_mode);
	if (err) {
		mosh_error("GNSS: Failed to set duty cycling policy, error: %d (%s)",
			   err, gnss_err_to_str(err));
	}

	return err;
}

int gnss_set_elevation_threshold(uint8_t elevation)
{
	int err;

	gnss_api_init();

	err = nrf_modem_gnss_elevation_threshold_set(elevation);
	if (err) {
		mosh_error("GNSS: Failed to set elevation threshold, error: %d (%s)",
			   err, gnss_err_to_str(err));
	} else {
		gnss_elevation = elevation;
	}

	return err;
}

int gnss_set_agnss_filtered_ephemerides(bool enable)
{
#if defined(CONFIG_NRF_CLOUD_AGNSS_FILTERED_RUNTIME)
	gnss_filtered_ephemerides_enabled = enable;
	return 0;
#else
	mosh_error("GNSS: A-GNSS filtered ephemerides are only supported with nRF Cloud REST-only "
		   "configuration.");
	return -EOPNOTSUPP;
#endif
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
		mosh_error("GNSS: Failed to set use case, error: %d (%s)",
			   err, gnss_err_to_str(err));
	}

	return err;
}

int gnss_set_nmea_mask(uint16_t nmea_mask)
{
	int err;

	gnss_api_init();

	err = nrf_modem_gnss_nmea_mask_set(nmea_mask);
	if (err) {
		mosh_error("GNSS: Failed to set NMEA mask, error: %d (%s)",
			   err, gnss_err_to_str(err));
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
		mosh_error("GNSS: Failed to set priority time windows, error: %d (%s)",
			   err, gnss_err_to_str(err));
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
		mosh_error("GNSS: Failed to change dynamics mode, error: %d (%s)",
			   err, gnss_err_to_str(err));
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
		mosh_error("GNSS: Failed to set QZSS NMEA mode, error: %d (%s)",
			   err, gnss_err_to_str(err));
	}

	return err;
}

int gnss_set_qzss_mask(uint16_t mask)
{
	int err;

	gnss_api_init();

	err = nrf_modem_gnss_qzss_prn_mask_set(mask);
	if (err) {
		mosh_error("GNSS: Failed to set QZSS PRN mask, error: %d (%s)",
			   err, gnss_err_to_str(err));
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
		mosh_error("GNSS: Failed to set 1PPS mode, error: %d (%s)",
			   err, gnss_err_to_str(err));
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
		mosh_error("GNSS: Failed to set timing source, error: %d (%s)",
			   err, gnss_err_to_str(err));
	}

	return err;
}

int gnss_set_agnss_data_enabled(bool ephe, bool alm, bool utc, bool klob,
				bool neq, bool time, bool pos, bool integrity)
{
#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_SUPL_CLIENT_LIB)
	agnss_inject_ephe = ephe;
	agnss_inject_alm = alm;
	agnss_inject_utc = utc;
	agnss_inject_klob = klob;
	agnss_inject_neq = neq;
	agnss_inject_time = time;
	agnss_inject_pos = pos;
	agnss_inject_int = integrity;

	return 0;
#else
	mosh_error("GNSS: Enable CONFIG_NRF_CLOUD_AGNSS or CONFIG_SUPL_CLIENT_LIB for "
		   "A-GNSS support");
	return -EOPNOTSUPP;
#endif
}

int gnss_set_agnss_automatic(bool value)
{
#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_SUPL_CLIENT_LIB)
	agnss_automatic = value;

	return 0;
#else
	mosh_error("GNSS: Enable CONFIG_NRF_CLOUD_AGNSS or CONFIG_SUPL_CLIENT_LIB for "
		   "A-GNSS support");
	return -EOPNOTSUPP;
#endif
}

#if defined(CONFIG_NRF_CLOUD_AGNSS)
static bool qzss_assistance_is_supported(void)
{
	char resp[32];

	if (nrf_modem_at_cmd(resp, sizeof(resp), "AT+CGMM") == 0) {
		/* nRF9160 does not support QZSS assistance, while nRF91x1 do. */
		if (strstr(resp, "nRF9160") != NULL) {
			return false;
		}
	}

	return true;
}
#endif /* CONFIG_NRF_CLOUD_AGNSS */

int gnss_inject_agnss_data(void)
{
#if defined(CONFIG_NRF_CLOUD_AGNSS) || defined(CONFIG_SUPL_CLIENT_LIB)
	gnss_api_init();

	/* Pretend modem requested all A-GNSS data */
	agnss_need.data_flags =
		NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST |
		NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_REQUEST |
		NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST |
		NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST |
		NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST |
		NRF_MODEM_GNSS_AGNSS_INTEGRITY_REQUEST;
	agnss_need.system_count = 1;
	agnss_need.system[0].system_id = NRF_MODEM_GNSS_SYSTEM_GPS;
	agnss_need.system[0].sv_mask_ephe = 0xffffffff;
	agnss_need.system[0].sv_mask_alm = 0xffffffff;
#if defined(CONFIG_NRF_CLOUD_AGNSS)
	if (qzss_assistance_is_supported()) {
		agnss_need.system_count = 2;
		agnss_need.system[1].system_id = NRF_MODEM_GNSS_SYSTEM_QZSS;
		agnss_need.system[1].sv_mask_ephe = 0x3ff;
		agnss_need.system[1].sv_mask_alm = 0x3ff;
	}
#endif

	k_work_submit_to_queue(&mosh_common_work_q, &get_agnss_data_work);

	return 0;
#else
	mosh_error("GNSS: Enable CONFIG_NRF_CLOUD_AGNSS or CONFIG_SUPL_CLIENT_LIB for "
		   "A-GNSS support");
	return -EOPNOTSUPP;
#endif
}

int gnss_enable_pgps(void)
{
#if defined(CONFIG_NRF_CLOUD_PGPS)
	int err;

	if (pgps_enabled) {
		mosh_error("GNSS: P-GPS already enabled");
		return -EPERM;
	}

	gnss_api_init();

#if !defined(CONFIG_NRF_CLOUD_MQTT)
	k_work_init(&get_pgps_data_work, get_pgps_data_work_fn);
#endif
	k_work_init(&inject_pgps_data_work, inject_pgps_data_work_fn);
	k_work_init(&notify_pgps_prediction_work, notify_pgps_prediction_work_fn);

	struct nrf_cloud_pgps_init_param pgps_param = {
		.event_handler = pgps_event_handler,
		/* storage is defined by CONFIG_NRF_CLOUD_PGPS_STORAGE */
		.storage_base = 0u,
		.storage_size = 0u
	};

	err = nrf_cloud_pgps_init(&pgps_param);
	if (err == -EACCES) {
		mosh_error("GNSS: Not connected to nRF Cloud");
		return err;
	} else if (err) {
		mosh_error("GNSS: Failed to initialize P-GPS, error: %d", err);
		return err;
	}

	pgps_enabled = true;

	return 0;
#else
	mosh_error("GNSS: Enable CONFIG_NRF_CLOUD_PGPS for P-GPS support");
	return -EOPNOTSUPP;
#endif
}

static void get_expiry_string(char *string, uint32_t string_len, uint16_t expiry)
{
	if (expiry == UINT16_MAX) {
		strncpy(string, "not used", string_len);
	} else if (expiry == 0) {
		strncpy(string, "expired", string_len);
	} else {
		snprintf(string, string_len, "%u", expiry);
	}
}

int gnss_get_agnss_expiry(void)
{
	int err;
	struct nrf_modem_gnss_agnss_expiry agnss_expiry;
	const char *system_string;
	char expiry_string[16];
	char expiry_string2[16];

	err = nrf_modem_gnss_agnss_expiry_get(&agnss_expiry);
	if (err) {
		mosh_error("GNSS: Failed to query A-GNSS data expiry, error: %d (%s)",
			   err, gnss_err_to_str(err));
		return err;
	}

	mosh_print("Data flags: 0x%x", agnss_expiry.data_flags);
	mosh_print("Time valid: %s",
		   agnss_expiry.data_flags & NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST ?
			"false" : "true");
	get_expiry_string(expiry_string, sizeof(expiry_string), agnss_expiry.utc_expiry);
	mosh_print("UTC:        %s", expiry_string);
	get_expiry_string(expiry_string, sizeof(expiry_string), agnss_expiry.klob_expiry);
	mosh_print("Klobuchar:  %s", expiry_string);
	get_expiry_string(expiry_string, sizeof(expiry_string), agnss_expiry.neq_expiry);
	mosh_print("NeQuick:    %s", expiry_string);
	get_expiry_string(expiry_string, sizeof(expiry_string), agnss_expiry.integrity_expiry);
	mosh_print("Integrity:  %s", expiry_string);
	get_expiry_string(expiry_string, sizeof(expiry_string), agnss_expiry.position_expiry);
	mosh_print("Position:   %s", expiry_string);

	for (int i = 0; i < agnss_expiry.sv_count; i++) {
		system_string = gnss_system_str_get(agnss_expiry.sv[i].system_id);
		get_expiry_string(expiry_string, sizeof(expiry_string),
				  agnss_expiry.sv[i].ephe_expiry);
		get_expiry_string(expiry_string2, sizeof(expiry_string2),
				  agnss_expiry.sv[i].alm_expiry);

		mosh_print("ID %3u (%s):    ephe: %-10s alm: %-10s",
			   agnss_expiry.sv[i].sv_id, system_string,
			   expiry_string, expiry_string2);
	}

	return 0;
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
