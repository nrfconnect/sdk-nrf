/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>
#include <ot_rpc_macros.h>
#include <ot_rpc_os.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nrf_rpc_cbor.h>

#include <openthread/link.h>

otError otLinkSetPollPeriod(otInstance *aInstance, uint32_t aPollPeriod)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 5);
	nrf_rpc_encode_uint(&ctx, aPollPeriod);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_LINK_SET_POLL_PERIOD, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}

uint32_t otLinkGetPollPeriod(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint32_t poll_period;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_LINK_GET_POLL_PERIOD, &ctx);
	poll_period = nrf_rpc_decode_uint(&ctx);

	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_rsp_decoding_error(OT_RPC_CMD_LINK_GET_POLL_PERIOD);
	}

	return poll_period;
}

void otLinkSetMaxFrameRetriesDirect(otInstance *aInstance, uint8_t aMaxFrameRetriesDirect)
{
	struct nrf_rpc_cbor_ctx ctx;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 2);
	nrf_rpc_encode_uint(&ctx, aMaxFrameRetriesDirect);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_DIRECT, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

void otLinkSetMaxFrameRetriesIndirect(otInstance *aInstance, uint8_t aMaxFrameRetriesIndirect)
{
	struct nrf_rpc_cbor_ctx ctx;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 2);
	nrf_rpc_encode_uint(&ctx, aMaxFrameRetriesIndirect);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_INDIRECT, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

otError otLinkSetEnabled(otInstance *aInstance, bool aEnable)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1);
	nrf_rpc_encode_bool(&ctx, aEnable);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_LINK_SET_ENABLED, &ctx, ot_rpc_decode_error,
				&error);

	return error;
}

static void ot_rpc_decode_eui64(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				void *handler_data)
{
	nrf_rpc_decode_buffer(ctx, handler_data, OT_EXT_ADDRESS_SIZE);
}

void otLinkGetFactoryAssignedIeeeEui64(otInstance *aInstance, otExtAddress *aEui64)
{
	struct nrf_rpc_cbor_ctx ctx;

	OT_RPC_UNUSED(aInstance);

	OT_RPC_ASSERT(aEui64 != NULL);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_LINK_GET_FACTORY_ASSIGNED_EUI64, &ctx,
				ot_rpc_decode_eui64, aEui64->m8);
}

const otMacCounters *otLinkGetCounters(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	static otMacCounters counters;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);
	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_LINK_GET_COUNTERS, &ctx);
	counters.mTxTotal = nrf_rpc_decode_uint(&ctx);
	counters.mTxUnicast = nrf_rpc_decode_uint(&ctx);
	counters.mTxBroadcast = nrf_rpc_decode_uint(&ctx);
	counters.mTxAckRequested = nrf_rpc_decode_uint(&ctx);
	counters.mTxAcked = nrf_rpc_decode_uint(&ctx);
	counters.mTxNoAckRequested = nrf_rpc_decode_uint(&ctx);
	counters.mTxData = nrf_rpc_decode_uint(&ctx);
	counters.mTxDataPoll = nrf_rpc_decode_uint(&ctx);
	counters.mTxBeacon = nrf_rpc_decode_uint(&ctx);
	counters.mTxBeaconRequest = nrf_rpc_decode_uint(&ctx);
	counters.mTxOther = nrf_rpc_decode_uint(&ctx);
	counters.mTxRetry = nrf_rpc_decode_uint(&ctx);
	counters.mTxDirectMaxRetryExpiry = nrf_rpc_decode_uint(&ctx);
	counters.mTxIndirectMaxRetryExpiry = nrf_rpc_decode_uint(&ctx);
	counters.mTxErrCca = nrf_rpc_decode_uint(&ctx);
	counters.mTxErrAbort = nrf_rpc_decode_uint(&ctx);
	counters.mTxErrBusyChannel = nrf_rpc_decode_uint(&ctx);
	counters.mRxTotal = nrf_rpc_decode_uint(&ctx);
	counters.mRxUnicast = nrf_rpc_decode_uint(&ctx);
	counters.mRxBroadcast = nrf_rpc_decode_uint(&ctx);
	counters.mRxData = nrf_rpc_decode_uint(&ctx);
	counters.mRxDataPoll = nrf_rpc_decode_uint(&ctx);
	counters.mRxBeacon = nrf_rpc_decode_uint(&ctx);
	counters.mRxBeaconRequest = nrf_rpc_decode_uint(&ctx);
	counters.mRxOther = nrf_rpc_decode_uint(&ctx);
	counters.mRxAddressFiltered = nrf_rpc_decode_uint(&ctx);
	counters.mRxDestAddrFiltered = nrf_rpc_decode_uint(&ctx);
	counters.mRxDuplicated = nrf_rpc_decode_uint(&ctx);
	counters.mRxErrNoFrame = nrf_rpc_decode_uint(&ctx);
	counters.mRxErrUnknownNeighbor = nrf_rpc_decode_uint(&ctx);
	counters.mRxErrInvalidSrcAddr = nrf_rpc_decode_uint(&ctx);
	counters.mRxErrSec = nrf_rpc_decode_uint(&ctx);
	counters.mRxErrFcs = nrf_rpc_decode_uint(&ctx);
	counters.mRxErrOther = nrf_rpc_decode_uint(&ctx);

	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_rsp_decoding_error(OT_RPC_CMD_LINK_GET_COUNTERS);
	}

	return &counters;
}

otError ot_rpc_set_factory_assigned_ieee_eui64(const otExtAddress *eui64)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	OT_RPC_ASSERT(eui64 != NULL);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + OT_EXT_ADDRESS_SIZE);
	nrf_rpc_encode_buffer(&ctx, &eui64->m8, OT_EXT_ADDRESS_SIZE);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_LINK_SET_FACTORY_ASSIGNED_EUI64, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}
