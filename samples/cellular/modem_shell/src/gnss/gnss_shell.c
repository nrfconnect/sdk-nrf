/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <net/nrf_cloud_agnss.h>

#include "mosh_print.h"
#include "gnss.h"

#define AGNSS_CMD_LINE_INJECT_MAX_LENGTH MIN(3500, CONFIG_SHELL_CMD_BUFF_SIZE)

static int cmd_gnss_start(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_start() == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_stop(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_stop() == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_delete_ephe(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_delete_data(GNSS_DATA_DELETE_EPHEMERIDES) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_delete_ekf(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_delete_data(GNSS_DATA_DELETE_EKF) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_delete_all(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_delete_data(GNSS_DATA_DELETE_ALL) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_delete_tcxo(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_delete_data(GNSS_DATA_DELETE_TCXO) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_delete_mask(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;
	uint32_t mask;

	mask = shell_strtoul(argv[1], 16, &err);

	if (err) {
		mosh_error("mask: invalid mask value %s", argv[1]);

		return -ENOEXEC;
	}

	return gnss_delete_data_custom(mask) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_mode_cont(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_continuous_mode() == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_mode_single(const struct shell *shell, size_t argc, char **argv)
{
	int timeout;

	timeout = atoi(argv[1]);
	if (timeout < 0 || timeout > UINT16_MAX) {
		mosh_error("single: invalid timeout value %d", timeout);
		return -EINVAL;
	}

	return gnss_set_single_fix_mode(timeout) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_mode_periodic(const struct shell *shell, size_t argc, char **argv)
{
	int interval;
	int timeout;

	interval = atoi(argv[1]);
	if (interval <= 0) {
		mosh_error(
			"periodic: invalid interval value %d, the value must be greater than 0",
			interval);
		return -EINVAL;
	}

	timeout = atoi(argv[2]);
	if (timeout < 0 || timeout > UINT16_MAX) {
		mosh_error(
			"periodic: invalid timeout value %d, the value must be 0...65535",
			timeout);
		return -EINVAL;
	}

	return gnss_set_periodic_fix_mode(interval, timeout) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_mode_periodic_gnss(const struct shell *shell, size_t argc, char **argv)
{
	int interval;
	int timeout;

	interval = atoi(argv[1]);
	if (interval < 10 || interval > UINT16_MAX) {
		mosh_error(
			"periodic_gnss: invalid interval value %d, the value must be 10...65535",
			interval);
		return -EINVAL;
	}

	timeout = atoi(argv[2]);
	if (timeout < 0 || timeout > UINT16_MAX) {
		mosh_error(
			"periodic_gnss: invalid timeout value %d, the value must be 0...65535",
			timeout);
		return -EINVAL;
	}

	return gnss_set_periodic_fix_mode_gnss(interval, timeout) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_dynamics_general(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_dynamics_mode(GNSS_DYNAMICS_MODE_GENERAL) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_dynamics_stationary(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_dynamics_mode(GNSS_DYNAMICS_MODE_STATIONARY) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_dynamics_pedestrian(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_dynamics_mode(GNSS_DYNAMICS_MODE_PEDESTRIAN) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_dynamics_automotive(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_dynamics_mode(GNSS_DYNAMICS_MODE_AUTOMOTIVE) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_config_system(const struct shell *shell, size_t argc, char **argv)
{
	int value;
	uint8_t system_mask;

	system_mask = 0x1; /* GPS bit always enabled */

	/* QZSS */
	value = atoi(argv[1]);
	if (value == 1) {
		system_mask |= 0x4;
	}

	return gnss_set_system_mask(system_mask) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_config_elevation(const struct shell *shell, size_t argc, char **argv)
{
	int elevation;

	if (argc != 2) {
		mosh_error("elevation: wrong parameter count");
		mosh_print("elevation: <angle>");
		mosh_print(
			"angle:\tElevation threshold angle (in degrees, default 5). "
			"Satellites with elevation angle less than the threshold are excluded.");
		return -EINVAL;
	}

	elevation = atoi(argv[1]);

	if (elevation < 0 || elevation > UINT8_MAX) {
		mosh_error("elevation: invalid elevation value %d", elevation);
		return -EINVAL;
	}

	return gnss_set_elevation_threshold(elevation) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_config_use_case(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t value;
	bool low_accuracy_enabled = false;
	bool scheduled_downloads_disabled = false;

	value = atoi(argv[1]);
	if (value == 1) {
		low_accuracy_enabled = true;
	}

	value = atoi(argv[2]);
	if (value == 1) {
		scheduled_downloads_disabled = true;
	}

	return gnss_set_use_case(low_accuracy_enabled, scheduled_downloads_disabled) == 0 ?
		0 : -ENOEXEC;
}

static int cmd_gnss_config_nmea(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t value;
	uint16_t nmea_mask;
	uint16_t nmea_mask_bit;

	nmea_mask = 0;
	nmea_mask_bit = 1;
	for (int i = 0; i < 5; i++) {
		value = atoi(argv[i + 1]);
		if (value == 1) {
			nmea_mask |= nmea_mask_bit;
		}
		nmea_mask_bit = nmea_mask_bit << 1;
	}

	return gnss_set_nmea_mask(nmea_mask) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_config_powersave_off(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_duty_cycling_policy(GNSS_DUTY_CYCLING_DISABLED) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_config_powersave_perf(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_duty_cycling_policy(GNSS_DUTY_CYCLING_PERFORMANCE) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_config_powersave_power(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_duty_cycling_policy(GNSS_DUTY_CYCLING_POWER) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_config_qzss_nmea_standard(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_qzss_nmea_mode(GNSS_QZSS_NMEA_MODE_STANDARD) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_config_qzss_nmea_custom(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_qzss_nmea_mode(GNSS_QZSS_NMEA_MODE_CUSTOM) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_config_qzss_mask(const struct shell *shell, size_t argc, char **argv)
{
	int value;
	uint16_t qzss_mask;
	uint16_t qzss_mask_bit;

	qzss_mask = 0;
	qzss_mask_bit = 1;
	for (int i = 0; i < 10; i++) {
		value = atoi(argv[i + 1]);
		if (value == 1) {
			qzss_mask |= qzss_mask_bit;
		}
		qzss_mask_bit = qzss_mask_bit << 1;
	}

	return gnss_set_qzss_mask(qzss_mask) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_config_timing_rtc(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_timing_source(GNSS_TIMING_SOURCE_RTC) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_config_timing_tcxo(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_timing_source(GNSS_TIMING_SOURCE_TCXO) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_priority_enable(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_priority_time_windows(true) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_priority_disable(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_priority_time_windows(false) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_agnss_automatic_enable(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_agnss_automatic(true) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_agnss_automatic_disable(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_agnss_automatic(false) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_agnss_filtered_enable(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_agnss_filtered_ephemerides(true) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_agnss_filtered_disable(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_set_agnss_filtered_ephemerides(false) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_agnss_inject(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;

	if (argc == 1) {
		/* Fetch assistance data from nRF cloud or from SUPL server and inject */
		if (gnss_inject_agnss_data() != 0) {
			ret = -ENOEXEC;
		}
	} else if (argc == 2) {
		/* Assistance data provided as command line argument */
#if defined(CONFIG_NRF_CLOUD_AGNSS)
		size_t bin_array_length = 0;

		if (strlen(argv[1]) <= AGNSS_CMD_LINE_INJECT_MAX_LENGTH) {
			char *buf = k_malloc(sizeof(char) * AGNSS_CMD_LINE_INJECT_MAX_LENGTH);

			if (buf == NULL) {
				mosh_error("Cannot allocate memory for the assistance data");
				return -ENOEXEC;
			}
			bin_array_length = hex2bin(argv[1],
						   strlen(argv[1]),
						   buf,
						   AGNSS_CMD_LINE_INJECT_MAX_LENGTH);

			if (bin_array_length) {
				mosh_print("Injecting %d bytes", bin_array_length);
				if (nrf_cloud_agnss_process(buf, bin_array_length) != 0) {
					ret = -EINVAL;
				}
			} else {
				mosh_error("Assistance data not in valid hexadecimal format");
				ret = -EINVAL;
			}
			k_free(buf);
		} else {
			mosh_error("Assistance data length %d exceeds the maximum length of %d",
				   strlen(argv[1]),
				   AGNSS_CMD_LINE_INJECT_MAX_LENGTH);
			ret = -EINVAL;
		}
#else
		mosh_error("GNSS: Enable CONFIG_NRF_CLOUD_AGNSS to enable the processing of "
			   "A-GNSS data");
		ret = -ENOEXEC;
#endif
	}
	return ret;
}

static int cmd_gnss_agnss_expiry(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_get_agnss_expiry() == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_agnss_ref_altitude(const struct shell *shell, size_t argc, char **argv)
{
	int altitude = 0;
	struct nrf_modem_gnss_agnss_data_location location = { 0 };

	if (argc != 2) {
		mosh_error("ref_altitude: wrong parameter count");
		mosh_print("ref_altitude: <altitude>");
		mosh_print(
			"altitude:\tReference altitude [m] with regard to the reference ellipsoid "
			"surface");
		return -EINVAL;
	}

	altitude = atoi(argv[1]);

	if (altitude < INT16_MIN || altitude > INT16_MAX) {
		mosh_error("ref_altitude: invalid altitude value %d", altitude);
		return -EINVAL;
	}

	/* Only inject altitude, thus set unc_semimajor and unc_semiminor to 255 which indicates
	 * that latitude and longitude are not to be used. Set confidence to 100 for maximum
	 * confidence.
	 */
	location.unc_semimajor = 255;
	location.unc_semiminor = 255;
	location.confidence = 100;
	location.altitude = altitude;
	/* The altitude uncertainty has to be less than 100 meters (coded number K has to
	 * be less than 48) for the altitude to be used for a 3-sat fix. GNSS increases
	 * the uncertainty depending on the age of the altitude and whether the device is
	 * stationary or moving. The uncertainty is set to 0 (meaning 0 meters), so that
	 * it remains usable for a 3-sat fix for as long as possible.
	 */
	location.unc_altitude = 0;

	return nrf_modem_gnss_agnss_write(
		&location,
		sizeof(location),
		NRF_MODEM_GNSS_AGNSS_LOCATION) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_pgps_enable(const struct shell *shell, size_t argc, char **argv)
{
	return gnss_enable_pgps() == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_agnss_filter(const struct shell *shell, size_t argc, char **argv)
{
	bool ephe_enabled;
	bool alm_enabled;
	bool utc_enabled;
	bool klob_enabled;
	bool neq_enabled;
	bool time_enabled;
	bool pos_enabled;
	bool int_enabled;

	if (argc != 9) {
		mosh_error("filter: wrong parameter count");
		mosh_print("filter: <ephe> <alm> <utc> <klob> <neq> <time> <pos> <integrity>");
		mosh_print("ephe:\n  0 = disabled\n  1 = enabled");
		mosh_print("alm:\n  0 = disabled\n  1 = enabled");
		mosh_print("utc:\n  0 = disabled\n  1 = enabled");
		mosh_print("klob:\n  0 = disabled\n  1 = enabled");
		mosh_print("neq:\n  0 = disabled\n  1 = enabled");
		mosh_print("time:\n  0 = disabled\n  1 = enabled");
		mosh_print("pos:\n  0 = disabled\n  1 = enabled");
		mosh_print("integrity:\n  0 = disabled\n  1 = enabled");
		return -EINVAL;
	}

	ephe_enabled = atoi(argv[1]) == 1 ? true : false;
	alm_enabled = atoi(argv[2]) == 1 ? true : false;
	utc_enabled = atoi(argv[3]) == 1 ? true : false;
	klob_enabled = atoi(argv[4]) == 1 ? true : false;
	neq_enabled = atoi(argv[5]) == 1 ? true : false;
	time_enabled = atoi(argv[6]) == 1 ? true : false;
	pos_enabled = atoi(argv[7]) == 1 ? true : false;
	int_enabled = atoi(argv[8]) == 1 ? true : false;

	return gnss_set_agnss_data_enabled(ephe_enabled, alm_enabled, utc_enabled,
					   klob_enabled, neq_enabled, time_enabled,
					   pos_enabled, int_enabled) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_1pps_enable(const struct shell *shell, size_t argc, char **argv)
{
	int interval;
	int pulse_width;

	interval = atoi(argv[1]);
	if (interval < 0 || interval > UINT16_MAX) {
		mosh_error("start: invalid interval value %d", interval);
		return -EINVAL;
	}

	pulse_width = atoi(argv[2]);
	if (pulse_width < 0 || pulse_width > UINT16_MAX) {
		mosh_error("start: invalid pulse width value %d", pulse_width);
		return -EINVAL;
	}

	struct gnss_1pps_mode mode = {
		.enable = true,
		.pulse_interval = interval,
		.pulse_width = pulse_width,
		.apply_start_time = false
	};

	return gnss_set_1pps_mode(&mode) == 0 ? 0 : -ENOEXEC;
}

/* Parses a date string in format "dd.mm.yyyy" */
static int parse_date_string(char *date_string, uint16_t *year, uint8_t *month, uint8_t *day)
{
	char number[5];
	int value;

	if (strlen(date_string) != 10) {
		return -1;
	}

	/* Day */
	number[0] = *date_string;
	date_string++;
	number[1] = *date_string;
	date_string++;
	number[2] = '\0';
	value = atoi(number);
	if (value >= 1 && value <= 31) {
		*day = value;
	} else {
		return -1;
	}

	if (*date_string != '.') {
		return -1;
	}
	date_string++;

	/* Month */
	number[0] = *date_string;
	date_string++;
	number[1] = *date_string;
	date_string++;
	number[2] = '\0';
	value = atoi(number);
	if (value >= 1 && value <= 12) {
		*month = value;
	} else {
		return -1;
	}

	if (*date_string != '.') {
		return -1;
	}
	date_string++;

	/* Year */
	number[0] = *date_string;
	date_string++;
	number[1] = *date_string;
	date_string++;
	number[2] = *date_string;
	date_string++;
	number[3] = *date_string;
	number[4] = '\0';
	value = atoi(number);
	if (value >= 0 && value <= 4000) {
		*year = value;
	} else {
		return -1;
	}

	return 0;
}

/* Parses a time string in format "hh:mm:ss" */
static int parse_time_string(char *time_string, uint8_t *hour, uint8_t *minute, uint8_t *second)
{
	char number[3];
	int value;

	if (strlen(time_string) != 8) {
		return -1;
	}

	/* Hour */
	number[0] = *time_string;
	time_string++;
	number[1] = *time_string;
	time_string++;
	number[2] = '\0';
	value = atoi(number);
	if (value >= 0 && value <= 23) {
		*hour = value;
	} else {
		return -1;
	}

	if (*time_string != ':') {
		return -1;
	}
	time_string++;

	/* Minute */
	number[0] = *time_string;
	time_string++;
	number[1] = *time_string;
	time_string++;
	number[2] = '\0';
	value = atoi(number);
	if (value >= 0 && value <= 59) {
		*minute = value;
	} else {
		return -1;
	}

	if (*time_string != ':') {
		return -1;
	}
	time_string++;

	/* Second */
	number[0] = *time_string;
	time_string++;
	number[1] = *time_string;
	number[2] = '\0';
	value = atoi(number);
	if (value >= 0 && value <= 59) {
		*second = value;
	} else {
		return -1;
	}

	return 0;
}

static int cmd_gnss_1pps_enable_at(const struct shell *shell, size_t argc, char **argv)
{
	int interval;
	int pulse_width;

	interval = atoi(argv[1]);
	if (interval < 0 || interval > UINT16_MAX) {
		mosh_error("start_at: invalid interval value %d", interval);
		return -EINVAL;
	}

	pulse_width = atoi(argv[2]);
	if (pulse_width < 0 || pulse_width > UINT16_MAX) {
		mosh_error("start_at: invalid pulse width value %d", pulse_width);
		return -EINVAL;
	}

	struct gnss_1pps_mode mode = {
		.enable = true,
		.pulse_interval = interval,
		.pulse_width = pulse_width,
		.apply_start_time = true
	};

	/* Parse date */
	if (parse_date_string(argv[3], &mode.year, &mode.month, &mode.day) != 0) {
		mosh_error("start_at: invalid date string %s", argv[3]);
		return -EINVAL;
	}

	/* Parse time */
	if (parse_time_string(argv[4], &mode.hour, &mode.minute, &mode.second) != 0) {
		mosh_error("start_at: invalid time string %s", argv[4]);
		return -EINVAL;
	}

	return gnss_set_1pps_mode(&mode) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_1pps_disable(const struct shell *shell, size_t argc, char **argv)
{
	struct gnss_1pps_mode mode = {
		.enable = false
	};

	return gnss_set_1pps_mode(&mode) == 0 ? 0 : -ENOEXEC;
}

static int cmd_gnss_output(const struct shell *shell, size_t argc, char **argv)
{
	int err;
	int pvt_level;
	int nmea_level;
	int event_level;

	if (argc != 4) {
		mosh_error("output: wrong parameter count");
		mosh_print("output: <pvt level> <nmea level> <event level>");
		mosh_print("pvt level:\n  0 = no PVT output\n  1 = PVT output");
		mosh_print("  2 = PVT output with SV information (default)");
		mosh_print("nmea level:\n  0 = no NMEA output (default)\n"
				"  1 = NMEA output");
		mosh_print("event level:\n  0 = no event output (default)\n"
				"  1 = event output");
		return -EINVAL;
	}

	pvt_level = atoi(argv[1]);
	nmea_level = atoi(argv[2]);
	event_level = atoi(argv[3]);

	err = gnss_set_pvt_output_level(pvt_level);
	if (err) {
		mosh_error("output: invalid PVT output level");
	}
	err = gnss_set_nmea_output_level(nmea_level);
	if (err) {
		mosh_error("output: invalid NMEA output level");
	}
	err = gnss_set_event_output_level(event_level);
	if (err) {
		mosh_error("output: invalid event output level");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_delete,
	SHELL_CMD_ARG(ephe, NULL, "Delete ephemerides (forces a warm start).\n"
				  "With modem firmware v2.0.1 or later, also the EKF state must "
				  "be deleted separately to force a warm start.",
		      cmd_gnss_delete_ephe, 1, 0),
	SHELL_CMD_ARG(ekf, NULL, "Delete Extended Kalman Filter (EKF) state.\n"
				 "Only supported by modem firmware v2.0.1 or later.",
		      cmd_gnss_delete_ekf, 1, 0),
	SHELL_CMD_ARG(all, NULL, "Delete all data, except the TCXO offset (forces a cold start).",
		      cmd_gnss_delete_all, 1, 0),
	SHELL_CMD_ARG(tcxo, NULL, "Delete the TCXO offset.",
		      cmd_gnss_delete_tcxo, 1, 0),
	SHELL_CMD_ARG(mask, NULL, "<bitmask>\nDelete data using a custom bitmask. The bitmask "
				  "is given in hex, for example \"17f\".",
		      cmd_gnss_delete_mask, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_mode,
	SHELL_CMD_ARG(cont, NULL, "Continuous tracking mode (default).", cmd_gnss_mode_cont, 1, 0),
	SHELL_CMD_ARG(single, NULL, "<timeout>\nSingle fix mode.", cmd_gnss_mode_single, 2, 0),
	SHELL_CMD_ARG(periodic, NULL,
		      "<interval> <timeout>\nPeriodic fix mode controlled by application.",
		      cmd_gnss_mode_periodic, 3, 0),
	SHELL_CMD_ARG(periodic_gnss, NULL,
		      "<interval> <timeout>\nPeriodic fix mode controlled by GNSS.",
		      cmd_gnss_mode_periodic_gnss, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_dynamics,
	SHELL_CMD_ARG(general, NULL, "General purpose.", cmd_gnss_dynamics_general, 1, 0),
	SHELL_CMD_ARG(stationary, NULL, "Optimize for stationary use.",
		      cmd_gnss_dynamics_stationary, 1, 0),
	SHELL_CMD_ARG(pedestrian, NULL, "Optimize for pedestrian use.",
		      cmd_gnss_dynamics_pedestrian, 1, 0),
	SHELL_CMD_ARG(automotive, NULL, "Optimize for automotive use.",
		      cmd_gnss_dynamics_automotive, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_config_powersave,
	SHELL_CMD_ARG(off, NULL, "Power saving off (default).",
		      cmd_gnss_config_powersave_off, 1, 0),
	SHELL_CMD_ARG(perf, NULL, "Power saving without significant performance degradation.",
		      cmd_gnss_config_powersave_perf, 1, 0),
	SHELL_CMD_ARG(power, NULL, "Power saving with acceptable performance degradation.",
		      cmd_gnss_config_powersave_power, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_config_qzss_nmea,
	SHELL_CMD_ARG(standard, NULL, "Standard NMEA reporting.",
		      cmd_gnss_config_qzss_nmea_standard, 1, 0),
	SHELL_CMD_ARG(custom, NULL, "Custom NMEA reporting (default).",
		      cmd_gnss_config_qzss_nmea_custom, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_config_qzss,
	SHELL_CMD(nmea, &sub_gnss_config_qzss_nmea, "QZSS NMEA mode.", mosh_print_help_shell),
	SHELL_CMD_ARG(mask, NULL,
		      "<193> <194> <195> <196> <197> <198> <199> <200> <201> <202>\n"
		      "QZSS NMEA mask for PRNs 193...202. 0 = disabled, 1 = enabled.",
		      cmd_gnss_config_qzss_mask, 11, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_config_timing,
	SHELL_CMD_ARG(rtc, NULL, "RTC (default).", cmd_gnss_config_timing_rtc, 1, 0),
	SHELL_CMD_ARG(tcxo, NULL, "TCXO.", cmd_gnss_config_timing_tcxo, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_config,
	SHELL_CMD_ARG(system, NULL,
		      "<QZSS enabled>\n"
		      "System mask. 0 = disabled, 1 = enabled. GPS L1 C/A is always enabled.",
		      cmd_gnss_config_system, 2, 0),
	SHELL_CMD(elevation, NULL, "<angle>\nElevation threshold angle.",
		  cmd_gnss_config_elevation),
	SHELL_CMD_ARG(use_case, NULL,
		      "<low accuracy allowed> <scheduled downloads disabled>\n"
		      "Use case configuration. 0 = option disabled, 1 = option enabled "
		      "(default all disabled).",
		      cmd_gnss_config_use_case, 3, 0),
	SHELL_CMD_ARG(nmea, NULL,
		      "<GGA enabled> <GLL enabled> <GSA enabled> <GSV enabled> <RMC enabled>\n"
		      "NMEA mask. 0 = disabled, 1 = enabled (default all enabled).",
		      cmd_gnss_config_nmea, 6, 0),
	SHELL_CMD(powersave, &sub_gnss_config_powersave, "Continuous tracking power saving mode.",
		  mosh_print_help_shell),
	SHELL_CMD(qzss, &sub_gnss_config_qzss, "QZSS configuration.", mosh_print_help_shell),
	SHELL_CMD(timing, &sub_gnss_config_timing, "Timing source during sleep.",
		  mosh_print_help_shell),
	SHELL_SUBCMD_SET_END
	);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_priority,
	SHELL_CMD_ARG(enable, NULL, "Enable priority time window requests.",
		      cmd_gnss_priority_enable, 1, 0),
	SHELL_CMD_ARG(disable, NULL, "Disable priority time window requests.",
		      cmd_gnss_priority_disable, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_agnss_automatic,
	SHELL_CMD_ARG(enable, NULL, "Enable automatic fetching of A-GNSS data.",
		      cmd_gnss_agnss_automatic_enable, 1, 0),
	SHELL_CMD_ARG(disable, NULL, "Disable automatic fetching of A-GNSS data (default).",
		      cmd_gnss_agnss_automatic_disable, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_agnss_filtered,
	SHELL_CMD_ARG(enable, NULL, "Enable A-GNSS filtered ephemerides.",
		      cmd_gnss_agnss_filtered_enable, 1, 0),
	SHELL_CMD_ARG(disable, NULL, "Disable A-GNSS filtered ephemerides (default).",
		      cmd_gnss_agnss_filtered_disable, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_agnss,
	SHELL_CMD(automatic, &sub_gnss_agnss_automatic,
		  "Enable/disable automatic fetching of A-GNSS data.", mosh_print_help_shell),
	SHELL_CMD_ARG(inject, NULL, "[assistance_data]\nFetch and inject A-GNSS data to GNSS. "
				    "Assistance data can be provided on command line in "
				    "hexadecimal format without spaces. If <assistance_data> is "
				    "left empty, the data is fetched from either nRF cloud or SUPL "
				    "server.",
		      cmd_gnss_agnss_inject, 1, 1),
	SHELL_CMD(filter, NULL,
		  "<ephe> <alm> <utc> <klob> <neq> <time> <pos> <integrity>\nSet filter for "
		  "allowed A-GNSS data. 0 = disabled, 1 = enabled (default all enabled).",
		  cmd_gnss_agnss_filter),
	SHELL_CMD(filtephem, &sub_gnss_agnss_filtered,
		  "Enable/disable A-GNSS filtered ephemerides.", mosh_print_help_shell),
	SHELL_CMD_ARG(expiry, NULL, "Query A-GNSS data expiry information from GNSS.",
		      cmd_gnss_agnss_expiry, 1, 0),
	SHELL_CMD(ref_altitude, NULL, "Reference altitude for 3-sat fix in meters with regard to "
				      "the reference ellipsoid surface.",
		  cmd_gnss_agnss_ref_altitude),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_pgps,
	SHELL_CMD_ARG(enable, NULL, "Enable P-GPS.",
		      cmd_gnss_pgps_enable, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss_1pps,
	SHELL_CMD_ARG(enable, NULL, "<interval (s)> <pulse length (ms)>\nEnable 1PPS.",
		      cmd_gnss_1pps_enable, 3, 0),
	SHELL_CMD_ARG(enable_at, NULL,
		      "<interval (s)> <pulse length (ms)> <dd.mm.yyyy> <hh:mm:ss>\n"
		      "Enable 1PPS at a specific time (UTC).", cmd_gnss_1pps_enable_at, 5, 0),
	SHELL_CMD(disable, NULL, "Disable 1PPS.", cmd_gnss_1pps_disable),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_gnss,
	SHELL_CMD_ARG(start, NULL, "Start GNSS.", cmd_gnss_start, 1, 0),
	SHELL_CMD_ARG(stop, NULL, "Stop GNSS.", cmd_gnss_stop, 1, 0),
	SHELL_CMD(delete, &sub_gnss_delete, "Delete GNSS data.", mosh_print_help_shell),
	SHELL_CMD(mode, &sub_gnss_mode, "Set tracking mode.", mosh_print_help_shell),
	SHELL_CMD(dynamics, &sub_gnss_dynamics, "Set dynamics mode.", mosh_print_help_shell),
	SHELL_CMD(config, &sub_gnss_config, "Set GNSS configuration.", mosh_print_help_shell),
	SHELL_CMD(priority, &sub_gnss_priority, "Enable/disable priority time window requests.",
		  mosh_print_help_shell),
	SHELL_CMD(agnss, &sub_gnss_agnss, "A-GNSS configuration and commands.",
		  mosh_print_help_shell),
	SHELL_CMD(pgps, &sub_gnss_pgps, "P-GPS commands.", mosh_print_help_shell),
	SHELL_CMD(1pps, &sub_gnss_1pps, "1PPS control.", mosh_print_help_shell),
	SHELL_CMD(output, NULL, "<pvt level> <nmea level> <event level>\nSet output levels.",
		  cmd_gnss_output),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(gnss, &sub_gnss, "Commands for controlling GNSS.", mosh_print_help_shell);
