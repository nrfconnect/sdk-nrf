/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * DECT NR+ RPC server: Receives IF_SEND (raw IPv6 from DECT NR+ RPC client), builds net_pkt, sets
 * lladdr from IPv6 dst, calls net_if_send(server_dect_if, pkt).
 * Packets received from the DECT NR+ modem are forwarded to the DECT NR+ RPC client via
 * IF_RECEIVE using the hook in DECT NR+ L2 (dect_net_l2_rpc_forward_register).
 * GET_ADDRS: reads DECT NR+ RPC server DECT iface IPv6 addrs/prefixes and returns them
 * so the DECT NR+ RPC client can mirror them.
 * See dect_rpc_server_ping.c (PING) and dect_rpc_server_shell_line.c (SHELL_LINE / L2 shell RPC).
 */

#include "dect_rpc_ids.h"
#include "dect_rpc_common.h"
#include "dect_rpc_server_internal.h"
#include <nrf_rpc.h>
#include <nrf_rpc_cbor.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/init.h>
#include <zephyr/sys/atomic.h>

#include <net/dect/dect_net_l2.h>
#include <net/dect/dect_net_l2_rpc.h>
#include <net/dect/dect_net_l2_mgmt.h>
#include <net/dect/dect_utils.h>

#if defined(CONFIG_NET_L2_DECT_CONN_MGR)
#include <zephyr/net/conn_mgr_connectivity.h>
#endif

#if defined(CONFIG_DECT_NR_RPC_SHELL)
#include <net/dect/dect_net_l2_shell_util.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dect_rpc, CONFIG_NET_DECT_RPC_LOG_LEVEL);

#define DECT_RPC_MAX_ADDRS   8
#define DECT_RPC_MAX_PREFIXES 4

static struct net_if *server_dect_if;

K_MEM_SLAB_DEFINE_STATIC(dect_rpc_rx_slab, DECT_RPC_MAX_IP_PKT,
			CONFIG_DECT_NR_RPC_SERVER_EVT_QUEUE_LEN, 4);

static void dect_rpc_rx_buf_free(uint8_t *buf);

enum dect_rpc_evt_type {
	DECT_RPC_EVT_IF_RECEIVE,
	DECT_RPC_EVT_IF_LINK_STATE,
};

struct dect_rpc_evt_msg {
	enum dect_rpc_evt_type type;
	union {
		struct {
			uint8_t *data;
			size_t len;
		} if_receive;
		struct {
			bool carrier_ok;
			bool dormant;
		} if_link_state;
	};
};

K_MSGQ_DEFINE(dect_rpc_evt_msgq, sizeof(struct dect_rpc_evt_msg),
	      CONFIG_DECT_NR_RPC_SERVER_EVT_QUEUE_LEN, 4);
static atomic_t dect_rpc_addrs_changed_pending;

static void dect_rpc_rx_buf_free(uint8_t *buf)
{
	if (buf != NULL) {
		k_mem_slab_free(&dect_rpc_rx_slab, (void *)buf);
	}
}

