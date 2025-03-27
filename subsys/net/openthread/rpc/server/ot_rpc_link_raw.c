/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>

#include <nrf_rpc_cbor.h>

#include <zephyr/net/openthread.h>

#include <openthread/link_raw.h>

static void ot_rpc_cmd_get_radio_time(const struct nrf_rpc_group *group,
				      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint64_t time;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_LINK_RAW_GET_RADIO_TIME);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	time = otLinkRawGetRadioTime(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 1 + sizeof(time));
	nrf_rpc_encode_uint64(&rsp_ctx, time);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_radio_time, OT_RPC_CMD_LINK_RAW_GET_RADIO_TIME,
			 ot_rpc_cmd_get_radio_time, NULL);
