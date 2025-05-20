/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_client_common.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nrf_rpc_cbor.h>

size_t ot_rpc_get_string(enum ot_rpc_cmd_server cmd, char *buffer, size_t buffer_size)
{
	struct nrf_rpc_cbor_ctx ctx;

	if (!buffer || !buffer_size) {
		ot_rpc_report_rsp_decoding_error(cmd);
		return 0;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);
	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, cmd, &ctx);

	nrf_rpc_decode_str(&ctx, buffer, buffer_size);

	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_rsp_decoding_error(cmd);

		/* In case of error, set buffer to an empty string */
		*buffer = '\0';
	}

	return strlen(buffer);
}

otError ot_rpc_set_string(enum ot_rpc_cmd_server cmd, const char *data)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, strlen(data) + 2);
	nrf_rpc_encode_str(&ctx, data, strlen(data));

	nrf_rpc_cbor_cmd_no_err(&ot_group, cmd, &ctx, ot_rpc_decode_error, &error);

	return error;
}