#if CONFIG_DECT_NR_RPC_SERVER_CLIENT_IDLE_TIMEOUT_SEC > 0
static void dect_rpc_client_idle_work_fn(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(dect_rpc_client_idle_work, dect_rpc_client_idle_work_fn);
#endif

static void dect_rpc_evt_work_fn(struct k_work *work);
static K_WORK_DEFINE(dect_rpc_evt_work, dect_rpc_evt_work_fn);

static void dect_rpc_server_drain_evt_queue(void)
{
	struct dect_rpc_evt_msg msg;

	while (k_msgq_get(&dect_rpc_evt_msgq, &msg, K_NO_WAIT) == 0) {
		if (msg.type == DECT_RPC_EVT_IF_RECEIVE && msg.if_receive.data != NULL) {
			dect_rpc_rx_buf_free(msg.if_receive.data);
		}
	}
}

static void dect_rpc_server_client_session_reset(void)
{
	dect_net_l2_rpc_client_set_connected(false);
#if CONFIG_DECT_NR_RPC_SERVER_CLIENT_IDLE_TIMEOUT_SEC > 0
	(void)k_work_cancel_delayable(&dect_rpc_client_idle_work);
#endif
	dect_rpc_server_drain_evt_queue();
}

void dect_rpc_server_note_client_activity(void)
{
	dect_net_l2_rpc_client_set_connected(true);
#if CONFIG_DECT_NR_RPC_SERVER_CLIENT_IDLE_TIMEOUT_SEC > 0
	(void)k_work_reschedule(&dect_rpc_client_idle_work,
				K_SECONDS(CONFIG_DECT_NR_RPC_SERVER_CLIENT_IDLE_TIMEOUT_SEC));
#endif
}

#if CONFIG_DECT_NR_RPC_SERVER_CLIENT_IDLE_TIMEOUT_SEC > 0
static void dect_rpc_client_idle_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	LOG_INF("DECT RPC client idle (%d s); resuming local L2 RX",
		CONFIG_DECT_NR_RPC_SERVER_CLIENT_IDLE_TIMEOUT_SEC);
	dect_rpc_server_client_session_reset();
}
#endif

static void dect_rpc_server_group_bound(const struct nrf_rpc_group *group)
{
	if (group != &dect_rpc_group) {
		return;
	}

	LOG_DBG("DECT RPC group bound; clearing client session until traffic resumes");
	dect_rpc_server_client_session_reset();
	if (atomic_get(&dect_rpc_addrs_changed_pending)) {
		k_work_submit(&dect_rpc_evt_work);
	}
}

static void dect_rpc_rsp_send_i32(const struct nrf_rpc_group *group, int32_t status)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 8);
	dect_rpc_int32_rsp_encode(&rsp_ctx, status);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void dect_rpc_evt_work_fn(struct k_work *work)
{
	struct dect_rpc_evt_msg msg;
	struct nrf_rpc_cbor_ctx ctx;
	bool have_msg;

	ARG_UNUSED(work);

	/* Process one queued event first, then service address sync before the next event. */
	have_msg = k_msgq_get(&dect_rpc_evt_msgq, &msg, K_NO_WAIT) == 0;
	if (!have_msg) {
		goto send_addrs_changed;
	}

	if (!NRF_RPC_GROUP_STATUS(dect_rpc_group)) {
		if (msg.type == DECT_RPC_EVT_IF_RECEIVE && msg.if_receive.data != NULL) {
			dect_rpc_rx_buf_free(msg.if_receive.data);
		}
		dect_rpc_server_client_session_reset();
		goto resubmit;
	}

	switch (msg.type) {
	case DECT_RPC_EVT_IF_RECEIVE:
		if (msg.if_receive.data != NULL && msg.if_receive.len > 0) {
			const size_t cbor_size = 10 + msg.if_receive.len;

			NRF_RPC_CBOR_ALLOC(&dect_rpc_group, ctx, cbor_size);
			if (zcbor_bstr_start_encode(ctx.zs)) {
				memcpy(ctx.zs[0].payload_mut, msg.if_receive.data,
				       msg.if_receive.len);
				ctx.zs->payload_mut += msg.if_receive.len;
				if (zcbor_bstr_end_encode(ctx.zs, NULL)) {
					nrf_rpc_cbor_evt_no_err(&dect_rpc_group,
								DECT_RPC_CMD_IF_RECEIVE, &ctx);
				}
			}
			dect_rpc_rx_buf_free(msg.if_receive.data);
		}
		break;
	case DECT_RPC_EVT_IF_LINK_STATE:
		NRF_RPC_CBOR_ALLOC(&dect_rpc_group, ctx, 4);
		nrf_rpc_encode_bool(&ctx, msg.if_link_state.carrier_ok);
		nrf_rpc_encode_bool(&ctx, msg.if_link_state.dormant);
		nrf_rpc_cbor_evt_no_err(&dect_rpc_group, DECT_RPC_CMD_IF_LINK_STATE, &ctx);
		LOG_DBG("Forwarded link state (carrier=%d dormant=%d) to RPC client",
			msg.if_link_state.carrier_ok, msg.if_link_state.dormant);
		break;
	}

	if (CONFIG_DECT_NR_RPC_SERVER_EVT_PACING_MS > 0) {
		if (IS_ENABLED(CONFIG_NRF_RPC_UART_RELIABLE) ||
		    msg.type != DECT_RPC_EVT_IF_RECEIVE) {
			k_msleep(CONFIG_DECT_NR_RPC_SERVER_EVT_PACING_MS);
		}
	}

send_addrs_changed:
	if (atomic_cas(&dect_rpc_addrs_changed_pending, 1, 0)) {
		if (!NRF_RPC_GROUP_STATUS(dect_rpc_group)) {
			atomic_set(&dect_rpc_addrs_changed_pending, 1);
			return;
		}

		NRF_RPC_CBOR_ALLOC(&dect_rpc_group, ctx, 4);
		nrf_rpc_encode_uint(&ctx, 0U);
		nrf_rpc_cbor_evt_no_err(&dect_rpc_group, DECT_RPC_CMD_IF_ADDRS_CHANGED, &ctx);
		LOG_DBG("Notified RPC client: addresses changed");

		if (CONFIG_DECT_NR_RPC_SERVER_EVT_PACING_MS > 0) {
			k_msleep(CONFIG_DECT_NR_RPC_SERVER_EVT_PACING_MS);
		}
	}

resubmit:
	/* If queued events or a coalesced state update remain, run work again. */
	if (k_msgq_num_used_get(&dect_rpc_evt_msgq) > 0 ||
	    (NRF_RPC_GROUP_STATUS(dect_rpc_group) &&
	     atomic_get(&dect_rpc_addrs_changed_pending))) {
		k_work_submit(&dect_rpc_evt_work);
	}
}

