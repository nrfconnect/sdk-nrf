/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_rpc_common.h"

#include <nrf_rpc/nrf_rpc_serialize.h>

#include <openthread/dataset.h>

void ot_rpc_decode_error(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			 void *handler_data)
{
	nrf_rpc_rsp_decode_i32(group, ctx, handler_data);
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

void ot_rpc_report_decoding_error(uint8_t cmd_evt_id)
{
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &ot_group, cmd_evt_id, NRF_RPC_PACKET_TYPE_CMD);
}
