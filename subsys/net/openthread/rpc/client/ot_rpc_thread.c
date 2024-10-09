/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/nrf_rpc_cbkproxy.h>

#include <nrf_rpc_cbor.h>

#include <openthread/thread.h>

static otError decode_ot_error(struct nrf_rpc_cbor_ctx *ctx)
{
	otError error;

	if (!zcbor_uint_decode(ctx->zs, &error, sizeof(error))) {
		error = OT_ERROR_PARSE;
	}

	nrf_rpc_cbor_decoding_done(&ot_group, ctx);

	return error;
}

static void ot_rpc_thread_discover_cb_rpc_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otHandleActiveScanResult callback;
	otActiveScanResult result;
	void *context;
	bool is_result_present =
		NULL != nrf_rpc_decode_buffer(ctx, result.mExtAddress.m8, OT_EXT_ADDRESS_SIZE);

	if (is_result_present) {
		nrf_rpc_decode_str(ctx, result.mNetworkName.m8, OT_NETWORK_NAME_MAX_SIZE + 1);
		nrf_rpc_decode_buffer(ctx, result.mExtendedPanId.m8, OT_EXT_PAN_ID_SIZE);
		nrf_rpc_decode_buffer(ctx, result.mSteeringData.m8, OT_STEERING_DATA_MAX_LENGTH);

		result.mPanId = nrf_rpc_decode_uint(ctx);
		result.mJoinerUdpPort = nrf_rpc_decode_uint(ctx);
		result.mChannel = nrf_rpc_decode_uint(ctx);
		result.mRssi = nrf_rpc_decode_int(ctx);
		result.mLqi = nrf_rpc_decode_uint(ctx);
		result.mVersion = nrf_rpc_decode_uint(ctx);
		result.mIsNative = nrf_rpc_decode_bool(ctx);
		result.mDiscover = nrf_rpc_decode_bool(ctx);
		result.mIsJoinable = nrf_rpc_decode_bool(ctx);
	}
	context = (void *)nrf_rpc_decode_uint(ctx);
	callback = (otHandleActiveScanResult)nrf_rpc_decode_callback_call(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_THREAD_DISCOVER_CB);
		return;
	}

	if (callback) {
		callback(is_result_present ? &result : NULL, context);
	}

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_thread_discover_cb, OT_RPC_CMD_THREAD_DISCOVER_CB,
			 ot_rpc_thread_discover_cb_rpc_handler, NULL);

otError otThreadDiscover(otInstance *aInstance, uint32_t aScanChannels, uint16_t aPanId,
			 bool aJoiner, bool aEnableEui64Filtering,
			 otHandleActiveScanResult aCallback, void *aCallbackContext)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 0;
	otError error;

	ARG_UNUSED(aInstance);

	cbor_buffer_size += sizeof(uint32_t) + 1;  /* aScanChannels */
	cbor_buffer_size += sizeof(uint16_t) + 1;  /* aPanId */
	cbor_buffer_size += 1;			   /* aJoiner */
	cbor_buffer_size += 1;			   /* aEnableEui64Filtering */
	cbor_buffer_size += sizeof(uintptr_t) + 1; /* aCallback */
	cbor_buffer_size += sizeof(uint32_t) + 1;  /* aCallbackContext */

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);

	nrf_rpc_encode_uint(&ctx, aScanChannels);
	nrf_rpc_encode_uint(&ctx, aPanId);
	nrf_rpc_encode_bool(&ctx, aJoiner);
	nrf_rpc_encode_bool(&ctx, aEnableEui64Filtering);
	nrf_rpc_encode_callback(&ctx, aCallback);
	nrf_rpc_encode_uint(&ctx, (uintptr_t)aCallbackContext);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_THREAD_DISCOVER, &ctx, ot_rpc_decode_error,
				&error);

	return error;
}

otError otThreadSetEnabled(otInstance *aInstance, bool aEnabled)
{
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1);

	if (!zcbor_bool_encode(ctx.zs, &aEnabled)) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		return OT_ERROR_INVALID_ARGS;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_THREAD_SET_ENABLED, &ctx);

	return decode_ot_error(&ctx);
}

otDeviceRole otThreadGetDeviceRole(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	otDeviceRole role = 0;
	bool decoded_ok;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_THREAD_GET_DEVICE_ROLE, &ctx);

	decoded_ok = zcbor_uint_decode(ctx.zs, &role, sizeof(role));
	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	if (!decoded_ok) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &ot_group,
			    OT_RPC_CMD_THREAD_GET_DEVICE_ROLE, NRF_RPC_PACKET_TYPE_RSP);
	}

	return role;
}

