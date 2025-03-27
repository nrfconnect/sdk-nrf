/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nrf_rpc_cbor.h>

#include <openthread/link_raw.h>

uint64_t otLinkRawGetRadioTime(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint64_t time;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);
	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_LINK_RAW_GET_RADIO_TIME, &ctx);

	time = nrf_rpc_decode_uint64(&ctx);

	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_rsp_decoding_error(OT_RPC_CMD_LINK_RAW_GET_RADIO_TIME);
	}

	return time;
}
