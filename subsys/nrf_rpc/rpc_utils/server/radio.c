/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <rpc_utils_group.h>

#include <nrf_rpc_cbor.h>
#include <nrf_802154.h>

static void rpc_util_sdm_mode_enable_set(const struct nrf_rpc_group *group,
				  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool enable;

	enable = nrf_rpc_decode_bool(ctx);

	nrf_rpc_cbor_decoding_done(group, ctx);

	nrf_802154_pa_modulation_fix_set(enable);

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group,
			 rpc_util_sdm_mode_enable_set,
			 RPC_UTIL_SDM_ENABLE_SET,
			 rpc_util_sdm_mode_enable_set,
			 NULL);

static void rpc_util_sdm_mode_enable_get(const struct nrf_rpc_group *group,
				  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool enabled;

	nrf_rpc_cbor_decoding_done(group, ctx);
	enabled = nrf_802154_pa_modulation_fix_get();

	nrf_rpc_rsp_send_bool(group, enabled);
}


NRF_RPC_CBOR_CMD_DECODER(rpc_utils_group,
			 rpc_util_sdm_mode_enable_get,
			 RPC_UTIL_SDM_ENABLE_GET,
			 rpc_util_sdm_mode_enable_get,
			 NULL);
