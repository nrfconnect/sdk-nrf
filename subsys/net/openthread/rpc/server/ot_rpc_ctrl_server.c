/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>

#include <nrf_rpc_cbor.h>

#include <openthread/ip6.h>
#include <openthread/instance.h>

#include <zephyr/net/openthread.h>

NRF_RPC_GROUP_DECLARE(ot_group);

/* TODO: move to common */
typedef struct ot_rpc_callback {
	uint32_t callback;
	uint32_t context;
} ot_rpc_callback_t;

static ot_rpc_callback_t ot_rpc_callbacks[10];

static struct ot_rpc_callback *ot_rpc_callback_put(uint32_t callback, uint32_t context)
{
	ot_rpc_callback_t *free = NULL;
	ot_rpc_callback_t *cb = ot_rpc_callbacks;

	while (cb < ot_rpc_callbacks + ARRAY_SIZE(ot_rpc_callbacks)) {
		if (cb->callback == 0) {
			free = cb;
		} else if (cb->callback == callback && cb->context == context) {
			return cb;
		}

		++cb;
	}

	if (free != NULL) {
		free->callback = callback;
		free->context = context;
	}

	return free;
}

static ot_rpc_callback_t *ot_rpc_callback_del(uint32_t callback, uint32_t context)
{
	ot_rpc_callback_t *cb = ot_rpc_callbacks;

	while (cb < ot_rpc_callbacks + ARRAY_SIZE(ot_rpc_callbacks)) {
		if (cb->callback == callback && cb->context == context) {
			cb->callback = 0;
			cb->context = 0;
			return cb;
		}

		++cb;
	}

	return NULL;
}

static void decode_void(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			void *handler_data)
{
	ARG_UNUSED(group);
	ARG_UNUSED(ctx);
	ARG_UNUSED(handler_data);
}

static void ot_rpc_cmd_ip6_get_unicast_addrs(const struct nrf_rpc_group *group,
					     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	const otNetifAddress *addr;
	size_t addr_count;

	nrf_rpc_cbor_decoding_done(group, ctx);

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

	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_state_changed_callback(otChangedFlags aFlags, void *aContext)
{
	struct nrf_rpc_cbor_ctx ctx;
	ot_rpc_callback_t *cb = aContext;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 15);
	zcbor_uint32_put(ctx.zs, cb->callback);
	zcbor_uint32_put(ctx.zs, cb->context);
	zcbor_uint32_put(ctx.zs, aFlags);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_STATE_CHANGED, &ctx, decode_void, NULL);
}

static void ot_rpc_cmd_set_state_changed_callback(const struct nrf_rpc_group *group,
						  struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	uint32_t callback;
	uint32_t context;
	bool decoded_ok;
	ot_rpc_callback_t *cb;
	otError error;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_uint32_decode(ctx->zs, &callback);
	decoded_ok = decoded_ok && zcbor_uint32_decode(ctx->zs, &context);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		error = OT_ERROR_INVALID_ARGS;
		goto out;
	}

	cb = ot_rpc_callback_put(callback, context);

	if (!cb) {
		error = OT_ERROR_NO_BUFS;
		goto out;
	}

	error = otSetStateChangedCallback(openthread_get_default_instance(),
					  ot_state_changed_callback, cb);

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 5);
	zcbor_uint32_put(rsp_ctx.zs, (uint32_t)error);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void ot_rpc_cmd_remove_state_changed_callback(const struct nrf_rpc_group *group,
						     struct nrf_rpc_cbor_ctx *ctx,
						     void *handler_data)
{
	uint32_t callback;
	uint32_t context;
	bool decoded_ok;
	ot_rpc_callback_t *cb;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_uint32_decode(ctx->zs, &callback);
	decoded_ok = decoded_ok && zcbor_uint32_decode(ctx->zs, &context);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		goto out;
	}

	cb = ot_rpc_callback_del(callback, context);

	if (!cb) {
		goto out;
	}

	otRemoveStateChangeCallback(openthread_get_default_instance(), ot_state_changed_callback,
				    cb);

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 0);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_ip6_get_unicast_addrs,
			 OT_RPC_CMD_IP6_GET_UNICAST_ADDRESSES, ot_rpc_cmd_ip6_get_unicast_addrs,
			 NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_set_state_changed_callback,
			 OT_RPC_CMD_SET_STATE_CHANGED_CALLBACK,
			 ot_rpc_cmd_set_state_changed_callback, NULL);

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_remove_state_changed_callback,
			 OT_RPC_CMD_REMOVE_STATE_CHANGED_CALLBACK,
			 ot_rpc_cmd_remove_state_changed_callback, NULL);