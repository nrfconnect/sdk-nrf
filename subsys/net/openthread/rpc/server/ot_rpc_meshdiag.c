/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc/nrf_rpc_serialize.h>
#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>
#include "ot_rpc_resource.h"

#include <nrf_rpc_cbor.h>

#include <zephyr/net/openthread.h>

#include <openthread/mesh_diag.h>

/* Register iterators as resource table of size 1, to avoid passing raw pointers */
OT_RPC_RESOURCE_TABLE_REGISTER(meshdiag_ip6_it, otMeshDiagIp6AddrIterator, 1);
OT_RPC_RESOURCE_TABLE_REGISTER(meshdiag_child_it, otMeshDiagChildIterator, 1);

static void ot_rpc_cmd_get_next_ip6_address(const struct nrf_rpc_group *group,
					    struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	otError error;
	otMeshDiagIp6AddrIterator *iterator;
	ot_rpc_res_tab_key iterator_key;
	otIp6Address address;

	iterator_key = nrf_rpc_decode_uint(ctx);
	iterator = ot_res_tab_meshdiag_ip6_it_get(iterator_key);

	if (!iterator || !nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESH_DIAG_GET_NEXT_IP6_ADDRESS);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otMeshDiagGetNextIp6Address(iterator, &address);
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (error != OT_ERROR_NONE) {
		nrf_rpc_rsp_send_uint(group, error);
		return;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 3 + OT_IP6_ADDRESS_SIZE);

	nrf_rpc_encode_uint(&rsp_ctx, error);
	nrf_rpc_encode_buffer(&rsp_ctx, address.mFields.m8, OT_IP6_ADDRESS_SIZE);

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_get_next_child_info(const struct nrf_rpc_group *group,
					   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	size_t cbor_buffer_size = 0;
	otError error;
	otMeshDiagChildIterator *iterator;
	ot_rpc_res_tab_key iterator_key;
	otMeshDiagChildInfo child_info;
	uint8_t child_flags = 0;

	iterator_key = nrf_rpc_decode_uint(ctx);
	iterator = ot_res_tab_meshdiag_child_it_get(iterator_key);

	if (!iterator || !nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_MESH_DIAG_GET_NEXT_CHILD_INFO);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otMeshDiagGetNextChildInfo(iterator, &child_info);
	openthread_api_mutex_unlock(openthread_get_default_context());

	if (error != OT_ERROR_NONE) {
		nrf_rpc_rsp_send_uint(group, error);
		return;
	}

	cbor_buffer_size += 2;			  /* aError */
	cbor_buffer_size += 1 + sizeof(uint16_t); /* mRloc16 */
	cbor_buffer_size += 1 + sizeof(uint8_t);  /* child_flags */
	cbor_buffer_size += 1 + sizeof(uint8_t);  /* mLinkQuality */

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, cbor_buffer_size);

	nrf_rpc_encode_uint(&rsp_ctx, error);
	nrf_rpc_encode_uint(&rsp_ctx, child_info.mRloc16);

	WRITE_BIT(child_flags, 0, child_info.mMode.mRxOnWhenIdle);
	WRITE_BIT(child_flags, 1, child_info.mMode.mDeviceType);
	WRITE_BIT(child_flags, 2, child_info.mMode.mNetworkData);
	WRITE_BIT(child_flags, 3, child_info.mIsThisDevice);
	WRITE_BIT(child_flags, 4, child_info.mIsBorderRouter);

	nrf_rpc_encode_uint(&rsp_ctx, child_flags);
	nrf_rpc_encode_uint(&rsp_ctx, child_info.mLinkQuality);

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void handle_mesh_diag_discover(otError aError, otMeshDiagRouterInfo *aRouterInfo,
				      void *aContext)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t cbor_buffer_size = 2; /* aError */
	uint8_t router_flags = 0;
	ot_rpc_res_tab_key child_iterator_key;
	ot_rpc_res_tab_key ip6_iterator_key;

	ARG_UNUSED(aContext);

	if (aError == OT_ERROR_RESPONSE_TIMEOUT) {
		NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);

		nrf_rpc_encode_uint(&ctx, aError);
		nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_MESH_DIAG_DISCOVER_TOPOLOGY_CB, &ctx,
					nrf_rpc_rsp_decode_void, NULL);
		return;
	}

	ip6_iterator_key = ot_res_tab_meshdiag_ip6_it_alloc(aRouterInfo->mIp6AddrIterator);
	child_iterator_key = ot_res_tab_meshdiag_child_it_alloc(aRouterInfo->mChildIterator);

	if ((!ip6_iterator_key && aRouterInfo->mIp6AddrIterator) ||
	    (!child_iterator_key && aRouterInfo->mChildIterator)) {
		nrf_rpc_err(-ENOMEM, NRF_RPC_ERR_SRC_SEND, &ot_group,
			    OT_RPC_CMD_MESH_DIAG_DISCOVER_TOPOLOGY_CB, NRF_RPC_PACKET_TYPE_CMD);
		return;
	}

	cbor_buffer_size += 1 + OT_EXT_ADDRESS_SIZE;	    /* mExtAddress */
	cbor_buffer_size += 1 + sizeof(uint16_t);	    /* mRloc16 */
	cbor_buffer_size += 1 + sizeof(uint8_t);	    /* mRouterId */
	cbor_buffer_size += 1 + sizeof(uint16_t);	    /* mVersion */
	cbor_buffer_size += 1 + sizeof(uint8_t);	    /* router_flags */
	cbor_buffer_size += 2 + OT_NETWORK_MAX_ROUTER_ID;   /* mLinkQualities */
	cbor_buffer_size += 1 + sizeof(ot_rpc_res_tab_key); /* ip6_iterator_key */
	cbor_buffer_size += 1 + sizeof(ot_rpc_res_tab_key); /* child_iterator_key */

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);

	WRITE_BIT(router_flags, 0, aRouterInfo->mIsThisDevice);
	WRITE_BIT(router_flags, 1, aRouterInfo->mIsThisDeviceParent);
	WRITE_BIT(router_flags, 2, aRouterInfo->mIsLeader);
	WRITE_BIT(router_flags, 3, aRouterInfo->mIsBorderRouter);

	nrf_rpc_encode_uint(&ctx, aError);
	nrf_rpc_encode_buffer(&ctx, aRouterInfo->mExtAddress.m8, OT_EXT_ADDRESS_SIZE);
	nrf_rpc_encode_uint(&ctx, aRouterInfo->mRloc16);
	nrf_rpc_encode_uint(&ctx, aRouterInfo->mRouterId);
	nrf_rpc_encode_uint(&ctx, aRouterInfo->mVersion);
	nrf_rpc_encode_uint(&ctx, router_flags);
	nrf_rpc_encode_buffer(&ctx, aRouterInfo->mLinkQualities, OT_NETWORK_MAX_ROUTER_ID + 1);
	nrf_rpc_encode_uint(&ctx, ip6_iterator_key);
	nrf_rpc_encode_uint(&ctx, child_iterator_key);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_MESH_DIAG_DISCOVER_TOPOLOGY_CB, &ctx,
				nrf_rpc_rsp_decode_void, NULL);

	ot_res_tab_meshdiag_ip6_it_free(ip6_iterator_key);
	ot_res_tab_meshdiag_child_it_free(child_iterator_key);
}

