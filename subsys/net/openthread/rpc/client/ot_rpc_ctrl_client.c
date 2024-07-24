/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>

#include <nrf_rpc_cbor.h>

#include <openthread/ip6.h>

#include <zephyr/sys/util_macro.h>

#include <string.h>

NRF_RPC_GROUP_DECLARE(ot_group);

#define OT_RPC_MAX_NUM_UNICAST_ADDRESSES 8

static struct ot_unicast_addresses {
	otNetifAddress *head;
	otNetifAddress buffer[OT_RPC_MAX_NUM_UNICAST_ADDRESSES];
} ot_unicast_addresses;

static void decode_void(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			void *handler_data)
{
	ARG_UNUSED(group);
	ARG_UNUSED(ctx);
	ARG_UNUSED(handler_data);
}

static void decode_unicast_addresses(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct ot_unicast_addresses *addrs = handler_data;
	size_t count = 0;

	while (zcbor_list_start_decode(ctx->zs) && count < OT_RPC_MAX_NUM_UNICAST_ADDRESSES) {
		otNetifAddress *addr = &addrs->buffer[count];
		struct zcbor_string ipv6_addr;
		uint16_t flags;

		if (!zcbor_bstr_decode(ctx->zs, &ipv6_addr) ||
		    !zcbor_uint_decode(ctx->zs, &addr->mPrefixLength, sizeof(uint8_t)) ||
		    !zcbor_uint_decode(ctx->zs, &addr->mAddressOrigin, sizeof(uint8_t)) ||
		    !zcbor_uint_decode(ctx->zs, &flags, sizeof(uint16_t)) ||
		    !zcbor_list_end_decode(ctx->zs) ||
		    ipv6_addr.len != sizeof(addr->mAddress.mFields.m8)) {
			break;
		}

		memcpy(addr->mAddress.mFields.m8, ipv6_addr.value, ipv6_addr.len);
		addr->mPreferred = (flags & BIT(OT_RPC_NETIF_ADDRESS_PREFERRED_OFFSET));
		addr->mValid = (flags & BIT(OT_RPC_NETIF_ADDRESS_VALID_OFFSET));
		addr->mScopeOverrideValid = (flags & BIT(OT_RPC_NETIF_ADDRESS_SCOPE_VALID_OFFSET));
		addr->mScopeOverride = (flags & OT_RPC_NETIF_ADDRESS_SCOPE_MASK) >>
				       OT_RPC_NETIF_ADDRESS_SCOPE_OFFSET;
		addr->mRloc = (flags & BIT(OT_RPC_NETIF_ADDRESS_RLOC_OFFSET));
		addr->mMeshLocal = (flags & BIT(OT_RPC_NETIF_ADDRESS_MESH_LOCAL_OFFSET));
		addr->mNext = NULL;

		if (count > 0) {
			addrs->buffer[count - 1].mNext = addr;
		}

		++count;
	}

	addrs->head = (count > 0) ? addrs->buffer : NULL;
}

const otNetifAddress *otIp6GetUnicastAddresses(otInstance *aInstance)
{
	const size_t cbor_buffer_size = 0;
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_IP6_GET_UNICAST_ADDRESSES, &ctx,
				decode_unicast_addresses, &ot_unicast_addresses);

	return ot_unicast_addresses.head;
}

static void decode_error(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			 void *handler_data)
{
	otError *error = handler_data;
	int32_t errorCode;

	*error = zcbor_int32_decode(ctx->zs, &errorCode) ? (otError)errorCode : -EINVAL;
}

otError otSetStateChangedCallback(otInstance *aInstance, otStateChangedCallback aCallback,
				  void *aContext)
{
	const size_t cbor_buffer_size = 10;
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	/* TODO: implement callback & address pseudonymization. */
	zcbor_uint32_put(ctx.zs, (uint32_t)aCallback);
	zcbor_uint32_put(ctx.zs, (uint32_t)aContext);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_SET_STATE_CHANGED_CALLBACK, &ctx,
				decode_error, &error);

	return error;
}

void otRemoveStateChangeCallback(otInstance *aInstance, otStateChangedCallback aCallback,
				 void *aContext)
{
	const size_t cbor_buffer_size = 10;
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	/* TODO: implement callback & address pseudonymization. */
	zcbor_uint32_put(ctx.zs, (uint32_t)aCallback);
	zcbor_uint32_put(ctx.zs, (uint32_t)aContext);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_REMOVE_STATE_CHANGED_CALLBACK, &ctx,
				decode_void, NULL);
}

static void ot_rpc_cmd_state_changed(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	uint32_t callback;
	uint32_t context;
	otChangedFlags flags;
	bool decoded_ok;

	decoded_ok = zcbor_uint32_decode(ctx->zs, &callback) &&
		     zcbor_uint32_decode(ctx->zs, &context) && zcbor_uint32_decode(ctx->zs, &flags);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		goto out;
	}

	/* TODO: implement callback & address pseudonymization. */
	((otStateChangedCallback)callback)(flags, (void *)context);

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 0);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_state_changed, OT_RPC_CMD_STATE_CHANGED,
			 ot_rpc_cmd_state_changed, NULL);