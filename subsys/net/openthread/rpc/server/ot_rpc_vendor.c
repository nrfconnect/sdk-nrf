/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_rpc_resource.h"

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>
#include <vendor_radio_power_limit.h>

#include <nrf_rpc_cbor.h>

static void ot_rpc_vendor_radio_power_limit_id_set(const struct nrf_rpc_group *group,
						   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint8_t id;
	otError error;

	id = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_VENDOR_RADIO_POWER_LIMIT_ID_SET);
		return;
	}

	error = vendor_radio_power_limit_id_set(id);

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_vendor_radio_power_limit_id_set,
			 OT_RPC_CMD_VENDOR_RADIO_POWER_LIMIT_ID_SET,
			 ot_rpc_vendor_radio_power_limit_id_set, NULL);