/** Resolve DECT NR+ interface by device name (CONFIG_DECT_MDM_DEVICE_NAME, e.g. "dect0"). */
static struct net_if *get_server_dect_if(void)
{
	int idx = net_if_get_by_name(CONFIG_DECT_MDM_DEVICE_NAME);

	if (idx < 0) {
		return NULL;
	}
	return net_if_get_by_index(idx);
}

/* Called from l2_dect (dect_net_l2_recv) for every received packet. Queue send (event);
 * server-originated command would arrive as event on client (dst=UNKNOWN) and never get RSP.
 */
void dect_rpc_server_forward_recv(struct net_if *iface, struct net_pkt *pkt)
{
	size_t len;
	uint8_t *copy;
	struct dect_rpc_evt_msg msg;

	ARG_UNUSED(iface);

	if (!NRF_RPC_GROUP_STATUS(dect_rpc_group)) {
		dect_rpc_server_client_session_reset();
		return;
	}

	if (!pkt) {
		return;
	}

	len = net_pkt_get_len(pkt);
	if (len == 0) {
		return;
	}

	if (len > DECT_RPC_MAX_IP_PKT) {
		LOG_WRN("DECT recv %zu bytes: exceeds RPC max IP pkt (%d), drop",
			len, DECT_RPC_MAX_IP_PKT);
		return;
	}

	if (k_mem_slab_alloc(&dect_rpc_rx_slab, (void **)&copy, K_NO_WAIT) != 0) {
		LOG_WRN("DECT recv %zu bytes: RPC RX pool exhausted, drop", len);
		return;
	}
	if (net_pkt_read(pkt, copy, len) < 0) {
		dect_rpc_rx_buf_free(copy);
		return;
	}

	msg.type = DECT_RPC_EVT_IF_RECEIVE;
	msg.if_receive.data = copy;
	msg.if_receive.len = len;
	if (k_msgq_put(&dect_rpc_evt_msgq, &msg, K_NO_WAIT) == 0) {
		k_work_submit(&dect_rpc_evt_work);
		LOG_DBG("DECT recv %zu bytes queued for RPC client", len);
	} else {
		dect_rpc_rx_buf_free(copy);
		LOG_WRN("DECT recv %zu bytes: event queue full, drop", len);
	}
}

