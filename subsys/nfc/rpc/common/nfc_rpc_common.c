/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nfc_rpc_common.h"

#include <nrf_rpc/nrf_rpc_serialize.h>

void nfc_rpc_decode_error(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			  void *handler_data)
{
	nrf_rpc_rsp_decode_i32(group, ctx, handler_data);
}

void nfc_rpc_report_decoding_error(uint8_t cmd_evt_id)
{
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &nfc_group, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}

void nfc_rpc_decode_parameter(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			      void *handler_data)
{
	struct nfc_rpc_param_getter *getter = (struct nfc_rpc_param_getter *)handler_data;
	const void *data;

	data = nrf_rpc_decode_buffer_ptr_and_size(ctx, getter->length);

	if (data == NULL) {
		getter->data = NULL;
	} else {
		memcpy(getter->data, data, *getter->length);
	}
}
