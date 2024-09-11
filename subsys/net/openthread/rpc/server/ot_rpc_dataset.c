/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc/nrf_rpc_cbkproxy.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>

#include <zephyr/net/openthread.h>

#include <openthread/dataset.h>

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
