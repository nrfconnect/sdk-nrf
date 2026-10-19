/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Client tunnel net_if: L2 send/recv, GET_ADDRS mirror, and IF_* event decoders
 * tied to that iface. See also dect_rpc_client_ping.c (PING) and
 * dect_rpc_client_shell_line.c (SHELL_LINE), dect_rpc_shell.c (shell commands).
 */

#include "dect_rpc_ids.h"
#include "dect_rpc_common.h"
#include <nrf_rpc.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>

#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_linkaddr.h>

#include <zephyr/net/mld.h>
#include "dect_rpc_client_net.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dect_rpc, CONFIG_NET_DECT_RPC_LOG_LEVEL);

#define DECT_RPC_MAX_ADDRS   8
#define DECT_RPC_MAX_PREFIXES 4

struct dect_rpc_l2_data {
};

#if defined(CONFIG_DECT_NR_RPC_ADDR_SYNC_INTERVAL_SEC)
static void dect_rpc_addr_sync_work_fn(struct k_work *work);
#endif

static enum net_verdict dect_rpc_l2_recv(struct net_if *iface, struct net_pkt *pkt)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(pkt);
	return NET_CONTINUE;
}

static int dect_rpc_l2_send(struct net_if *iface, struct net_pkt *pkt)
{
	ARG_UNUSED(iface);

	bool encoded_ok = false;
	const size_t len = net_pkt_get_len(pkt);
	const size_t cbor_buffer_size = 10 + len;
	struct nrf_rpc_cbor_ctx ctx;

	if (len > DECT_RPC_MAX_IP_PKT) {
		LOG_ERR("RPC send: packet too large (%zu > %d)", len, DECT_RPC_MAX_IP_PKT);
		net_pkt_unref(pkt);
		return -EMSGSIZE;
	}

	NRF_RPC_CBOR_ALLOC(&dect_rpc_group, ctx, cbor_buffer_size);
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

	LOG_DBG("RPC send %zu bytes -> server", len);
	encoded_ok = true;

out:
	if (!encoded_ok) {
		LOG_ERR("RPC send: failed to encode packet");
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	int32_t send_status = -EIO;
	const int rpc_err = nrf_rpc_cbor_cmd(&dect_rpc_group, DECT_RPC_CMD_IF_SEND, &ctx,
					     dect_rpc_decode_if_send_rsp, &send_status);

	net_pkt_unref(pkt);
	if (rpc_err < 0) {
		LOG_DBG("RPC send failed: %d", rpc_err);
		return rpc_err;
	}
	if (send_status < 0) {
		LOG_DBG("RPC IF_SEND rejected by server: %d", (int)send_status);
		return (int)send_status;
	}

	return (int)send_status;
}

static int dect_rpc_l2_enable(struct net_if *iface, bool state)
{
	if (state && iface) {
		/* No GET_ADDRS here (OT pattern: L2 enable does not block on server RPC). */
#if defined(CONFIG_DECT_NR_RPC_ADDR_SYNC_INTERVAL_SEC)
		{
			static struct k_work_delayable addr_sync_work;
			static bool work_initialized;

			if (!work_initialized) {
				k_work_init_delayable(&addr_sync_work, dect_rpc_addr_sync_work_fn);
				work_initialized = true;
			}
			if (CONFIG_DECT_NR_RPC_ADDR_SYNC_INTERVAL_SEC > 0) {
				k_work_reschedule(&addr_sync_work,
						  K_SECONDS(
							CONFIG_DECT_NR_RPC_ADDR_SYNC_INTERVAL_SEC));
			}
		}
#endif
	}
	return 0;
}

static enum net_l2_flags dect_rpc_l2_flags(struct net_if *iface)
{
	ARG_UNUSED(iface);
	return NET_L2_MULTICAST | NET_L2_MULTICAST_SKIP_JOIN_SOLICIT_NODE;
}

#if defined(CONFIG_MDNS_RESPONDER) || defined(CONFIG_MDNS_RESOLVER)
static void dect_rpc_mdns_join_schedule_if_ready(bool carrier_ok, bool dormant);
#endif

struct dect_rpc_ipv6_prefix {
	struct in6_addr addr;
	uint8_t len;
};

static bool dect_rpc_ipv6_addr_array_contains(const struct in6_addr *addrs, uint32_t count,
					      const struct in6_addr *addr)
{
	for (uint32_t i = 0; i < count; i++) {
		if (memcmp(&addrs[i], addr, sizeof(*addr)) == 0) {
			return true;
		}
	}

	return false;
}

/* Decode IF_GET_ADDRS response CBOR and mirror to client iface (addrs, prefixes, MTU, link). */
static bool dect_rpc_client_apply_get_addrs_rsp(struct net_if *iface, struct nrf_rpc_cbor_ctx *ctx)
{
	struct net_if_ipv6 *ipv6;
	struct in6_addr addrs[DECT_RPC_MAX_ADDRS];
	struct dect_rpc_ipv6_prefix prefixes[DECT_RPC_MAX_PREFIXES];
	uint32_t n_addrs, n_prefixes, n_addrs_to_apply, n_prefixes_to_apply;
	uint32_t prefix_count = 0;
	uint32_t mtu;
	const uint8_t *p;
	size_t sz;
	struct in6_addr addr;
	uint32_t prefix_len;
	bool carrier_ok;
	bool dormant;
	int i;

	if (!iface || !nrf_rpc_decode_valid(ctx)) {
		goto decode_error;
	}

	n_addrs = nrf_rpc_decode_uint(ctx);
	if (!nrf_rpc_decode_valid(ctx)) {
		goto decode_error;
	}
	n_addrs_to_apply = MIN(n_addrs, (uint32_t)DECT_RPC_MAX_ADDRS);
	for (uint32_t j = 0; j < n_addrs_to_apply; j++) {
		p = nrf_rpc_decode_buffer_ptr_and_size(ctx, &sz);
		if (!p || sz != sizeof(struct in6_addr)) {
			goto decode_error;
		}
		memcpy(&addrs[j], p, sizeof(addrs[j]));
	}
	dect_rpc_decode_skip_extra_ipv6_addrs(ctx, n_addrs, n_addrs_to_apply);
	if (!nrf_rpc_decode_valid(ctx)) {
		goto decode_error;
	}

	n_prefixes = nrf_rpc_decode_uint(ctx);
	if (!nrf_rpc_decode_valid(ctx)) {
		goto decode_error;
	}
	n_prefixes_to_apply = MIN(n_prefixes, (uint32_t)DECT_RPC_MAX_PREFIXES);
	for (uint32_t j = 0; j < n_prefixes_to_apply; j++) {
		p = nrf_rpc_decode_buffer_ptr_and_size(ctx, &sz);
		if (!p || sz != sizeof(struct in6_addr)) {
			goto decode_error;
		}
		memcpy(&addr, p, sizeof(addr));
		prefix_len = nrf_rpc_decode_uint(ctx);
		if (!nrf_rpc_decode_valid(ctx)) {
			goto decode_error;
		}
		if (prefix_len <= 128) {
			prefixes[prefix_count].addr = addr;
			prefixes[prefix_count].len = (uint8_t)prefix_len;
			prefix_count++;
		}
	}
	dect_rpc_decode_skip_extra_ipv6_prefixes(ctx, n_prefixes, n_prefixes_to_apply);
	if (!nrf_rpc_decode_valid(ctx)) {
		goto decode_error;
	}
	mtu = nrf_rpc_decode_uint(ctx);
	if (!nrf_rpc_decode_valid(ctx)) {
		goto decode_error;
	}
	carrier_ok = nrf_rpc_decode_bool(ctx);
	if (!nrf_rpc_decode_valid(ctx)) {
		goto decode_error;
	}
	dormant = nrf_rpc_decode_bool(ctx);
	if (!nrf_rpc_decoding_done_and_check(&dect_rpc_group, ctx)) {
		nrf_rpc_decoding_done(&dect_rpc_group, ctx->in_packet);
		return false;
	}

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		return false;
	}

	/* Preserve matching entries so routine refreshes do not emit address events. */
	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (ipv6->unicast[i].is_used &&
		    !dect_rpc_ipv6_addr_array_contains(addrs, n_addrs_to_apply,
						      &ipv6->unicast[i].address.in6_addr)) {
			addr = ipv6->unicast[i].address.in6_addr;
			net_if_ipv6_addr_rm(iface, &addr);
		}
	}
	for (uint32_t j = 0; j < n_addrs_to_apply; j++) {
		bool found = false;

		for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
			if (ipv6->unicast[i].is_used &&
			    memcmp(&ipv6->unicast[i].address.in6_addr, &addrs[j],
				   sizeof(addrs[j])) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			net_if_ipv6_addr_add(iface, &addrs[j], NET_ADDR_AUTOCONF, 0);
		}
	}
	for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
		if (!ipv6->prefix[i].is_used) {
			continue;
		}
		bool found = false;

		for (uint32_t j = 0; j < prefix_count; j++) {
			if (ipv6->prefix[i].len == prefixes[j].len &&
			    memcmp(&ipv6->prefix[i].prefix, &prefixes[j].addr,
				   sizeof(prefixes[j].addr)) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			struct in6_addr old_prefix = ipv6->prefix[i].prefix;

			net_if_ipv6_prefix_rm(iface, &old_prefix, ipv6->prefix[i].len);
		}
	}
	for (uint32_t j = 0; j < prefix_count; j++) {
		bool found = false;

		for (i = 0; i < NET_IF_MAX_IPV6_PREFIX; i++) {
			if (ipv6->prefix[i].is_used && ipv6->prefix[i].len == prefixes[j].len &&
			    memcmp(&ipv6->prefix[i].prefix, &prefixes[j].addr,
				   sizeof(prefixes[j].addr)) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			net_if_ipv6_prefix_add(iface, &prefixes[j].addr, prefixes[j].len, 0);
		}
	}

	/* MTU: mirror server dect0 to client iface only. */
	if (mtu > 0 && mtu <= 0xFFFF) {
		net_if_set_mtu(iface, (uint16_t)mtu);
	}
	/* Carrier and dormant: mirror server link state. */
	if (carrier_ok) {
		net_if_carrier_on(iface);
	} else {
		net_if_carrier_off(iface);
	}
	if (dormant) {
		net_if_dormant_on(iface);
	} else {
		net_if_dormant_off(iface);
	}
#if defined(CONFIG_MDNS_RESPONDER) || defined(CONFIG_MDNS_RESOLVER)
	dect_rpc_mdns_join_schedule_if_ready(carrier_ok, dormant);
#endif

	/* Do not net_if_config_ipv6_put(): put() detaches config and clears
	 * iface->config.ip.ipv6 (interface shows "IPv6 not enabled").
	 * Keep config attached with mirrored state.
	 */
	LOG_DBG("Synced %u addrs, %u prefixes from server", n_addrs, n_prefixes);
	return true;

decode_error:
	nrf_rpc_decoding_done(&dect_rpc_group, ctx->in_packet);
	return false;
}

