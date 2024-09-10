/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>

#include <nrf_rpc/nrf_rpc_serialize.h>

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
