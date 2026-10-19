/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * DECT_RPC_CMD_SHELL_LINE transmit path and L2 shell print hooks for RPC.
 */

#include "dect_rpc_ids.h"
#include "dect_rpc_common.h"
#include "dect_rpc_server_internal.h"

#include <net/dect/dect_net_l2_shell_util.h>

#include <nrf_rpc.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dect_rpc, CONFIG_NET_DECT_RPC_LOG_LEVEL);

K_MUTEX_DEFINE(dect_rpc_shell_session_mutex);

#define DECT_RPC_SHELL_LINE_LEN 256
#define DECT_RPC_SHELL_LINE_QUEUE_LEN 48
#define DECT_RPC_SHELL_LINE_PUT_TIMEOUT_MS 2000

struct dect_rpc_shell_line_msg {
	char buf[DECT_RPC_SHELL_LINE_LEN];
	size_t len;
};

K_MSGQ_DEFINE(dect_rpc_shell_line_msgq, sizeof(struct dect_rpc_shell_line_msg),
	      DECT_RPC_SHELL_LINE_QUEUE_LEN, 4);

static void dect_rpc_shell_line_work_fn(struct k_work *work)
{
	struct dect_rpc_shell_line_msg msg;
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(work);
	while (k_msgq_get(&dect_rpc_shell_line_msgq, &msg, K_NO_WAIT) == 0) {
		if (msg.len == 0 || !NRF_RPC_GROUP_STATUS(dect_rpc_group)) {
			continue;
		}
		NRF_RPC_CBOR_ALLOC(&dect_rpc_group, ctx, 4 + msg.len);
		nrf_rpc_encode_buffer(&ctx, (const uint8_t *)msg.buf, msg.len);
		nrf_rpc_cbor_evt_no_err(&dect_rpc_group, DECT_RPC_CMD_SHELL_LINE, &ctx);
		if (CONFIG_DECT_NR_RPC_SERVER_EVT_PACING_MS > 0) {
			k_msleep(CONFIG_DECT_NR_RPC_SERVER_EVT_PACING_MS);
		}
	}
}

static K_WORK_DEFINE(dect_rpc_shell_line_work, dect_rpc_shell_line_work_fn);

static void dect_rpc_shell_line_flush(const struct shell *shell, const char *fmt, va_list ap)
{
	struct dect_rpc_shell_line_msg msg;
	int n;
	const struct shell *shell_display;

	n = vsnprintf(msg.buf, sizeof(msg.buf), fmt, ap);
	if (n <= 0) {
		return;
	}
	msg.len = (size_t)n;
	if (msg.len >= sizeof(msg.buf)) {
		msg.len = sizeof(msg.buf) - 1;
	}
	if (msg.len > 0 && msg.buf[msg.len - 1] != '\n') {
		if (msg.len + 1 < sizeof(msg.buf)) {
			msg.buf[msg.len++] = '\n';
			msg.buf[msg.len] = '\0';
		}
	}

	if (k_msgq_put(
		&dect_rpc_shell_line_msgq, &msg, K_MSEC(DECT_RPC_SHELL_LINE_PUT_TIMEOUT_MS)) != 0) {
		LOG_WRN("Shell line RPC queue full after %d ms; client may miss output",
			DECT_RPC_SHELL_LINE_PUT_TIMEOUT_MS);
	} else {
		k_work_submit(&dect_rpc_shell_line_work);
	}

	shell_display = (shell != NULL) ? shell : shell_backend_uart_get_ptr();
	if (shell_display != NULL) {
		shell_fprintf(shell_display, SHELL_NORMAL, "%.*s", (int)msg.len, msg.buf);
		if (msg.len > 0 && msg.buf[msg.len - 1] != '\n') {
			shell_fprintf(shell_display, SHELL_NORMAL, "\n");
		}
	}
}

static void dect_rpc_shell_print_fn(const struct shell *shell, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	dect_rpc_shell_line_flush(shell, fmt, ap);
	va_end(ap);
}

static void dect_rpc_shell_error_fn(const struct shell *shell, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	dect_rpc_shell_line_flush(shell, fmt, ap);
	va_end(ap);
}

static void dect_rpc_shell_warn_fn(const struct shell *shell, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	dect_rpc_shell_line_flush(shell, fmt, ap);
	va_end(ap);
}

void dect_rpc_shell_session_lock(void)
{
	k_mutex_lock(&dect_rpc_shell_session_mutex, K_FOREVER);
}

void dect_rpc_shell_session_unlock(void)
{
	k_mutex_unlock(&dect_rpc_shell_session_mutex);
}

int dect_rpc_shell_session_try_lock(void)
{
	return k_mutex_lock(&dect_rpc_shell_session_mutex, K_NO_WAIT);
}

void dect_rpc_server_shell_line_l2_init(void)
{
	struct dect_net_lib_shell_print_fns rpc_print_fns = {
		.print_fn = dect_rpc_shell_print_fn,
		.error_fn = dect_rpc_shell_error_fn,
		.warn_fn  = dect_rpc_shell_warn_fn,
	};

	(void)dect_net_l2_shell_init(&rpc_print_fns);
}
