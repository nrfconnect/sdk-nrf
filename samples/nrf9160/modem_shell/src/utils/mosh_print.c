/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <zephyr.h>
#include <posix/time.h>
#include <sys/cbprintf.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>

#include "mosh_print.h"

static const struct shell *mosh_shell;

/** Configuration on whether timestamps are added to the print. */
bool mosh_print_timestamp_use;

/** Buffer used for printing the timestamp. */
static char timestamp_str[30];
/** Buffer used for generating the text to be printed. */
static char mosh_print_buf[CONFIG_MOSH_PRINT_BUFFER_SIZE];
/** Mutex for protecting mosh_print_buf */
K_MUTEX_DEFINE(mosh_print_buf_mutex);

static bool mosh_print_shell_ptr_update(void)
{
	if (mosh_shell == NULL) {
		mosh_shell = shell_backend_uart_get_ptr();
		if (mosh_shell == NULL) {
			printk("%s: CANNOT OBTAIN SHELL BACKEND. THIS IS FATAL INTERNAL ERROR.\n",
			       __func__);
			return false;
		}
	}
	return true;
}

static const char *create_timestamp_string(void)
{
	uint32_t year;
	uint32_t month;
	uint32_t day;

	uint32_t hours;
	uint32_t mins;
	uint32_t secs;
	uint32_t msec;

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

	sprintf(timestamp_str,
		"[%04d-%02d-%02d %02d:%02d:%02d.%03d] ",
		year, month, day, hours, mins, secs, msec);

	return timestamp_str;
}

void mosh_fprintf(enum mosh_print_level print_level, const char *fmt, ...)
{
	va_list args;
	int chars = 0;
	const char *timestamp;

	k_mutex_lock(&mosh_print_buf_mutex, K_FOREVER);

	if (!mosh_print_shell_ptr_update()) {
		vsnprintf(mosh_print_buf, sizeof(mosh_print_buf), fmt, args);
		printk("%s", mosh_print_buf);
		k_mutex_unlock(&mosh_print_buf_mutex);
		return;
	}

	va_start(args, fmt);

	/* Add timestamp to print buffer if requested */
	if (mosh_print_timestamp_use) {
		timestamp = create_timestamp_string();
		chars = snprintf(mosh_print_buf, sizeof(mosh_print_buf), "%s", timestamp);
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

	k_mutex_unlock(&mosh_print_buf_mutex);
	va_end(args);
}

void mosh_print_no_format(const char *usage)
{
	if (!mosh_print_shell_ptr_update()) {
		printk("%s", usage);
		return;
	}

	shell_print(mosh_shell, "%s", usage);
}
