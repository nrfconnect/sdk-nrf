/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_rpc_common.h"

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

	if (zcbor_nil_expect(ctx->zs, NULL)) {
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

void ot_rpc_report_decoding_error(uint8_t cmd_evt_id)
{
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &ot_group, cmd_evt_id, NRF_RPC_PACKET_TYPE_CMD);
}