/* Unique request payload per GET_ADDRS so CRC differs (avoids UART duplicate drop). */
static uint32_t dect_rpc_get_addrs_seq;

/* Ref pattern: synchronous cmd_rsp, decode in place, then apply (like status). */
int dect_rpc_client_sync_addrs(struct net_if *iface)
{
	struct nrf_rpc_cbor_ctx ctx;
	int err;

	if (!iface) {
		return -EINVAL;
	}

	NRF_RPC_CBOR_ALLOC(&dect_rpc_group, ctx, 8);
	nrf_rpc_encode_uint(&ctx, dect_rpc_get_addrs_seq++);

	err = nrf_rpc_cbor_cmd_rsp(&dect_rpc_group, DECT_RPC_CMD_IF_GET_ADDRS, &ctx);
	if (err < 0) {
		return err;
	}

	if (!dect_rpc_client_apply_get_addrs_rsp(iface, &ctx)) {
		return -EIO;
	}

	return 0;
}

/* Sync from server while admin-down (NO_AUTO_START), then bring the iface up. */
static int dect_rpc_sync_addrs_and_up(struct net_if *iface)
{
	int err;

	if (!iface) {
		return -EINVAL;
	}

	err = dect_rpc_client_sync_addrs(iface);
	if (err == 0) {
		(void)net_if_up(iface);
	}

	return err;
}

