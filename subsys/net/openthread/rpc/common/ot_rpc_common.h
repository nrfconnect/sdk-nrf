/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_RPC_COMMON_H_
#define OT_RPC_COMMON_H_

#include <openthread/dataset.h>

#include <nrf_rpc_cbor.h>
#include <openthread/ip6.h>

#define OT_RPC_TIMESTAMP_LENGTH(timestamp) \
	sizeof((timestamp).mSeconds) + 1 + \
	sizeof((timestamp).mTicks) + 1 + \
	sizeof((timestamp).mAuthoritative)

#define OT_RPC_SECURITY_POLICY_LENGTH(policy) \
	sizeof((policy).mRotationTime) + 1 + 10

#define OT_RPC_OPERATIONAL_COMPONENTS_LENGTH(components) \
	sizeof((components).mIsActiveTimestampPresent) + \
	sizeof((components).mIsPendingTimestampPresent) + \
	sizeof((components).mIsNetworkKeyPresent) + \
	sizeof((components).mIsNetworkNamePresent) + \
	sizeof((components).mIsExtendedPanIdPresent) + \
	sizeof((components).mIsMeshLocalPrefixPresent) + \
	sizeof((components).mIsDelayPresent) + \
	sizeof((components).mIsPanIdPresent) + \
	sizeof((components).mIsChannelPresent) + \
	sizeof((components).mIsPskcPresent) + \
	sizeof((components).mIsSecurityPolicyPresent) + \
	sizeof((components).mIsChannelMaskPresent)

#define OPERATIONAL_DATASET_LENGTH(dataset) \
	OT_RPC_TIMESTAMP_LENGTH((dataset)->mActiveTimestamp) + \
	OT_RPC_TIMESTAMP_LENGTH((dataset)->mPendingTimestamp) + \
	sizeof((dataset)->mNetworkKey.m8) + 1 + \
	sizeof((dataset)->mNetworkName.m8) + 1 + \
	sizeof((dataset)->mExtendedPanId.m8) + 1 + \
	sizeof((dataset)->mMeshLocalPrefix.m8) + 1 + \
	sizeof((dataset)->mDelay) + 1 + \
	sizeof((dataset)->mPanId) + 1 + \
	sizeof((dataset)->mChannel) + 1 + \
	sizeof((dataset)->mPskc.m8) + 1 + \
	OT_RPC_SECURITY_POLICY_LENGTH((dataset)->mSecurityPolicy) + \
	sizeof((dataset)->mChannelMask) + 1 + \
	OT_RPC_OPERATIONAL_COMPONENTS_LENGTH((dataset)->mComponents)

NRF_RPC_GROUP_DECLARE(ot_group);

void ot_rpc_decode_error(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			 void *handler_data);
void ot_rpc_decode_void(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			void *handler_data);
void ot_rpc_decode_dataset_tlvs(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				void *handler_data);
void ot_rpc_decode_dataset(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			   void *handler_data);
void ot_rpc_encode_dataset(struct nrf_rpc_cbor_ctx *ctx, const otOperationalDataset *dataset);
/* The function reports about command decoding error (not responses). */
void ot_rpc_report_decoding_error(uint8_t cmd_evt_id);

bool ot_rpc_decode_message_info(struct nrf_rpc_cbor_ctx *ctx, otMessageInfo *aMessageInfo);
bool ot_rpc_encode_message_info(struct nrf_rpc_cbor_ctx *ctx, const otMessageInfo *aMessageInfo);
#endif /* OT_RPC_COMMON_H_ */
