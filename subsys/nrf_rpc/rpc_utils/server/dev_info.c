/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ncs_commit.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>

static void get_server_version(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			       void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	struct nrf_rpc_cbor_ctx rsp_ctx;
	size_t cbor_buffer_size = 0;

	cbor_buffer_size += 2 + sizeof(NCS_COMMIT_STRING) - 1;
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, cbor_buffer_size);

	nrf_rpc_encode_str(&rsp_ctx, NCS_COMMIT_STRING, sizeof(NCS_COMMIT_STRING) - 1);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, get_server_version, RPC_UTIL_DEV_INFO_GET_VERSION,
			 get_server_version, NULL);
