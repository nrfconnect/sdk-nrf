/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * DECT_RPC_CMD_SHELL_LINE event decoder: remote DECT L2 shell output on the client UART.
 */

#include "dect_rpc_ids.h"
#include "dect_rpc_common.h"

#include <nrf_rpc.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

static void dect_rpc_cmd_shell_line(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				    void *handler_data)
{
	const uint8_t *p;
	size_t sz;
	const struct shell *shell;
	char line_buf[256];
	size_t copy_sz = 0;

	ARG_UNUSED(handler_data);
	if (!nrf_rpc_decode_valid(ctx)) {
		nrf_rpc_decoding_done(group, ctx->in_packet);
		return;
	}
	p = nrf_rpc_decode_buffer_ptr_and_size(ctx, &sz);
	if (p != NULL && sz > 0) {
		copy_sz = sz;
		if (copy_sz >= sizeof(line_buf)) {
			copy_sz = sizeof(line_buf) - 1;
		}
		memcpy(line_buf, p, copy_sz);
		line_buf[copy_sz] = '\0';
	}
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_decoding_done(group, ctx->in_packet);
		return;
	}
	if (p != NULL && sz > 0) {
		shell = shell_backend_uart_get_ptr();
		if (shell != NULL) {
			shell_fprintf(shell, SHELL_NORMAL, "%s", line_buf);
			if (copy_sz > 0 && line_buf[copy_sz - 1] != '\n') {
				shell_fprintf(shell, SHELL_NORMAL, "\n");
			}
		} else {
			printk("%s", line_buf);
			if (copy_sz > 0 && line_buf[copy_sz - 1] != '\n') {
				printk("\n");
			}
		}
	}
}

NRF_RPC_CBOR_EVT_DECODER(dect_rpc_group, dect_rpc_cmd_shell_line, DECT_RPC_CMD_SHELL_LINE,
			 dect_rpc_cmd_shell_line, NULL);
