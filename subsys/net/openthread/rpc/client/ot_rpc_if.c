/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>
#include <ot_rpc_macros.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>

#include <openthread/ip6.h>
#include <openthread/link.h>

#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_core.h>

LOG_MODULE_DECLARE(ot_rpc, LOG_LEVEL_DBG);

struct ot_rpc_l2_data {
};

static enum net_verdict ot_rpc_l2_recv(struct net_if *iface, struct net_pkt *pkt)
{
	OT_RPC_UNUSED(iface);
	OT_RPC_UNUSED(pkt);

	return NET_CONTINUE;
}

static int ot_rpc_l2_send(struct net_if *iface, struct net_pkt *pkt)
{
	OT_RPC_UNUSED(iface);

	bool encoded_ok = false;
	const size_t len = net_pkt_get_len(pkt);
	const size_t cbor_buffer_size = 10 + len;
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	if (!zcbor_bstr_start_encode(ctx.zs)) {
		goto out;
	}

	for (struct net_buf *buf = pkt->buffer; buf; buf = buf->frags) {
		memcpy(ctx.zs[0].payload_mut, buf->data, buf->len);
		ctx.zs->payload_mut += buf->len;
	}

	if (!zcbor_bstr_end_encode(ctx.zs, NULL)) {
		goto out;
	}

	NET_DBG("Sending Ip6 packet to RPC server");

	encoded_ok = true;
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_IF_SEND, &ctx, ot_rpc_decode_void, NULL);

out:
	if (!encoded_ok) {
		NET_ERR("Failed to encode packet data");
	}

	net_pkt_unref(pkt);

	return (int)len;
}

static inline bool is_anycast_locator(const otNetifAddress *addr)
{
	return addr->mMeshLocal && addr->mAddress.mFields.m16[4] == net_htons(0x0000) &&
	       addr->mAddress.mFields.m16[5] == net_htons(0x00ff) &&
	       addr->mAddress.mFields.m16[6] == net_htons(0xfe00) &&
	       addr->mAddress.mFields.m8[14] == 0xfc;
}

static void add_ipv6_addr_to_zephyr(struct net_if *iface, const otNetifAddress *addr)
{
	struct net_if_addr *if_addr;
	enum net_addr_type addr_type;

	for (; addr; addr = addr->mNext) {
		if (addr->mRloc || is_anycast_locator(addr)) {
			continue;
		}

		switch (addr->mAddressOrigin) {
		case OT_ADDRESS_ORIGIN_THREAD:
		case OT_ADDRESS_ORIGIN_SLAAC:
			addr_type = NET_ADDR_AUTOCONF;
			break;
		case OT_ADDRESS_ORIGIN_DHCPV6:
			addr_type = NET_ADDR_DHCP;
			break;
		case OT_ADDRESS_ORIGIN_MANUAL:
			addr_type = NET_ADDR_MANUAL;
			break;
		default:
			NET_ERR("Unknown OpenThread address origin ignored.");
			continue;
		}

		if_addr = net_if_ipv6_addr_add(iface, (struct net_in6_addr *)(&addr->mAddress),
					       addr_type, 0);

		if (if_addr == NULL) {
			NET_ERR("Cannot add OpenThread unicast address");
			continue;
		}

		if_addr->is_mesh_local = addr->mMeshLocal;
		if_addr->addr_state = addr->mPreferred ? NET_ADDR_PREFERRED : NET_ADDR_DEPRECATED;
	}
}

static void rm_ipv6_addr_from_zephyr(struct net_if *iface, const otNetifAddress *ot_addr)
{
	struct net_if_ipv6 *ipv6;

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		return;
	}

	for (int i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		struct net_if_addr *if_addr = &ipv6->unicast[i];
		bool used = false;

		if (!if_addr->is_used) {
			continue;
		}

		for (const otNetifAddress *addr = ot_addr; addr; addr = addr->mNext) {
			if (net_ipv6_addr_cmp((struct net_in6_addr *)(&addr->mAddress),
					      &if_addr->address.in6_addr)) {
				used = true;
				break;
			}
		}

		if (!used) {
			net_if_ipv6_addr_rm(iface, &if_addr->address.in6_addr);
		}
	}
}

