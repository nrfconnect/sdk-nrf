/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_common.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_macros.h>
#include <ot_rpc_os.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nrf_rpc_cbor.h>

#include <openthread/link_raw.h>

#define RADIO_TIME_REFRESH_PERIOD (CONFIG_OPENTHREAD_RPC_CLIENT_RADIO_TIME_REFRESH_PERIOD * 1000000)

static bool sync_radio_time_valid;
static uint64_t sync_local_time;
static uint64_t sync_radio_time;

uint64_t otLinkRawGetRadioTime(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint64_t now;

	OT_RPC_UNUSED(aInstance);

	now = ot_rpc_os_time_us();

	if (!sync_radio_time_valid || now >= sync_local_time + RADIO_TIME_REFRESH_PERIOD) {
		NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);
		nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_LINK_RAW_GET_RADIO_TIME, &ctx);

		/*
		 * Calculate sync_local_time as if the radio time was read by the peer exactly
		 * halfway between constructing a request and receiving the response.
		 */
		sync_radio_time_valid = true;
		sync_local_time = now / 2;
		now = ot_rpc_os_time_us();
		sync_local_time += now / 2;
		sync_radio_time = nrf_rpc_decode_uint64(&ctx);

		if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
			ot_rpc_report_rsp_decoding_error(OT_RPC_CMD_LINK_RAW_GET_RADIO_TIME);
		}
	}

	return sync_radio_time + (now - sync_local_time);
}