/* Notify client that addresses/prefixes/link may have changed; client will re-sync. */
void dect_rpc_server_notify_addrs_changed(void)
{
	atomic_set(&dect_rpc_addrs_changed_pending, 1);
	k_work_submit(&dect_rpc_evt_work);
}

/* Called from L2 when carrier/dormant state may have changed. Queue send to client. */
static void dect_rpc_server_link_state_changed(struct net_if *iface)
{
	struct dect_rpc_evt_msg msg;

	if (!iface) {
		return;
	}

	msg.type = DECT_RPC_EVT_IF_LINK_STATE;
	msg.if_link_state.carrier_ok = net_if_is_carrier_ok(iface);
	msg.if_link_state.dormant = net_if_is_dormant(iface);
	if (k_msgq_put(&dect_rpc_evt_msgq, &msg, K_NO_WAIT) == 0) {
		k_work_submit(&dect_rpc_evt_work);
	}

	/* Tell client to re-sync addresses/prefixes so it gets full state. */
	dect_rpc_server_notify_addrs_changed();
}

static void dect_rpc_cmd_if_send(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				  void *handler_data)
{
	const uint8_t *pkt_data;
	size_t pkt_data_len;
	struct net_pkt *pkt = NULL;
	struct in6_addr dst;
	uint32_t target_long_rd_id;
	int ret;
	int32_t rsp_status = -EINVAL;

	ARG_UNUSED(handler_data);

	dect_rpc_server_note_client_activity();
	LOG_DBG("Received IPv6 packet from RPC client");

	pkt_data = nrf_rpc_decode_buffer_ptr_and_size(ctx, &pkt_data_len);
	/* Need a full IPv6 header (dst addr is read at offset 24, 16 bytes long) before
	 * touching pkt_data below; a shorter buffer would cause an out-of-bounds read.
	 */
	if (!pkt_data || pkt_data_len < NET_IPV6H_LEN) {
		LOG_ERR("RPC IF_SEND: payload too short for IPv6 header (%zu bytes)",
			pkt_data_len);
		goto out;
	}
	if (pkt_data_len > DECT_RPC_MAX_IP_PKT) {
		LOG_ERR("RPC IF_SEND: payload too large (%zu > %d)", pkt_data_len,
			DECT_RPC_MAX_IP_PKT);
		rsp_status = -EMSGSIZE;
		goto out;
	}

	if (!server_dect_if) {
		server_dect_if = get_server_dect_if();
	}
	if (!server_dect_if) {
		LOG_ERR("No DECT interface for RPC send");
		rsp_status = -ENODEV;
		goto out;
	}

	/* Parse IPv6 header to get destination for lladdr (dst addr starts right after the
	 * fixed 8-byte IPv6 header fields + 16-byte src addr, i.e. at offset 24).
	 */
	BUILD_ASSERT(24 + sizeof(struct in6_addr) == NET_IPV6H_LEN);
	memcpy(&dst, pkt_data + 24, sizeof(struct in6_addr));
	target_long_rd_id = dect_utils_lib_long_rd_id_from_ipv6_addr(&dst);
	if (!target_long_rd_id) {
		LOG_ERR("No long_rd_id from IPv6 dst");
		goto out;
	}

	pkt = net_pkt_alloc_with_buffer(server_dect_if, pkt_data_len, AF_INET6, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("Failed to allocate net_pkt");
		rsp_status = -ENOMEM;
		goto out;
	}

	ret = net_pkt_write(pkt, pkt_data, pkt_data_len);
	if (ret < 0) {
		LOG_ERR("net_pkt_write failed: %d", ret);
		net_pkt_unref(pkt);
		rsp_status = ret;
		goto out;
	}

	net_pkt_set_family(pkt, AF_INET6);
	target_long_rd_id = htonl(target_long_rd_id);
	net_pkt_lladdr_dst(pkt)->len = sizeof(target_long_rd_id);
	memcpy(net_pkt_lladdr_dst(pkt)->addr, &target_long_rd_id, sizeof(target_long_rd_id));

	/* Reset cursor so driver's net_pkt_read() reads from the start of the packet. */
	net_pkt_cursor_init(pkt);

	LOG_DBG("RPC recv %zu bytes -> DECT send", pkt_data_len);

	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		dect_rpc_report_cmd_decoding_error(DECT_RPC_CMD_IF_SEND);
		net_pkt_unref(pkt);
		return;
	}
	/* Reply before DECT TX: net_if_send_data() can block while the modem/DECT stack
	 * catches up after connect; the client waits on this RPC response per packet.
	 * Positive response = accepted byte count; negative = reject errno.
	 */
	dect_rpc_rsp_send_i32(group, (int32_t)pkt_data_len);

	if (net_if_send_data(server_dect_if, pkt) != NET_OK) {
		LOG_WRN("RPC recv %zu bytes: DECT send failed", pkt_data_len);
		net_pkt_unref(pkt);
	}
	return;

