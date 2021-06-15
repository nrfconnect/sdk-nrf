/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr.h>
#include <init.h>
#include <shell/shell.h>
#include <nrf_socket.h>
#if defined(CONFIG_SUPL_CLIENT_LIB)
#include <supl_session.h>
#include <supl_os_client.h>
#endif

#include "gnss.h"
#if defined(CONFIG_SUPL_CLIENT_LIB)
#include "gnss_supl_support.h"
#endif

#define GNSS_SOCKET_THREAD_STACK_SIZE 768
#define GNSS_SOCKET_THREAD_PRIORITY 5

#define GNSS_WORKQ_THREAD_STACK_SIZE 2048
#define GNSS_WORKQ_THREAD_PRIORITY 5

K_THREAD_STACK_DEFINE(gnss_workq_stack_area, GNSS_WORKQ_THREAD_STACK_SIZE);

static struct k_work_q gnss_work_q;
static struct k_work_delayable gnss_start_work;
static struct k_work_delayable gnss_timeout_work;

#if defined(CONFIG_SUPL_CLIENT_LIB)
static struct k_work get_agps_data_work;

static struct nrf_modem_gnss_agps_data_frame agps_data;
#endif

enum gnss_operation_mode {
	GNSS_OP_MODE_CONTINUOUS,
	GNSS_OP_MODE_SINGLE_FIX,
	GNSS_OP_MODE_PERIODIC_FIX,
	GNSS_OP_MODE_PERIODIC_FIX_GNSS
};

extern const struct shell *shell_global;

static int fd = -1;
static bool gnss_running;
static nrf_gnss_data_frame_t raw_gnss_data;

/* GNSS configuration */
static enum gnss_operation_mode operation_mode = GNSS_OP_MODE_CONTINUOUS;
static uint32_t fix_interval;
static uint32_t fix_retry;
static enum gnss_data_delete data_delete = GNSS_DATA_DELETE_NONE;
static int8_t elevation_threshold = -1;
static bool low_accuracy;
static nrf_gnss_nmea_mask_t nmea_mask =
	NRF_GNSS_NMEA_GGA_MASK | NRF_GNSS_NMEA_GLL_MASK |
	NRF_GNSS_NMEA_GSA_MASK | NRF_GNSS_NMEA_GSV_MASK |
	NRF_GNSS_NMEA_RMC_MASK;
static enum gnss_duty_cycling_policy duty_cycling_policy = GNSS_DUTY_CYCLING_DISABLED;
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

/* Output configuration */
static uint8_t pvt_output_level = 2;
static uint8_t nmea_output_level;
static uint8_t event_output_level;

static K_SEM_DEFINE(gnss_sem, 0, 1);

static void gnss_init(void);
static int stop_gnss(void);

static void create_timestamp_string(uint32_t timestamp, char *timestamp_str)
{
	uint32_t hours;
	uint32_t mins;
	uint32_t secs;

	secs = timestamp / 1000;
	mins = secs / 60;
	hours = mins / 60;
	secs = secs % 60;
	mins = mins % 60;

	sprintf(timestamp_str, "%02d:%02d:%02d.%03d", hours, mins, secs,
		timestamp % 1000);
}

static void print_pvt_flags(nrf_gnss_pvt_data_frame_t *pvt)
{
	shell_print(shell_global, "\nFix valid:          %s",
		    (pvt->flags & NRF_GNSS_PVT_FLAG_FIX_VALID_BIT) ==
		    NRF_GNSS_PVT_FLAG_FIX_VALID_BIT ?
		    "true" :
		    "false");
	shell_print(shell_global, "Leap second valid:  %s",
		    (pvt->flags & NRF_GNSS_PVT_FLAG_LEAP_SECOND_VALID) ==
		    NRF_GNSS_PVT_FLAG_LEAP_SECOND_VALID ?
		    "true" :
		    "false");
	shell_print(shell_global, "Sleep between PVT:  %s",
		    (pvt->flags & NRF_GNSS_PVT_FLAG_SLEEP_BETWEEN_PVT) ==
		    NRF_GNSS_PVT_FLAG_SLEEP_BETWEEN_PVT ?
		    "true" :
		    "false");
	shell_print(shell_global, "Deadline missed:    %s",
		    (pvt->flags & NRF_GNSS_PVT_FLAG_DEADLINE_MISSED) ==
		    NRF_GNSS_PVT_FLAG_DEADLINE_MISSED ?
		    "true" :
		    "false");
	shell_print(shell_global, "Insuf. time window: %s",
		    (pvt->flags & NRF_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME) ==
		    NRF_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME ?
		    "true" :
		    "false");
}

