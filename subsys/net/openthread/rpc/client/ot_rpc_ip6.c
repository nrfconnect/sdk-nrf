/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_types.h>
#include <ot_rpc_common.h>
#include <ot_rpc_macros.h>
#include <ot_rpc_os.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <nrf_rpc_cbor.h>

#include <openthread/ip6.h>

#include <stdio.h>

#define OT_RPC_MAX_NUM_UNICAST_ADDRESSES   8
#define OT_RPC_MAX_NUM_MULTICAST_ADDRESSES 8

const otNetifAddress *otIp6GetUnicastAddresses(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	int rc = 0;
	size_t count = 0;
	static otNetifAddress addrs[OT_RPC_MAX_NUM_UNICAST_ADDRESSES];

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_IP6_GET_UNICAST_ADDRESSES, &ctx);

	while (zcbor_list_start_decode(ctx.zs)) {
		otNetifAddress *addr;
		struct zcbor_string ipv6_addr;
		uint16_t flags;

		if (count >= OT_RPC_ARRAY_SIZE(addrs)) {
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

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_IP6_GET_MULTICAST_ADDRESSES, &ctx);

	while (zcbor_bstr_decode(ctx.zs, &ipv6_addr)) {
		otNetifMulticastAddress *addr;

		if (count >= OT_RPC_ARRAY_SIZE(addrs)) {
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
	otError error;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1);
	nrf_rpc_encode_bool(&ctx, aEnabled);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_IP6_SET_ENABLED, &ctx, ot_rpc_decode_error,
				&error);

	return error;
}

bool otIp6IsEnabled(otInstance *aInstance)
{
	struct nrf_rpc_cbor_ctx ctx;
	bool enabled;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 0);

	nrf_rpc_cbor_cmd_rsp_no_err(&ot_group, OT_RPC_CMD_IP6_IS_ENABLED, &ctx);

	enabled = nrf_rpc_decode_bool(&ctx);

	if (!nrf_rpc_decoding_done_and_check(&ot_group, &ctx)) {
		ot_rpc_report_rsp_decoding_error(OT_RPC_CMD_IP6_IS_ENABLED);
	}

	return enabled;
}

otError otIp6SubscribeMulticastAddress(otInstance *aInstance, const otIp6Address *aAddress)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + OT_IP6_ADDRESS_SIZE);
	nrf_rpc_encode_buffer(&ctx, (const char *)aAddress, OT_IP6_ADDRESS_SIZE);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_IP6_SUBSCRIBE_MADDR, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}

otError otIp6UnsubscribeMulticastAddress(otInstance *aInstance, const otIp6Address *aAddress)
{
	struct nrf_rpc_cbor_ctx ctx;
	otError error;

	OT_RPC_UNUSED(aInstance);

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, 1 + OT_IP6_ADDRESS_SIZE);

	nrf_rpc_encode_buffer(&ctx, (const char *)aAddress, OT_IP6_ADDRESS_SIZE);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_IP6_UNSUBSCRIBE_MADDR, &ctx,
				ot_rpc_decode_error, &error);

	return error;
}

void otIp6AddressToString(const otIp6Address *aAddress, char *aBuffer, uint16_t aSize)
{
	snprintf(aBuffer, aSize, "%x:%x:%x:%x:%x:%x:%x:%x",
		 ot_rpc_os_htons(aAddress->mFields.m16[0]),
		 ot_rpc_os_htons(aAddress->mFields.m16[1]),
		 ot_rpc_os_htons(aAddress->mFields.m16[2]),
		 ot_rpc_os_htons(aAddress->mFields.m16[3]),
		 ot_rpc_os_htons(aAddress->mFields.m16[4]),
		 ot_rpc_os_htons(aAddress->mFields.m16[5]),
		 ot_rpc_os_htons(aAddress->mFields.m16[6]),
		 ot_rpc_os_htons(aAddress->mFields.m16[7]));
}
