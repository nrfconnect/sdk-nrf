/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>

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
	zcbor_tstr_encode_ptr(ctx.zs, output_line_buffer, num_written);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_CLI_OUTPUT, &ctx, ot_rpc_decode_void, NULL);

	return num_written;
}

static void ot_rpc_cmd_cli_init(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;

	/* Parse the input */

	nrf_rpc_cbor_decoding_done(group, ctx);

	/* Initialize OT CLI */

	openthread_api_mutex_lock(openthread_get_default_context());
	otCliInit(openthread_get_default_instance(), ot_cli_output_callback, NULL /* aContext*/);
	openthread_api_mutex_unlock(openthread_get_default_context());

	/* Encode and send the response */

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 0);

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_cli_init, OT_RPC_CMD_CLI_INIT, ot_rpc_cmd_cli_init,
			 NULL);

static void ot_rpc_cmd_cli_input_line(const struct nrf_rpc_group *group,
				      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct zcbor_string input_line;
	char input_line_buffer[256];
	struct nrf_rpc_cbor_ctx rsp_ctx;

	/* Parse the input */

	if (!zcbor_tstr_decode(ctx->zs, &input_line)) {
		goto error;
	}

	if (input_line.len >= sizeof(input_line_buffer)) {
		goto error;
	}

	memcpy(input_line_buffer, input_line.value, input_line.len);
	input_line_buffer[input_line.len] = '\0';

	nrf_rpc_cbor_decoding_done(group, ctx);

	/* Execute OT CLI command */

	openthread_api_mutex_lock(openthread_get_default_context());
	otCliInputLine(input_line_buffer);
	openthread_api_mutex_unlock(openthread_get_default_context());

	/* Encode and send the response */

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 0);

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);

error:
	return;
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_cli_input_line, OT_RPC_CMD_CLI_INPUT_LINE,
			 ot_rpc_cmd_cli_input_line, NULL);