static void print_pvt(nrf_gnss_pvt_data_frame_t *pvt)
{
	if (pvt_output_level == 0) {
		return;
	}

	print_pvt_flags(pvt);

	if ((pvt->flags & NRF_GNSS_PVT_FLAG_FIX_VALID_BIT) ==
	    NRF_GNSS_PVT_FLAG_FIX_VALID_BIT) {
		shell_print(shell_global,
			    "Time:      %02d.%02d.%04d %02d:%02d:%02d.%03d",
			    pvt->datetime.day, pvt->datetime.month,
			    pvt->datetime.year, pvt->datetime.hour,
			    pvt->datetime.minute, pvt->datetime.seconds,
			    pvt->datetime.ms);
		shell_print(shell_global,
			    "Latitude:  %f\n"
			    "Longitude: %f\n"
			    "Altitude:  %.1f m\n"
			    "Accuracy:  %.1f m\n"
			    "Speed:     %.1f m/s\n"
			    "Heading:   %.1f deg\n"
			    "PDOP:      %.1f\n"
			    "HDOP:      %.1f\n"
			    "VDOP:      %.1f\n"
			    "TDOP:      %.1f",
			    pvt->latitude, pvt->longitude, pvt->altitude,
			    pvt->accuracy, pvt->speed, pvt->heading, pvt->pdop,
			    pvt->hdop, pvt->vdop, pvt->tdop);
	}

	if (pvt_output_level < 2) {
		return;
	}

	/* SV data */
	for (int i = 0; i < NRF_GNSS_MAX_SATELLITES; i++) {
		if (pvt->sv[i].sv == 0) {
			/* SV not valid, skip */
			continue;
		}

		shell_print(
			shell_global,
			"SV: %3d C/N0: %4.1f el: %2d az: %3d signal: %d in fix: %d unhealthy: %d",
			pvt->sv[i].sv, pvt->sv[i].cn0 * 0.1,
			pvt->sv[i].elevation, pvt->sv[i].azimuth,
			pvt->sv[i].signal,
			(pvt->sv[i].flags & NRF_GNSS_SV_FLAG_USED_IN_FIX) ==
			NRF_GNSS_SV_FLAG_USED_IN_FIX ?
			1 :
			0,
			(pvt->sv[i].flags & NRF_GNSS_SV_FLAG_UNHEALTHY) ==
			NRF_GNSS_SV_FLAG_UNHEALTHY ?
			1 :
			0);
	}
}

static void print_nmea(nrf_gnss_nmea_data_frame_t *nmea)
{
	if (nmea_output_level == 0) {
		return;
	}

	for (int i = 0; i < NRF_GNSS_NMEA_MAX_LEN; i++) {
		if ((*nmea)[i] == '\r' || (*nmea)[i] == '\n') {
			(*nmea)[i] = '\0';
			break;
		}
	}
	shell_print(shell_global, "%s", nmea);
}

static void get_agps_data_flags_string(char *flags_string, uint32_t data_flags)
{
	size_t len;

	*flags_string = '\0';

	if (data_flags & BIT(NRF_GNSS_AGPS_GPS_UTC_REQUEST)) {
		(void)strcat(flags_string, "utc | ");
	}
	if (data_flags & BIT(NRF_GNSS_AGPS_KLOBUCHAR_REQUEST)) {
		(void)strcat(flags_string, "klob | ");
	}
	if (data_flags & BIT(NRF_GNSS_AGPS_NEQUICK_REQUEST)) {
		(void)strcat(flags_string, "neq | ");
	}
	if (data_flags & BIT(NRF_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST)) {
		(void)strcat(flags_string, "time | ");
	}
	if (data_flags & BIT(NRF_GNSS_AGPS_POSITION_REQUEST)) {
		(void)strcat(flags_string, "pos | ");
	}
	if (data_flags & BIT(NRF_GNSS_AGPS_INTEGRITY_REQUEST)) {
		(void)strcat(flags_string, "int | ");
	}

	len = strlen(flags_string);
	if (len == 0) {
		(void)strcpy(flags_string, "none");
	} else {
		flags_string[len - 3] = '\0';
	}
}

