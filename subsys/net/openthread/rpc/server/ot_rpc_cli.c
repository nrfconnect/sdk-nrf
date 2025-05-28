/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>
#include <ot_rpc_lock.h>

#include <nrf_rpc_cbor.h>

#include <openthread/cli.h>
#include <zephyr/net/openthread.h>

#include <stdio.h>

static int ot_cli_output_callback(void *aContext, const char *aFormat, va_list aArguments)
{
	size_t cbor_buffer_size = 6;
	struct nrf_rpc_cbor_ctx ctx;
	char output_line_buffer[256];
	int result;
	size_t num_written;

	ARG_UNUSED(aContext);
	result = vsnprintf(output_line_buffer, sizeof(output_line_buffer), aFormat, aArguments);

	if (result < 0) {
		return result;
	}

	num_written = MIN((size_t)result, sizeof(output_line_buffer));
	cbor_buffer_size += num_written;
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_str(&ctx, output_line_buffer, num_written);

	ot_rpc_mutex_unlock();
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_CLI_OUTPUT, &ctx, ot_rpc_decode_void, NULL);
	ot_rpc_mutex_lock();

	return num_written;
}

static void ot_rpc_cmd_cli_init(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	ot_rpc_mutex_lock();
	otCliInit(openthread_get_default_instance(), ot_cli_output_callback, NULL /* aContext*/);
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_cli_init, OT_RPC_CMD_CLI_INIT, ot_rpc_cmd_cli_init,
			 NULL);

static void ot_rpc_cmd_cli_input_line(const struct nrf_rpc_group *group,
				      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	char *buffer;
	const void *ptr;
	size_t len;
	bool reply_before_exec;

	/* Parse the input */
	ptr = nrf_rpc_decode_str_ptr_and_len(ctx, &len);

	if (ptr) {
		buffer = malloc(len + 1);
		if (buffer) {
			memcpy(buffer, ptr, len);
			buffer[len] = '\0';
		}
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!ptr) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_CLI_INPUT_LINE);
		return;
	}

	if (!buffer) {
		nrf_rpc_err(-ENOMEM, NRF_RPC_ERR_SRC_RECV, group, OT_RPC_CMD_CLI_INPUT_LINE,
			    NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	reply_before_exec = !strcmp(buffer, "factoryreset");

	if (reply_before_exec) {
		nrf_rpc_rsp_send_void(group);
	}

	ot_rpc_mutex_lock();
	otCliInputLine(buffer);
	ot_rpc_mutex_unlock();
	free(buffer);

	if (!reply_before_exec) {
		nrf_rpc_rsp_send_void(group);
	}
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_cli_input_line, OT_RPC_CMD_CLI_INPUT_LINE,
			 ot_rpc_cmd_cli_input_line, NULL);