out:
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		dect_rpc_report_cmd_decoding_error(DECT_RPC_CMD_IF_SEND);
		return;
	}
	dect_rpc_rsp_send_i32(group, rsp_status);
}

NRF_RPC_CBOR_CMD_DECODER(dect_rpc_group, dect_rpc_cmd_if_send, DECT_RPC_CMD_IF_SEND,
			 dect_rpc_cmd_if_send, NULL);

static void dect_rpc_cmd_if_enable(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				   void *handler_data)
{
	ARG_UNUSED(handler_data);

	dect_rpc_server_note_client_activity();
	/* Optional: decode bool enable and sync with server state. */
	(void)nrf_rpc_decode_bool(ctx);
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		dect_rpc_report_cmd_decoding_error(DECT_RPC_CMD_IF_ENABLE);
		return;
	}
	nrf_rpc_rsp_send_void(group);
}

NRF_RPC_CBOR_CMD_DECODER(dect_rpc_group, dect_rpc_cmd_if_enable, DECT_RPC_CMD_IF_ENABLE,
			 dect_rpc_cmd_if_enable, NULL);

/* GET_ADDRS: read-only mirror of dect0. We never modify server dect0 (no put, no down, no set_mtu).
 * Response: n_addrs, addrs[], n_prefixes, prefixes[], mtu, carrier_ok, dormant.
 */
