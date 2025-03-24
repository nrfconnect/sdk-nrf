/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>
#include "ot_rpc_resource.h"

#include <nrf_rpc_cbor.h>

#include <zephyr/net/openthread.h>

#include <openthread/netdiag.h>

static size_t ot_rpc_network_diag_tlv_size(otNetworkDiagTlv *aNetworkDiagTlv)
{
	size_t size = 1 + sizeof(aNetworkDiagTlv->mType);

	switch (aNetworkDiagTlv->mType) {
	case OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS:
	case OT_NETWORK_DIAGNOSTIC_TLV_EUI64:
		return size + 1 + OT_EXT_ADDRESS_SIZE;
	case OT_NETWORK_DIAGNOSTIC_TLV_MODE:
		return size + 1;
	case OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY:
		return size + 2 + sizeof(uint16_t) + sizeof(int8_t) + 7 * (1 + sizeof(uint8_t));
	case OT_NETWORK_DIAGNOSTIC_TLV_ROUTE:
		return size + 6 + aNetworkDiagTlv->mData.mRoute.mRouteCount * 5;
	case OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA:
		return size + 1 + sizeof(uint32_t) + 4 * (1 + sizeof(uint8_t));
	case OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS:
		return size + 9 * (1 + sizeof(uint32_t));
	case OT_NETWORK_DIAGNOSTIC_TLV_MLE_COUNTERS:
		return size + 9 * (1 + sizeof(uint16_t)) + 6 * (1 + sizeof(uint64_t));
	case OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL:
		return size + 1 + sizeof(int8_t);
	case OT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT:
	case OT_NETWORK_DIAGNOSTIC_TLV_MAX_CHILD_TIMEOUT:
		return size + 1 + sizeof(uint32_t);
	case OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS:
	case OT_NETWORK_DIAGNOSTIC_TLV_SUPPLY_VOLTAGE:
	case OT_NETWORK_DIAGNOSTIC_TLV_VERSION:
		return size + 1 + sizeof(uint16_t);
	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_NAME:
		return size + 2 + strlen(aNetworkDiagTlv->mData.mVendorName);
	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_MODEL:
		return size + 2 + strlen(aNetworkDiagTlv->mData.mVendorModel);
	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_SW_VERSION:
		return size + 2 + strlen(aNetworkDiagTlv->mData.mVendorSwVersion);
	case OT_NETWORK_DIAGNOSTIC_TLV_THREAD_STACK_VERSION:
		return size + 2 + strlen(aNetworkDiagTlv->mData.mThreadStackVersion);
	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_APP_URL:
		return size + 2 + strlen(aNetworkDiagTlv->mData.mVendorAppUrl);
	case OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA:
		return size + 4 + aNetworkDiagTlv->mData.mNetworkData.mCount;
	case OT_NETWORK_DIAGNOSTIC_TLV_CHANNEL_PAGES:
		return size + 4 + aNetworkDiagTlv->mData.mChannelPages.mCount;
	case OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST:
		return size + 4 + aNetworkDiagTlv->mData.mIp6AddrList.mCount *
			(1 + OT_IP6_ADDRESS_SIZE);
	case OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE:
		return size + 4 + aNetworkDiagTlv->mData.mChildTable.mCount * 7;
	default:
		return size;
	}
}