static int dect_rpc_client_conn_rpc(uint8_t cmd)
{
	struct nrf_rpc_cbor_ctx ctx;
	int err;
	int32_t ret;

	if (!NRF_RPC_GROUP_STATUS(dect_rpc_group)) {
		return -ENOTCONN;
	}

	NRF_RPC_CBOR_ALLOC(&dect_rpc_group, ctx, 4);

	err = nrf_rpc_cbor_cmd_rsp(&dect_rpc_group, cmd, &ctx);
	if (err < 0) {
		return err;
	}

	if (!dect_rpc_int32_rsp_decode(&ctx, &ret)) {
		return -EIO;
	}

	if (!nrf_rpc_decoding_done_and_check(&dect_rpc_group, &ctx)) {
		return -EIO;
	}

	return (int)ret;
}

#if defined(CONFIG_DECT_NR_RPC_CONN_MGR)
int dect_rpc_client_connect(void)
{
	return dect_rpc_client_conn_rpc(DECT_RPC_CMD_CONNECT);
}

int dect_rpc_client_disconnect(void)
{
	return dect_rpc_client_conn_rpc(DECT_RPC_CMD_DISCONNECT);
}
#endif /* CONFIG_DECT_NR_RPC_CONN_MGR */

#if defined(CONFIG_DECT_NR_RPC_AUTO_SYNC)
static void dect_rpc_auto_sync_work_fn(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(dect_rpc_auto_sync_work, dect_rpc_auto_sync_work_fn);

static void dect_rpc_auto_sync_work_fn(struct k_work *work)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));

	ARG_UNUSED(work);
	if (!iface) {
		LOG_INF("DECT RPC: no interface for auto-sync");
		return;
	}
	/* Sync even when interface is admin down (NO_AUTO_START); addresses/MTU/status apply.
	 * After first successful sync, bring the interface up so it effectively "auto-starts"
	 * once the server has synced (no need for the user to run "net iface up").
	 */
	if (dect_rpc_sync_addrs_and_up(iface) == 0) {
		struct net_if_ipv6 *ipv6;
		char buf[NET_INET6_ADDRSTRLEN];
		bool logged_first = false;

		if (net_if_config_ipv6_get(iface, &ipv6) >= 0) {
			for (int i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
				if (ipv6->unicast[i].is_used &&
				    net_addr_ntop(NET_AF_INET6,
						  &ipv6->unicast[i].address.in6_addr,
						  buf, sizeof(buf))) {
					LOG_DBG("DECT RPC: auto-synced addresses from server, "
						"first: %s",
						buf);
					logged_first = true;
					break;
				}
			}
			/* do not put(): keep config attached */
		}
		if (!logged_first) {
			LOG_DBG("DECT RPC: auto-synced addresses from server");
		}
		LOG_DBG("DECT RPC: interface up after sync");
	} else {
		LOG_WRN("DECT RPC: auto-sync failed, retry in 5 s");
		k_work_reschedule(&dect_rpc_auto_sync_work, K_SECONDS(5));
	}
}
#endif /* CONFIG_DECT_NR_RPC_AUTO_SYNC */

