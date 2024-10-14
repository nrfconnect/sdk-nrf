/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <openthread/ip6.h>
#include <openthread/netdata.h>

#include "ot_rpc_common.h"
#include "zcbor_common.h"
#include "zcbor_encode.h"
#include <string.h>

#include <nrf_rpc/nrf_rpc_serialize.h>

void ot_rpc_decode_error(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			 void *handler_data)
{
	nrf_rpc_rsp_decode_uint(group, ctx, handler_data, sizeof(otError));
}

void ot_rpc_decode_void(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			void *handler_data)
{
	nrf_rpc_rsp_decode_void(group, ctx, handler_data);
}

void ot_rpc_decode_dataset_tlvs(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				void *handler_data)
{
	otOperationalDatasetTlvs *dataset = *(otOperationalDatasetTlvs **)handler_data;
	const void *data;
	size_t length;

	data = nrf_rpc_decode_buffer_ptr_and_size(ctx, &length);

	if (data == NULL) {
		*(otOperationalDatasetTlvs **)handler_data = NULL;
	} else {
		dataset->mLength = length;
		memcpy(dataset->mTlvs, data, dataset->mLength);
	}
}

static void ot_rpc_encode_timestamp(struct nrf_rpc_cbor_ctx *ctx, const otTimestamp *timestamp)
{
	nrf_rpc_encode_uint64(ctx, timestamp->mSeconds);
	nrf_rpc_encode_uint(ctx, timestamp->mTicks);
	nrf_rpc_encode_bool(ctx, timestamp->mAuthoritative);
}

static void ot_rpc_decode_timestamp(struct nrf_rpc_cbor_ctx *ctx, otTimestamp *timestamp)
{
	timestamp->mSeconds = nrf_rpc_decode_uint64(ctx);
	timestamp->mTicks = nrf_rpc_decode_uint(ctx);
	timestamp->mAuthoritative = nrf_rpc_decode_bool(ctx);
}

void ot_rpc_encode_dataset(struct nrf_rpc_cbor_ctx *ctx, const otOperationalDataset *dataset)
{
	ot_rpc_encode_timestamp(ctx, &dataset->mActiveTimestamp);
	ot_rpc_encode_timestamp(ctx, &dataset->mPendingTimestamp);
	nrf_rpc_encode_buffer(ctx, dataset->mNetworkKey.m8, sizeof(dataset->mNetworkKey.m8));
	nrf_rpc_encode_str(ctx, dataset->mNetworkName.m8, strlen(dataset->mNetworkName.m8));
	nrf_rpc_encode_buffer(ctx, dataset->mExtendedPanId.m8, sizeof(dataset->mExtendedPanId.m8));
	nrf_rpc_encode_buffer(ctx, dataset->mMeshLocalPrefix.m8,
			      sizeof(dataset->mMeshLocalPrefix.m8));
	nrf_rpc_encode_uint(ctx, dataset->mDelay);
	nrf_rpc_encode_uint(ctx, dataset->mPanId);
	nrf_rpc_encode_uint(ctx, dataset->mChannel);
	nrf_rpc_encode_buffer(ctx, dataset->mPskc.m8, sizeof(dataset->mPskc.m8));

	nrf_rpc_encode_uint(ctx, dataset->mSecurityPolicy.mRotationTime);
	nrf_rpc_encode_bool(ctx, !!dataset->mSecurityPolicy.mObtainNetworkKeyEnabled);
	nrf_rpc_encode_bool(ctx, !!dataset->mSecurityPolicy.mNativeCommissioningEnabled);
	nrf_rpc_encode_bool(ctx, !!dataset->mSecurityPolicy.mRoutersEnabled);
	nrf_rpc_encode_bool(ctx, !!dataset->mSecurityPolicy.mExternalCommissioningEnabled);
	nrf_rpc_encode_bool(ctx, !!dataset->mSecurityPolicy.mCommercialCommissioningEnabled);
	nrf_rpc_encode_bool(ctx, !!dataset->mSecurityPolicy.mAutonomousEnrollmentEnabled);
	nrf_rpc_encode_bool(ctx, !!dataset->mSecurityPolicy.mNetworkKeyProvisioningEnabled);
	nrf_rpc_encode_bool(ctx, !!dataset->mSecurityPolicy.mTobleLinkEnabled);
	nrf_rpc_encode_bool(ctx, !!dataset->mSecurityPolicy.mNonCcmRoutersEnabled);
	nrf_rpc_encode_uint(ctx, dataset->mSecurityPolicy.mVersionThresholdForRouting);

	nrf_rpc_encode_uint(ctx, dataset->mChannelMask);

	nrf_rpc_encode_bool(ctx, dataset->mComponents.mIsActiveTimestampPresent);
	nrf_rpc_encode_bool(ctx, dataset->mComponents.mIsPendingTimestampPresent);
	nrf_rpc_encode_bool(ctx, dataset->mComponents.mIsNetworkKeyPresent);
	nrf_rpc_encode_bool(ctx, dataset->mComponents.mIsNetworkNamePresent);
	nrf_rpc_encode_bool(ctx, dataset->mComponents.mIsExtendedPanIdPresent);
	nrf_rpc_encode_bool(ctx, dataset->mComponents.mIsMeshLocalPrefixPresent);
	nrf_rpc_encode_bool(ctx, dataset->mComponents.mIsDelayPresent);
	nrf_rpc_encode_bool(ctx, dataset->mComponents.mIsPanIdPresent);
	nrf_rpc_encode_bool(ctx, dataset->mComponents.mIsChannelPresent);
	nrf_rpc_encode_bool(ctx, dataset->mComponents.mIsPskcPresent);
	nrf_rpc_encode_bool(ctx, dataset->mComponents.mIsSecurityPolicyPresent);
	nrf_rpc_encode_bool(ctx, dataset->mComponents.mIsChannelMaskPresent);
}

