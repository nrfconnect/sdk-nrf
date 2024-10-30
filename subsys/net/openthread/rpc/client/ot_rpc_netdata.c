/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nrf_rpc_cbor.h>

#include <openthread/netdata.h>

otError otNetDataGet(otInstance *aInstance, bool aStable, uint8_t *aData, uint8_t *aDataLength)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error = OT_ERROR_NONE;
	size_t size = 0;
	const void *buf = NULL;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 5);
	nrf_rpc_encode_bool(&ctx, aStable);
	nrf_rpc_encode_uint(&ctx, *aDataLength);
	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_NETDATA_GET, &ctx);

	buf = nrf_rpc_decode_buffer_ptr_and_size(&ctx, &size);

	if (buf && size) {
		memcpy(aData, buf, MIN(size, *aDataLength));
	}

	error = nrf_rpc_decode_uint(&ctx);

	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_NETDATA_GET);
		return OT_ERROR_FAILED;
	}

	return error;
}

otError otNetDataGetNextService(otInstance *aInstance, otNetworkDataIterator *aIterator,
				otServiceConfig *aConfig)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(*aIterator) + 1);
	nrf_rpc_encode_uint(&ctx, *aIterator);
	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_NETDATA_GET_NEXT_SERVICE, &ctx);

	*aIterator = nrf_rpc_decode_uint(&ctx);
	ot_rpc_decode_service_config(&ctx, aConfig);
	error = nrf_rpc_decode_uint(&ctx);

	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_NETDATA_GET_NEXT_SERVICE);
		return OT_ERROR_FAILED;
	}

	return error;
}

otError otNetDataGetNextOnMeshPrefix(otInstance *aInstance, otNetworkDataIterator *aIterator,
				     otBorderRouterConfig *aConfig)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error = OT_ERROR_NONE;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(*aIterator) + 1);
	nrf_rpc_encode_uint(&ctx, *aIterator);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_NETDATA_GET_NEXT_ON_MESH_PREFIX, &ctx);

	*aIterator = nrf_rpc_decode_uint(&ctx);
	ot_rpc_decode_border_router_config(&ctx, aConfig);
	error = nrf_rpc_decode_uint(&ctx);

	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_NETDATA_GET_NEXT_ON_MESH_PREFIX);
		return OT_ERROR_FAILED;
	}

	return error;
}
