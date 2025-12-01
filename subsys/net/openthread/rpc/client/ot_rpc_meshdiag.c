/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_client_common.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_lock.h>
#include <ot_rpc_macros.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <openthread/mesh_diag.h>

static otMeshDiagDiscoverCallback mesh_diag_discover_cb;
static void *mesh_diag_discover_cb_context;

otError otMeshDiagGetNextIp6Address(otMeshDiagIp6AddrIterator *aIterator, otIp6Address *aIp6Address)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(ot_rpc_res_tab_key));
	nrf_rpc_encode_uint(&ctx, (ot_rpc_res_tab_key)aIterator);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_MESH_DIAG_GET_NEXT_IP6_ADDRESS, &ctx);

	error = nrf_rpc_decode_uint(&ctx);
	if (error != OT_ERROR_NONE) {
		goto exit;
	}

	nrf_rpc_decode_buffer(&ctx, aIp6Address->mFields.m8, OT_IP6_ADDRESS_SIZE);

exit:
	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_rsp_decoding_error(OT_RPC_CMD_MESH_DIAG_GET_NEXT_IP6_ADDRESS);
	}

	return error;
}

otError otMeshDiagGetNextChildInfo(otMeshDiagChildIterator *aIterator,
				   otMeshDiagChildInfo *aChildInfo)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;
	uint8_t child_flags;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + sizeof(ot_rpc_res_tab_key));
	nrf_rpc_encode_uint(&ctx, (ot_rpc_res_tab_key)aIterator);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_MESH_DIAG_GET_NEXT_CHILD_INFO, &ctx);

	error = nrf_rpc_decode_uint(&ctx);
	if (error != OT_ERROR_NONE) {
		goto exit;
	}

	aChildInfo->mRloc16 = nrf_rpc_decode_uint(&ctx);
	child_flags = nrf_rpc_decode_uint(&ctx);
	aChildInfo->mLinkQuality = nrf_rpc_decode_uint(&ctx);

	aChildInfo->mMode.mRxOnWhenIdle = (bool)(child_flags & BIT(0));
	aChildInfo->mMode.mDeviceType = (bool)(child_flags & BIT(1));
	aChildInfo->mMode.mNetworkData = (bool)(child_flags & BIT(2));
	aChildInfo->mIsThisDevice = (bool)(child_flags & BIT(3));
	aChildInfo->mIsBorderRouter = (bool)(child_flags & BIT(4));

exit:
	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_rsp_decoding_error(OT_RPC_CMD_MESH_DIAG_GET_NEXT_CHILD_INFO);
	}

	return error;
}

otError otMeshDiagDiscoverTopology(otInstance *aInstance, const otMeshDiagDiscoverConfig *aConfig,
				   otMeshDiagDiscoverCallback aCallback, void *aContext)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;
	uint8_t config = 0;

	OT_RPC_UNUSED(aInstance);

	mesh_diag_discover_cb = aCallback;
	mesh_diag_discover_cb_context = aContext;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1);

	WRITE_BIT(config, 0, aConfig->mDiscoverIp6Addresses);
	WRITE_BIT(config, 1, aConfig->mDiscoverChildTable);
	nrf_rpc_encode_uint(&ctx, config);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_MESH_DIAG_DISCOVER_TOPOLOGY, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}

void otMeshDiagCancel(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_MESH_DIAG_CANCEL, &ctx, ot_rpc_decode_void,
				NULL);
}

static void ot_rpc_mesh_diag_discover_topology_cb(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	otError error;
	otMeshDiagRouterInfo *router_info = NULL;
	uint8_t router_flags;

	error = nrf_rpc_decode_uint(ctx);

	if (error == OT_ERROR_RESPONSE_TIMEOUT) {
		goto exit;
	}

	router_info = malloc(sizeof(otMeshDiagRouterInfo));
	if (!router_info) {
		nrf_rpc_err(-ENOMEM, NRF_RPC_ERR_SRC_RECV, &ot_group,
			    OT_RPC_CMD_MESH_DIAG_DISCOVER_TOPOLOGY_CB,
			    NRF_RPC_PACKET_TYPE_CMD);
		goto exit;
	}

	nrf_rpc_decode_buffer(ctx, router_info->mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
	router_info->mRloc16 = nrf_rpc_decode_uint(ctx);
	router_info->mRouterId = nrf_rpc_decode_uint(ctx);
	router_info->mVersion = nrf_rpc_decode_uint(ctx);
	router_flags = nrf_rpc_decode_uint(ctx);
	nrf_rpc_decode_buffer(ctx, router_info->mLinkQualities, OT_NETWORK_MAX_ROUTER_ID + 1);
	router_info->mIp6AddrIterator = (otMeshDiagIp6AddrIterator *)nrf_rpc_decode_uint(ctx);
	router_info->mChildIterator = (otMeshDiagChildIterator *)nrf_rpc_decode_uint(ctx);

	router_info->mIsThisDevice = (bool)(router_flags & BIT(0));
	router_info->mIsThisDeviceParent = (bool)(router_flags & BIT(1));
	router_info->mIsLeader = (bool)(router_flags & BIT(2));
	router_info->mIsBorderRouter = (bool)(router_flags & BIT(3));

exit:
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		free(router_info);
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESH_DIAG_DISCOVER_TOPOLOGY_CB);
		return;
	}

	ot_rpc_mutex_lock();

	if (mesh_diag_discover_cb != NULL) {
		mesh_diag_discover_cb(error, router_info, mesh_diag_discover_cb_context);
	}

	ot_rpc_mutex_unlock();
	free(router_info);
	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_mesh_diag_discover_topology_cb,
			 OT_RPC_CMD_MESH_DIAG_DISCOVER_TOPOLOGY_CB,
			 ot_rpc_mesh_diag_discover_topology_cb, NULL);