void ot_rpc_decode_dataset(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			   void *handler_data)
{
	otOperationalDataset *dataset = *(otOperationalDataset **)handler_data;

	if (nrf_rpc_decode_is_null(ctx)) {
		*(otOperationalDataset **)handler_data = NULL;
		return;
	}

	ot_rpc_decode_timestamp(ctx, &dataset->mActiveTimestamp);
	ot_rpc_decode_timestamp(ctx, &dataset->mPendingTimestamp);
	nrf_rpc_decode_buffer(ctx, dataset->mNetworkKey.m8, sizeof(dataset->mNetworkKey.m8));
	nrf_rpc_decode_str(ctx, dataset->mNetworkName.m8, sizeof(dataset->mNetworkName.m8));
	nrf_rpc_decode_buffer(ctx, dataset->mExtendedPanId.m8, sizeof(dataset->mExtendedPanId.m8));
	nrf_rpc_decode_buffer(ctx, dataset->mMeshLocalPrefix.m8,
			      sizeof(dataset->mMeshLocalPrefix.m8));
	dataset->mDelay = nrf_rpc_decode_uint(ctx);
	dataset->mPanId = nrf_rpc_decode_uint(ctx);
	dataset->mChannel = nrf_rpc_decode_uint(ctx);
	nrf_rpc_decode_buffer(ctx, dataset->mPskc.m8, sizeof(dataset->mPskc.m8));

	dataset->mSecurityPolicy.mRotationTime = nrf_rpc_decode_uint(ctx);
	dataset->mSecurityPolicy.mObtainNetworkKeyEnabled = nrf_rpc_decode_bool(ctx) ? 1 : 0;
	dataset->mSecurityPolicy.mNativeCommissioningEnabled = nrf_rpc_decode_bool(ctx) ? 1 : 0;
	dataset->mSecurityPolicy.mRoutersEnabled = nrf_rpc_decode_bool(ctx) ? 1 : 0;
	dataset->mSecurityPolicy.mExternalCommissioningEnabled = nrf_rpc_decode_bool(ctx) ? 1 : 0;
	dataset->mSecurityPolicy.mCommercialCommissioningEnabled = nrf_rpc_decode_bool(ctx) ? 1 : 0;
	dataset->mSecurityPolicy.mAutonomousEnrollmentEnabled = nrf_rpc_decode_bool(ctx) ? 1 : 0;
	dataset->mSecurityPolicy.mNetworkKeyProvisioningEnabled = nrf_rpc_decode_bool(ctx) ? 1 : 0;
	dataset->mSecurityPolicy.mTobleLinkEnabled = nrf_rpc_decode_bool(ctx) ? 1 : 0;
	dataset->mSecurityPolicy.mNonCcmRoutersEnabled = nrf_rpc_decode_bool(ctx) ? 1 : 0;
	dataset->mSecurityPolicy.mVersionThresholdForRouting = nrf_rpc_decode_uint(ctx);

	dataset->mChannelMask = nrf_rpc_decode_uint(ctx);

	dataset->mComponents.mIsActiveTimestampPresent = nrf_rpc_decode_bool(ctx);
	dataset->mComponents.mIsPendingTimestampPresent = nrf_rpc_decode_bool(ctx);
	dataset->mComponents.mIsNetworkKeyPresent = nrf_rpc_decode_bool(ctx);
	dataset->mComponents.mIsNetworkNamePresent = nrf_rpc_decode_bool(ctx);
	dataset->mComponents.mIsExtendedPanIdPresent = nrf_rpc_decode_bool(ctx);
	dataset->mComponents.mIsMeshLocalPrefixPresent = nrf_rpc_decode_bool(ctx);
	dataset->mComponents.mIsDelayPresent = nrf_rpc_decode_bool(ctx);
	dataset->mComponents.mIsPanIdPresent = nrf_rpc_decode_bool(ctx);
	dataset->mComponents.mIsChannelPresent = nrf_rpc_decode_bool(ctx);
	dataset->mComponents.mIsPskcPresent = nrf_rpc_decode_bool(ctx);
	dataset->mComponents.mIsSecurityPolicyPresent = nrf_rpc_decode_bool(ctx);
	dataset->mComponents.mIsChannelMaskPresent = nrf_rpc_decode_bool(ctx);
}

