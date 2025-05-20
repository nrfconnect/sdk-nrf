/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
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
#include <zephyr/shell/shell_uart.h>
#include <net/nrf_cloud.h>

#include "mosh_print.h"
#include "icmp_ping.h"
#include "icmp_ping_print.h"

/** Configuration on whether timestamps are added to the print. */
extern bool mosh_print_timestamp_use;

/** Buffer used for printing the timestamp. */
static char timestamp_str[30];

#define PING_PRINT_LINEBUFF_LEN 1024
static char linebuffer[PING_PRINT_LINEBUFF_LEN];

void ping_fprintf(struct icmp_ping_shell_cmd_argv *ping_args, enum mosh_print_level print_level,
	const char *fmt, ...)
{
	va_list args;
	va_list save;

	int chars = 0;

	assert(ping_args != NULL);
	va_start(args, fmt);

	if (ping_args->print_buf == NULL && !ping_args->print_buf_len) {
		va_copy(save, args);
		mosh_fprintf_valist(print_level, fmt, save);
		va_end(save);
		goto exit;
	}

	sprintf(linebuffer, "\n");

	/* Add timestamp to print buffer if requested */
	if (mosh_print_timestamp_use) {
		(void)create_timestamp_string(timestamp_str, sizeof(timestamp_str));
		chars = strlen(linebuffer);
		if (snprintf(linebuffer + chars,
			     PING_PRINT_LINEBUFF_LEN - chars, "%s", timestamp_str) < 0) {
			sprintf(linebuffer, "Error while printing timestamp...");
		}
	}
	chars = strlen(linebuffer);

	chars += vsnprintfcb(linebuffer + chars, PING_PRINT_LINEBUFF_LEN - chars, fmt, args);

	if (chars >= PING_PRINT_LINEBUFF_LEN) {
		sprintf(linebuffer, "Cutting too long string while printing...");
	} else if (chars < 0) {
		sprintf(linebuffer, "Error while printing...");
	}

	/* Now copy to printing buffer */
	chars = strlen(ping_args->print_buf) + strlen(linebuffer);
	if (chars >= ping_args->print_buf_len) {
		strcpy(ping_args->print_buf,
		       "WARNING: response buffer was too short and was flushed\n"
			"...and started from the beginning\n");
	}
	strcat(ping_args->print_buf, linebuffer);
exit:
	va_end(args);
}
