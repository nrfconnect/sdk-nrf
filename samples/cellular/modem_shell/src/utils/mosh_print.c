/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/time.h>
#include <zephyr/sys/cbprintf.h>
#include <zephyr/shell/shell.h>
#include <net/nrf_cloud.h>

#include "mosh_print.h"

extern const struct shell *mosh_shell;

#if defined(CONFIG_MOSH_AT_CMD_MODE)
extern bool at_cmd_mode_dont_print;
#endif

/** Configuration on whether timestamps are added to the print. */
bool mosh_print_timestamp_use;

#if defined(CONFIG_MOSH_CLOUD_MQTT)
/** Wrap all mosh prints into JSON and send to nRF Cloud over MQTT */
bool mosh_print_cloud_echo;
#endif

/** Buffer used for printing the timestamp. */
static char timestamp_str[30];
/** Buffer used for generating the text to be printed. */
static char mosh_print_buf[CONFIG_MOSH_PRINT_BUFFER_SIZE];
/** Mutex for protecting mosh_print_buf */
K_MUTEX_DEFINE(mosh_print_buf_mutex);

bool create_timestamp_string(char *timestamp_buf, int timestamp_buf_len)
{
	uint32_t year;
	uint32_t month;
	uint32_t day;

	uint32_t hours;
	uint32_t mins;
	uint32_t secs;
	uint32_t msec;

	int chars = 0;

	struct timespec tp;
	struct tm ltm = { 0 };

	clock_gettime(CLOCK_REALTIME, &tp);
	gmtime_r(&tp.tv_sec, &ltm);

	msec = tp.tv_nsec / 1000000;
	secs = ltm.tm_sec;
	mins = ltm.tm_min;
	hours = ltm.tm_hour;
	day = ltm.tm_mday;
	/* Range is 0-11, as per POSIX */
	month = ltm.tm_mon + 1;
	/* Relative to 1900, as per POSIX */
	year = 1900 + ltm.tm_year;

	chars = snprintf(timestamp_buf, timestamp_buf_len,
		"[%04d-%02d-%02d %02d:%02d:%02d.%03d] ",
		year, month, day, hours, mins, secs, msec);
	if (chars < 0) {
		return false;
	}
	return true;
}

void mosh_fprintf_valist(enum mosh_print_level print_level, const char *fmt, va_list args)
{
	int chars = 0;

#if defined(CONFIG_MOSH_AT_CMD_MODE)
	if (at_cmd_mode_dont_print) {
		return;
	}
#endif

	k_mutex_lock(&mosh_print_buf_mutex, K_FOREVER);

	/* Add timestamp to print buffer if requested */
	if (mosh_print_timestamp_use) {
		(void)create_timestamp_string(timestamp_str, sizeof(timestamp_str));
		chars = snprintf(mosh_print_buf, sizeof(mosh_print_buf), "%s", timestamp_str);
		if (chars < 0) {
			shell_error(mosh_shell, "Error while printing timestamp...");
			chars = 0;
		}
	}

	/* Add requested printf-like string.
	 * We need to use vsnprintfcb, which is Zephyr specific version to save memory,
	 * to make more specifiers available. Normal vsnprintf() had issues with %lld specifier.
	 * It printed wrong number and at least next %s specifier was corrupted.
	 */
	chars += vsnprintfcb(mosh_print_buf + chars, sizeof(mosh_print_buf) - chars, fmt, args);
	if (chars >= sizeof(mosh_print_buf)) {
		shell_error(mosh_shell, "Cutting too long string while printing...");
	} else if (chars < 0) {
		shell_error(mosh_shell, "Error while printing...");
	}

	/* Print with given level */
	switch (print_level) {
	case MOSH_PRINT_LEVEL_PRINT:
		shell_print(mosh_shell, "%s", mosh_print_buf);
		break;
	case MOSH_PRINT_LEVEL_WARN:
		shell_warn(mosh_shell, "%s", mosh_print_buf);
		break;
	case MOSH_PRINT_LEVEL_ERROR:
		shell_error(mosh_shell, "%s", mosh_print_buf);
		break;
	default:
		shell_error(mosh_shell, "Unknown print level for next log. Please fix it.");
		shell_print(mosh_shell, "%s", mosh_print_buf);
		break;
	}

#if defined(CONFIG_MOSH_CLOUD_MQTT)
	if (mosh_print_cloud_echo) {
		struct nrf_cloud_sensor_data mosh_cloud_print = {
			.type = NRF_CLOUD_DEVICE_INFO,
			.data.ptr = mosh_print_buf,
			.data.len = strlen(mosh_print_buf),
			.ts_ms = NRF_CLOUD_NO_TIMESTAMP
		};

		nrf_cloud_sensor_data_stream(&mosh_cloud_print);
	}
#endif

	k_mutex_unlock(&mosh_print_buf_mutex);
}

void mosh_fprintf(enum mosh_print_level print_level, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	mosh_fprintf_valist(print_level, fmt, args);
	va_end(args);
}

void mosh_print_no_format(const char *usage)
{
	shell_print(mosh_shell, "%s", usage);
}

int mosh_print_help_shell(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 1;

	if (argc > 1 && strcmp(argv[1], "-h") != 0 && strcmp(argv[1], "--help") != 0) {
		mosh_error("%s: subcommand not found", argv[1]);
		ret = -EINVAL;
	}

	shell_help(shell);

	return ret;
}
