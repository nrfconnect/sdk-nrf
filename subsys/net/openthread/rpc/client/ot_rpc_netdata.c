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
	struct zcbor_string zstr;
	struct nrf_rpc_cbor_ctx ctx;
	otError error = OT_ERROR_NONE;
	bool decoded_ok = true;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 5);

	if (!zcbor_bool_put(ctx.zs, aStable)) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	if (!zcbor_uint_encode(ctx.zs, aDataLength, sizeof(*aDataLength))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_NETDATA_GET, &ctx);

	decoded_ok = zcbor_nil_expect(ctx.zs, NULL);

	if (decoded_ok) {
		*aDataLength = 0;
		goto decode_error;
	}

	if (ctx.zs->constant_state->error != ZCBOR_ERR_WRONG_TYPE) {
		decoded_ok = false;
		error = OT_ERROR_FAILED;
		goto exit;
	}

	zcbor_pop_error(ctx.zs);

	decoded_ok = zcbor_bstr_decode(ctx.zs, &zstr);

	if (!decoded_ok) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	memcpy(aData, zstr.value, (zstr.len < *aDataLength ? zstr.len : *aDataLength));

decode_error:
	if (!zcbor_uint_decode(ctx.zs, &error, sizeof(error))) {
		error = OT_ERROR_FAILED;
	}

	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

exit:
	if (!decoded_ok) {
		nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
		ot_rpc_report_decoding_error(OT_RPC_CMD_NETDATA_GET);
	}

	return error;
}

otError otNetDataGetNextService(otInstance *aInstance, otNetworkDataIterator *aIterator,
				otServiceConfig *aConfig)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;
	bool decoded_ok = true;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(*aIterator) + 1);

	if (!zcbor_uint_encode(ctx.zs, aIterator, sizeof(*aIterator))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_NETDATA_GET_NEXT_SERVICE, &ctx);

	decoded_ok = zcbor_uint_decode(ctx.zs, aIterator, sizeof(*aIterator));

	if (!decoded_ok) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	decoded_ok = ot_rpc_decode_service_config(&ctx, aConfig);

	if (!decoded_ok) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	decoded_ok = zcbor_uint_decode(ctx.zs, &error, sizeof(error));

	if (!decoded_ok) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

exit:
	if (!decoded_ok) {
		nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
		ot_rpc_report_decoding_error(OT_RPC_CMD_NETDATA_GET_NEXT_SERVICE);
	}

	return error;
}

otError otNetDataGetNextOnMeshPrefix(otInstance *aInstance, otNetworkDataIterator *aIterator,
				     otBorderRouterConfig *aConfig)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error = OT_ERROR_NONE;
	bool decoded_ok = true;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, sizeof(*aIterator) + 1);

	if (!zcbor_uint_encode(ctx.zs, aIterator, sizeof(*aIterator))) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_NETDATA_GET_NEXT_ON_MESH_PREFIX, &ctx);

	decoded_ok = zcbor_uint_decode(ctx.zs, aIterator, sizeof(*aIterator));

	if (!decoded_ok) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	decoded_ok = ot_rpc_decode_border_router_config(&ctx, aConfig);

	if (!decoded_ok) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	decoded_ok = zcbor_uint_decode(ctx.zs, &error, sizeof(error));

	if (!decoded_ok) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

exit:
	if (!decoded_ok) {
		ot_rpc_report_decoding_error(OT_RPC_CMD_NETDATA_GET_NEXT_ON_MESH_PREFIX);
		nrf_rpc_cbor_decoding_done(&ot_group, &ctx);
	}

	return error;
}