static void update_netif_addrs(struct net_if *iface)
{
	const otNetifAddress *addr = otIp6GetUnicastAddresses(NULL);

	rm_ipv6_addr_from_zephyr(iface, addr);
	add_ipv6_addr_to_zephyr(iface, addr);
}

static void ot_state_changed_handler(otChangedFlags flags, void *context)
{
	struct net_if *iface = context;

	NET_DBG("OpenThread state change: %x", flags);

	if (flags & (OT_CHANGED_IP6_ADDRESS_ADDED | OT_CHANGED_IP6_ADDRESS_REMOVED)) {
		update_netif_addrs(iface);
	}
}

static int ot_rpc_l2_enable(struct net_if *iface, bool state)
{
	const size_t cbor_buffer_size = 1;
	struct nrf_rpc_cbor_ctx ctx;

	if (!state) {
		otRemoveStateChangeCallback(NULL, ot_state_changed_handler, iface);
	}

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	nrf_rpc_encode_bool(&ctx, state);
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_IF_ENABLE, &ctx, ot_rpc_decode_void, NULL);

	if (state) {
		net_if_set_link_addr(iface, (uint8_t *)otLinkGetExtendedAddress(NULL)->m8,
				     OT_EXT_ADDRESS_SIZE, NET_LINK_IEEE802154);
		update_netif_addrs(iface);
		if (otSetStateChangedCallback(NULL, ot_state_changed_handler, iface)) {
			NET_ERR("otSetStateChangedCallback failed");
		}
	}

	return 0;
}

static enum net_l2_flags ot_rpc_l2_flags(struct net_if *iface)
{
	return NET_L2_MULTICAST | NET_L2_MULTICAST_SKIP_JOIN_SOLICIT_NODE;
}

NET_L2_INIT(OPENTHREAD, ot_rpc_l2_recv, ot_rpc_l2_send, ot_rpc_l2_enable, ot_rpc_l2_flags);

static int ot_rpc_dev_init(const struct device *dev)
{
	return 0;
}

static void ot_rpc_if_init(struct net_if *iface)
{
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_flag_set(iface, NET_IF_IPV6_NO_ND);
	net_if_flag_set(iface, NET_IF_IPV6_NO_MLD);
}

struct net_if_api ot_rpc_if_api = {.init = ot_rpc_if_init};

NET_DEVICE_INIT(ot_rpc, "ot_rpc", ot_rpc_dev_init, /* pm */ NULL, /* device instance data */ NULL,
		/* config */ NULL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ot_rpc_if_api, OPENTHREAD,
		struct ot_rpc_l2_data, 1280);

static void ot_rpc_cmd_if_receive(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				  void *handler_data)
{
	const uint8_t *pkt_data;
	size_t pkt_data_len = 0;
	struct net_if *iface;
	struct net_pkt *pkt = NULL;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	pkt_data = nrf_rpc_decode_buffer_ptr_and_size(ctx, &pkt_data_len);
	if (pkt_data && pkt_data_len) {
		iface = net_if_get_first_by_type(&NET_L2_GET_NAME(OPENTHREAD));
		if (iface) {
			pkt = net_pkt_rx_alloc_with_buffer(iface, pkt_data_len, NET_AF_UNSPEC, 0,
							   K_NO_WAIT);
		} else {
			NET_ERR("There is no net interface for OpenThread");
		}

		if (pkt) {
			net_pkt_write(pkt, pkt_data, pkt_data_len);
		} else {
			NET_ERR("Failed to reserve net pkt");
		}
	}

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		NET_ERR("Failed to decode packet data");
		ot_rpc_report_cmd_decoding_error(OT_RPC_CMD_IF_RECEIVE);
		if (pkt) {
			net_pkt_unref(pkt);
		}
		return;
	}

	if (pkt) {
		NET_DBG("Passing Ip6 packet to Zephyr net stack");
		if (net_recv_data(iface, pkt) < 0) {
			NET_ERR("net_recv_data failed");
			net_pkt_unref(pkt);
		}
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 0);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_if_receive, OT_RPC_CMD_IF_RECEIVE,
			 ot_rpc_cmd_if_receive, NULL);
