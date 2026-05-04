/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>

static void nested_cmd_test(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	int err;
	struct nrf_rpc_cbor_ctx nested_ctx;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group,
			    RPC_UTIL_NESTED_CMD_TEST, NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	NRF_RPC_CBOR_ALLOC(group, nested_ctx, 0);

	err = nrf_rpc_cbor_nested_cmd(group, RPC_UTIL_NESTED_CMD_CALLBACK, &nested_ctx);

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 1);

	nrf_rpc_encode_bool(&rsp_ctx, err == 0);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, nested_cmd_test, RPC_UTIL_NESTED_CMD_TEST,
			 nested_cmd_test, NULL);

