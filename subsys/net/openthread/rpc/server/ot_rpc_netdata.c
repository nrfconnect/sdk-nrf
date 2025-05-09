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

#include <nrf_rpc_cbor.h>

#include <zephyr/net/openthread.h>

#include <openthread/netdata.h>

static void ot_rpc_cmd_netdata_get(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				   void *handler_data)
{
	bool stable;
	uint8_t length;
	otError error;
	uint8_t *netdata;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	size_t cbor_buffer_size = 0;

	stable = nrf_rpc_decode_bool(ctx);
	length = nrf_rpc_decode_uint(ctx);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_NETDATA_GET);
		return;
	}

	netdata = malloc(UINT8_MAX);

	if (!netdata) {
		nrf_rpc_err(-ENOMEM, NRF_RPC_ERR_SRC_RECV, group, OT_RPC_CMD_NETDATA_GET,
			    NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	ot_rpc_mutex_lock();
	error = otNetDataGet(openthread_get_default_instance(), stable, netdata, &length);
	ot_rpc_mutex_unlock();

	cbor_buffer_size += 1 + sizeof(error);
	cbor_buffer_size += (error == OT_ERROR_NONE) ? 2 + length : 0;

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, cbor_buffer_size);
	nrf_rpc_encode_uint(&rsp_ctx, error);

	if (error == OT_ERROR_NONE) {
		nrf_rpc_encode_buffer(&rsp_ctx, netdata, length);
	}

	free(netdata);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_get_next_service(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otError error;
	otNetworkDataIterator iterator;
	otServiceConfig service_config;

	iterator = nrf_rpc_decode_uint(ctx);
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_NETDATA_GET_NEXT_SERVICE);
		return;
	}

	ot_rpc_mutex_lock();
	error = otNetDataGetNextService(openthread_get_default_instance(), &iterator,
					&service_config);
	ot_rpc_mutex_unlock();

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + sizeof(service_config) + 7);

	nrf_rpc_encode_uint(&rsp_ctx, iterator);
	ot_rpc_encode_service_config(&rsp_ctx, error == OT_ERROR_NONE ? &service_config : NULL);
	nrf_rpc_encode_uint(&rsp_ctx, error);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_netdata_get_next_on_mesh_prefix(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otError error;
	otNetworkDataIterator iterator;
	otBorderRouterConfig config;

	iterator = nrf_rpc_decode_uint(ctx);
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_NETDATA_GET_NEXT_ON_MESH_PREFIX);
		return;
	}

	ot_rpc_mutex_lock();
	error = otNetDataGetNextOnMeshPrefix(openthread_get_default_instance(), &iterator, &config);
	ot_rpc_mutex_unlock();

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, sizeof(error) + sizeof(config) + 14);

	nrf_rpc_encode_uint(&rsp_ctx, iterator);
	ot_rpc_encode_border_router_config(&rsp_ctx, error == OT_ERROR_NONE ? &config : NULL);
	nrf_rpc_encode_uint(&rsp_ctx, error);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_netdata_get, OT_RPC_CMD_NETDATA_GET,
			 ot_rpc_cmd_netdata_get, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_netdata_get_next_on_mesh_prefix,
			 OT_RPC_CMD_NETDATA_GET_NEXT_ON_MESH_PREFIX,
			 ot_rpc_cmd_netdata_get_next_on_mesh_prefix, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_next_service, OT_RPC_CMD_NETDATA_GET_NEXT_SERVICE,
			 ot_rpc_cmd_get_next_service, NULL);
