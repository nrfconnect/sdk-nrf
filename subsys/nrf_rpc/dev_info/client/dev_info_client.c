/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "zephyr/sys/util.h"
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>

#include <ncs_commit.h>
#include <dev_info_rpc_ids.h>

NRF_RPC_GROUP_DECLARE(dev_info_group);

static char version[16];

const char *nrf_rpc_get_ncs_commit_sha(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t size = ARRAY_SIZE(version);

	memset(version, 0, ARRAY_SIZE(version));

	NRF_RPC_CBOR_ALLOC(&dev_info_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&dev_info_group, DEV_INFO_RPC_GET_VERSION, &ctx);

	nrf_rpc_decode_str(&ctx, version, size);

	if (!nrf_rpc_decoding_done_and_check(&dev_info_group, &ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &dev_info_group,
			    DEV_INFO_RPC_GET_VERSION, NRF_RPC_PACKET_TYPE_RSP);
		return NULL;
	}

	return version;
}
