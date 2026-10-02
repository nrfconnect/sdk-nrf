/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_ICMP_PING_SHELL_CTX_H__
#define DECT_ICMP_PING_SHELL_CTX_H__

#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

#include <net/dect/dect_net_lib_shell.h>

/** Shell session and print hooks (owned by dect_icmp_ping_shell.c). */
struct dect_icmp_ping_shell_ctx {
	const struct shell *shell;
	struct dect_net_lib_shell_print_fns print_fns;
	bool custom_print_enabled;
};

extern struct dect_icmp_ping_shell_ctx dect_icmp_ping_shell_ctx;

static inline void dect_icmp_ping_set_shell(const struct shell *shell)
{
	if (shell != NULL) {
		dect_icmp_ping_shell_ctx.shell = shell;
	}
}

/* Same output rules as dect_l2_shell. */
#define dect_icmp_ping_print(...)                                                                  \
	do {                                                                                       \
		struct dect_icmp_ping_shell_ctx *_ctx = &dect_icmp_ping_shell_ctx;                 \
                                                                                                   \
		if (_ctx->custom_print_enabled && _ctx->print_fns.print_fn) {                      \
			_ctx->print_fns.print_fn(_ctx->shell, ##__VA_ARGS__);                      \
		} else if (_ctx->shell != NULL) {                                                  \
			shell_print(_ctx->shell, ##__VA_ARGS__);                                   \
		} else {                                                                           \
			printk(__VA_ARGS__);                                                       \
		}                                                                                  \
	} while (0)

#define dect_icmp_ping_error(...)                                                                  \
	do {                                                                                       \
		struct dect_icmp_ping_shell_ctx *_ctx = &dect_icmp_ping_shell_ctx;                 \
                                                                                                   \
		if (_ctx->custom_print_enabled && _ctx->print_fns.error_fn) {                      \
			_ctx->print_fns.error_fn(_ctx->shell, ##__VA_ARGS__);                      \
		} else if (_ctx->shell != NULL) {                                                  \
			shell_error(_ctx->shell, ##__VA_ARGS__);                                   \
		} else {                                                                           \
			printk(__VA_ARGS__);                                                       \
		}                                                                                  \
	} while (0)

#define dect_icmp_ping_warn(...)                                                                   \
	do {                                                                                       \
		struct dect_icmp_ping_shell_ctx *_ctx = &dect_icmp_ping_shell_ctx;                 \
                                                                                                   \
		if (_ctx->custom_print_enabled && _ctx->print_fns.warn_fn) {                       \
			_ctx->print_fns.warn_fn(_ctx->shell, ##__VA_ARGS__);                       \
		} else if (_ctx->shell != NULL) {                                                  \
			shell_warn(_ctx->shell, ##__VA_ARGS__);                                    \
		} else {                                                                           \
			printk(__VA_ARGS__);                                                       \
		}                                                                                  \
	} while (0)

#endif /* DECT_ICMP_PING_SHELL_CTX_H__ */