otMessageSettings *ot_rpc_decode_message_settings(struct nrf_rpc_cbor_ctx *ctx,
						  otMessageSettings *settings)
{
	if (nrf_rpc_decode_is_null(ctx)) {
		return NULL;
	}

	settings->mLinkSecurityEnabled = nrf_rpc_decode_bool(ctx);
	settings->mPriority = nrf_rpc_decode_uint(ctx);

	return settings;
}

void ot_rpc_encode_message_settings(struct nrf_rpc_cbor_ctx *ctx, const otMessageSettings *settings)
{
	if (!settings) {
		nrf_rpc_encode_null(ctx);
		return;
	}

	nrf_rpc_encode_bool(ctx, settings->mLinkSecurityEnabled);
	nrf_rpc_encode_uint(ctx, settings->mPriority);
}

void ot_rpc_report_cmd_decoding_error(uint8_t cmd_evt_id)
{
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &ot_group, cmd_evt_id, NRF_RPC_PACKET_TYPE_CMD);
}

void ot_rpc_report_rsp_decoding_error(uint8_t cmd_evt_id)
{
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &ot_group, cmd_evt_id, NRF_RPC_PACKET_TYPE_RSP);
}

void ot_rpc_encode_message_info(struct nrf_rpc_cbor_ctx *ctx, const otMessageInfo *aMessageInfo)
{
	nrf_rpc_encode_buffer(ctx, aMessageInfo->mSockAddr.mFields.m8,
			      sizeof(aMessageInfo->mSockAddr));
	nrf_rpc_encode_buffer(ctx, aMessageInfo->mPeerAddr.mFields.m8,
			      sizeof(aMessageInfo->mPeerAddr));
	nrf_rpc_encode_uint(ctx, aMessageInfo->mSockPort);
	nrf_rpc_encode_uint(ctx, aMessageInfo->mPeerPort);
	nrf_rpc_encode_uint(ctx, aMessageInfo->mHopLimit);
	nrf_rpc_encode_uint(ctx, aMessageInfo->mEcn);
	nrf_rpc_encode_bool(ctx, aMessageInfo->mIsHostInterface);
	nrf_rpc_encode_bool(ctx, aMessageInfo->mAllowZeroHopLimit);
	nrf_rpc_encode_bool(ctx, aMessageInfo->mMulticastLoop);
}

