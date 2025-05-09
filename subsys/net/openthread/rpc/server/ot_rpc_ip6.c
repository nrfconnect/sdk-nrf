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

#include <openthread/ip6.h>

static void ot_rpc_cmd_ip6_get_unicast_addrs(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	const otNetifAddress *addr;
	size_t addr_count;

	nrf_rpc_cbor_decoding_done(group, ctx);

	ot_rpc_mutex_lock();
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
			((uint16_t)addr->mScopeOverride << OT_RPC_NETIF_ADDRESS_SCOPE_OFFSET) |
			((uint16_t)addr->mRloc << OT_RPC_NETIF_ADDRESS_RLOC_OFFSET) |
			((uint16_t)addr->mMeshLocal << OT_RPC_NETIF_ADDRESS_MESH_LOCAL_OFFSET);

		zcbor_list_start_encode(rsp_ctx.zs, 4);
		nrf_rpc_encode_buffer(&rsp_ctx, addr->mAddress.mFields.m8,
				      sizeof(addr->mAddress.mFields.m8));
		nrf_rpc_encode_uint(&rsp_ctx, addr->mPrefixLength);
		nrf_rpc_encode_uint(&rsp_ctx, addr->mAddressOrigin);
		nrf_rpc_encode_uint(&rsp_ctx, flags);
		zcbor_list_end_encode(rsp_ctx.zs, 4);
	}

	ot_rpc_mutex_unlock();

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_ip6_get_multicast_addrs(const struct nrf_rpc_group *group,
					       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	const otNetifMulticastAddress *addr;
	size_t addr_count;

	nrf_rpc_cbor_decoding_done(group, ctx);

	ot_rpc_mutex_lock();
	addr = otIp6GetMulticastAddresses(openthread_get_default_instance());
	addr_count = 0;

	for (const otNetifMulticastAddress *p = addr; p != NULL; p = p->mNext) {
		++addr_count;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, addr_count * 17);

	for (; addr; addr = addr->mNext) {
		nrf_rpc_encode_buffer(&rsp_ctx, addr->mAddress.mFields.m8, OT_IP6_ADDRESS_SIZE);
	}

	ot_rpc_mutex_unlock();

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_ip6_set_enabled(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool enabled;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	enabled = nrf_rpc_decode_bool(ctx);
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_IP6_SET_ENABLED);
		return;
	}

	ot_rpc_mutex_lock();
	error = otIp6SetEnabled(openthread_get_default_instance(), enabled);
	ot_rpc_mutex_unlock();

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	nrf_rpc_encode_uint(&rsp_ctx, error);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_ip6_is_enabled(const struct nrf_rpc_group *group,
				      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	bool enabled;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	nrf_rpc_cbor_decoding_done(group, ctx);

	ot_rpc_mutex_lock();
	enabled = otIp6IsEnabled(openthread_get_default_instance());
	ot_rpc_mutex_unlock();

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 1);
	nrf_rpc_encode_bool(&rsp_ctx, enabled);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_ip6_subscribe_multicast_address(const struct nrf_rpc_group *group,
						       struct nrf_rpc_cbor_ctx *ctx,
						       void *handler_data)
{
	const uint8_t *addr_str;
	size_t addr_size = 0;
	otIp6Address addr;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	addr_str = nrf_rpc_decode_buffer_ptr_and_size(ctx, &addr_size);

	if (addr_size == OT_IP6_ADDRESS_SIZE) {
		memcpy(addr.mFields.m8, addr_str, addr_size);
	} else {
		nrf_rpc_decoder_invalid(ctx, ZCBOR_ERR_UNKNOWN);
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_IP6_SUBSCRIBE_MADDR);
		return;
	}

	ot_rpc_mutex_lock();
	error = otIp6SubscribeMulticastAddress(openthread_get_default_instance(), &addr);
	ot_rpc_mutex_unlock();

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	nrf_rpc_encode_uint(&rsp_ctx, error);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_ip6_unsubscribe_multicast_address(const struct nrf_rpc_group *group,
							 struct nrf_rpc_cbor_ctx *ctx,
							 void *handler_data)
{
	const uint8_t *addr_str;
	size_t addr_size = 0;
	otIp6Address addr;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	addr_str = nrf_rpc_decode_buffer_ptr_and_size(ctx, &addr_size);

	if (addr_size == OT_IP6_ADDRESS_SIZE) {
		memcpy(addr.mFields.m8, addr_str, addr_size);
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_IP6_UNSUBSCRIBE_MADDR);
		return;
	}

	if (addr_size != OT_IP6_ADDRESS_SIZE) {
		error = OT_ERROR_INVALID_ARGS;
		goto out;
	}

	ot_rpc_mutex_lock();
	error = otIp6UnsubscribeMulticastAddress(openthread_get_default_instance(), &addr);
	ot_rpc_mutex_unlock();

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	nrf_rpc_encode_uint(&rsp_ctx, error);
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