static void process_gnss_data(nrf_gnss_data_frame_t *gnss_data)
{
	static uint32_t fix_timestamp;
	char timestamp_str[16];

	switch (gnss_data->data_id) {
	case NRF_GNSS_PVT_DATA_ID:
		/* In periodic mode GNSS is stopped here if we had a fix on the previous
		 * PVT notification. This is done to get the NMEAs before GNSS is stopped.
		 */
		if (operation_mode == GNSS_OP_MODE_PERIODIC_FIX &&
		    fix_timestamp != 0) {
			if (event_output_level > 0) {
				create_timestamp_string(fix_timestamp,
							timestamp_str);

				shell_print(shell_global,
					    "[%s] GNSS: Fix, going to sleep",
					    timestamp_str);
			}
			fix_timestamp = 0;
			stop_gnss();
			break;
		}

		print_pvt(&gnss_data->pvt);

		/* In periodic fix mode if we have a fix, the fix timestamp is stored,
		 * but GNSS is not stopped yet in order to also get the NMEAs. GNSS is
		 * stopped on the next PVT notification.
		 */
		if (operation_mode == GNSS_OP_MODE_PERIODIC_FIX &&
		    gnss_data->pvt.flags & NRF_GNSS_PVT_FLAG_FIX_VALID_BIT) {
			fix_timestamp = k_uptime_get_32();
			k_work_cancel_delayable(&gnss_timeout_work);
		}
		break;
	case NRF_GNSS_NMEA_DATA_ID:
		print_nmea(&gnss_data->nmea);
		break;
	case NRF_GNSS_AGPS_DATA_ID:
		if (event_output_level > 0) {
			char flags_string[48];

			create_timestamp_string(k_uptime_get_32(),
						timestamp_str);
			get_agps_data_flags_string(flags_string,
						   gnss_data->agps.data_flags);
			shell_print(
				shell_global,
				"[%s] GNSS: AGPS data needed (ephe: 0x%08x, alm: 0x%08x, flags: %s)"
				"",
				timestamp_str, gnss_data->agps.sv_mask_ephe,
				gnss_data->agps.sv_mask_alm, flags_string);
		}
#if defined(CONFIG_SUPL_CLIENT_LIB)
		if (agps_automatic) {
			(void)memset(&agps_data, 0, sizeof(agps_data));
			if (agps_inject_ephe) {
				agps_data.sv_mask_ephe =
					gnss_data->agps.sv_mask_ephe;
			}
			if (agps_inject_alm) {
				agps_data.sv_mask_alm =
					gnss_data->agps.sv_mask_alm;
			}
			if (agps_inject_utc) {
				agps_data.data_flags |=
					gnss_data->agps.data_flags &
					BIT(NRF_GNSS_AGPS_GPS_UTC_REQUEST);
			}
			if (agps_inject_klob) {
				agps_data.data_flags |=
					gnss_data->agps.data_flags &
					BIT(NRF_GNSS_AGPS_KLOBUCHAR_REQUEST);
			}
			if (agps_inject_neq) {
				agps_data.data_flags |=
					gnss_data->agps.data_flags &
					BIT(NRF_GNSS_AGPS_NEQUICK_REQUEST);
			}
			if (agps_inject_time) {
				agps_data.data_flags |=
					gnss_data->agps.data_flags &
					BIT(NRF_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST);
			}
			if (agps_inject_pos) {
				agps_data.data_flags |=
					gnss_data->agps.data_flags &
					BIT(NRF_GNSS_AGPS_POSITION_REQUEST);
			}
			if (agps_inject_int) {
				agps_data.data_flags |=
					gnss_data->agps.data_flags &
					BIT(NRF_GNSS_AGPS_INTEGRITY_REQUEST);
			}
			k_work_submit_to_queue(&gnss_work_q,
					       &get_agps_data_work);
		}
#endif
		break;
	}
}