static void ot_rpc_encode_network_diag_tlv(struct nrf_rpc_cbor_ctx *ctx,
					   otNetworkDiagTlv *aNetworkDiagTlv)
{
	nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mType);

	switch (aNetworkDiagTlv->mType) {
	case OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS:
		nrf_rpc_encode_buffer(ctx, aNetworkDiagTlv->mData.mExtAddress.m8,
				      OT_EXT_ADDRESS_SIZE);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_EUI64:
		nrf_rpc_encode_buffer(ctx, aNetworkDiagTlv->mData.mEui64.m8, OT_EXT_ADDRESS_SIZE);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_MODE:
		uint8_t mode = 0;

		WRITE_BIT(mode, 0, aNetworkDiagTlv->mData.mMode.mRxOnWhenIdle);
		WRITE_BIT(mode, 1, aNetworkDiagTlv->mData.mMode.mDeviceType);
		WRITE_BIT(mode, 2, aNetworkDiagTlv->mData.mMode.mNetworkData);
		nrf_rpc_encode_uint(ctx, mode);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY:
		nrf_rpc_encode_int(ctx, aNetworkDiagTlv->mData.mConnectivity.mParentPriority);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mConnectivity.mLinkQuality3);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mConnectivity.mLinkQuality2);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mConnectivity.mLinkQuality1);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mConnectivity.mLeaderCost);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mConnectivity.mIdSequence);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mConnectivity.mActiveRouters);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mConnectivity.mSedBufferSize);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mConnectivity.mSedDatagramCount);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_ROUTE:
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mRoute.mIdSequence);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mRoute.mRouteCount);
		zcbor_list_start_encode(ctx->zs, aNetworkDiagTlv->mData.mRoute.mRouteCount);
		for (int i = 0; i < aNetworkDiagTlv->mData.mRoute.mRouteCount; i++) {
			nrf_rpc_encode_uint(ctx,
				aNetworkDiagTlv->mData.mRoute.mRouteData[i].mRouterId);
			nrf_rpc_encode_uint(ctx,
				aNetworkDiagTlv->mData.mRoute.mRouteData[i].mLinkQualityIn);
			nrf_rpc_encode_uint(ctx,
				aNetworkDiagTlv->mData.mRoute.mRouteData[i].mLinkQualityOut);
			nrf_rpc_encode_uint(ctx,
				aNetworkDiagTlv->mData.mRoute.mRouteData[i].mRouteCost);
		}
		zcbor_list_end_encode(ctx->zs, aNetworkDiagTlv->mData.mRoute.mRouteCount);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA:
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mLeaderData.mPartitionId);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mLeaderData.mWeighting);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mLeaderData.mDataVersion);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mLeaderData.mStableDataVersion);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mLeaderData.mLeaderRouterId);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS:
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMacCounters.mIfInUnknownProtos);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMacCounters.mIfInErrors);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMacCounters.mIfOutErrors);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMacCounters.mIfInUcastPkts);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMacCounters.mIfInBroadcastPkts);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMacCounters.mIfInDiscards);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMacCounters.mIfOutUcastPkts);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMacCounters.mIfOutBroadcastPkts);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMacCounters.mIfOutDiscards);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_MLE_COUNTERS:
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMleCounters.mDisabledRole);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMleCounters.mDetachedRole);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMleCounters.mChildRole);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMleCounters.mRouterRole);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMleCounters.mLeaderRole);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMleCounters.mAttachAttempts);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMleCounters.mPartitionIdChanges);
		nrf_rpc_encode_uint(ctx,
				aNetworkDiagTlv->mData.mMleCounters.mBetterPartitionAttachAttempts);
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMleCounters.mParentChanges);
		nrf_rpc_encode_uint64(ctx, aNetworkDiagTlv->mData.mMleCounters.mTrackedTime);
		nrf_rpc_encode_uint64(ctx, aNetworkDiagTlv->mData.mMleCounters.mDisabledTime);
		nrf_rpc_encode_uint64(ctx, aNetworkDiagTlv->mData.mMleCounters.mDetachedTime);
		nrf_rpc_encode_uint64(ctx, aNetworkDiagTlv->mData.mMleCounters.mChildTime);
		nrf_rpc_encode_uint64(ctx, aNetworkDiagTlv->mData.mMleCounters.mRouterTime);
		nrf_rpc_encode_uint64(ctx, aNetworkDiagTlv->mData.mMleCounters.mLeaderTime);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL:
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mBatteryLevel);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT:
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mTimeout);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_MAX_CHILD_TIMEOUT:
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mMaxChildTimeout);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS:
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mAddr16);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_SUPPLY_VOLTAGE:
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mSupplyVoltage);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_VERSION:
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mVersion);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_NAME:
		nrf_rpc_encode_str(ctx, aNetworkDiagTlv->mData.mVendorName, -1);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_MODEL:
		nrf_rpc_encode_str(ctx, aNetworkDiagTlv->mData.mVendorModel, -1);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_SW_VERSION:
		nrf_rpc_encode_str(ctx, aNetworkDiagTlv->mData.mVendorSwVersion, -1);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_THREAD_STACK_VERSION:
		nrf_rpc_encode_str(ctx, aNetworkDiagTlv->mData.mThreadStackVersion, -1);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_APP_URL:
		nrf_rpc_encode_str(ctx, aNetworkDiagTlv->mData.mVendorAppUrl, -1);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA:
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mNetworkData.mCount);
		nrf_rpc_encode_buffer(ctx, aNetworkDiagTlv->mData.mNetworkData.m8,
				      aNetworkDiagTlv->mData.mNetworkData.mCount);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_CHANNEL_PAGES:
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mChannelPages.mCount);
		nrf_rpc_encode_buffer(ctx, aNetworkDiagTlv->mData.mChannelPages.m8,
				      aNetworkDiagTlv->mData.mChannelPages.mCount);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST:
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mIp6AddrList.mCount);
		zcbor_list_start_encode(ctx->zs, aNetworkDiagTlv->mData.mIp6AddrList.mCount);
		for (int i = 0; i < aNetworkDiagTlv->mData.mIp6AddrList.mCount; i++) {
			nrf_rpc_encode_buffer(ctx,
				aNetworkDiagTlv->mData.mIp6AddrList.mList[i].mFields.m8,
				OT_IP6_ADDRESS_SIZE);
		}
		zcbor_list_end_encode(ctx->zs, aNetworkDiagTlv->mData.mIp6AddrList.mCount);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE:
		nrf_rpc_encode_uint(ctx, aNetworkDiagTlv->mData.mChildTable.mCount);
		zcbor_list_start_encode(ctx->zs, aNetworkDiagTlv->mData.mChildTable.mCount);
		for (int i = 0; i < aNetworkDiagTlv->mData.mChildTable.mCount; i++) {
			uint8_t mode = 0;

			nrf_rpc_encode_uint(ctx,
				aNetworkDiagTlv->mData.mChildTable.mTable[i].mTimeout);
			nrf_rpc_encode_uint(ctx,
				aNetworkDiagTlv->mData.mChildTable.mTable[i].mLinkQuality);
			nrf_rpc_encode_uint(ctx,
				aNetworkDiagTlv->mData.mChildTable.mTable[i].mChildId);
			WRITE_BIT(mode, 0,
				  aNetworkDiagTlv->mData.mChildTable.mTable[i].mMode.mRxOnWhenIdle);
			WRITE_BIT(mode, 1,
				  aNetworkDiagTlv->mData.mChildTable.mTable[i].mMode.mDeviceType);
			WRITE_BIT(mode, 2,
				  aNetworkDiagTlv->mData.mChildTable.mTable[i].mMode.mNetworkData);
			nrf_rpc_encode_uint(ctx, mode);
		}
		zcbor_list_end_encode(ctx->zs, aNetworkDiagTlv->mData.mChildTable.mCount);
		break;
	default:
		break;
	}
}

