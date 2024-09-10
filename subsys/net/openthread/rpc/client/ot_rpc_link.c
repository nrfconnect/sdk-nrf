/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nrf_rpc_cbor.h>

#include <openthread/link.h>

static otError decode_ot_error(struct nrf_rpc_cbor_ctx *ctx)
{
	otError error;

	if (!zcbor_uint_decode(ctx->zs, &error, sizeof(error))) {
		error = OT_ERROR_PARSE;
	}

	nrf_rpc_cbor_decoding_done(&ot_group, ctx);

	return error;
}

otError otLinkSetPollPeriod(otInstance *aInstance, uint32_t aPollPeriod)
{
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 5);

	if (!zcbor_uint_encode(ctx.zs, &aPollPeriod, sizeof(aPollPeriod))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		return OT_ERROR_INVALID_ARGS;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_LINK_SET_POLL_PERIOD, &ctx);

	return decode_ot_error(&ctx);
}

uint32_t otLinkGetPollPeriod(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint32_t poll_period = 0;
	bool decoded_ok;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_LINK_GET_POLL_PERIOD, &ctx);

	decoded_ok = zcbor_uint_decode(ctx.zs, &poll_period, sizeof(poll_period));
	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	if (!decoded_ok) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &ot_group,
			    OT_RPC_CMD_LINK_GET_POLL_PERIOD, NRF_RPC_PACKET_TYPE_RSP);
	}

	return poll_period;
}

void otLinkSetMaxFrameRetriesDirect(otInstance *aInstance, uint8_t aMaxFrameRetriesDirect)
{
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 2);
	nrf_rpc_encode_uint(&ctx, aMaxFrameRetriesDirect);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_DIRECT, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

void otLinkSetMaxFrameRetriesIndirect(otInstance *aInstance, uint8_t aMaxFrameRetriesIndirect)
{
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 2);
	nrf_rpc_encode_uint(&ctx, aMaxFrameRetriesIndirect);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_LINK_SET_MAX_FRAME_RETRIES_INDIRECT, &ctx,
				nrf_rpc_rsp_decode_void, NULL);
}

otError otLinkSetEnabled(otInstance *aInstance, bool aEnable)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	ARG_UNUSED(aInstance);

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

	ARG_UNUSED(aInstance);

	if (aEui64 == NULL) {
		return;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_LINK_GET_FACTORY_ASSIGNED_EUI64, &ctx,
				ot_rpc_decode_eui64, aEui64->m8);
}
