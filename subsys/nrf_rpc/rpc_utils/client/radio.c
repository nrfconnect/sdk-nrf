/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>


void nrf_802154_pa_modulation_fix_set(bool enable)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 1);
	nrf_rpc_encode_bool(&ctx, enable);
	nrf_rpc_cbor_cmd_no_err(&rpc_utils_group,
				RPC_UTIL_SDM_ENABLE_SET,
				&ctx,
				nrf_rpc_rsp_decode_void,
				NULL);
}


bool nrf_802154_pa_modulation_fix_get(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool enabled;

	NRF_RPC_CBOR_ALLOC(&rpc_utils_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&rpc_utils_group,
				RPC_UTIL_SDM_ENABLE_GET,
				&ctx,
				nrf_rpc_rsp_decode_bool,
				&enabled);

	return enabled;
}
