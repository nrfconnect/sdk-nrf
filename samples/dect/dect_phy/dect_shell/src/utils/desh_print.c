/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
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
#include <modem/modem_info.h>
#include <net/nrf_cloud.h>

#include <ncs_version.h>
#include <ncs_commit.h>

#include "desh_print.h"

extern const struct shell *desh_shell;

/** Configuration on whether timestamps are added to the print. */
bool desh_print_timestamp_use;

/** Buffer used for printing the timestamp. */
static char timestamp_str[30];
/** Buffer used for generating the text to be printed. */
static char desh_print_buf[CONFIG_DESH_PRINT_BUFFER_SIZE];
/** Mutex for protecting desh_print_buf */
K_MUTEX_DEFINE(desh_print_buf_mutex);

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

void desh_fprintf_valist(enum desh_print_level print_level, const char *fmt, va_list args)
{
	int chars = 0;

	k_mutex_lock(&desh_print_buf_mutex, K_FOREVER);

	/* Add timestamp to print buffer if requested */
	if (desh_print_timestamp_use) {
		(void)create_timestamp_string(timestamp_str, sizeof(timestamp_str));
		chars = snprintf(desh_print_buf, sizeof(desh_print_buf), "%s", timestamp_str);
		if (chars < 0) {
			shell_error(desh_shell, "Error while printing timestamp...");
			chars = 0;
		}
	}

	/* Add requested printf-like string.
	 * We need to use vsnprintfcb, which is Zephyr specific version to save memory,
	 * to make more specifiers available. Normal vsnprintf() had issues with %lld specifier.
	 * It printed wrong number and at least next %s specifier was corrupted.
	 */
	chars += vsnprintfcb(desh_print_buf + chars, sizeof(desh_print_buf) - chars, fmt, args);
	if (chars >= sizeof(desh_print_buf)) {
		shell_error(desh_shell, "Cutting too long string while printing...");
	} else if (chars < 0) {
		shell_error(desh_shell, "Error while printing...");
	}

	/* Print with given level */
	switch (print_level) {
	case DESH_PRINT_LEVEL_PRINT:
		shell_print(desh_shell, "%s", desh_print_buf);
		break;
	case DESH_PRINT_LEVEL_WARN:
		shell_warn(desh_shell, "%s", desh_print_buf);
		break;
	case DESH_PRINT_LEVEL_ERROR:
		shell_error(desh_shell, "%s", desh_print_buf);
		break;
	default:
		shell_error(desh_shell, "Unknown print level for next log. Please fix it.");
		shell_print(desh_shell, "%s", desh_print_buf);
		break;
	}

	k_mutex_unlock(&desh_print_buf_mutex);
}

void desh_fprintf(enum desh_print_level print_level, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	desh_fprintf_valist(print_level, fmt, args);
	va_end(args);
}

void desh_print_no_format(const char *usage)
{
	shell_print(desh_shell, "%s", usage);
}

int desh_print_help_shell(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 1;

	if (argc > 1 && strcmp(argv[1], "-h") != 0 && strcmp(argv[1], "--help") != 0) {
		desh_error("%s: subcommand not found", argv[1]);
		ret = -EINVAL;
	}

	shell_help(shell);

	return ret;
}

void desh_print_version_info(void)
{
	desh_print("DESH version:       v%s-%s", NCS_VERSION_STRING, NCS_COMMIT_STRING);
#if defined(BUILD_ID)
	desh_print("DESH build id:      v%s", STRINGIFY(BUILD_ID));
#else
	desh_print("DESH build id:      custom");
#endif

#if defined(BUILD_VARIANT)
#if defined(BRANCH_NAME)
	desh_print("DESH build variant: %s/%s", STRINGIFY(BRANCH_NAME), STRINGIFY(BUILD_VARIANT));
#else
	desh_print("DESH build variant: %s", STRINGIFY(BUILD_VARIANT));
#endif
#else
	desh_print("DESH build variant: dev");
#endif

}
