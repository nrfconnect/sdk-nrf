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

#include <zephyr/net/openthread.h>

#include <openthread/link.h>

#define ACTIVE_SCAN_RESULTS_LENGTH(result) \
	sizeof((result)->mExtAddress.m8) + 1 + \
	sizeof((result)->mNetworkName.m8) + 1 + \
	sizeof((result)->mExtendedPanId.m8) + 1 + \
	sizeof((result)->mSteeringData.m8) + 1 + \
	sizeof((result)->mPanId) + 1 + \
	sizeof((result)->mJoinerUdpPort) + 1 + \
	sizeof((result)->mChannel) + 1 + \
	sizeof((result)->mRssi) + 1 + \
	sizeof((result)->mLqi) + 1 + \
	sizeof(uint8_t) + 1 + 3  /* mIsNative, mDiscover, mIsJoinable as booleans */

static void ot_thread_discover_cb(otActiveScanResult *result, void *context, uint32_t callback_slot)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = result ? ACTIVE_SCAN_RESULTS_LENGTH(result) : 1;

	cbor_buffer_size += 10; /* for context and callback slot */
	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);

	if (result) {
		nrf_rpc_encode_buffer(&ctx, result->mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
		nrf_rpc_encode_str(&ctx, result->mNetworkName.m8, strlen(result->mNetworkName.m8));
		nrf_rpc_encode_buffer(&ctx, result->mExtendedPanId.m8, OT_EXT_PAN_ID_SIZE);
		nrf_rpc_encode_buffer(&ctx, result->mSteeringData.m8, OT_STEERING_DATA_MAX_LENGTH);
		nrf_rpc_encode_uint(&ctx, result->mPanId);
		nrf_rpc_encode_uint(&ctx, result->mJoinerUdpPort);
		nrf_rpc_encode_uint(&ctx, result->mChannel);
		nrf_rpc_encode_int(&ctx, result->mRssi);
		nrf_rpc_encode_uint(&ctx, result->mLqi);
		nrf_rpc_encode_uint(&ctx, result->mVersion);
		nrf_rpc_encode_bool(&ctx, result->mIsNative);
		nrf_rpc_encode_bool(&ctx, result->mDiscover);
		nrf_rpc_encode_bool(&ctx, result->mIsJoinable);
	} else {
		nrf_rpc_encode_null(&ctx);
	}

	nrf_rpc_encode_uint(&ctx, (uintptr_t)context);
	nrf_rpc_encode_uint(&ctx, callback_slot);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_THREAD_DISCOVER_CB, &ctx, ot_rpc_decode_void,
				NULL);
}

NRF_RPC_CBKPROXY_HANDLER(ot_thread_discover_cb_encoder, ot_thread_discover_cb,
			 (otActiveScanResult *result, void *context), (result, context));

static void ot_rpc_thread_discover_rpc_handler(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t scan_channels;
	uint16_t pan_id;
	bool joiner;
	bool enable_eui64_filtering;
	otHandleActiveScanResult cb;
	void *cb_ctx;
	otError error;

	scan_channels = nrf_rpc_decode_uint(ctx);
	pan_id = nrf_rpc_decode_uint(ctx);
	joiner = nrf_rpc_decode_bool(ctx);
	enable_eui64_filtering = nrf_rpc_decode_bool(ctx);
	cb = (otHandleActiveScanResult)nrf_rpc_decode_callbackd(ctx, ot_thread_discover_cb_encoder);
	cb_ctx = (void *)nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_THREAD_DISCOVER);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otThreadDiscover(openthread_get_default_instance(), scan_channels, pan_id, joiner,
				 enable_eui64_filtering, cb, cb_ctx);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_thread_discover, OT_RPC_CMD_THREAD_DISCOVER,
			 ot_rpc_thread_discover_rpc_handler, NULL);

static void ot_rpc_dataset_is_commissioned_rpc_handler(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	bool result;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_DATASET_IS_COMMISSIONED);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	result = otDatasetIsCommissioned(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_bool(group, result);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dataset_is_commissioned,
			 OT_RPC_CMD_DATASET_IS_COMMISSIONED,
			 ot_rpc_dataset_is_commissioned_rpc_handler, NULL);