static void gnss_socket_thread_fn(void)
{
	int len;

	k_sem_take(&gnss_sem, K_FOREVER);

	while (true) {
		while ((len = nrf_recv(fd, &raw_gnss_data,
				       sizeof(nrf_gnss_data_frame_t), 0)) > 0) {
			process_gnss_data(&raw_gnss_data);
		}

		k_sleep(K_MSEC(500));
	}
}

K_THREAD_DEFINE(gnss_socket_thread, GNSS_SOCKET_THREAD_STACK_SIZE,
		gnss_socket_thread_fn, NULL, NULL, NULL,
		K_PRIO_PREEMPT(GNSS_SOCKET_THREAD_PRIORITY), 0, 0);

#if defined(CONFIG_SUPL_CLIENT_LIB)
static void get_agps_data(struct k_work *item)
{
	ARG_UNUSED(item);

	char flags_string[48];

	get_agps_data_flags_string(flags_string, agps_data.data_flags);

	shell_print(
		shell_global,
		"GNSS: Getting AGPS data (ephe: 0x%08x, alm: 0x%08x, flags: %s)...",
		agps_data.sv_mask_ephe, agps_data.sv_mask_alm, flags_string);

	if (open_supl_socket() == 0) {
		supl_session(&agps_data);
		close_supl_socket();
	}
}

static void get_agps_data_type_string(char *type_string,
				      nrf_gnss_agps_data_type_t type)
{
	switch (type) {
	case NRF_GNSS_AGPS_UTC_PARAMETERS:
		(void)strcpy(type_string, "utc");
		break;
	case NRF_GNSS_AGPS_EPHEMERIDES:
		(void)strcpy(type_string, "ephe");
		break;
	case NRF_GNSS_AGPS_ALMANAC:
		(void)strcpy(type_string, "alm");
		break;
	case NRF_GNSS_AGPS_KLOBUCHAR_IONOSPHERIC_CORRECTION:
		(void)strcpy(type_string, "klob");
		break;
	case NRF_GNSS_AGPS_NEQUICK_IONOSPHERIC_CORRECTION:
		(void)strcpy(type_string, "neq");
		break;
	case NRF_GNSS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS:
		(void)strcpy(type_string, "time");
		break;
	case NRF_GNSS_AGPS_LOCATION:
		(void)strcpy(type_string, "pos");
		break;
	case NRF_GNSS_AGPS_INTEGRITY:
		(void)strcpy(type_string, "int");
		break;
	default:
		(void)strcpy(type_string, "UNKNOWN");
		break;
	}
}

static int inject_agps_data(void *agps, size_t agps_size,
			    nrf_gnss_agps_data_type_t type, void *user_data)
{
	ARG_UNUSED(user_data);

	int err;
	char type_string[16];

	err = nrf_sendto(fd, agps, agps_size, 0, &type, sizeof(type));

	get_agps_data_type_string(type_string, type);

	if (err) {
		shell_error(
			shell_global,
			"GNSS: Failed to send AGPS data, type: %s (err: %d)",
			type_string, errno);
		return err;
	}

	shell_print(shell_global,
		    "GNSS: Injected AGPS data, type: %s, size: %d", type_string,
		    agps_size);

	return 0;
}
#endif

void start_gnss(struct k_work *item)
{
	ARG_UNUSED(item);

	int err;
	nrf_gnss_delete_mask_t delete_mask = 0;
	char timestamp_str[16];

	if (!gnss_running) {
		err = nrf_setsockopt(fd, NRF_SOL_GNSS, NRF_SO_GNSS_START,
				     &delete_mask, sizeof(delete_mask));

		if (err) {
			shell_error(shell_global,
				    "GNSS: Failed to start GNSS");
		} else {
			gnss_running = true;

			if (event_output_level > 0) {
				create_timestamp_string(k_uptime_get_32(),
							timestamp_str);

				shell_print(shell_global,
					    "[%s] GNSS: Search started",
					    timestamp_str);
			}
		}
	}

	k_work_schedule_for_queue(&gnss_work_q, &gnss_start_work,
				  K_SECONDS(fix_interval));

	if (fix_retry > 0 && fix_retry < fix_interval) {
		k_work_schedule_for_queue(&gnss_work_q, &gnss_timeout_work,
					  K_SECONDS(fix_retry));
	}
}