static void ot_rpc_cmd_mesh_diag_discover_topology(const struct nrf_rpc_group *group,
						    struct nrf_rpc_cbor_ctx *ctx,
						    void *handler_data)
{
	otError error;
	uint8_t config;
	otMeshDiagDiscoverConfig mesh_diag_discover_config;

	config = nrf_rpc_decode_uint(ctx);
	mesh_diag_discover_config.mDiscoverIp6Addresses = (bool)(config & BIT(0));
	mesh_diag_discover_config.mDiscoverChildTable = (bool)(config & BIT(1));

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_THREAD_SEND_DIAGNOSTIC_GET);
		return;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otMeshDiagDiscoverTopology(openthread_get_default_instance(),
					   &mesh_diag_discover_config, handle_mesh_diag_discover,
					   NULL);
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_uint(group, error);
}

static void ot_rpc_cmd_mesh_diag_cancel(const struct nrf_rpc_group *group,
					struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	otMeshDiagCancel(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_next_ip6_address,
			 OT_RPC_CMD_MESH_DIAG_GET_NEXT_IP6_ADDRESS,
			 ot_rpc_cmd_get_next_ip6_address, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_get_next_child_info,
			 OT_RPC_CMD_MESH_DIAG_GET_NEXT_CHILD_INFO,
			 ot_rpc_cmd_get_next_child_info, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_mesh_diag_cancel, OT_RPC_CMD_MESH_DIAG_CANCEL,
			 ot_rpc_cmd_mesh_diag_cancel, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_mesh_diag_discover_topology,
			 OT_RPC_CMD_MESH_DIAG_DISCOVER_TOPOLOGY,
			 ot_rpc_cmd_mesh_diag_discover_topology, NULL);
