/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_NET_LIB_SHELL_H__
#define DECT_NET_LIB_SHELL_H__

/**
 * @file dect_net_lib_shell.h
 * @brief Shared shell output hooks for DECT NR+ network libraries.
 */

#include <stdarg.h>

struct shell;

/**
 * @brief Print function pointer type matching shell_print signature.
 */
typedef void (*dect_net_lib_shell_print_fn_t)(const struct shell *shell, const char *fmt, ...);

/**
 * @brief Optional custom print functions (for example, timestamped output).
 */
struct dect_net_lib_shell_print_fns {
	dect_net_lib_shell_print_fn_t print_fn;
	dect_net_lib_shell_print_fn_t error_fn;
	dect_net_lib_shell_print_fn_t warn_fn;
};

#endif /* DECT_NET_LIB_SHELL_H__ */
