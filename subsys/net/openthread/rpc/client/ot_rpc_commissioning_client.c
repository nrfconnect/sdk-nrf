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
	cbor_buffer_size += 1;                     /* aJoiner */
	cbor_buffer_size += 1;                     /* aEnableEui64Filtering */
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

bool otDatasetIsCommissioned(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool result;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_DATASET_IS_COMMISSIONED, &ctx,
				nrf_rpc_rsp_decode_bool, &result);

	return result;
}

otError otDatasetSetActiveTlvs(otInstance *aInstance, const otOperationalDatasetTlvs *aDataset)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size;
	otError error;

	ARG_UNUSED(aInstance);

	if (aDataset == NULL || aDataset->mLength > OT_OPERATIONAL_DATASET_MAX_LENGTH) {
		return OT_ERROR_INVALID_ARGS;
	}

	cbor_buffer_size = aDataset->mLength + 2;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);

	nrf_rpc_encode_buffer(&ctx, aDataset->mTlvs, aDataset->mLength);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_DATASET_SET_ACTIVE_TLVS, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}

otError otDatasetGetActiveTlvs(otInstance *aInstance, otOperationalDatasetTlvs *aDataset)
{
	struct nrf_rpc_cbor_ctx ctx;
	otOperationalDatasetTlvs *dataset = aDataset;

	ARG_UNUSED(aInstance);

	if (aDataset == NULL) {
		return OT_ERROR_NOT_FOUND;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_DATASET_GET_ACTIVE_TLVS, &ctx,
				ot_rpc_decode_dataset_tlvs, &dataset);

	return dataset ? OT_ERROR_NONE : OT_ERROR_NOT_FOUND;
}

otError otDatasetSetActive(otInstance *aInstance, const otOperationalDataset *aDataset)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size;
	otError error;

	ARG_UNUSED(aInstance);

	if (aDataset == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	cbor_buffer_size = OPERATIONAL_DATASET_LENGTH(aDataset);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	ot_rpc_encode_dataset(&ctx, aDataset);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_DATASET_SET_ACTIVE, &ctx, ot_rpc_decode_error,
				&error);

	return error;
}

otError otDatasetGetActive(otInstance *aInstance, otOperationalDataset *aDataset)
{
	struct nrf_rpc_cbor_ctx ctx;
	otOperationalDataset *dataset = aDataset;

	ARG_UNUSED(aInstance);

	if (aDataset == NULL) {
		return OT_ERROR_NOT_FOUND;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_DATASET_GET_ACTIVE, &ctx,
				ot_rpc_decode_dataset, &dataset);

	return dataset ? OT_ERROR_NONE : OT_ERROR_NOT_FOUND;
}