static void ot_rpc_data_set_active_tlvs_rpc_handler(const struct nrf_rpc_group *group,
						    struct nrf_rpc_cbor_ctx *ctx,
						    void *handler_data)
{
	otOperationalDatasetTlvs dataset;
	void *data_ptr = &dataset;
	otError error;

	ot_rpc_decode_dataset_tlvs(group, ctx, &data_ptr);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_DATASET_SET_ACTIVE_TLVS);
		return;
	}

	if (data_ptr) {
		openthread_api_mutex_lock(openthread_get_default_context());
		error = otDatasetSetActiveTlvs(openthread_get_default_instance(), &dataset);
		openthread_api_mutex_unlock(openthread_get_default_context());
	} else {
		error = OT_ERROR_INVALID_ARGS;
	}

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_data_set_active_tlvs, OT_RPC_CMD_DATASET_SET_ACTIVE_TLVS,
			 ot_rpc_data_set_active_tlvs_rpc_handler, NULL);

static void ot_rpc_rsp_send_dataset(otOperationalDatasetTlvs *dataset)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, dataset ? dataset->mLength + 2 : 1);
	nrf_rpc_encode_buffer(&ctx, dataset ? dataset->mTlvs : NULL,
			      dataset ? dataset->mLength : 0);
	nrf_rpc_cbor_rsp_no_err(&ot_group, &ctx);
}

static void ot_rpc_data_get_active_tlvs_rpc_handler(const struct nrf_rpc_group *group,
						    struct nrf_rpc_cbor_ctx *ctx,
						    void *handler_data)
{
	otOperationalDatasetTlvs dataset;
	otError error;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_DATASET_GET_ACTIVE_TLVS);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otDatasetGetActiveTlvs(openthread_get_default_instance(), &dataset);
	openthread_api_mutex_unlock(openthread_get_default_context());

	ot_rpc_rsp_send_dataset(error == OT_ERROR_NONE ? &dataset : NULL);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_data_get_active_tlvs, OT_RPC_CMD_DATASET_GET_ACTIVE_TLVS,
			 ot_rpc_data_get_active_tlvs_rpc_handler, NULL);

static void ot_rpc_dataset_set_active_rpc_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otOperationalDataset dataset;
	void *data_ptr = &dataset;
	otError error;

	ot_rpc_decode_dataset(group, ctx, &data_ptr);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_DATASET_SET_ACTIVE);
		return;
	}

	if (data_ptr) {
		openthread_api_mutex_lock(openthread_get_default_context());
		error = otDatasetSetActive(openthread_get_default_instance(), &dataset);
		openthread_api_mutex_unlock(openthread_get_default_context());
	} else {
		error = OT_ERROR_INVALID_ARGS;
	}

	nrf_rpc_rsp_send_uint(group, error);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dataset_set_active, OT_RPC_CMD_DATASET_SET_ACTIVE,
			 ot_rpc_dataset_set_active_rpc_handler, NULL);

static void ot_rpc_dataset_get_active_rpc_handler(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otOperationalDataset dataset;
	struct nrf_rpc_cbor_ctx tx_ctx;
	size_t cbor_buffer_size = OPERATIONAL_DATASET_LENGTH(&dataset);
	otError error;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_DATASET_GET_ACTIVE);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otDatasetGetActive(openthread_get_default_instance(), &dataset);
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (error != OT_ERROR_NONE) {
		NRF_RPC_CBOR_ALLOC(&ot_group, tx_ctx, 1);
		nrf_rpc_encode_null(&tx_ctx);
	} else {
		NRF_RPC_CBOR_ALLOC(&ot_group, tx_ctx, cbor_buffer_size);
		ot_rpc_encode_dataset(&tx_ctx, &dataset);
	}

	nrf_rpc_cbor_rsp_no_err(&ot_group, &tx_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dataset_get_active, OT_RPC_CMD_DATASET_GET_ACTIVE,
			 ot_rpc_dataset_get_active_rpc_handler, NULL);
