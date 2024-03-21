/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ot_rpc_ids.h>

#include <nrf_rpc_cbor.h>

#include <openthread/cli.h>

#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_core.h>

NRF_RPC_GROUP_DECLARE(ot_group);
LOG_MODULE_DECLARE(ot_rpc, LOG_LEVEL_DBG);

static void decode_void(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			void *handler_data)
{
	ARG_UNUSED(group);
	ARG_UNUSED(ctx);
	ARG_UNUSED(handler_data);
}

struct ot_rpc_l2_data {
};

static enum net_verdict ot_rpc_l2_recv(struct net_if *iface, struct net_pkt *pkt)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(pkt);

	return NET_CONTINUE;
}

static int ot_rpc_l2_send(struct net_if *iface, struct net_pkt *pkt)
{
	ARG_UNUSED(iface);

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
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_IF_SEND, &ctx, decode_void, NULL);

out:
	if (!encoded_ok) {
		NET_ERR("Failed to encode packet data");
	}

	net_pkt_unref(pkt);

	return (int)len;
}

static int ot_rpc_l2_enable(struct net_if *iface, bool state)
{
	const size_t cbor_buffer_size = 1;
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(&ot_group, ctx, cbor_buffer_size);
	zcbor_bool_put(ctx.zs, state);

	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_IF_ENABLE, &ctx, decode_void, NULL);

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
	/* TODO: auto-start the interface when nRF RPC transport is ready? */
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
	struct zcbor_string pkt_data;
	bool decoded_ok;
	struct net_if *iface;
	struct net_pkt *pkt = NULL;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_bstr_decode(ctx->zs, &pkt_data);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		NET_ERR("Failed to decode packet data");
		goto out;
	}

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(OPENTHREAD));
	if (!iface) {
		NET_ERR("There is no net interface for OpenThread");
		goto out;
	}

	pkt = net_pkt_rx_alloc_with_buffer(iface, pkt_data.len, AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		NET_ERR("Failed to reserve net pkt");
		goto out;
	}

	net_pkt_write(pkt, pkt_data.value, pkt_data.len);

	NET_DBG("Passing Ip6 packet to Zephyr net stack");

	if (net_recv_data(iface, pkt) < 0) {
		NET_ERR("net_recv_data failed");
		goto out;
	}

	pkt = NULL;

out:
	if (pkt) {
		net_pkt_unref(pkt);
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 0);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_if_receive, OT_RPC_CMD_IF_RECEIVE,
			 ot_rpc_cmd_if_receive, NULL);
