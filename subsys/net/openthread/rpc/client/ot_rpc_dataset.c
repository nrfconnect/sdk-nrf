/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>
#include <ot_rpc_macros.h>
#include <ot_rpc_os.h>

#include <nrf_rpc/nrf_rpc_serialize.h>

bool otDatasetIsCommissioned(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool result;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_DATASET_IS_COMMISSIONED, &ctx,
				nrf_rpc_rsp_decode_bool, &result);

	return result;
}

static otError ot_rpc_dataset_set_tlvs(uint8_t cmd, const otOperationalDatasetTlvs *dataset)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	OT_RPC_ASSERT(dataset);

	if (dataset->mLength > OT_OPERATIONAL_DATASET_MAX_LENGTH) {
		return OT_ERROR_INVALID_ARGS;
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 2 + dataset->mLength);
	nrf_rpc_encode_buffer(&ctx, dataset->mTlvs, dataset->mLength);
	nrf_rpc_cbor_cmd_no_err(&ot_group, cmd, &ctx, ot_rpc_decode_error, &error);

	return error;
}

static otError ot_rpc_dataset_get_tlvs(uint8_t cmd, otOperationalDatasetTlvs *dataset)
{
	struct nrf_rpc_cbor_ctx ctx;

	OT_RPC_ASSERT(dataset);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);
	nrf_rpc_cbor_cmd_no_err(&ot_group, cmd, &ctx, ot_rpc_decode_dataset_tlvs, &dataset);

	return dataset ? OT_ERROR_NONE : OT_ERROR_NOT_FOUND;
}

static otError ot_rpc_dataset_set(uint8_t cmd, const otOperationalDataset *dataset)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	OT_RPC_ASSERT(dataset);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, OPERATIONAL_DATASET_LENGTH(dataset));
	ot_rpc_encode_dataset(&ctx, dataset);
	nrf_rpc_cbor_cmd_no_err(&ot_group, cmd, &ctx, ot_rpc_decode_error, &error);

	return error;
}

static otError ot_rpc_dataset_get(uint8_t cmd, otOperationalDataset *dataset)
{
	struct nrf_rpc_cbor_ctx ctx;

	OT_RPC_ASSERT(dataset);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);
	nrf_rpc_cbor_cmd_no_err(&ot_group, cmd, &ctx, ot_rpc_decode_dataset, &dataset);

	return dataset ? OT_ERROR_NONE : OT_ERROR_NOT_FOUND;
}

otError otDatasetSetActiveTlvs(otInstance *aInstance, const otOperationalDatasetTlvs *aDataset)
{
	OT_RPC_UNUSED(aInstance);

	return ot_rpc_dataset_set_tlvs(OT_RPC_CMD_DATASET_SET_ACTIVE_TLVS, aDataset);
}

otError otDatasetGetActiveTlvs(otInstance *aInstance, otOperationalDatasetTlvs *aDataset)
{
	OT_RPC_UNUSED(aInstance);

	return ot_rpc_dataset_get_tlvs(OT_RPC_CMD_DATASET_GET_ACTIVE_TLVS, aDataset);
}

otError otDatasetSetActive(otInstance *aInstance, const otOperationalDataset *aDataset)
{
	OT_RPC_UNUSED(aInstance);

	return ot_rpc_dataset_set(OT_RPC_CMD_DATASET_SET_ACTIVE, aDataset);
}

otError otDatasetGetActive(otInstance *aInstance, otOperationalDataset *aDataset)
{
	OT_RPC_UNUSED(aInstance);

	return ot_rpc_dataset_get(OT_RPC_CMD_DATASET_GET_ACTIVE, aDataset);
}

otError otDatasetSetPendingTlvs(otInstance *aInstance, const otOperationalDatasetTlvs *aDataset)
{
	OT_RPC_UNUSED(aInstance);

	return ot_rpc_dataset_set_tlvs(OT_RPC_CMD_DATASET_SET_PENDING_TLVS, aDataset);
}

otError otDatasetGetPendingTlvs(otInstance *aInstance, otOperationalDatasetTlvs *aDataset)
{
	OT_RPC_UNUSED(aInstance);

	return ot_rpc_dataset_get_tlvs(OT_RPC_CMD_DATASET_GET_PENDING_TLVS, aDataset);
}

otError otDatasetSetPending(otInstance *aInstance, const otOperationalDataset *aDataset)
{
	OT_RPC_UNUSED(aInstance);

	return ot_rpc_dataset_set(OT_RPC_CMD_DATASET_SET_PENDING, aDataset);
}

otError otDatasetGetPending(otInstance *aInstance, otOperationalDataset *aDataset)
{
	OT_RPC_UNUSED(aInstance);

	return ot_rpc_dataset_get(OT_RPC_CMD_DATASET_GET_PENDING, aDataset);
}
