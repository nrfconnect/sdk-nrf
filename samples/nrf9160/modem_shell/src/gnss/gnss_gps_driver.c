/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr.h>
#include <shell/shell.h>
#include <drivers/gps.h>
#include <nrf_socket.h>

#include "gnss.h"

extern const struct shell *shell_global;

static const struct device *gps_dev;
static bool gnss_initialized;

/* Default GNSS configuration */
struct gps_config gnss_conf = {
	.nav_mode = GPS_NAV_MODE_CONTINUOUS,
	.power_mode = GPS_POWER_MODE_DISABLED,
	.use_case = GPS_USE_CASE_MULTIPLE_HOT_START,
	.accuracy = GPS_ACCURACY_NORMAL,
	.interval = 0,
	.timeout = 0,
	.delete_agps_data = false,
	.priority = false
};

/* Default output configuration */
uint8_t pvt_output_level = 2;
uint8_t nmea_output_level;
uint8_t event_output_level;

static void print_pvt(const struct gps_pvt *pvt, bool is_fix)
{
	char output_buffer[256];

	if (pvt_output_level == 0) {
		return;
	}

	if (is_fix) {
		shell_print(shell_global, "\nFix valid: true");
		shell_print(shell_global,
			    "Time:      %02d.%02d.%04d %02d:%02d:%02d.%03d",
			    pvt->datetime.day, pvt->datetime.month,
			    pvt->datetime.year, pvt->datetime.hour,
			    pvt->datetime.minute, pvt->datetime.seconds,
			    pvt->datetime.ms);
		sprintf(output_buffer,
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
		/* GDOP is not printed, because it's not supported by nRF91 GPS */
		shell_print(shell_global, "%s", output_buffer);
	} else {
		shell_print(shell_global, "\nFix valid: false");
	}

	if (pvt_output_level < 2) {
		return;
	}

	/* SV data */
	for (int i = 0; i < GPS_PVT_MAX_SV_COUNT; i++) {
		if (pvt->sv[i].sv == 0) {
			/* SV not valid, skip */
			continue;
		}

		sprintf(output_buffer,
			"SV: %3d C/N0: %4.1f el: %2d az: %3d signal: %d in fix: %d unhealthy: %d",
			pvt->sv[i].sv, pvt->sv[i].cn0 * 0.1,
			pvt->sv[i].elevation, pvt->sv[i].azimuth,
			pvt->sv[i].signal, pvt->sv[i].in_fix,
			pvt->sv[i].unhealthy);
		shell_print(shell_global, "%s", output_buffer);
	}
}

static void print_nmea(const struct gps_nmea *nmea)
{
	char print_buf[GPS_NMEA_SENTENCE_MAX_LENGTH + 1];

	if (nmea_output_level == 0) {
		return;
	}

	strncpy(print_buf, nmea->buf, nmea->len);
	print_buf[nmea->len] = '\0';

	/* Remove CRLF */
	for (int i = 0; i < nmea->len; i++) {
		if (print_buf[i] == '\r' || print_buf[i] == '\n') {
			print_buf[i] = '\0';
		}
	}

	shell_print(shell_global, "%s", print_buf);
}

static void gps_event_handler(const struct device *dev, struct gps_event *evt)
{
	switch (evt->type) {
	case GPS_EVT_SEARCH_STARTED:
		if (event_output_level > 0) {
			shell_print(shell_global, "GNSS: Search started");
		}
		break;
	case GPS_EVT_SEARCH_STOPPED:
		if (event_output_level > 0) {
			shell_print(shell_global, "GNSS: Search stopped");
		}
		break;
	case GPS_EVT_SEARCH_TIMEOUT:
		if (event_output_level > 0) {
			shell_print(shell_global, "GNSS: Search timeout");
		}
		break;
	case GPS_EVT_PVT:
		print_pvt(&evt->pvt, false);
		break;
	case GPS_EVT_PVT_FIX:
		print_pvt(&evt->pvt, true);
		break;
	case GPS_EVT_NMEA:
		print_nmea(&evt->nmea);
		break;
	case GPS_EVT_NMEA_FIX:
		print_nmea(&evt->nmea);
		break;
	case GPS_EVT_OPERATION_BLOCKED:
		if (event_output_level > 0) {
			shell_print(shell_global,
				    "GNSS: Operation blocked by LTE");
		}
		break;
	case GPS_EVT_OPERATION_UNBLOCKED:
		if (event_output_level > 0) {
			shell_print(shell_global,
				    "GNSS: Operation unblocked");
		}
		break;
	case GPS_EVT_AGPS_DATA_NEEDED:
		if (event_output_level > 0) {
			shell_print(shell_global,
				    "GNSS: AGPS data needed");
		}
		break;
	case GPS_EVT_ERROR:
		shell_error(shell_global, "GNSS: GPS error: %d",
			    evt->error);
		break;
	default:
		shell_error(shell_global,
			    "GNSS: Unknown GPS event type: %d", evt->type);
		break;
	}
}

static void gnss_init(void)
{
	int err;

	if (gnss_initialized) {
		return;
	}

	gps_dev = device_get_binding(CONFIG_NRF9160_GPS_DEV_NAME);
	if (gps_dev == NULL) {
		shell_error(shell_global, "Could not get %s device",
			    CONFIG_NRF9160_GPS_DEV_NAME);
		return;
	}

	err = gps_init(gps_dev, gps_event_handler);
	if (err) {
		shell_error(shell_global,
			    "Could not initialize GPS, error: %d", err);
		return;
	}

	gnss_initialized = true;
}

int gnss_start(void)
{
	gnss_init();

	return gps_start(gps_dev, &gnss_conf);
}

int gnss_stop(void)
{
	gnss_init();

	return gps_stop(gps_dev);
}

int gnss_delete_data(enum gnss_data_delete data)
{
	gnss_init();

	if (data != GNSS_DATA_DELETE_ALL) {
		shell_error(shell_global,
			    "GNSS: In GPS driver mode only deleting all data is supported");
		return -1;
	}

	gnss_conf.delete_agps_data = true;
	(void)gps_start(gps_dev, &gnss_conf);

	gnss_conf.delete_agps_data = false;
	(void)gps_stop(gps_dev);

	return 0;
}

int gnss_set_continuous_mode(void)
{
	gnss_conf.nav_mode = GPS_NAV_MODE_CONTINUOUS;

	return 0;
}

int gnss_set_single_fix_mode(uint16_t fix_retry)
{
	gnss_conf.nav_mode = GPS_NAV_MODE_SINGLE_FIX;
	gnss_conf.timeout = fix_retry;

	return 0;
}

int gnss_set_periodic_fix_mode(uint32_t fix_interval, uint16_t fix_retry)
{
	shell_error(shell_global,
		    "GNSS: Operation not supported in GPS driver mode");
	return -EOPNOTSUPP;
}

int gnss_set_periodic_fix_mode_gnss(uint16_t fix_interval, uint16_t fix_retry)
{
	if (fix_interval < 10 || fix_interval > 1800) {
		return -EINVAL;
	}

	gnss_conf.nav_mode = GPS_NAV_MODE_PERIODIC;
	gnss_conf.interval = fix_interval;
	gnss_conf.timeout = fix_retry;

	return 0;
}

int gnss_set_system_mask(uint8_t system_mask)
{
	shell_error(shell_global,
		    "GNSS: Operation not supported in GPS driver mode");
	return -EOPNOTSUPP;
}

int gnss_set_duty_cycling_policy(enum gnss_duty_cycling_policy policy)
{
	int err = 0;

	switch (policy) {
	case GNSS_DUTY_CYCLING_DISABLED:
		gnss_conf.power_mode = GPS_POWER_MODE_DISABLED;
		break;
	case GNSS_DUTY_CYCLING_PERFORMANCE:
		gnss_conf.power_mode = GPS_POWER_MODE_PERFORMANCE;
		break;
	case GNSS_DUTY_CYCLING_POWER:
		gnss_conf.power_mode = GPS_POWER_MODE_SAVE;
		break;
	default:
		err = -EINVAL;
		break;
	}

	return err;
}

int gnss_set_elevation_threshold(uint8_t elevation)
{
	shell_error(shell_global,
		    "GNSS: Operation not supported in GPS driver mode");
	return -EOPNOTSUPP;
}

int gnss_set_use_case(bool low_accuracy_enabled, bool scheduled_downloads_disabled)
{
	shell_error(shell_global,
		    "GNSS: Operation not supported in GPS driver mode");
	return -EOPNOTSUPP;
}

int gnss_set_nmea_mask(uint16_t mask)
{
	shell_error(shell_global,
		    "GNSS: Operation not supported in GPS driver mode");
	return -EOPNOTSUPP;
}

int gnss_set_priority_time_windows(bool value)
{
	shell_error(shell_global,
		    "GNSS: Operation not supported in GPS driver mode");
	return -EOPNOTSUPP;
}

int gnss_set_dynamics_mode(enum gnss_dynamics_mode mode)
{
	shell_error(shell_global,
		    "GNSS: Operation not supported in GPS driver mode");
	return -EOPNOTSUPP;
}

int gnss_set_qzss_nmea_mode(enum gnss_qzss_nmea_mode nmea_mode)
{
	shell_error(shell_global,
		    "GNSS: Operation not supported in GPS driver mode");
	return -EOPNOTSUPP;
}

int gnss_set_qzss_mask(uint16_t mask)
{
	shell_error(shell_global,
		    "GNSS: Operation not supported in GPS driver mode");
	return -EOPNOTSUPP;
}

int gnss_set_1pps_mode(const struct gnss_1pps_mode *config)
{
	shell_error(shell_global,
		    "GNSS: Operation not supported in GPS driver mode");
	return -EOPNOTSUPP;
}

int gnss_set_timing_source(enum gnss_timing_source source)
{
	shell_error(shell_global,
		    "GNSS: Operation not supported in GPS driver mode");
	return -EOPNOTSUPP;
}

int gnss_set_agps_data_enabled(bool ephe, bool alm, bool utc, bool klob,
			       bool neq, bool time, bool pos, bool integrity)
{
	shell_error(shell_global,
		    "GNSS: Operation not supported in GPS driver mode");
	return -EOPNOTSUPP;
}

int gnss_set_agps_automatic(bool value)
{
	shell_error(shell_global,
		    "GNSS: Operation not supported in GPS driver mode");
	return -EOPNOTSUPP;
}

int gnss_inject_agps_data(void)
{
	shell_error(shell_global,
		    "GNSS: Operation not supported in GPS driver mode");
	return -EOPNOTSUPP;
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
