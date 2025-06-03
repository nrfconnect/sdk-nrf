/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/rpc_utils/system_health.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>

void nrf_rpc_system_health_get(struct nrf_rpc_system_health *out)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 0);
	nrf_rpc_cbor_cmd_rsp_no_err(&rpc_utils_group, RPC_UTIL_SYSTEM_HEALTH_GET, &ctx);

	out->hung_threads = nrf_rpc_decode_uint(&ctx);

	if (!nrf_rpc_decoding_done_and_check(&rpc_utils_group, &ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &rpc_utils_group,
			    RPC_UTIL_SYSTEM_HEALTH_GET, NRF_RPC_PACKET_TYPE_RSP);
	}
}
