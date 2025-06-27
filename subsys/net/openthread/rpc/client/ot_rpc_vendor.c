/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>

#include <nrf_rpc_cbor.h>

#include <openthread/error.h>

otError ot_rpc_vendor_radio_power_limit_id_set(uint8_t id)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(id) + 1);
	nrf_rpc_encode_uint(&ctx, id);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_VENDOR_RADIO_POWER_LIMIT_ID_SET, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}