void handle_timeout(struct k_work *item)
{
	ARG_UNUSED(item);

	int err;
	char timestamp_str[16];

	if (event_output_level > 0) {
		create_timestamp_string(k_uptime_get_32(), timestamp_str);

		shell_print(shell_global, "[%s] GNSS: Search timeout",
			    timestamp_str);
	}

	err = stop_gnss();
	if (err) {
		shell_error(shell_global, "GNSS: Failed to stop GNSS");
	}
}

int gnss_start(void)
{
	int ret;
	nrf_gnss_fix_interval_t interval;
	nrf_gnss_fix_retry_t retry;
	nrf_gnss_power_save_mode_t ps_mode;
	nrf_gnss_delete_mask_t delete_mask;
	nrf_gnss_elevation_mask_t elevation_mask;
	nrf_gnss_use_case_t use_case;
	char timestamp_str[16];

	gnss_init();

	/* Configure GNSS */
	switch (operation_mode) {
	case GNSS_OP_MODE_CONTINUOUS:
		interval = 1;
		retry = 0;
		switch (duty_cycling_policy) {
		case GNSS_DUTY_CYCLING_DISABLED:
			ps_mode = NRF_GNSS_PSM_DISABLED;
			break;
		case GNSS_DUTY_CYCLING_PERFORMANCE:
			ps_mode = NRF_GNSS_PSM_DUTY_CYCLING_PERFORMANCE;
			break;
		case GNSS_DUTY_CYCLING_POWER:
			ps_mode = NRF_GNSS_PSM_DUTY_CYCLING_POWER;
			break;
		default:
			shell_error(shell_global,
				    "GNSS: Invalid duty cycling policy");
			return -EINVAL;
		}
		break;
	case GNSS_OP_MODE_SINGLE_FIX:
		interval = 0;
		retry = (nrf_gnss_fix_retry_t)fix_retry;
		ps_mode = NRF_GNSS_PSM_DISABLED;
		break;
	case GNSS_OP_MODE_PERIODIC_FIX:
		/* Periodic fix mode controlled by the application, GNSS
		 * is used in continuous tracking mode and it's started and
		 * stopped as needed
		 */
		interval = 1;
		retry = 0;
		ps_mode = NRF_GNSS_PSM_DISABLED;
		break;
	case GNSS_OP_MODE_PERIODIC_FIX_GNSS:
		/* Periodic fix mode controlled by the GNSS */
		interval = (nrf_gnss_fix_interval_t)fix_interval;
		retry = (nrf_gnss_fix_retry_t)fix_retry;
		ps_mode = NRF_GNSS_PSM_DISABLED;
		break;
	default:
		shell_error(shell_global, "GNSS: Invalid operation mode");
		return -EINVAL;
	}

	ret = nrf_setsockopt(fd, NRF_SOL_GNSS, NRF_SO_GNSS_FIX_INTERVAL,
			     &interval, sizeof(interval));
	if (ret != 0) {
		shell_error(shell_global,
			    "GNSS: Failed to set fix interval");
		return -EINVAL;
	}

	ret = nrf_setsockopt(fd, NRF_SOL_GNSS, NRF_SO_GNSS_FIX_RETRY, &retry,
			     sizeof(retry));
	if (ret != 0) {
		shell_error(shell_global, "GNSS: Failed to set fix retry");
		return -EINVAL;
	}

	ret = nrf_setsockopt(fd, NRF_SOL_GNSS, NRF_SO_GNSS_POWER_SAVE_MODE,
			     &ps_mode, sizeof(ps_mode));
	if (ret != 0) {
		shell_error(shell_global,
			    "GNSS: Failed to set power saving mode");
		return -EINVAL;
	}

	/* The lowest bit currently doesn't affect GNSS behavior, so the value
	 * is hard coded.
	 */
	use_case = NRF_GNSS_USE_CASE_MULTIPLE_HOT_START;
	if (low_accuracy) {
		use_case |= NRF_GNSS_USE_CASE_LOW_ACCURACY;
	}
	ret = nrf_setsockopt(fd, NRF_SOL_GNSS, NRF_SO_GNSS_USE_CASE, &use_case,
			     sizeof(use_case));
	if (ret != 0) {
		shell_error(shell_global, "GNSS: Failed to set use case");
		return -EINVAL;
	}

	if (elevation_threshold > -1) {
		elevation_mask = elevation_threshold;
		ret = nrf_setsockopt(fd, NRF_SOL_GNSS,
				     NRF_SO_GNSS_ELEVATION_MASK,
				     &elevation_mask, sizeof(elevation_mask));
		if (ret != 0) {
			shell_error(shell_global,
				    "GNSS: Failed to set elevation mask");
			return -EINVAL;
		}
	}

	ret = nrf_setsockopt(fd, NRF_SOL_GNSS, NRF_SO_GNSS_NMEA_MASK,
			     &nmea_mask, sizeof(nmea_mask));
	if (ret != 0) {
		shell_error(shell_global, "GNSS: Failed to set NMEA mask");
		return -EINVAL;
	}

	/* Start GNSS */
	switch (data_delete) {
	case GNSS_DATA_DELETE_EPHEMERIDES:
		/* Delete ephemerides data */
		delete_mask = 0x01;
		break;
	case GNSS_DATA_DELETE_ALL:
		/* Delete everything else but TCXO frequency offset data */
		delete_mask = 0x7f;
		break;
	default:
		delete_mask = 0x0;
		break;
	}
	ret = nrf_setsockopt(fd, NRF_SOL_GNSS, NRF_SO_GNSS_START, &delete_mask,
			     sizeof(delete_mask));
	if (ret == 0) {
		gnss_running = true;

		if (event_output_level > 0) {
			create_timestamp_string(k_uptime_get_32(),
						timestamp_str);

			shell_print(shell_global,
				    "[%s] GNSS: Search started", timestamp_str);
		}
	} else {
		shell_error(shell_global, "GNSS: Failed to start GNSS");
	}

	if (operation_mode == GNSS_OP_MODE_PERIODIC_FIX) {
		/* Start work is called on purpose although GNSS was already started,
		 * the first start is done here to apply the delete mask, but
		 * everything else is handled by the work.
		 */
		k_work_schedule_for_queue(&gnss_work_q, &gnss_start_work,
					  K_NO_WAIT);
	}

	k_sem_give(&gnss_sem);

	return ret;
}