static void dect_rpc_cmd_if_get_addrs(const struct nrf_rpc_group *group,
				      struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct net_if_ipv6 *ipv6;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	int n_addrs = 0;
	int n_prefixes = 0;
	int j;
	size_t rsp_size = 4 + DECT_RPC_MAX_ADDRS * (2 + 16) + 4
		+ DECT_RPC_MAX_PREFIXES * (2 + 16 + 1) + 4;

	ARG_UNUSED(handler_data);

	dect_rpc_server_note_client_activity();
	/* Consume optional request payload (client may send null) */
	(void)nrf_rpc_decode_skip(ctx);
	nrf_rpc_decoding_done(group, ctx->in_packet);

	if (!server_dect_if) {
		server_dect_if = get_server_dect_if();
	}
	if (!server_dect_if || net_if_config_ipv6_get(server_dect_if, &ipv6) < 0) {
		NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 16);
		nrf_rpc_encode_uint(&rsp_ctx, 0);
		nrf_rpc_encode_uint(&rsp_ctx, 0);
		nrf_rpc_encode_uint(&rsp_ctx, (uint32_t)NET_IPV6_MTU);
		nrf_rpc_encode_bool(&rsp_ctx, false);
		nrf_rpc_encode_bool(&rsp_ctx, true);
		nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
		return;
	}

	for (int i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (ipv6->unicast[i].is_used) {
			n_addrs++;
		}
	}
	for (int i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		if (ipv6->prefix[i].is_used) {
			n_prefixes++;
		}
	}
	if (n_addrs > DECT_RPC_MAX_ADDRS) {
		n_addrs = DECT_RPC_MAX_ADDRS;
	}
	if (n_prefixes > DECT_RPC_MAX_PREFIXES) {
		n_prefixes = DECT_RPC_MAX_PREFIXES;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, rsp_size);
	nrf_rpc_encode_uint(&rsp_ctx, (uint32_t)n_addrs);
	j = 0;
	for (int i = 0; i < NET_IF_MAX_IPV6_ADDR && j < n_addrs; i++) {
		if (ipv6->unicast[i].is_used) {
			nrf_rpc_encode_buffer(&rsp_ctx, &ipv6->unicast[i].address.in6_addr,
					     sizeof(struct in6_addr));
			j++;
		}
	}
	nrf_rpc_encode_uint(&rsp_ctx, (uint32_t)n_prefixes);
	j = 0;
	for (int i = 0; i < NET_IF_MAX_IPV6_PREFIX && j < n_prefixes; i++) {
		if (ipv6->prefix[i].is_used) {
			nrf_rpc_encode_buffer(&rsp_ctx, &ipv6->prefix[i].prefix,
					     sizeof(struct in6_addr));
			nrf_rpc_encode_uint(&rsp_ctx, (uint32_t)ipv6->prefix[i].len);
			j++;
		}
	}
	nrf_rpc_encode_uint(&rsp_ctx, (uint32_t)net_if_get_mtu(server_dect_if));
	nrf_rpc_encode_bool(&rsp_ctx, net_if_is_carrier_ok(server_dect_if));
	nrf_rpc_encode_bool(&rsp_ctx, net_if_is_dormant(server_dect_if));
	/* Do not modify server dect0: no put(), down(), or set_mtu(). */
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(dect_rpc_group, dect_rpc_cmd_if_get_addrs, DECT_RPC_CMD_IF_GET_ADDRS,
			 dect_rpc_cmd_if_get_addrs, NULL);

/* DECT_STATUS: optional seq (uint); response = status + associations + fw_version. */
static void dect_rpc_cmd_if_status(const struct nrf_rpc_group *group,
				   struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct dect_status_info status;
	struct nrf_rpc_cbor_ctx rsp_ctx;
	int ret;

	ARG_UNUSED(handler_data);

	dect_rpc_server_note_client_activity();
	(void)nrf_rpc_decode_skip(ctx);
	/* Signal decode done so transport RX can receive next packet (ref b5dfb04). */
	nrf_rpc_decoding_done(group, ctx->in_packet);

	if (!server_dect_if) {
		server_dect_if = get_server_dect_if();
	}
	if (!server_dect_if) {
		dect_rpc_status_init_empty(&status);
		NRF_RPC_CBOR_ALLOC(group, rsp_ctx, dect_rpc_status_rsp_encode_size(&status));
		dect_rpc_status_rsp_encode(&rsp_ctx, &status);
		nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
		return;
	}

	ret = net_mgmt(NET_REQUEST_DECT_STATUS_INFO_GET, server_dect_if, &status, sizeof(status));
	if (ret < 0) {
		dect_rpc_status_init_empty(&status);
		NRF_RPC_CBOR_ALLOC(group, rsp_ctx, dect_rpc_status_rsp_encode_size(&status));
		dect_rpc_status_rsp_encode(&rsp_ctx, &status);
		nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
		return;
	}

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, dect_rpc_status_rsp_encode_size(&status));
	dect_rpc_status_rsp_encode(&rsp_ctx, &status);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(dect_rpc_group, dect_rpc_cmd_if_status, DECT_RPC_CMD_IF_STATUS,
			 dect_rpc_cmd_if_status, NULL);

