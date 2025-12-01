/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_client_common.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_lock.h>
#include <ot_rpc_macros.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <openthread/netdiag.h>

static char vendor_name[OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH + 1];
static char vendor_model[OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH + 1];
static char vendor_sw_version[OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH + 1];

static otReceiveDiagnosticGetCallback receive_diag_get_cb;
static void *receive_diag_get_cb_context;

void ot_rpc_decode_network_diag_tlv(struct nrf_rpc_cbor_ctx *ctx, otNetworkDiagTlv *aNetworkDiagTlv)
{
	uint8_t mode;

	aNetworkDiagTlv->mType = nrf_rpc_decode_uint(ctx);

	switch (aNetworkDiagTlv->mType) {
	case OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS:
		nrf_rpc_decode_buffer(ctx, aNetworkDiagTlv->mData.mExtAddress.m8,
				      OT_EXT_ADDRESS_SIZE);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_EUI64:
		nrf_rpc_decode_buffer(ctx, aNetworkDiagTlv->mData.mEui64.m8, OT_EXT_ADDRESS_SIZE);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_MODE:
		mode = nrf_rpc_decode_uint(ctx);

		aNetworkDiagTlv->mData.mMode.mRxOnWhenIdle = (bool)(mode & BIT(0));
		aNetworkDiagTlv->mData.mMode.mDeviceType = (bool)(mode & BIT(1));
		aNetworkDiagTlv->mData.mMode.mNetworkData = (bool)(mode & BIT(2));
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY:
		aNetworkDiagTlv->mData.mConnectivity.mParentPriority = nrf_rpc_decode_int(ctx);
		aNetworkDiagTlv->mData.mConnectivity.mLinkQuality3 = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mConnectivity.mLinkQuality2 = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mConnectivity.mLinkQuality1 = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mConnectivity.mLeaderCost = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mConnectivity.mIdSequence = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mConnectivity.mActiveRouters = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mConnectivity.mSedBufferSize = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mConnectivity.mSedDatagramCount = nrf_rpc_decode_uint(ctx);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_ROUTE:
		aNetworkDiagTlv->mData.mRoute.mIdSequence = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mRoute.mRouteCount = nrf_rpc_decode_uint(ctx);

		zcbor_list_start_decode(ctx->zs);
		for (int i = 0; i < aNetworkDiagTlv->mData.mRoute.mRouteCount; i++) {
			aNetworkDiagTlv->mData.mRoute.mRouteData[i].mRouterId =
				nrf_rpc_decode_uint(ctx);
			aNetworkDiagTlv->mData.mRoute.mRouteData[i].mLinkQualityIn =
				nrf_rpc_decode_uint(ctx);
			aNetworkDiagTlv->mData.mRoute.mRouteData[i].mLinkQualityOut =
				nrf_rpc_decode_uint(ctx);
			aNetworkDiagTlv->mData.mRoute.mRouteData[i].mRouteCost =
				nrf_rpc_decode_uint(ctx);
		}
		zcbor_list_end_decode(ctx->zs);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA:
		aNetworkDiagTlv->mData.mLeaderData.mPartitionId = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mLeaderData.mWeighting = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mLeaderData.mDataVersion = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mLeaderData.mStableDataVersion = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mLeaderData.mLeaderRouterId = nrf_rpc_decode_uint(ctx);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS:
		aNetworkDiagTlv->mData.mMacCounters.mIfInUnknownProtos = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMacCounters.mIfInErrors = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMacCounters.mIfOutErrors = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMacCounters.mIfInUcastPkts = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMacCounters.mIfInBroadcastPkts = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMacCounters.mIfInDiscards = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMacCounters.mIfOutUcastPkts = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMacCounters.mIfOutBroadcastPkts = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMacCounters.mIfOutDiscards = nrf_rpc_decode_uint(ctx);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_MLE_COUNTERS:
		aNetworkDiagTlv->mData.mMleCounters.mDisabledRole = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMleCounters.mDetachedRole = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMleCounters.mChildRole = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMleCounters.mRouterRole = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMleCounters.mLeaderRole = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMleCounters.mAttachAttempts = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMleCounters.mPartitionIdChanges = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMleCounters.mBetterPartitionAttachAttempts =
			nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMleCounters.mParentChanges = nrf_rpc_decode_uint(ctx);
		aNetworkDiagTlv->mData.mMleCounters.mTrackedTime = nrf_rpc_decode_uint64(ctx);
		aNetworkDiagTlv->mData.mMleCounters.mDisabledTime = nrf_rpc_decode_uint64(ctx);
		aNetworkDiagTlv->mData.mMleCounters.mDetachedTime = nrf_rpc_decode_uint64(ctx);
		aNetworkDiagTlv->mData.mMleCounters.mChildTime = nrf_rpc_decode_uint64(ctx);
		aNetworkDiagTlv->mData.mMleCounters.mRouterTime = nrf_rpc_decode_uint64(ctx);
		aNetworkDiagTlv->mData.mMleCounters.mLeaderTime = nrf_rpc_decode_uint64(ctx);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL:
		aNetworkDiagTlv->mData.mBatteryLevel = nrf_rpc_decode_uint(ctx);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT:
		aNetworkDiagTlv->mData.mTimeout = nrf_rpc_decode_uint(ctx);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_MAX_CHILD_TIMEOUT:
		aNetworkDiagTlv->mData.mMaxChildTimeout = nrf_rpc_decode_uint(ctx);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS:
		aNetworkDiagTlv->mData.mAddr16 = nrf_rpc_decode_uint(ctx);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_SUPPLY_VOLTAGE:
		aNetworkDiagTlv->mData.mSupplyVoltage = nrf_rpc_decode_uint(ctx);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_VERSION:
		aNetworkDiagTlv->mData.mVersion = nrf_rpc_decode_uint(ctx);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_NAME:
		nrf_rpc_decode_str(ctx, aNetworkDiagTlv->mData.mVendorName,
				   sizeof(aNetworkDiagTlv->mData.mVendorName));
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_MODEL:
		nrf_rpc_decode_str(ctx, aNetworkDiagTlv->mData.mVendorModel,
				   sizeof(aNetworkDiagTlv->mData.mVendorModel));
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_SW_VERSION:
		nrf_rpc_decode_str(ctx, aNetworkDiagTlv->mData.mVendorSwVersion,
				   sizeof(aNetworkDiagTlv->mData.mVendorSwVersion));
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_THREAD_STACK_VERSION:
		nrf_rpc_decode_str(ctx, aNetworkDiagTlv->mData.mThreadStackVersion,
				   sizeof(aNetworkDiagTlv->mData.mThreadStackVersion));
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_APP_URL:
		nrf_rpc_decode_str(ctx, aNetworkDiagTlv->mData.mVendorAppUrl,
				   sizeof(aNetworkDiagTlv->mData.mVendorAppUrl));
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA:
		aNetworkDiagTlv->mData.mNetworkData.mCount = nrf_rpc_decode_uint(ctx);
		nrf_rpc_decode_buffer(ctx, aNetworkDiagTlv->mData.mNetworkData.m8,
				      sizeof(aNetworkDiagTlv->mData.mNetworkData.m8));
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_CHANNEL_PAGES:
		aNetworkDiagTlv->mData.mChannelPages.mCount = nrf_rpc_decode_uint(ctx);
		nrf_rpc_decode_buffer(ctx, aNetworkDiagTlv->mData.mChannelPages.m8,
				      sizeof(aNetworkDiagTlv->mData.mChannelPages.m8));
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST:
		aNetworkDiagTlv->mData.mIp6AddrList.mCount = nrf_rpc_decode_uint(ctx);
		zcbor_list_start_decode(ctx->zs);
		for (int i = 0; i < aNetworkDiagTlv->mData.mIp6AddrList.mCount; i++) {
			nrf_rpc_decode_buffer(ctx,
				aNetworkDiagTlv->mData.mIp6AddrList.mList[i].mFields.m8,
				OT_IP6_ADDRESS_SIZE);
		}
		zcbor_list_end_decode(ctx->zs);
		break;
	case OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE:
		aNetworkDiagTlv->mData.mChildTable.mCount = nrf_rpc_decode_uint(ctx);
		zcbor_list_start_decode(ctx->zs);
		for (int i = 0; i < aNetworkDiagTlv->mData.mChildTable.mCount; i++) {
			uint8_t mode;

			aNetworkDiagTlv->mData.mChildTable.mTable[i].mTimeout =
				nrf_rpc_decode_uint(ctx);
			aNetworkDiagTlv->mData.mChildTable.mTable[i].mLinkQuality =
				nrf_rpc_decode_uint(ctx);
			aNetworkDiagTlv->mData.mChildTable.mTable[i].mChildId =
				nrf_rpc_decode_uint(ctx);
			mode = nrf_rpc_decode_uint(ctx);
			aNetworkDiagTlv->mData.mChildTable.mTable[i].mMode.mRxOnWhenIdle =
				(bool)(mode & BIT(0));
			aNetworkDiagTlv->mData.mChildTable.mTable[i].mMode.mDeviceType =
				(bool)(mode & BIT(1));
			aNetworkDiagTlv->mData.mChildTable.mTable[i].mMode.mNetworkData =
				(bool)(mode & BIT(2));
		}
		zcbor_list_end_decode(ctx->zs);
		break;
	default:
		break;
	}
}