#if defined(CONFIG_DECT_NR_RPC_ADDR_SYNC_INTERVAL_SEC)
static void dect_rpc_addr_sync_work_fn(struct k_work *work)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));

	if (iface) {
		(void)dect_rpc_sync_addrs_and_up(iface);
	}
	if (CONFIG_DECT_NR_RPC_ADDR_SYNC_INTERVAL_SEC > 0) {
		k_work_reschedule((struct k_work_delayable *)work,
				  K_SECONDS(CONFIG_DECT_NR_RPC_ADDR_SYNC_INTERVAL_SEC));
	}
}
#endif /* CONFIG_DECT_NR_RPC_ADDR_SYNC_INTERVAL_SEC */

NET_L2_INIT(DECT_RPC_L2, dect_rpc_l2_recv, dect_rpc_l2_send, dect_rpc_l2_enable, dect_rpc_l2_flags);

static int dect_rpc_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

/* Dummy link address so net_if_up() does not assert (link_addr->len > 0). */
static const uint8_t dect_rpc_lladdr[6] = { 0x02, 0x00, 0x5e, 0x00, 0x52, 0x01 };

static void dect_rpc_if_init(struct net_if *iface)
{
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_flag_set(iface, NET_IF_IPV6_NO_ND);
#if !defined(CONFIG_NET_IPV6_MLD)
	net_if_flag_set(iface, NET_IF_IPV6_NO_MLD);
#endif
	net_if_set_link_addr(iface, (uint8_t *)dect_rpc_lladdr, sizeof(dect_rpc_lladdr),
			     NET_LINK_DUMMY);
#if defined(CONFIG_NET_INTERFACE_NAME)
	{
		int name_err = net_if_set_name(iface, CONFIG_DECT_NR_RPC_CLIENT_IFACE_NAME);

		if (name_err < 0) {
			LOG_ERR("net_if_set_name(\"%s\") failed (%d); "
				"check CONFIG_NET_INTERFACE_NAME_LEN",
				CONFIG_DECT_NR_RPC_CLIENT_IFACE_NAME, name_err);
		}
	}
#endif
}

