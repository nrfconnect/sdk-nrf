/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_PRINT_H
#define MOSH_PRINT_H

#include <stdarg.h>

enum mosh_print_level {
	MOSH_PRINT_LEVEL_PRINT,
	MOSH_PRINT_LEVEL_WARN,
	MOSH_PRINT_LEVEL_ERROR,
};

/* Forward declarations */
struct shell;

/**
 * Print without formatting, i.e., without including timestamp or any other formatting.
 * This is intended to be used for printing usage information for a command
 * but can be used elsewhere too.
 */
void mosh_print_no_format(const char *usage);

/**
 * Print help for the current command and subcommand from the Zephyr shell command macros.
 */
int mosh_print_help_shell(const struct shell *shell, size_t argc, char **argv);

/**
 * Create a timestamp for printing.
 */
bool create_timestamp_string(char *timestamp_buf, int timestamp_buf_len);

/**
 * printf-like function which sends formatted data stream to the shell output.
 * Not inteded to be used outside below macros.
 */
void mosh_fprintf(enum mosh_print_level print_level, const char *fmt, ...);

/**
 * Similar to mosh_fprintf but vargs are passed as va_list.
 */
void mosh_fprintf_valist(enum mosh_print_level print_level, const char *fmt, va_list args);

/** Print normal level information to output. This should be used for normal application output. */
#define mosh_print(fmt, ...) mosh_fprintf(MOSH_PRINT_LEVEL_PRINT, fmt, ##__VA_ARGS__)
/** Print warning level information to output. */
#define mosh_warn(fmt, ...)  mosh_fprintf(MOSH_PRINT_LEVEL_WARN, fmt, ##__VA_ARGS__)
/** Print error level information to output. */
#define mosh_error(fmt, ...) mosh_fprintf(MOSH_PRINT_LEVEL_ERROR, fmt, ##__VA_ARGS__)

/** Print application version information. */
void mosh_print_version_info(void);

#endif