void ot_rpc_decode_message_info(struct nrf_rpc_cbor_ctx *ctx, otMessageInfo *aMessageInfo)
{
	size_t size = 0;
	const void *buf = NULL;

	buf = nrf_rpc_decode_buffer_ptr_and_size(ctx, &size);
	if (buf && size) {
		size = MIN(size, sizeof(aMessageInfo->mSockAddr.mFields.m8));
		memcpy(aMessageInfo->mSockAddr.mFields.m8, buf, size);
	}

	buf = nrf_rpc_decode_buffer_ptr_and_size(ctx, &size);
	if (buf && size) {
		size = MIN(size, sizeof(aMessageInfo->mPeerAddr.mFields.m8));
		memcpy(aMessageInfo->mPeerAddr.mFields.m8, buf, size);
	}

	aMessageInfo->mSockPort = nrf_rpc_decode_uint(ctx);
	aMessageInfo->mPeerPort = nrf_rpc_decode_uint(ctx);
	aMessageInfo->mHopLimit = nrf_rpc_decode_uint(ctx);
	aMessageInfo->mEcn = nrf_rpc_decode_uint(ctx);
	aMessageInfo->mIsHostInterface = nrf_rpc_decode_bool(ctx);
	aMessageInfo->mAllowZeroHopLimit = nrf_rpc_decode_bool(ctx);
	aMessageInfo->mMulticastLoop = nrf_rpc_decode_bool(ctx);
}

void ot_rpc_encode_service_config(struct nrf_rpc_cbor_ctx *ctx, const otServiceConfig *config)
{

	if (config == NULL) {
		nrf_rpc_encode_null(ctx);
		return;
	}

	nrf_rpc_encode_uint(ctx, config->mServiceId);
	nrf_rpc_encode_uint(ctx, config->mEnterpriseNumber);
	nrf_rpc_encode_buffer(ctx, config->mServiceData, config->mServiceDataLength);
	nrf_rpc_encode_bool(ctx, config->mServerConfig.mStable);
	nrf_rpc_encode_buffer(ctx, config->mServerConfig.mServerData,
			      config->mServerConfig.mServerDataLength);
	nrf_rpc_encode_uint(ctx, config->mServerConfig.mRloc16);
}

void ot_rpc_decode_service_config(struct nrf_rpc_cbor_ctx *ctx, otServiceConfig *config)
{
	size_t size = 0;
	const void *buf = NULL;

	if (nrf_rpc_decode_is_null(ctx)) {
		memset(config, 0, sizeof(otServerConfig));
		return;
	}

	config->mServiceId = nrf_rpc_decode_uint(ctx);
	config->mEnterpriseNumber = nrf_rpc_decode_uint(ctx);

	buf = nrf_rpc_decode_buffer_ptr_and_size(ctx, &size);
	if (buf && size) {
		size = MIN(size, sizeof(config->mServiceData));
		memcpy(config->mServiceData, buf, size);
		config->mServiceDataLength = size;
	}

	config->mServerConfig.mStable = nrf_rpc_decode_bool(ctx);

	buf = nrf_rpc_decode_buffer_ptr_and_size(ctx, &size);
	if (buf && size) {
		size = MIN(size, sizeof(config->mServerConfig.mServerData));
		memcpy(config->mServerConfig.mServerData, buf, size);
		config->mServerConfig.mServerDataLength = size;
	}

	config->mServerConfig.mRloc16 = nrf_rpc_decode_uint(ctx);
}

void ot_rpc_encode_border_router_config(struct nrf_rpc_cbor_ctx *ctx,
					const otBorderRouterConfig *config)
{
	if (config == NULL) {
		nrf_rpc_encode_null(ctx);
		return;
	}

	nrf_rpc_encode_buffer(ctx, config->mPrefix.mPrefix.mFields.m8,
			      OT_IP6_ADDRESS_SIZE);
	nrf_rpc_encode_uint(ctx, config->mPrefix.mLength);
	nrf_rpc_encode_int(ctx, config->mPreference);
	nrf_rpc_encode_bool(ctx, config->mPreferred);
	nrf_rpc_encode_bool(ctx, config->mSlaac);
	nrf_rpc_encode_bool(ctx, config->mDhcp);
	nrf_rpc_encode_bool(ctx, config->mConfigure);
	nrf_rpc_encode_bool(ctx, config->mDefaultRoute);
	nrf_rpc_encode_bool(ctx, config->mOnMesh);
	nrf_rpc_encode_bool(ctx, config->mStable);
	nrf_rpc_encode_bool(ctx, config->mNdDns);
	nrf_rpc_encode_bool(ctx, config->mDp);
	nrf_rpc_encode_uint(ctx, config->mRloc16);
}

