/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DESH_PRINT_H
#define DESH_PRINT_H

#include <stdarg.h>

enum desh_print_level {
	DESH_PRINT_LEVEL_PRINT,
	DESH_PRINT_LEVEL_WARN,
	DESH_PRINT_LEVEL_ERROR,
};

/* Forward declarations */
struct shell;

/**
 * Print without formatting, i.e., without including timestamp or any other formatting.
 * This is intended to be used for printing usage information for a command
 * but can be used elsewhere too.
 */
void desh_print_no_format(const char *usage);

/**
 * Print help for the current command and subcommand from the Zephyr shell command macros.
 */
int desh_print_help_shell(const struct shell *shell, size_t argc, char **argv);

/**
 * Create a timestamp for printing.
 */
bool create_timestamp_string(char *timestamp_buf, int timestamp_buf_len);

/**
 * printf-like function that sends formatted data stream to the shell output.
 * Not intended to be used outside below macros.
 */
void desh_fprintf(enum desh_print_level print_level, const char *fmt, ...);

/**
 * Similar to desh_fprintf but vargs are passed as va_list.
 */
void desh_fprintf_valist(enum desh_print_level print_level, const char *fmt, va_list args);

/** Print normal level information to output. This should be used for normal application output. */
#define desh_print(fmt, ...) desh_fprintf(DESH_PRINT_LEVEL_PRINT, fmt, ##__VA_ARGS__)
/** Print warning level information to output. */
#define desh_warn(fmt, ...)  desh_fprintf(DESH_PRINT_LEVEL_WARN, fmt, ##__VA_ARGS__)
/** Print error level information to output. */
#define desh_error(fmt, ...) desh_fprintf(DESH_PRINT_LEVEL_ERROR, fmt, ##__VA_ARGS__)

/** Print application version information. */
void desh_print_version_info(void);

#endif
