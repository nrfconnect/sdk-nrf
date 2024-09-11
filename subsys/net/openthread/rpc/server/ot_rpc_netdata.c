/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>

#include <nrf_rpc_cbor.h>

#include <zephyr/net/openthread.h>

#include <openthread/netdata.h>

static void ot_rpc_cmd_netdata_get(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				   void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otError error = OT_ERROR_NONE;
	bool stable;
	bool decoded_ok;
	bool buff_allocated = false;
	uint8_t length = 0;

	decoded_ok = zcbor_bool_decode(ctx->zs, &stable);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	decoded_ok = zcbor_uint_decode(ctx->zs, &length, sizeof(length));
	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + length + 3);
	buff_allocated = true;

	if (!zcbor_bstr_start_encode(rsp_ctx.zs)) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	openthread_api_mutex_lock(openthread_get_default_context());

	error = otNetDataGet(openthread_get_default_instance(), stable, rsp_ctx.zs[0].payload_mut,
			     &length);
	rsp_ctx.zs[0].payload_mut += length;

	openthread_api_mutex_unlock(openthread_get_default_context());

	if (!zcbor_bstr_end_encode(rsp_ctx.zs, NULL)) {
		goto exit;
	}

exit:
	if (!decoded_ok) {
		nrf_rpc_cbor_decoding_done(group, ctx);
	}

	if (!buff_allocated) {
		NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + 1);
	}

	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_get_next_service(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otError error = OT_ERROR_NONE;
	otNetworkDataIterator iterator;
	bool decoded_ok = false;
	otServiceConfig service_config;

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + sizeof(service_config) + 7);

	decoded_ok = zcbor_uint_decode(ctx->zs, &iterator, sizeof(iterator));

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otNetDataGetNextService(openthread_get_default_instance(), &iterator,
					&service_config);
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (!zcbor_uint_encode(rsp_ctx.zs, &iterator, sizeof(iterator))) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	if (error == OT_ERROR_NONE) {
		ot_rpc_encode_service_config(&rsp_ctx, &service_config);
	} else {
		ot_rpc_encode_service_config(&rsp_ctx, NULL);
	}

exit:
	if (!decoded_ok) {
		nrf_rpc_cbor_decoding_done(group, ctx);
	}
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_netdata_get_next_on_mesh_prefix(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otError error = OT_ERROR_NONE;
	otNetworkDataIterator iterator;
	bool decoded_ok = false;
	otBorderRouterConfig config;

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + sizeof(config) + 14);

	decoded_ok = zcbor_uint_decode(ctx->zs, &iterator, sizeof(iterator));

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto exit;
	}

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otNetDataGetNextOnMeshPrefix(openthread_get_default_instance(), &iterator, &config);
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (!zcbor_uint_encode(rsp_ctx.zs, &iterator, sizeof(iterator))) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	if (error == OT_ERROR_NONE) {
		ot_rpc_encode_border_router_config(&rsp_ctx, &config);
	} else {
		ot_rpc_encode_border_router_config(&rsp_ctx, NULL);
	}

exit:
	if (!decoded_ok) {
		nrf_rpc_cbor_decoding_done(group, ctx);
	}
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_netdata_get, OT_RPC_CMD_NETDATA_GET,
			 ot_rpc_cmd_netdata_get, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_netdata_get_next_on_mesh_prefix,
			 OT_RPC_CMD_NETDATA_GET_NEXT_ON_MESH_PREFIX,
			 ot_rpc_cmd_netdata_get_next_on_mesh_prefix, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_next_service, OT_RPC_CMD_NETDATA_GET_NEXT_SERVICE,
			 ot_rpc_cmd_get_next_service, NULL);
