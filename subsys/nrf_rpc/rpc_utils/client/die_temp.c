/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/rpc_utils/die_temp.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>

int nrf_rpc_die_temp_get(int32_t *temperature)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint32_t error_code;

	if (!temperature) {
		return -EINVAL;
	}

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 0);
	nrf_rpc_cbor_cmd_rsp_no_err(&rpc_utils_group, RPC_UTIL_DIE_TEMP_GET, &ctx);

	error_code = nrf_rpc_decode_uint(&ctx);
	*temperature = nrf_rpc_decode_int(&ctx);

	if (!nrf_rpc_decoding_done_and_check(&rpc_utils_group, &ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &rpc_utils_group, RPC_UTIL_DIE_TEMP_GET,
			    NRF_RPC_PACKET_TYPE_RSP);
		return -EBADMSG;
	}

	return (int)error_code;
}
