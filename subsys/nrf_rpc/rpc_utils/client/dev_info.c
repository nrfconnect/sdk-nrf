/*
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "decode_helpers.h"

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/rpc_utils/dev_info.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>

char *nrf_rpc_get_ncs_commit_sha(void)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&rpc_utils_group, RPC_UTIL_DEV_INFO_GET_VERSION, &ctx);

	return allocate_buffer_and_decode_str(&ctx, RPC_UTIL_DEV_INFO_GET_VERSION);
}