static struct net_if_api dect_rpc_if_api = {
	.init = dect_rpc_if_init,
};

NET_DEVICE_INIT(dect_rpc, CONFIG_DECT_NR_RPC_CLIENT_IFACE_NAME, dect_rpc_dev_init, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &dect_rpc_if_api, DECT_RPC_L2,
		struct dect_rpc_l2_data, DECT_RPC_MAX_IP_PKT);

#if defined(CONFIG_DECT_NR_RPC_CONN_MGR)
#include <net/dect/dect_rpc_conn_mgr.h>
CONNECTIVITY_DECT_RPC_MGMT_BIND(dect_rpc);
#endif

const struct nrf_rpc_group *dect_rpc_client_get_group(void)
{
	return &dect_rpc_group;
}

void dect_rpc_client_notify_rpc_init_done(void)
{
#if defined(CONFIG_DECT_NR_RPC_AUTO_SYNC)
	k_work_schedule(&dect_rpc_auto_sync_work,
			K_SECONDS(CONFIG_DECT_NR_RPC_AUTO_SYNC_DELAY_SEC));
#endif
}

/* Decoder for IF_RECEIVE: server sends raw IPv6 packet to client (event; server uses event
 * because command would have dst=UNKNOWN and client would run event path, so no RSP).
 */
static void dect_rpc_cmd_if_receive(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				     void *handler_data)
{
	const uint8_t *pkt_data;
	size_t pkt_data_len = 0;
	struct net_if *iface;
	struct net_pkt *pkt = NULL;

	pkt_data = nrf_rpc_decode_buffer_ptr_and_size(ctx, &pkt_data_len);
	if (pkt_data && pkt_data_len > DECT_RPC_MAX_IP_PKT) {
		LOG_ERR("RPC IF_RECEIVE: payload too large (%zu > %d)", pkt_data_len,
			DECT_RPC_MAX_IP_PKT);
		pkt_data = NULL;
		pkt_data_len = 0;
	}
	if (pkt_data && pkt_data_len > 0) {
		iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));
		if (iface) {
			pkt = net_pkt_rx_alloc_with_buffer(iface, pkt_data_len, NET_AF_UNSPEC, 0,
							   K_NO_WAIT);
		} else {
			LOG_ERR("No DECT RPC net interface");
		}

		if (pkt) {
			net_pkt_write(pkt, pkt_data, pkt_data_len);
		} else {
			LOG_ERR("Failed to allocate net_pkt for RPC receive");
		}
	}

	/* Release RX buffer early so UART rx_work can process the next frame (ACK when reliable) */
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		dect_rpc_report_cmd_decoding_error(DECT_RPC_CMD_IF_RECEIVE);
		nrf_rpc_decoding_done(group, ctx->in_packet);
		if (pkt) {
			net_pkt_unref(pkt);
		}
		return;
	}

	if (pkt) {
		LOG_DBG("RPC recv %zu bytes -> net stack", pkt_data_len);
		if (net_recv_data(iface, pkt) < 0) {
			LOG_ERR("RPC recv %zu bytes: net_recv_data failed (drop)", pkt_data_len);
			net_pkt_unref(pkt);
		}
	}
}

NRF_RPC_CBOR_EVT_DECODER(dect_rpc_group, dect_rpc_cmd_if_receive, DECT_RPC_CMD_IF_RECEIVE,
			 dect_rpc_cmd_if_receive, NULL);

#if defined(CONFIG_MDNS_RESPONDER) || defined(CONFIG_MDNS_RESOLVER)
static void dect_rpc_net_join_ipv6_mdns_group(struct net_if *iface)
{
	struct in6_addr ipv6mr_multiaddr;
	int ret;

	/* Well known IPv6 ff02::fb address */
	net_ipv6_addr_create(&ipv6mr_multiaddr, 0xff02, 0, 0, 0, 0, 0, 0, 0x00fb);

	ret = net_ipv6_mld_join(iface, &ipv6mr_multiaddr);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Iface %p, cannot add/join mDNS (ff02::fb) (%d)", iface, ret);
	}
}

static void dect_rpc_mdns_join_work_fn(struct k_work *work)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));

	ARG_UNUSED(work);

	if (iface == NULL) {
		return;
	}

	/* Defer MLD join out of the nRF RPC handler; net_if_is_up() is set after carrier/dormant
	 * updates have run through update_operational_state().
	 */
	if (net_if_is_up(iface)) {
		dect_rpc_net_join_ipv6_mdns_group(iface);
	}
}

