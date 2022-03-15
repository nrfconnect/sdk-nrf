/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_ICMP_PING_PRINT_H
#define MOSH_ICMP_PING_PRINT_H

#include "mosh_print.h"

#if defined(CONFIG_MOSH_WORKER_THREADS)

#include "icmp_ping.h"

void ping_fprintf(struct icmp_ping_shell_cmd_argv *ping_args, enum mosh_print_level print_level,
	const char *fmt,  ...);
/** Print normal level information to output.  */
#define ping_print(ping_args, fmt, ...) \
	ping_fprintf(ping_args, MOSH_PRINT_LEVEL_PRINT, fmt, ##__VA_ARGS__)

/** Print warning level information to output. */
#define ping_warn(ping_args, fmt, ...)  \
	ping_fprintf(ping_args, MOSH_PRINT_LEVEL_WARN, fmt, ##__VA_ARGS__)

/** Print error level information to output. */
#define ping_error(ping_args, fmt, ...) \
	ping_fprintf(ping_args, MOSH_PRINT_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#else

/** Print normal level information to output.  */
#define ping_print(ping_args, fmt, ...) mosh_fprintf(MOSH_PRINT_LEVEL_PRINT, fmt, ##__VA_ARGS__)
/** Print warning level information to output. */
#define ping_warn(ping_args, fmt, ...)  mosh_fprintf(MOSH_PRINT_LEVEL_WARN, fmt, ##__VA_ARGS__)
/** Print error level information to output. */
#define ping_error(ping_args, fmt, ...) mosh_fprintf(MOSH_PRINT_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#endif

#endif /* MOSH_ICMP_PING_PRINT_H */
