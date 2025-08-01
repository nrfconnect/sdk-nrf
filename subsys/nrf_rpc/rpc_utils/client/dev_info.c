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

int nrf_rpc_get_crash_info(struct nrf_rpc_crash_info *info)
{
	struct nrf_rpc_cbor_ctx ctx;
	const void *filename;
	size_t size;

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&rpc_utils_group, RPC_UTIL_DEV_INFO_GET_CRASH_INFO, &ctx);

	if (nrf_rpc_decode_is_null(&ctx)) {
		return -ENOENT;
	}

	info->uuid = nrf_rpc_decode_uint(&ctx);
	info->reason = nrf_rpc_decode_uint(&ctx);
	info->pc = nrf_rpc_decode_uint(&ctx);
	info->lr = nrf_rpc_decode_uint(&ctx);
	info->sp = nrf_rpc_decode_uint(&ctx);
	info->xpsr = nrf_rpc_decode_uint(&ctx);
	info->assert_line = nrf_rpc_decode_uint(&ctx);

	nrf_rpc_decode_str(&ctx, info->assert_filename, sizeof(info->assert_filename));

	if (!nrf_rpc_decoding_done_and_check(&rpc_utils_group, &ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &rpc_utils_group,
			    RPC_UTIL_DEV_INFO_GET_CRASH_INFO, NRF_RPC_PACKET_TYPE_RSP);
	}

	return 0;
}
