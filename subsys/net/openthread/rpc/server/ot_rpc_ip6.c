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

#include <openthread/ip6.h>

static void ot_rpc_cmd_ip6_get_unicast_addrs(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	const otNetifAddress *addr;
	size_t addr_count;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	addr = otIp6GetUnicastAddresses(openthread_get_default_instance());
	addr_count = 0;

	for (const otNetifAddress *p = addr; p != NULL; p = p->mNext) {
		++addr_count;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, addr_count * 25);

	for (; addr; addr = addr->mNext) {
		uint16_t flags =
			((uint16_t)addr->mPreferred << OT_RPC_NETIF_ADDRESS_PREFERRED_OFFSET) |
			((uint16_t)addr->mValid << OT_RPC_NETIF_ADDRESS_VALID_OFFSET) |
			((uint16_t)addr->mScopeOverrideValid
			 << OT_RPC_NETIF_ADDRESS_SCOPE_VALID_OFFSET) |
			((uint16_t)addr->mScopeOverride << OT_RPC_NETIF_ADDRESS_VALID_OFFSET) |
			((uint16_t)addr->mRloc << OT_RPC_NETIF_ADDRESS_RLOC_OFFSET) |
			((uint16_t)addr->mMeshLocal << OT_RPC_NETIF_ADDRESS_MESH_LOCAL_OFFSET);

		zcbor_list_start_encode(rsp_ctx.zs, 4);
		zcbor_bstr_encode_ptr(rsp_ctx.zs, (const char *)addr->mAddress.mFields.m8,
				      sizeof(addr->mAddress.mFields.m8));
		zcbor_uint32_put(rsp_ctx.zs, addr->mPrefixLength);
		zcbor_uint32_put(rsp_ctx.zs, addr->mAddressOrigin);
		zcbor_uint32_put(rsp_ctx.zs, flags);
		zcbor_list_end_encode(rsp_ctx.zs, 4);
	}

	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_ip6_get_multicast_addrs(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	const otNetifMulticastAddress *addr;
	size_t addr_count;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	addr = otIp6GetMulticastAddresses(openthread_get_default_instance());
	addr_count = 0;

	for (const otNetifMulticastAddress *p = addr; p != NULL; p = p->mNext) {
		++addr_count;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, addr_count * 17);

	for (; addr; addr = addr->mNext) {
		zcbor_bstr_encode_ptr(rsp_ctx.zs, (const char *)addr->mAddress.mFields.m8,
				      OT_IP6_ADDRESS_SIZE);
	}

	openthread_api_mutex_unlock(openthread_get_default_context());

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_ip6_set_enabled(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool enabled;
	bool decoded_ok;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_bool_decode(ctx->zs, &enabled);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto out;
	}

	openthread_api_mutex_lock(openthread_get_default_context());
	error = otIp6SetEnabled(openthread_get_default_instance(), enabled);
	openthread_api_mutex_unlock(openthread_get_default_context());

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_ip6_is_enabled(const struct nrf_rpc_group *group,
				      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool enabled;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	openthread_api_mutex_lock(openthread_get_default_context());
	enabled = otIp6IsEnabled(openthread_get_default_instance());
	openthread_api_mutex_unlock(openthread_get_default_context());

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 1);
	zcbor_bool_encode(rsp_ctx.zs, &enabled);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_ip6_subscribe_multicast_address(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	struct zcbor_string addr_bstr;
	otIp6Address addr;
	bool decoded_ok;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_bstr_decode(ctx->zs, &addr_bstr);
	decoded_ok = decoded_ok && (addr_bstr.len == OT_IP6_ADDRESS_SIZE);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto out;
	}

	memcpy(addr.mFields.m8, addr_bstr.value, addr_bstr.len);
	openthread_api_mutex_lock(openthread_get_default_context());
	error = otIp6SubscribeMulticastAddress(openthread_get_default_instance(), &addr);
	openthread_api_mutex_unlock(openthread_get_default_context());

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_ip6_unsubscribe_multicast_address(const struct nrf_rpc_group *group,
							 struct nrf_rpc_cbor_ctx *ctx,
							 void *handler_data)
{
	struct zcbor_string addr_bstr;
	otIp6Address addr;
	bool decoded_ok;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_bstr_decode(ctx->zs, &addr_bstr);
	decoded_ok = decoded_ok && (addr_bstr.len == OT_IP6_ADDRESS_SIZE);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto out;
	}

	memcpy(addr.mFields.m8, addr_bstr.value, addr_bstr.len);
	openthread_api_mutex_lock(openthread_get_default_context());
	error = otIp6UnsubscribeMulticastAddress(openthread_get_default_instance(), &addr);
	openthread_api_mutex_unlock(openthread_get_default_context());

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint_encode(rsp_ctx.zs, &error, sizeof(error));
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_ip6_get_unicast_addrs,
			 OT_RPC_CMD_IP6_GET_UNICAST_ADDRESSES, ot_rpc_cmd_ip6_get_unicast_addrs,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_ip6_get_multicast_addrs,
			 OT_RPC_CMD_IP6_GET_MULTICAST_ADDRESSES, ot_rpc_cmd_ip6_get_multicast_addrs,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_ip6_set_enabled, OT_RPC_CMD_IP6_SET_ENABLED,
			 ot_rpc_cmd_ip6_set_enabled, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_ip6_is_enabled, OT_RPC_CMD_IP6_IS_ENABLED,
			 ot_rpc_cmd_ip6_is_enabled, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_ip6_subscribe_multicast_address,
			 OT_RPC_CMD_IP6_SUBSCRIBE_MADDR, ot_rpc_cmd_ip6_subscribe_multicast_address,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_ip6_unsubscribe_multicast_address,
			 OT_RPC_CMD_IP6_UNSUBSCRIBE_MADDR,
			 ot_rpc_cmd_ip6_unsubscribe_multicast_address, NULL);