static void dect_rpc_cmd_conn_rsp(const struct nrf_rpc_group *group, int32_t status)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;

	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 8);
	dect_rpc_int32_rsp_encode(&rsp_ctx, status);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

static void dect_rpc_cmd_connect(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				 void *handler_data)
{
	struct net_if *iface;
	int ret = -ENODEV;

	ARG_UNUSED(handler_data);

	dect_rpc_server_note_client_activity();
	(void)nrf_rpc_decode_skip(ctx);
	nrf_rpc_decoding_done(group, ctx->in_packet);

	iface = get_server_dect_if();
	if (iface == NULL) {
		dect_rpc_cmd_conn_rsp(group, -ENODEV);
		return;
	}

#if defined(CONFIG_NET_L2_DECT_CONN_MGR)
	ret = conn_mgr_if_connect(iface);
#else
	ret = -ENOTSUP;
#endif

	dect_rpc_cmd_conn_rsp(group, ret);
}

static void dect_rpc_cmd_disconnect(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				    void *handler_data)
{
	struct net_if *iface;
	int ret = -ENODEV;

	ARG_UNUSED(handler_data);

	dect_rpc_server_note_client_activity();
	(void)nrf_rpc_decode_skip(ctx);
	nrf_rpc_decoding_done(group, ctx->in_packet);

	iface = get_server_dect_if();
	if (iface == NULL) {
		dect_rpc_cmd_conn_rsp(group, -ENODEV);
		return;
	}

#if defined(CONFIG_NET_L2_DECT_CONN_MGR)
	ret = conn_mgr_if_disconnect(iface);
#else
	ret = -ENOTSUP;
#endif

	dect_rpc_cmd_conn_rsp(group, ret);
}

NRF_RPC_CBOR_CMD_DECODER(dect_rpc_group, dect_rpc_cmd_connect, DECT_RPC_CMD_CONNECT,
			 dect_rpc_cmd_connect, NULL);
NRF_RPC_CBOR_CMD_DECODER(dect_rpc_group, dect_rpc_cmd_disconnect, DECT_RPC_CMD_DISCONNECT,
			 dect_rpc_cmd_disconnect, NULL);

/* DECT_RPC_CMD_SHELL: request = argc (uint), argv[0]..argv[argc-1] (bstr).
 * Response = status (uint: 0 OK, 1 error/BUSY), output (bstr).
 * If CONFIG_DECT_NR_RPC_SHELL is not set, returns "DECT L2 shell not configured on server".
 * Otherwise server shell wins: if local shell holds the session lock, RPC returns BUSY.
 *
 * Stack: Runs on the nRF RPC thread pool ("rpc" threads), not on "rpc uart rx". Set
 * CONFIG_NRF_RPC_THREAD_STACK_SIZE. With CONFIG_DECT_CLUSTER_MAX_CHILD_ASSOCIATION_COUNT=10,
 * Output is flushed to RPC per line via DECT_RPC_CMD_SHELL_LINE; response is status only.
 */
static void dect_rpc_cmd_shell(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			       void *handler_data)
{
	struct nrf_rpc_cbor_ctx rsp_ctx;
	uint32_t argc;
	const uint8_t *p;
	size_t sz;
	int i;
#if defined(CONFIG_DECT_NR_RPC_SHELL)
	char argv_bufs[DECT_RPC_SHELL_MAX_ARGC][DECT_RPC_SHELL_ARG_LEN];
	char *argv_ptrs[DECT_RPC_SHELL_MAX_ARGC];
	int ret;
#endif

	ARG_UNUSED(handler_data);

	dect_rpc_server_note_client_activity();
	if (!nrf_rpc_decode_valid(ctx)) {
		goto err_decode;
	}
	argc = nrf_rpc_decode_uint(ctx);
	if (argc == 0 || argc > DECT_RPC_SHELL_MAX_ARGC) {
		goto err_decode;
	}
	for (i = 0; i < (int)argc; i++) {
		p = nrf_rpc_decode_buffer_ptr_and_size(ctx, &sz);
		if (!p || sz >= DECT_RPC_SHELL_ARG_LEN) {
			goto err_decode;
		}
#if defined(CONFIG_DECT_NR_RPC_SHELL)
		memcpy(argv_bufs[i], p, sz);
		argv_bufs[i][sz] = '\0';
		argv_ptrs[i] = argv_bufs[i];
#endif
	}
	nrf_rpc_decoding_done(group, ctx->in_packet);
	/* Request fully decoded; run shell (may block) then send response. */

#if !defined(CONFIG_DECT_NR_RPC_SHELL)
	/* Server not built with DECT L2 shell over RPC; return clear error. */
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 64);
	nrf_rpc_encode_uint(&rsp_ctx, 1U);
	nrf_rpc_encode_buffer(&rsp_ctx,
			      (const uint8_t *)"DECT L2 shell not configured on server", 38);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
	return;