static K_WORK_DELAYABLE_DEFINE(dect_rpc_mdns_join_work, dect_rpc_mdns_join_work_fn);

static void dect_rpc_mdns_join_schedule_if_ready(bool carrier_ok, bool dormant)
{
	if (carrier_ok && !dormant) {
		(void)k_work_schedule(&dect_rpc_mdns_join_work, K_NO_WAIT);
	} else {
		(void)k_work_cancel_delayable(&dect_rpc_mdns_join_work);
	}
}
#endif /* MDNS_RESPONDER || MDNS_RESOLVER */

/* IF_LINK_STATE: server pushes carrier/dormant so client iface mirrors server (event). */
static void dect_rpc_cmd_if_link_state(const struct nrf_rpc_group *group,
				       struct nrf_rpc_cbor_ctx *ctx, void *handler_data)
{
	struct net_if *iface;
	bool carrier_ok;
	bool dormant;

	ARG_UNUSED(handler_data);
	if (!nrf_rpc_decode_valid(ctx)) {
		nrf_rpc_decoding_done(group, ctx->in_packet);
		return;
	}
	carrier_ok = nrf_rpc_decode_bool(ctx);
	if (!nrf_rpc_decode_valid(ctx)) {
		nrf_rpc_decoding_done(group, ctx->in_packet);
		return;
	}
	dormant = nrf_rpc_decode_bool(ctx);
	/* Release RX buffer so UART rx_work can process the next frame (ACK when reliable). */
	if (!nrf_rpc_decoding_done_and_check(group, ctx)) {
		dect_rpc_report_cmd_decoding_error(DECT_RPC_CMD_IF_LINK_STATE);
		nrf_rpc_decoding_done(group, ctx->in_packet);
		return;
	}
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));
	if (iface) {
		if (carrier_ok) {
			net_if_carrier_on(iface);
		} else {
			net_if_carrier_off(iface);
		}
		if (dormant) {
			net_if_dormant_on(iface);
		} else {
			net_if_dormant_off(iface);
		}
#if defined(CONFIG_MDNS_RESPONDER) || defined(CONFIG_MDNS_RESOLVER)
		dect_rpc_mdns_join_schedule_if_ready(carrier_ok, dormant);
#endif
		LOG_DBG("Link state from server: carrier=%d dormant=%d", carrier_ok, dormant);
	}
}

NRF_RPC_CBOR_EVT_DECODER(dect_rpc_group, dect_rpc_cmd_if_link_state, DECT_RPC_CMD_IF_LINK_STATE,
			 dect_rpc_cmd_if_link_state, NULL);

/* Server may emit several IF_ADDRS_CHANGED events in a row (link up + each prefix). */
#define DECT_RPC_ADDRS_CHANGED_DEBOUNCE_MS 50

static void dect_rpc_addrs_changed_work_fn(struct k_work *work)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT_RPC_L2));

	ARG_UNUSED(work);

	if (iface && dect_rpc_sync_addrs_and_up(iface) == 0) {
		LOG_DBG("DECT RPC: re-synced addresses (server notified change)");
	}
}

static K_WORK_DELAYABLE_DEFINE(dect_rpc_addrs_changed_work, dect_rpc_addrs_changed_work_fn);

/* Server notifies client that addresses/link changed; client re-syncs (event). */
static void dect_rpc_cmd_if_addrs_changed(const struct nrf_rpc_group *group,
					  struct nrf_rpc_cbor_ctx *ctx,
					  void *handler_data)
{
	ARG_UNUSED(handler_data);
	(void)nrf_rpc_decode_skip(ctx);
	nrf_rpc_decoding_done(group, ctx->in_packet);

	/* Defer GET_ADDRS: avoid nested blocking RPC in the event decoder; debounce bursts. */
	k_work_reschedule(&dect_rpc_addrs_changed_work,
			  K_MSEC(DECT_RPC_ADDRS_CHANGED_DEBOUNCE_MS));
}

NRF_RPC_CBOR_EVT_DECODER(dect_rpc_group, dect_rpc_cmd_if_addrs_changed,
			 DECT_RPC_CMD_IF_ADDRS_CHANGED, dect_rpc_cmd_if_addrs_changed, NULL);
