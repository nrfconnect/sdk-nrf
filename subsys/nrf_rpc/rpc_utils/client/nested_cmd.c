/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/rpc_utils/nested_cmd.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>
#include <zephyr/sys/atomic.h>

static atomic_t nested_cmd_received;

static void nested_cmd_callback(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				void *handler_data)
{
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group,
			    RPC_UTIL_NESTED_CMD_CALLBACK, NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	atomic_set(&nested_cmd_received, 1);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group, nested_cmd_callback, RPC_UTIL_NESTED_CMD_CALLBACK,
			 nested_cmd_callback, NULL);

bool nrf_rpc_utils_nested_cmd_test(void)
{
	int err;
	bool server_sent_nested_cmd;
	struct nrf_rpc_cbor_ctx ctx;

	atomic_set(&nested_cmd_received, 0);

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 0);

	err = nrf_rpc_cbor_cmd_rsp(&rpc_utils_group, RPC_UTIL_NESTED_CMD_TEST, &ctx);
	if (err < 0) {
		nrf_rpc_err(err, NRF_RPC_ERR_SRC_SEND, &rpc_utils_group,
			    RPC_UTIL_NESTED_CMD_TEST, NRF_RPC_PACKET_TYPE_CMD);
		return false;
	}

	server_sent_nested_cmd = nrf_rpc_decode_bool(&ctx);
	if (!nrf_rpc_decoding_done_and_check(&rpc_utils_group, &ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &rpc_utils_group,
			    RPC_UTIL_NESTED_CMD_TEST, NRF_RPC_PACKET_TYPE_RSP);
		return false;
	}

	return server_sent_nested_cmd && atomic_get(&nested_cmd_received);
}