#else
	if (dect_rpc_shell_session_try_lock() != 0) {
		/* Server (local) shell is in use; RPC returns BUSY. */
		NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 32);
		nrf_rpc_encode_uint(&rsp_ctx, 1);
		nrf_rpc_encode_buffer(&rsp_ctx, (const uint8_t *)"BUSY", 4);
		nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
		return;
	}
	ret = dect_shell_exec_by_name(argv_ptrs[0], (int)argc, argv_ptrs, NULL, 0);
	dect_rpc_shell_session_unlock();
	/* Response is status only; output flushed per line via DECT_RPC_CMD_SHELL_LINE. */
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 8);
	nrf_rpc_encode_uint(&rsp_ctx, ret == 0 ? 0U : 1U);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
	return;
#endif

err_decode:
	nrf_rpc_decoding_done(group, ctx->in_packet);
	NRF_RPC_CBOR_ALLOC(group, rsp_ctx, 24);
	nrf_rpc_encode_uint(&rsp_ctx, 1U);
	nrf_rpc_encode_buffer(&rsp_ctx, (const uint8_t *)"decode error", 12);
	nrf_rpc_cbor_rsp_no_err(group, &rsp_ctx);
}

NRF_RPC_CBOR_CMD_DECODER(dect_rpc_group, dect_rpc_cmd_shell, DECT_RPC_CMD_SHELL,
			 dect_rpc_cmd_shell, NULL);

/* When IPv6 addr/prefix changes on DECT iface, notify client to re-sync. */
static void dect_rpc_server_ipv6_event_cb(struct net_mgmt_event_callback *cb,
					 uint64_t mgmt_event, struct net_if *iface)
{
	struct net_if *dect_if = server_dect_if ? server_dect_if : get_server_dect_if();

	ARG_UNUSED(cb);
	ARG_UNUSED(mgmt_event);

	if (dect_if && iface == dect_if) {
		LOG_DBG("DECT iface IPv6 change, notifying client");
		dect_rpc_server_notify_addrs_changed();
	}
}

static struct net_mgmt_event_callback dect_rpc_server_ipv6_cb;

static int dect_rpc_server_init(void)
{
	nrf_rpc_set_bound_handler(dect_rpc_server_group_bound);
	dect_net_l2_rpc_forward_register(dect_rpc_server_forward_recv);
	dect_net_l2_link_state_register(dect_rpc_server_link_state_changed);

	server_dect_if = get_server_dect_if();
	net_mgmt_init_event_callback(&dect_rpc_server_ipv6_cb,
				     dect_rpc_server_ipv6_event_cb,
				     NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_ADDR_DEL
				     | NET_EVENT_IPV6_PREFIX_ADD | NET_EVENT_IPV6_PREFIX_DEL);
	net_mgmt_add_event_callback(&dect_rpc_server_ipv6_cb);

#if defined(CONFIG_DECT_NR_RPC_SHELL)
	dect_rpc_server_shell_line_l2_init();
#endif
	return 0;
}

SYS_INIT(dect_rpc_server_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
