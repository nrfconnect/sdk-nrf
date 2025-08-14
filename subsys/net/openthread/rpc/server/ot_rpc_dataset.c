/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>
#include <ot_rpc_lock.h>

#include <zephyr/net/openthread.h>

#include <openthread/dataset.h>

static void ot_rpc_dataset_is_commissioned_handler(const struct nrf_rpc_group *group,
						   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool result;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_DATASET_IS_COMMISSIONED);
		return;
	}

	ot_rpc_mutex_lock();
	result = otDatasetIsCommissioned(openthread_get_default_instance());
	ot_rpc_mutex_unlock();

	nrf_rpc_rsp_send_bool(group, result);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dataset_is_commissioned,
			 OT_RPC_CMD_DATASET_IS_COMMISSIONED, ot_rpc_dataset_is_commissioned_handler,
			 NULL);

struct ot_rpc_dataset_set_tlvs_params {
	uint8_t cmd;
	otError (*set)(otInstance *instance, const otOperationalDatasetTlvs *tlvs);
};

static void ot_rpc_dataset_set_tlvs_handler(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const struct ot_rpc_dataset_set_tlvs_params *handler_params = handler_data;
	otOperationalDatasetTlvs dataset_buf;
	otOperationalDatasetTlvs *dataset = &dataset_buf;
	otError error;

	ot_rpc_decode_dataset_tlvs(group, ctx, &dataset);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(handler_params->cmd);
		return;
	}

	if (dataset) {
		ot_rpc_mutex_lock();
		error = handler_params->set(openthread_get_default_instance(), dataset);
		ot_rpc_mutex_unlock();
	} else {
		error = OT_ERROR_INVALID_ARGS;
	}

	nrf_rpc_rsp_send_uint(group, error);
}

const static struct ot_rpc_dataset_set_tlvs_params ot_rpc_dataset_set_active_tlvs_params = {
	.cmd = OT_RPC_CMD_DATASET_SET_ACTIVE_TLVS,
	.set = otDatasetSetActiveTlvs,
};

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dataset_set_active_tlvs,
			 OT_RPC_CMD_DATASET_SET_ACTIVE_TLVS, ot_rpc_dataset_set_tlvs_handler,
			 (void *)&ot_rpc_dataset_set_active_tlvs_params);

const static struct ot_rpc_dataset_set_tlvs_params ot_rpc_dataset_set_pending_tlvs_params = {
	.cmd = OT_RPC_CMD_DATASET_SET_PENDING_TLVS,
	.set = otDatasetSetPendingTlvs,
};

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dataset_set_pending_tlvs,
			 OT_RPC_CMD_DATASET_SET_PENDING_TLVS, ot_rpc_dataset_set_tlvs_handler,
			 (void *)&ot_rpc_dataset_set_pending_tlvs_params);

static void ot_rpc_rsp_send_dataset(otOperationalDatasetTlvs *dataset)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, dataset ? dataset->mLength + 2 : 1);
	nrf_rpc_encode_buffer(&ctx, dataset ? dataset->mTlvs : NULL,
			      dataset ? dataset->mLength : 0);
	nrf_rpc_cbor_rsp_no_err(&ot_group, &ctx);
}

struct ot_rpc_dataset_get_tlvs_params {
	uint8_t cmd;
	otError (*get)(otInstance *instance, otOperationalDatasetTlvs *tlvs);
};

static void ot_rpc_dataset_get_tlvs_handler(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const struct ot_rpc_dataset_get_tlvs_params *handler_params = handler_data;
	otOperationalDatasetTlvs dataset;
	otError error;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(handler_params->cmd);
		return;
	}

	ot_rpc_mutex_lock();
	error = handler_params->get(openthread_get_default_instance(), &dataset);
	ot_rpc_mutex_unlock();

	ot_rpc_rsp_send_dataset(error == OT_ERROR_NONE ? &dataset : NULL);
}

const static struct ot_rpc_dataset_get_tlvs_params ot_rpc_dataset_get_active_tlvs_params = {
	.cmd = OT_RPC_CMD_DATASET_GET_ACTIVE_TLVS,
	.get = otDatasetGetActiveTlvs,
};

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dataset_get_active_tlvs,
			 OT_RPC_CMD_DATASET_GET_ACTIVE_TLVS, ot_rpc_dataset_get_tlvs_handler,
			 (void *)&ot_rpc_dataset_get_active_tlvs_params);