otError otThreadGetNextDiagnosticTlv(const otMessage *aMessage, otNetworkDiagIterator *aIterator,
				     otNetworkDiagTlv *aNetworkDiagTlv)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(ot_rpc_res_tab_key) + 2 +
					   sizeof(otNetworkDiagIterator));
	nrf_rpc_encode_uint(&ctx, (ot_rpc_res_tab_key)aMessage);
	nrf_rpc_encode_uint(&ctx, *aIterator);
	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV, &ctx);

	error = nrf_rpc_decode_uint(&ctx);

	if (error != OT_ERROR_NONE) {
		goto exit;
	}

	*aIterator = nrf_rpc_decode_uint(&ctx);
	ot_rpc_decode_network_diag_tlv(&ctx, aNetworkDiagTlv);

exit:
	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_rsp_decoding_error(OT_RPC_CMD_THREAD_GET_NEXT_DIAGNOSTIC_TLV);
	}

	return error;
}

static void ot_rpc_cmd_thread_send_diagnostic_get_cb(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	otError error;
	otMessage *message;
	otMessageInfo message_info;

	error = nrf_rpc_decode_uint(ctx);
	message = (otMessage *)nrf_rpc_decode_uint(ctx);
	ot_rpc_decode_message_info(ctx, &message_info);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_GET_CB);
		return;
	}

	ot_rpc_mutex_lock();

	if (receive_diag_get_cb != NULL) {
		receive_diag_get_cb(error, message, &message_info, receive_diag_get_cb_context);
	}

	ot_rpc_mutex_unlock();
	nrf_rpc_rsp_send_void(group);
}