static void ot_rpc_cmd_get_next_diagnostic_tlv(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	ot_rpc_res_tab_key message_rep;
	const otMessage *message;
	otNetworkDiagIterator iterator;
	otNetworkDiagTlv tlv;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	message_rep = nrf_rpc_decode_uint(ctx);
	iterator = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV);
		return;
	}

	message = ot_res_tab_msg_get(message_rep);

	if (!message) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otThreadGetNextDiagnosticTlv(message, &iterator, &tlv);
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (error != OT_ERROR_NONE) {
		nrf_rpc_rsp_send_uint(group, error);
		return;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5 + ot_rpc_network_diag_tlv_size(&tlv));

	nrf_rpc_encode_uint(&rsp_ctx, error);
	nrf_rpc_encode_uint(&rsp_ctx, iterator);
	ot_rpc_encode_network_diag_tlv(&rsp_ctx, &tlv);

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

void handle_receive_diagnostic_get(otError aError, otMessage *aMessage,
					  const otMessageInfo *aMessageInfo, void *aContext)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_rpc_res_tab_key message_rep;

	ARG_UNUSED(aContext);

	message_rep = ot_res_tab_msg_alloc(aMessage);

	if (message_rep == 0) {
		nrf_rpc_err(-ENOMEM, NRF_RPC_ERR_SRC_SEND, &ot_group,
			    OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_GET_CB, NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 3 + sizeof(message_rep) +
			   OT_RPC_MESSAGE_INFO_LENGTH(aMessageInfo));

	nrf_rpc_encode_uint(&ctx, aError);
	nrf_rpc_encode_uint(&ctx, message_rep);
	ot_rpc_encode_message_info(&ctx, aMessageInfo);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_GET_CB, &ctx,
				nrf_rpc_rsp_decode_void, NULL);

	ot_res_tab_msg_free(message_rep);
}