otError otThreadSetLinkMode(otInstance *aInstance, otLinkModeConfig aConfig)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint8_t mode_mask = 0;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 2);

	if (aConfig.mRxOnWhenIdle) {
		mode_mask |= BIT(OT_RPC_LINK_MODE_RX_ON_WHEN_IDLE_OFFSET);
	}

	if (aConfig.mDeviceType) {
		mode_mask |= BIT(OT_RPC_LINK_MODE_DEVICE_TYPE_OFFSET);
	}

	if (aConfig.mNetworkData) {
		mode_mask |= BIT(OT_RPC_LINK_MODE_NETWORK_DATA_OFFSET);
	}

	if (!zcbor_uint_encode(ctx.zs, &mode_mask, sizeof(mode_mask))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		return OT_ERROR_INVALID_ARGS;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_THREAD_SET_LINK_MODE, &ctx);

	return decode_ot_error(&ctx);
}

otLinkModeConfig otThreadGetLinkMode(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint8_t mode_mask = 0;
	otLinkModeConfig mode;
	bool decoded_ok;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_THREAD_GET_LINK_MODE, &ctx);

	decoded_ok = zcbor_uint_decode(ctx.zs, &mode_mask, sizeof(mode_mask));
	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	if (!decoded_ok) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &ot_group,
			    OT_RPC_CMD_THREAD_GET_LINK_MODE, NRF_RPC_PACKET_TYPE_RSP);
	}

	mode.mRxOnWhenIdle = (mode_mask & BIT(OT_RPC_LINK_MODE_RX_ON_WHEN_IDLE_OFFSET)) != 0;
	mode.mDeviceType = (mode_mask & BIT(OT_RPC_LINK_MODE_DEVICE_TYPE_OFFSET)) != 0;
	mode.mNetworkData = (mode_mask & BIT(OT_RPC_LINK_MODE_NETWORK_DATA_OFFSET)) != 0;

	return mode;
}

uint16_t otThreadGetVersion(void)
{
	struct nrf_rpc_cbor_ctx ctx;
	uint16_t version;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_THREAD_GET_VERSION, &ctx,
				nrf_rpc_rsp_decode_u16, &version);

	return version;
}

const char *otThreadErrorToString(otError error)
{
	static const char *const error_str[OT_NUM_ERRORS] = {
		"OK",			      /* (0)  kErrorNone */
		"Failed",		      /* (1)  kErrorFailed */
		"Drop",			      /* (2)  kErrorDrop */
		"NoBufs",		      /* (3)  kErrorNoBufs */
		"NoRoute",		      /* (4)  kErrorNoRoute */
		"Busy",			      /* (5)  kErrorBusy */
		"Parse",		      /* (6)  kErrorParse */
		"InvalidArgs",		      /* (7)  kErrorInvalidArgs */
		"Security",		      /* (8)  kErrorSecurity */
		"AddressQuery",		      /* (9)  kErrorAddressQuery */
		"NoAddress",		      /* (10) kErrorNoAddress */
		"Abort",		      /* (11) kErrorAbort */
		"NotImplemented",	      /* (12) kErrorNotImplemented */
		"InvalidState",		      /* (13) kErrorInvalidState */
		"NoAck",		      /* (14) kErrorNoAck */
		"ChannelAccessFailure",	      /* (15) kErrorChannelAccessFailure */
		"Detached",		      /* (16) kErrorDetached */
		"FcsErr",		      /* (17) kErrorFcs */
		"NoFrameReceived",	      /* (18) kErrorNoFrameReceived */
		"UnknownNeighbor",	      /* (19) kErrorUnknownNeighbor */
		"InvalidSourceAddress",	      /* (20) kErrorInvalidSourceAddress */
		"AddressFiltered",	      /* (21) kErrorAddressFiltered */
		"DestinationAddressFiltered", /* (22) kErrorDestinationAddressFiltered */
		"NotFound",		      /* (23) kErrorNotFound */
		"Already",		      /* (24) kErrorAlready */
		"ReservedError25",	      /* (25) Error 25 is reserved */
		"Ipv6AddressCreationFailure", /* (26) kErrorIp6AddressCreationFailure */
		"NotCapable",		      /* (27) kErrorNotCapable */
		"ResponseTimeout",	      /* (28) kErrorResponseTimeout */
		"Duplicated",		      /* (29) kErrorDuplicated */
		"ReassemblyTimeout",	      /* (30) kErrorReassemblyTimeout */
		"NotTmf",		      /* (31) kErrorNotTmf */
		"NonLowpanDataFrame",	      /* (32) kErrorNotLowpanDataFrame */
		"ReservedError33",	      /* (33) Error 33 is reserved */
		"LinkMarginLow",	      /* (34) kErrorLinkMarginLow */
		"InvalidCommand",	      /* (35) kErrorInvalidCommand */
		"Pending",		      /* (36) kErrorPending */
		"Rejected",		      /* (37) kErrorRejected */
	};

	return error < OT_NUM_ERRORS ? error_str[error] : "UnknownErrorType";
}
