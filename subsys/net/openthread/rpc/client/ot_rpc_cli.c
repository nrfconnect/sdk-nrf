/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>

#include <nrf_rpc_cbor.h>

#include <openthread/cli.h>

#include <string.h>

otCliOutputCallback cli_output_callback;
void *cli_output_context;

void otCliInit(otInstance *aInstance, otCliOutputCallback aCallback, void *aContext)
{
	const size_t cbor_buffer_size = 0;
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);
	cli_output_callback = aCallback;
	cli_output_context = aContext;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_CLI_INIT, &ctx, ot_rpc_decode_void, NULL);
}

void otCliInputLine(char *aBuf)
{
	const size_t buffer_len = strlen(aBuf);
	const size_t cbor_buffer_size = 5 + buffer_len;
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	zcbor_tstr_encode_ptr(ctx.zs, aBuf, buffer_len);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_CLI_INPUT_LINE, &ctx, ot_rpc_decode_void,
				NULL);
}

static void invoke_cli_output_callback(const char *format, ...)
{
	va_list args;

	if (cli_output_callback == NULL) {
		/* Not yet initialized, drop the output */
		return;
	}

	va_start(args, format);
	cli_output_callback(cli_output_context, format, args);
	va_end(args);
}

static void ot_rpc_cmd_cli_output(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				  void *handler_data)
{
	struct zcbor_string output_line;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	/* Parse the input */

	if (!zcbor_tstr_decode(ctx->zs, &output_line)) {
		goto error;
	}

	/* Invoke the client's CLI output callback and release the input packet */

	invoke_cli_output_callback("%.*s", output_line.len, output_line.value);
	nrf_rpc_cbor_decoding_done(group, ctx);

	/* Encode and send the response */

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 0);

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);

error:
	return;
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_cli_output, OT_RPC_CMD_CLI_OUTPUT,
			 ot_rpc_cmd_cli_output, NULL);