static int stop_gnss(void)
{
	nrf_gnss_delete_mask_t delete_mask = 0;

	if (!gnss_running) {
		return 0;
	}

	gnss_running = false;

	return nrf_setsockopt(fd, NRF_SOL_GNSS, NRF_SO_GNSS_STOP, &delete_mask,
			      sizeof(delete_mask));
}

int gnss_stop(void)
{
	int ret;
	char timestamp_str[16];

	k_work_cancel_delayable(&gnss_timeout_work);
	k_work_cancel_delayable(&gnss_start_work);

	gnss_init();

	ret = stop_gnss();
	if (ret == 0) {
		if (event_output_level > 0) {
			create_timestamp_string(k_uptime_get_32(),
						timestamp_str);

			shell_print(shell_global,
				    "[%s] GNSS: Search stopped", timestamp_str);
		}
	} else {
		shell_error(shell_global, "GNSS: Failed to stop GNSS");
	}

	return ret;
}

int gnss_set_continuous_mode(void)
{
	operation_mode = GNSS_OP_MODE_CONTINUOUS;

	return 0;
}

int gnss_set_single_fix_mode(uint16_t retry)
{
	operation_mode = GNSS_OP_MODE_SINGLE_FIX;
	fix_retry = retry;

	return 0;
}

int gnss_set_periodic_fix_mode(uint32_t interval, uint16_t retry)
{
	operation_mode = GNSS_OP_MODE_PERIODIC_FIX;
	fix_interval = interval;
	fix_retry = retry;

	return 0;
}

int gnss_set_periodic_fix_mode_gnss(uint16_t interval, uint16_t retry)
{
	if (interval < 10) {
		return -EINVAL;
	}

	operation_mode = GNSS_OP_MODE_PERIODIC_FIX_GNSS;
	fix_interval = interval;
	fix_retry = retry;

	return 0;
}