void ot_rpc_decode_border_router_config(struct nrf_rpc_cbor_ctx *ctx, otBorderRouterConfig *config)
{
	size_t size = 0;
	const void *buf = NULL;

	if (nrf_rpc_decode_is_null(ctx)) {
		memset(config, 0, sizeof(otBorderRouterConfig));
		return;
	}

	buf = nrf_rpc_decode_buffer_ptr_and_size(ctx, &size);
	if (buf && size) {
		size = MIN(size, sizeof(config->mPrefix.mPrefix.mFields.m8));
		memcpy(config->mPrefix.mPrefix.mFields.m8, buf, size);
	}

	config->mPrefix.mLength = nrf_rpc_decode_uint(ctx);
	config->mPreference = nrf_rpc_decode_int(ctx);
	config->mPreferred = nrf_rpc_decode_bool(ctx);
	config->mSlaac = nrf_rpc_decode_bool(ctx);
	config->mDhcp = nrf_rpc_decode_bool(ctx);
	config->mConfigure = nrf_rpc_decode_bool(ctx);
	config->mDefaultRoute = nrf_rpc_decode_bool(ctx);
	config->mOnMesh = nrf_rpc_decode_bool(ctx);
	config->mStable = nrf_rpc_decode_bool(ctx);
	config->mNdDns = nrf_rpc_decode_bool(ctx);
	config->mDp = nrf_rpc_decode_bool(ctx);
	config->mRloc16 = nrf_rpc_decode_uint(ctx);
}

void ot_rpc_encode_sockaddr(struct nrf_rpc_cbor_ctx *ctx, const otSockAddr *sockaddr)
{
	nrf_rpc_encode_buffer(ctx, sockaddr->mAddress.mFields.m8, OT_IP6_ADDRESS_SIZE);
	nrf_rpc_encode_uint(ctx, sockaddr->mPort);
}

void ot_rpc_decode_sockaddr(struct nrf_rpc_cbor_ctx *ctx, otSockAddr *sockaddr)
{
	nrf_rpc_decode_buffer(ctx, sockaddr->mAddress.mFields.m8, OT_IP6_ADDRESS_SIZE);
	sockaddr->mPort = nrf_rpc_decode_uint(ctx);
}

void ot_rpc_encode_dns_query_config(struct nrf_rpc_cbor_ctx *ctx,
				    const struct otDnsQueryConfig *config)
{
	if (config == NULL) {
		nrf_rpc_encode_null(ctx);
		return;
	}

	nrf_rpc_encode_buffer(ctx, config->mServerSockAddr.mAddress.mFields.m8,
			      OT_IP6_ADDRESS_SIZE);
	nrf_rpc_encode_uint(ctx, config->mServerSockAddr.mPort);
	nrf_rpc_encode_uint(ctx, config->mResponseTimeout);
	nrf_rpc_encode_uint(ctx, config->mMaxTxAttempts);
	nrf_rpc_encode_uint(ctx, config->mRecursionFlag);
	nrf_rpc_encode_uint(ctx, config->mNat64Mode);
	nrf_rpc_encode_uint(ctx, config->mServiceMode);
	nrf_rpc_encode_uint(ctx, config->mTransportProto);
}

bool ot_rpc_decode_dns_query_config(struct nrf_rpc_cbor_ctx *ctx, struct otDnsQueryConfig *config)
{
	if (nrf_rpc_decode_is_null(ctx)) {
		return false;
	}

	nrf_rpc_decode_buffer(ctx, config->mServerSockAddr.mAddress.mFields.m8,
			      OT_IP6_ADDRESS_SIZE);

	config->mServerSockAddr.mPort = nrf_rpc_decode_uint(ctx);
	config->mResponseTimeout = nrf_rpc_decode_uint(ctx);
	config->mMaxTxAttempts = nrf_rpc_decode_uint(ctx);
	config->mRecursionFlag = nrf_rpc_decode_uint(ctx);
	config->mNat64Mode = nrf_rpc_decode_uint(ctx);
	config->mServiceMode = nrf_rpc_decode_uint(ctx);
	config->mTransportProto = nrf_rpc_decode_uint(ctx);

	return true;
}