const static struct ot_rpc_dataset_get_tlvs_params ot_rpc_dataset_get_pending_tlvs_params = {
	.cmd = OT_RPC_CMD_DATASET_GET_PENDING_TLVS,
	.get = otDatasetGetPendingTlvs,
};

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dataset_get_pending_tlvs,
			 OT_RPC_CMD_DATASET_GET_PENDING_TLVS, ot_rpc_dataset_get_tlvs_handler,
			 (void *)&ot_rpc_dataset_get_pending_tlvs_params);

struct ot_rpc_dataset_set_params {
	uint8_t cmd;
	otError (*set)(otInstance *instance, const otOperationalDataset *dataset);
};

static void ot_rpc_dataset_set_handler(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const struct ot_rpc_dataset_set_params *handler_params = handler_data;
	otOperationalDataset dataset_buf;
	otOperationalDataset *dataset = &dataset_buf;
	otError error;

	ot_rpc_decode_dataset(group, ctx, &dataset);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(handler_params->cmd);
		return;
	}

	if (dataset) {
		ot_rpc_mutex_lock();
		error = handler_params->set(openthread_get_default_instance(), dataset);
		ot_rpc_mutex_unlock();
	} else {
		error = OT_ERROR_INVALID_ARGS;
	}

	nrf_rpc_rsp_send_uint(group, error);
}

const static struct ot_rpc_dataset_set_params ot_rpc_dataset_set_active_params = {
	.cmd = OT_RPC_CMD_DATASET_SET_ACTIVE,
	.set = otDatasetSetActive,
};

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dataset_set_active, OT_RPC_CMD_DATASET_SET_ACTIVE,
			 ot_rpc_dataset_set_handler, (void *)&ot_rpc_dataset_set_active_params);

const static struct ot_rpc_dataset_set_params ot_rpc_dataset_set_pending_params = {
	.cmd = OT_RPC_CMD_DATASET_SET_PENDING,
	.set = otDatasetSetPending,
};

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dataset_set_pending, OT_RPC_CMD_DATASET_SET_PENDING,
			 ot_rpc_dataset_set_handler, (void *)&ot_rpc_dataset_set_pending_params);

struct ot_rpc_dataset_get_params {
	uint8_t cmd;
	otError (*get)(otInstance *instance, otOperationalDataset *dataset);
};

static void ot_rpc_dataset_get_handler(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	const struct ot_rpc_dataset_get_params *handler_params = handler_data;
	otOperationalDataset dataset;
	struct nrf_rpc_cbor_ctx tx_ctx;
	size_t cbor_buffer_size = OPERATIONAL_DATASET_LENGTH(&dataset);
	otError error;

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(handler_params->cmd);
		return;
	}

	ot_rpc_mutex_lock();
	error = handler_params->get(openthread_get_default_instance(), &dataset);
	ot_rpc_mutex_unlock();

	if (error != OT_ERROR_NONE) {
		NRF_RPC_CBOR_ALLOC(&ot_group, tx_ctx, 1);
		nrf_rpc_encode_null(&tx_ctx);
	} else {
		NRF_RPC_CBOR_ALLOC(&ot_group, tx_ctx, cbor_buffer_size);
		ot_rpc_encode_dataset(&tx_ctx, &dataset);
	}

	nrf_rpc_cbor_rsp_no_err(&ot_group, &tx_ctx);
}

const static struct ot_rpc_dataset_get_params ot_rpc_dataset_get_active_params = {
	.cmd = OT_RPC_CMD_DATASET_GET_ACTIVE,
	.get = otDatasetGetActive,
};

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dataset_get_active, OT_RPC_CMD_DATASET_GET_ACTIVE,
			 ot_rpc_dataset_get_handler, (void *)&ot_rpc_dataset_get_active_params);

const static struct ot_rpc_dataset_get_params ot_rpc_dataset_get_pending_params = {
	.cmd = OT_RPC_CMD_DATASET_GET_PENDING,
	.get = otDatasetGetPending,
};

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_dataset_get_pending, OT_RPC_CMD_DATASET_GET_PENDING,
			 ot_rpc_dataset_get_handler, (void *)&ot_rpc_dataset_get_pending_params);
