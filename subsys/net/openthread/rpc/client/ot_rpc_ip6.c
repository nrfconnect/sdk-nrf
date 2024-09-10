/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <zephyr/sys/util_macro.h>

#include <nrf_rpc_cbor.h>

#include <openthread/ip6.h>

#define OT_RPC_MAX_NUM_UNICAST_ADDRESSES   8
#define OT_RPC_MAX_NUM_MULTICAST_ADDRESSES 8

static otError decode_ot_error(struct nrf_rpc_cbor_ctx *ctx)
{
	otError error;

	if (!zcbor_uint_decode(ctx->zs, &error, sizeof(error))) {
		error = OT_ERROR_PARSE;
	}

	nrf_rpc_cbor_decoding_done(&ot_group, ctx);

	return error;
}

const otNetifAddress *otIp6GetUnicastAddresses(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	int rc = 0;
	size_t count = 0;
	static otNetifAddress addrs[OT_RPC_MAX_NUM_UNICAST_ADDRESSES];

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_IP6_GET_UNICAST_ADDRESSES, &ctx);

	while (zcbor_list_start_decode(ctx.zs)) {
		otNetifAddress *addr;
		struct zcbor_string ipv6_addr;
		uint16_t flags;

		if (count >= ARRAY_SIZE(addrs)) {
			rc = -ENOBUFS;
			break;
		}

		addr = &addrs[count];
		memset(addr, 0, sizeof(*addr));

		if (!zcbor_bstr_decode(ctx.zs, &ipv6_addr) ||
		    !zcbor_uint_decode(ctx.zs, &addr->mPrefixLength, sizeof(addr->mPrefixLength)) ||
		    !zcbor_uint_decode(ctx.zs, &addr->mAddressOrigin,
				       sizeof(addr->mAddressOrigin)) ||
		    !zcbor_uint_decode(ctx.zs, &flags, sizeof(flags)) ||
		    !zcbor_list_end_decode(ctx.zs) || ipv6_addr.len != OT_IP6_ADDRESS_SIZE) {
			rc = -EBADMSG;
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

		if (count > 0) {
			addrs[count - 1].mNext = addr;
		}

		++count;
	}

	if (rc) {
		nrf_rpc_err(rc, NRF_RPC_ERR_SRC_RECV, &ot_group,
			    OT_RPC_CMD_IP6_GET_UNICAST_ADDRESSES, NRF_RPC_PACKET_TYPE_RSP);
	}

	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	return (count > 0) ? addrs : NULL;
}

const otNetifMulticastAddress *otIp6GetMulticastAddresses(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	int rc = 0;
	size_t count = 0;
	struct zcbor_string ipv6_addr;
	static otNetifMulticastAddress addrs[OT_RPC_MAX_NUM_MULTICAST_ADDRESSES];

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_IP6_GET_MULTICAST_ADDRESSES, &ctx);

	while (zcbor_bstr_decode(ctx.zs, &ipv6_addr)) {
		otNetifMulticastAddress *addr;

		if (count >= ARRAY_SIZE(addrs)) {
			rc = -ENOBUFS;
			break;
		}

		addr = &addrs[count];
		memset(addr, 0, sizeof(*addr));
		memcpy(addr->mAddress.mFields.m8, ipv6_addr.value, ipv6_addr.len);

		if (count > 0) {
			addrs[count - 1].mNext = addr;
		}

		++count;
	}

	if (rc) {
		nrf_rpc_err(rc, NRF_RPC_ERR_SRC_RECV, &ot_group,
			    OT_RPC_CMD_IP6_GET_MULTICAST_ADDRESSES, NRF_RPC_PACKET_TYPE_RSP);
	}

	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	return (count > 0) ? addrs : NULL;
}

otError otIp6SetEnabled(otInstance *aInstance, bool aEnabled)
{
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1);

	if (!zcbor_bool_encode(ctx.zs, &aEnabled)) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		return OT_ERROR_INVALID_ARGS;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_IP6_SET_ENABLED, &ctx);

	return decode_ot_error(&ctx);
}

bool otIp6IsEnabled(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool enabled = false;
	bool decoded_ok;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_IP6_IS_ENABLED, &ctx);

	decoded_ok = zcbor_bool_decode(ctx.zs, &enabled);
	nrf_rpc_cbor_decoding_done(&ot_group, &ctx);

	if (!decoded_ok) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &ot_group, OT_RPC_CMD_IP6_IS_ENABLED,
			    NRF_RPC_PACKET_TYPE_RSP);
	}

	return enabled;
}

otError otIp6SubscribeMulticastAddress(otInstance *aInstance, const otIp6Address *aAddress)
{
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + OT_IP6_ADDRESS_SIZE);

	if (!zcbor_bstr_encode_ptr(ctx.zs, (const char *)aAddress, OT_IP6_ADDRESS_SIZE)) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		return OT_ERROR_INVALID_ARGS;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_IP6_SUBSCRIBE_MADDR, &ctx);

	return decode_ot_error(&ctx);
}

otError otIp6UnsubscribeMulticastAddress(otInstance *aInstance, const otIp6Address *aAddress)
{
	struct nrf_rpc_cbor_ctx ctx;

	ARG_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + OT_IP6_ADDRESS_SIZE);

	if (!zcbor_bstr_encode_ptr(ctx.zs, (const char *)aAddress, OT_IP6_ADDRESS_SIZE)) {
		NRF_RPC_CBOR_DISCARD(&ot_group, ctx);
		return OT_ERROR_INVALID_ARGS;
	}

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_IP6_UNSUBSCRIBE_MADDR, &ctx);

	return decode_ot_error(&ctx);
}