static void ot_rpc_cmd_send_diagnostic_get(const struct nrf_rpc_group *group,
					   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otIp6Address addr;
	otError error = OT_ERROR_INVALID_ARGS;
	size_t count;
	const uint8_t *tlvTypes;

	nrf_rpc_decode_buffer(ctx, addr.mFields.m8, OT_IP6_ADDRESS_SIZE);

	tlvTypes = nrf_rpc_decode_buffer_ptr_and_size(ctx, &count);

	if (tlvTypes) {
		openthread_api_mutex_lock(openthread_get_default_context());
		error = otThreadSendDiagnosticGet(openthread_get_default_instance(), &addr,
						  tlvTypes, count, handle_receive_diagnostic_get,
						  NULL);
		openthread_api_mutex_unlock(openthread_get_default_context());
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_GET);
		return;
	}

	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_cmd_send_diagnostic_reset(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otIp6Address addr;
	otError error = OT_ERROR_INVALID_ARGS;
	size_t count;
	const uint8_t *tlvTypes;

	nrf_rpc_decode_buffer(ctx, addr.mFields.m8, OT_IP6_ADDRESS_SIZE);

	tlvTypes = nrf_rpc_decode_buffer_ptr_and_size(ctx, &count);

	if (tlvTypes) {
		openthread_api_mutex_lock(openthread_get_default_context());
		error = otThreadSendDiagnosticReset(openthread_get_default_instance(), &addr,
						    tlvTypes, count);
		openthread_api_mutex_unlock(openthread_get_default_context());
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_RESET);
		return;
	}

	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_cmd_set_vendor_name(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	char buffer[256];

	nrf_rpc_decode_str(ctx, buffer, sizeof(buffer));

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_THREAD_SET_VENDOR_NAME);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otThreadSetVendorName(openthread_get_default_instance(), buffer);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_cmd_set_vendor_model(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	char buffer[256];

	nrf_rpc_decode_str(ctx, buffer, sizeof(buffer));

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_THREAD_SET_VENDOR_MODEL);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otThreadSetVendorModel(openthread_get_default_instance(), buffer);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_cmd_set_vendor_sw_version(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	char buffer[256];

	nrf_rpc_decode_str(ctx, buffer, sizeof(buffer));

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otThreadSetVendorSwVersion(openthread_get_default_instance(), buffer);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_cmd_get_vendor_name(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const char *data;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	data = otThreadGetVendorName(openthread_get_default_instance());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 2 + strlen(data));
	nrf_rpc_encode_str(&rsp_ctx, data, strlen(data));
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_get_vendor_model(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const char *data;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	data = otThreadGetVendorModel(openthread_get_default_instance());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 2 + strlen(data));
	nrf_rpc_encode_str(&rsp_ctx, data, strlen(data));
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_get_vendor_sw_version(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const char *data;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	data = otThreadGetVendorSwVersion(openthread_get_default_instance());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 2 + strlen(data));
	nrf_rpc_encode_str(&rsp_ctx, data, strlen(data));
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_next_diagnostic_tlv,
			 OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV,
			 ot_rpc_cmd_get_next_diagnostic_tlv, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_send_diagnostic_reset,
			 OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_RESET,
			 ot_rpc_cmd_send_diagnostic_reset, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_send_diagnostic_get,
			 OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_GET,
			 ot_rpc_cmd_send_diagnostic_get, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_vendor_name, OT_RPC_CMD_THREAD_SET_VENDOR_NAME,
			 ot_rpc_cmd_set_vendor_name, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_vendor_model, OT_RPC_CMD_THREAD_SET_VENDOR_MODEL,
			 ot_rpc_cmd_set_vendor_model, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_vendor_sw_version,
			 OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION,
			 ot_rpc_cmd_set_vendor_sw_version, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_vendor_name, OT_RPC_CMD_THREAD_GET_VENDOR_NAME,
			 ot_rpc_cmd_get_vendor_name, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_vendor_model, OT_RPC_CMD_THREAD_GET_VENDOR_MODEL,
			 ot_rpc_cmd_get_vendor_model, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_vendor_sw_version,
			 OT_RPC_CMD_THREAD_GET_VENDOR_SW_VERSION,
			 ot_rpc_cmd_get_vendor_sw_version, NULL);