int gnss_set_duty_cycling_policy(enum gnss_duty_cycling_policy policy)
{
	duty_cycling_policy = policy;

	return 0;
}

int gnss_set_data_delete(enum gnss_data_delete value)
{
	data_delete = value;

	return 0;
}

int gnss_set_elevation_threshold(uint8_t elevation)
{
	if (elevation < 0 || elevation > 90) {
		shell_error(shell_global,
			    "GNSS: Invalid elevation value %d", elevation);
		return -EINVAL;
	}

	elevation_threshold = elevation;

	return 0;
}

int gnss_set_low_accuracy(bool value)
{
	low_accuracy = value;

	return 0;
}

int gnss_set_nmea_mask(uint16_t mask)
{
	if (mask & 0xffe0) {
		/* More than five lowest bits set, invalid bitmask */
		shell_error(shell_global,
			    "GNSS: Invalid NMEA bitmask 0x%x", mask);
		return -EINVAL;
	}

	nmea_mask = mask;

	return 0;
}

int gnss_set_priority_time_windows(bool value)
{
	int err;
	int opt;

	if (value) {
		opt = NRF_SO_GNSS_ENABLE_PRIORITY;
	} else {
		opt = NRF_SO_GNSS_DISABLE_PRIORITY;
	}

	err = nrf_setsockopt(fd, NRF_SOL_GNSS, opt, NULL, 0);
	if (err) {
		shell_error(shell_global,
			    "GNSS: Failed to set priority time windows");
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
	shell_error(shell_global,
		    "GNSS: Enable CONFIG_SUPL_CLIENT_LIB for AGPS support");
	return -EOPNOTSUPP;
#endif
}

int gnss_set_agps_automatic(bool value)
{
#if defined(CONFIG_SUPL_CLIENT_LIB)
	agps_automatic = value;

	return 0;
#else
	shell_error(shell_global,
		    "GNSS: Enable CONFIG_SUPL_CLIENT_LIB for AGPS support");
	return -EOPNOTSUPP;
#endif
}

int gnss_inject_agps_data(void)
{
#if defined(CONFIG_SUPL_CLIENT_LIB)
	gnss_init();

	(void)memset(&agps_data, 0, sizeof(agps_data));
	if (agps_inject_ephe) {
		agps_data.sv_mask_ephe = 0xffffffff;
	}
	if (agps_inject_alm) {
		agps_data.sv_mask_alm = 0xffffffff;
	}
	if (agps_inject_utc) {
		agps_data.data_flags |= BIT(NRF_GNSS_AGPS_GPS_UTC_REQUEST);
	}
	if (agps_inject_klob) {
		agps_data.data_flags |= BIT(NRF_GNSS_AGPS_KLOBUCHAR_REQUEST);
	}
	if (agps_inject_neq) {
		agps_data.data_flags |= BIT(NRF_GNSS_AGPS_NEQUICK_REQUEST);
	}
	if (agps_inject_time) {
		agps_data.data_flags |=
			BIT(NRF_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST);
	}
	if (agps_inject_pos) {
		agps_data.data_flags |= BIT(NRF_GNSS_AGPS_POSITION_REQUEST);
	}
	if (agps_inject_int) {
		agps_data.data_flags |= BIT(NRF_GNSS_AGPS_INTEGRITY_REQUEST);
	}

	k_work_submit_to_queue(&gnss_work_q, &get_agps_data_work);

	return 0;
#else
	shell_error(shell_global,
		    "GNSS: Enable CONFIG_SUPL_CLIENT_LIB for AGPS support");
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

static void gnss_init(void)
{
	if (fd > -1) {
		return;
	}

	k_work_queue_start(&gnss_work_q, gnss_workq_stack_area,
			   K_THREAD_STACK_SIZEOF(gnss_workq_stack_area),
			   GNSS_WORKQ_THREAD_PRIORITY, NULL);

	k_work_init_delayable(&gnss_start_work, start_gnss);
	k_work_init_delayable(&gnss_timeout_work, handle_timeout);

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

	fd = nrf_socket(NRF_AF_LOCAL, NRF_SOCK_DGRAM, NRF_PROTO_GNSS);
}
