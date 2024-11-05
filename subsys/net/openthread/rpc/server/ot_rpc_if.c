/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ot_rpc_ids.h>
#include <ot_rpc_common.h>

#include <nrf_rpc_cbor.h>

#include <openthread/cli.h>

#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/openthread.h>

LOG_MODULE_DECLARE(ot_rpc, LOG_LEVEL_DBG);

struct net_context *recv_net_context;

static void ot_rpc_if_recv_cb(struct net_context *context, struct net_pkt *pkt,
			      union net_ip_header *ip_hdr, union net_proto_header *proto_hdr,
			      int status, void *user_data)
{
	ARG_UNUSED(context);
	ARG_UNUSED(ip_hdr);
	ARG_UNUSED(proto_hdr);
	ARG_UNUSED(status);
	ARG_UNUSED(user_data);

	if (pkt == NULL) {
		return;
	}

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

	NET_DBG("Passing Ip6 packet to RPC client");

	encoded_ok = true;
	nrf_rpc_cbor_cmd_no_err(&ot_group, OT_RPC_CMD_IF_RECEIVE, &ctx, ot_rpc_decode_void, NULL);

out:
	if (!encoded_ok) {
		NET_ERR("Failed to encode packet data");
	}

	net_pkt_unref(pkt);
}

static void ot_rpc_cmd_if_enable(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				 void *handler_data)
{
	bool enable;
	bool decoded_ok;
	int ret;
	struct net_if *iface;
	struct nrf_rpc_cbor_ctx rsp_ctx;

	decoded_ok = zcbor_bool_decode(ctx->zs, &enable);
	nrf_rpc_cbor_decoding_done(group, ctx);

	if (!decoded_ok) {
		NET_ERR("Failed to decode RPC argument");
		goto out;
	}

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(OPENTHREAD));
	if (!iface) {
		NET_ERR("There is no net interface for OpenThread");
		goto out;
	}

	if (recv_net_context != NULL) {
		net_context_put(recv_net_context);
		recv_net_context = NULL;
	}

	if (enable) {
		struct sockaddr_ll addr;

		ret = net_context_get(AF_PACKET, SOCK_RAW, IPPROTO_RAW, &recv_net_context);
		if (ret) {
			NET_ERR("Failed to allocate recv net context");
			goto out;
		}

		addr.sll_family = AF_PACKET;
		addr.sll_protocol = htons(IPPROTO_RAW);
		addr.sll_ifindex = net_if_get_by_iface(iface);

		ret = net_context_bind(recv_net_context, (const struct sockaddr *)&addr,
				       sizeof(addr));
		if (ret) {
			NET_ERR("Failed to bind net context");
			goto out;
		}

		ret = net_context_recv(recv_net_context, ot_rpc_if_recv_cb, K_NO_WAIT, NULL);
		if (ret) {
			NET_ERR("Failed to recv from net context");
			goto out;
		}
	}

out:
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 0);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_if_enable, OT_RPC_CMD_IF_ENABLE, ot_rpc_cmd_if_enable,
			 NULL);

static void ot_rpc_cmd_if_send(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
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

	pkt = net_pkt_alloc_with_buffer(iface, pkt_data.len, AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		NET_ERR("Failed to reserve net pkt");
		goto out;
	}

	net_pkt_write(pkt, pkt_data.value, pkt_data.len);

	NET_DBG("Sending Ip6 packet to OpenThread");

	if (net_send_data(pkt) < 0) {
		NET_ERR("net_send_data failed");
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

NRF_RPC_CBOR_CMD_DECODER(ot_group, ot_rpc_cmd_if_send, OT_RPC_CMD_IF_SEND, ot_rpc_cmd_if_send,
			 NULL);