otError otThreadSendDiagnosticGet(otInstance *aInstance, const otIp6Address *aDestination,
				  const uint8_t *aTlvTypes, uint8_t aCount,
				  otReceiveDiagnosticGetCallback aCallback, void *aCallbackContext)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	OT_RPC_UNUSED(aInstance);

	receive_diag_get_cb = aCallback;
	receive_diag_get_cb_context = aCallbackContext;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + OT_IP6_ADDRESS_SIZE + 2 + aCount);
	nrf_rpc_encode_buffer(&ctx, aDestination->mFields.m8, OT_IP6_ADDRESS_SIZE);
	nrf_rpc_encode_buffer(&ctx, aTlvTypes, aCount);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_GET,
				&ctx, ot_rpc_decode_error, &error);

	return error;
}

otError otThreadSendDiagnosticReset(otInstance *aInstance, const otIp6Address *aDestination,
				    const uint8_t aTlvTypes[], uint8_t aCount)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + OT_IP6_ADDRESS_SIZE + 2 + aCount);
	nrf_rpc_encode_buffer(&ctx, aDestination->mFields.m8, OT_IP6_ADDRESS_SIZE);
	nrf_rpc_encode_buffer(&ctx, aTlvTypes, aCount);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_RESET,
				&ctx, ot_rpc_decode_error, &error);

	return error;
}

otError otThreadSetVendorName(otInstance *aInstance, const char *aVendorName)
{
	OT_RPC_UNUSED(aInstance);

	return ot_rpc_set_string(OT_RPC_CMD_THREAD_SET_VENDOR_NAME, aVendorName);
}

otError otThreadSetVendorModel(otInstance *aInstance, const char *aVendorModel)
{
	OT_RPC_UNUSED(aInstance);

	return ot_rpc_set_string(OT_RPC_CMD_THREAD_SET_VENDOR_MODEL, aVendorModel);
}

otError otThreadSetVendorSwVersion(otInstance *aInstance, const char *aVendorSwVersion)
{
	OT_RPC_UNUSED(aInstance);

	return ot_rpc_set_string(OT_RPC_CMD_THREAD_SET_VENDOR_SW_VERSION, aVendorSwVersion);
}

const char *otThreadGetVendorName(otInstance *aInstance)
{
	OT_RPC_UNUSED(aInstance);

	ot_rpc_get_string(OT_RPC_CMD_THREAD_GET_VENDOR_NAME, vendor_name, sizeof(vendor_name));
	return vendor_name;
}

const char *otThreadGetVendorModel(otInstance *aInstance)
{
	OT_RPC_UNUSED(aInstance);

	ot_rpc_get_string(OT_RPC_CMD_THREAD_GET_VENDOR_MODEL, vendor_model, sizeof(vendor_model));
	return vendor_model;
}

const char *otThreadGetVendorSwVersion(otInstance *aInstance)
{
	OT_RPC_UNUSED(aInstance);

	ot_rpc_get_string(OT_RPC_CMD_THREAD_GET_VENDOR_SW_VERSION, vendor_sw_version,
			  sizeof(vendor_sw_version));
	return vendor_sw_version;
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_thread_send_diagnostic_get_cb,
			 OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_GET_CB,
			 ot_rpc_cmd_thread_send_diagnostic_get_cb, NULL);
