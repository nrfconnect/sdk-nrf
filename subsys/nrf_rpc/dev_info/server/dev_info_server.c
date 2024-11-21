/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>

#include <ncs_commit.h>
#include <dev_info_rpc_ids.h>

NRF_RPC_GROUP_DECLARE(dev_info_group);

static void get_server_version(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			       void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	size_t cbor_buffer_size = 0;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group, DEV_INFO_RPC_GET_VERSION,
			    NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	cbor_buffer_size += 2 + strlen(NCS_COMMIT_STRING);

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, cbor_buffer_size);

	nrf_rpc_encode_str(&rsp_ctx, NCS_COMMIT_STRING, strlen(NCS_COMMIT_STRING));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(dev_info_group, get_server_version, DEV_INFO_RPC_GET_VERSION,
			 get_server_version, NULL);
