/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/init.h>
#include <zephyr/shell/shell.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>

#include <net/dect/dect_icmp_ping.h>

#include "dect_icmp_ping_internal.h"
#include "dect_icmp_ping_shell_ctx.h"

static struct k_poll_signal *abort_signal;

static int64_t ipv6_connected_status_updated_uptime;

static bool dect_icmp_ping_current_conn_info_set(struct dect_icmp_ping_argv *ping_args,
						 struct dect_icmp_ping_argv *ping_argv);

/*
 * ICMPv6 echo via net_icmp: echo replies are delivered
 * through the registered ICMP handler path in the network stack.
 */
static struct net_icmp_ctx icmp_ctx;
static struct k_sem icmp_sem;
static uint16_t icmp_expect_id;
static uint16_t icmp_expect_seq;
static uint32_t icmp_rtt_ms;
static atomic_t dect_icmp_ping_busy;

void dect_icmp_ping_set_abort_signal(struct k_poll_signal *signal)
{
	abort_signal = signal;
}

static enum net_verdict dect_icmpv6_echo_reply_handler(struct net_icmp_ctx *ctx,
						      struct net_pkt *pkt,
						      struct net_icmp_ip_hdr *ip_hdr,
						      struct net_icmp_hdr *icmp_hdr,
						      void *user_data)
{
	uint8_t id_seq[4];
	uint32_t sent_cycles;

	ARG_UNUSED(ctx);
	ARG_UNUSED(user_data);
	ARG_UNUSED(ip_hdr);
	ARG_UNUSED(icmp_hdr);

	if (net_pkt_read(pkt, id_seq, sizeof(id_seq))) {
		return NET_DROP;
	}

	uint16_t id = sys_get_be16(&id_seq[0]);
	uint16_t seq = sys_get_be16(&id_seq[2]);

	if (id != icmp_expect_id || seq != icmp_expect_seq) {
		return NET_CONTINUE;
	}

	if (net_pkt_remaining_data(pkt) >= sizeof(uint32_t)) {
		if (net_pkt_read_be32(pkt, &sent_cycles)) {
			return NET_DROP;
		}

		icmp_rtt_ms =
			(uint32_t)(k_cyc_to_ns_floor64(k_cycle_get_32() - sent_cycles) / 1000000U);
	} else {
		icmp_rtt_ms = 0U;
	}

	k_sem_give(&icmp_sem);
	return NET_OK;
}

static uint32_t send_ping_wait_reply(struct dect_icmp_ping_argv *ping_args)
{
	struct zsock_addrinfo *si = ping_args->src;
	struct net_icmp_ping_params params;
	static uint16_t echo_seq;
	int64_t start_t;
	int ret;

	if (si->ai_family != AF_INET6) {
		dect_icmp_ping_error("ping: only IPv6 is supported");
		return 0U;
	}

	echo_seq++;
	if (echo_seq == 0) {
		echo_seq = 1;
	}

	memset(&params, 0, sizeof(params));
	icmp_expect_id = (uint16_t)(sys_rand32_get() & 0xFFFFu);
	if (icmp_expect_id == 0U) {
		icmp_expect_id = 1U;
	}
	icmp_expect_seq = echo_seq;

	params.identifier = icmp_expect_id;
	params.sequence = icmp_expect_seq;
	params.tc_tos = 0U;
	params.priority = -1;
	params.data = NULL;
	params.data_size = ping_args->len;

	k_sem_init(&icmp_sem, 0, 1);

	start_t = k_uptime_get();
	ret = net_icmp_send_echo_request_no_wait(&icmp_ctx, ping_args->ping_iface,
						 (struct net_sockaddr *)ping_args->dest->ai_addr,
						 &params, ping_args);
	if (ret < 0) {
		dect_icmp_ping_error("ICMP send failed: %d", ret);
		return 0U;
	}

	ret = k_sem_take(&icmp_sem, K_MSEC(ping_args->timeout));
	if (ret != 0) {
		int32_t after_tx = ping_args->timeout - (int32_t)k_uptime_delta(&start_t);

		dect_icmp_ping_print("Pinging %s results: no response in given timeout %u msec "
				     "(timeout after TX %d)",
				     ping_args->target_name, ping_args->timeout, after_tx);
		return 0U;
	}

	uint32_t rtt_ms = icmp_rtt_ms;

	if (rtt_ms == 0U) {
		rtt_ms = (uint32_t)k_uptime_delta(&start_t);
	}

	dect_icmp_ping_print("Pinging %s results: time=%u.%03usecs, payload sent: %u, "
			     "payload received %u",
			     ping_args->target_name, rtt_ms / 1000U, rtt_ms % 1000U, ping_args->len,
			     ping_args->len);

	return rtt_ms;
}

int dect_icmp_ping_start(struct dect_icmp_ping_argv *ping_args)
{
	struct dect_icmp_ping_argv current_ping_args = {0};
	uint32_t sum = 0;
	uint32_t count = 0;
	uint32_t rtt_min = 0xFFFFFFFF;
	uint32_t rtt_max = 0;
	int set, res;
	uint32_t ping_t;
	int ret = 0;

	if (ping_args->shell != NULL) {
		dect_icmp_ping_set_shell(ping_args->shell);
	}

	if (!atomic_cas(&dect_icmp_ping_busy, 0, 1)) {
		dect_icmp_ping_error("ping already in progress");
		return -EBUSY;
	}

	if (!dect_icmp_ping_current_conn_info_set(ping_args, &current_ping_args)) {
		ret = -1;
		goto out;
	}

	if (current_ping_args.dest->ai_family != AF_INET6) {
		dect_icmp_ping_error("ping: only IPv6 destination is supported");
		zsock_freeaddrinfo(current_ping_args.src);
		zsock_freeaddrinfo(current_ping_args.dest);
		current_ping_args.src = NULL;
		current_ping_args.dest = NULL;
		ret = -1;
		goto out;
	}

	ret = net_icmp_init_ctx(&icmp_ctx, NET_AF_INET6, NET_ICMPV6_ECHO_REPLY, 0,
				dect_icmpv6_echo_reply_handler);
	if (ret < 0) {
		dect_icmp_ping_error("net_icmp_init_ctx failed: %d", ret);
		zsock_freeaddrinfo(current_ping_args.src);
		zsock_freeaddrinfo(current_ping_args.dest);
		current_ping_args.src = NULL;
		current_ping_args.dest = NULL;
		goto out;
	}

	for (int i = 0; i < current_ping_args.count; i++) {
		if (current_ping_args.conn_info_read_uptime <
		    ipv6_connected_status_updated_uptime) {
			dect_icmp_ping_print("Re-reading conn info...");
			if (current_ping_args.dest) {
				zsock_freeaddrinfo(current_ping_args.dest);
			}
			current_ping_args.dest = NULL;
			if (current_ping_args.src) {
				zsock_freeaddrinfo(current_ping_args.src);
			}
			current_ping_args.src = NULL;
			if (!dect_icmp_ping_current_conn_info_set(ping_args, &current_ping_args)) {
				dect_icmp_ping_warn("Failed to re-read conn info - continue");
				k_sleep(K_MSEC(current_ping_args.interval));
				continue;
			}
		}
		ping_t = send_ping_wait_reply(&current_ping_args);

		if (current_ping_args.kill_signal != NULL) {
			k_poll_signal_check(current_ping_args.kill_signal, &set, &res);
			if (set) {
				k_poll_signal_reset(current_ping_args.kill_signal);
				dect_icmp_ping_error("KILL signal received - exiting");
				break;
			}
		}

		if (ping_t > 0) {
			count++;
			sum += ping_t;
			rtt_max = MAX(rtt_max, ping_t);
			rtt_min = MIN(rtt_min, ping_t);
		}
		k_sleep(K_MSEC(current_ping_args.interval));
	}

	uint32_t lost = current_ping_args.count - count;

	dect_icmp_ping_print("Ping statistics for %s:", current_ping_args.target_name);
	dect_icmp_ping_print("    Packets: Sent = %d, Received = %d, Lost = %d (%d%% loss)",
			     current_ping_args.count, count, lost,
			     current_ping_args.count > 0 ?
				     (int)(lost * 100 / current_ping_args.count) :
				     0);

	if (count > 0) {
		dect_icmp_ping_print("Approximate round trip times in milli-seconds:");
		dect_icmp_ping_print("    Minimum = %dms, Maximum = %dms, Average = %dms", rtt_min,
				     rtt_max, sum / count);
	}

	(void)net_icmp_cleanup_ctx(&icmp_ctx);

	zsock_freeaddrinfo(current_ping_args.src);
	current_ping_args.src = NULL;
	zsock_freeaddrinfo(current_ping_args.dest);
	current_ping_args.dest = NULL;

	dect_icmp_ping_print("Pinging DONE");

out:
	atomic_set(&dect_icmp_ping_busy, 0);
	return ret;
}

void dect_icmp_ping_cmd_defaults_set(struct dect_icmp_ping_argv *ping_args)
{
	memset(ping_args, 0, sizeof(struct dect_icmp_ping_argv));
	ping_args->kill_signal = abort_signal;
	ping_args->count = DECT_ICMP_PARAM_COUNT_DEFAULT;
	ping_args->interval = DECT_ICMP_PARAM_INTERVAL_DEFAULT;
	ping_args->timeout = DECT_ICMP_PARAM_TIMEOUT_DEFAULT;
	ping_args->len = DECT_ICMP_PARAM_LENGTH_DEFAULT;
	ping_args->mtu = DECT_ICMP_DEFAULT_LINK_MTU;

	ping_args->ping_iface =
		net_if_get_by_index(net_if_get_by_name(CONFIG_DECT_ICMP_PING_DEFAULT_IFACE));
	if (!ping_args->ping_iface) {
		dect_icmp_ping_error("%s: Interface %s not found", __func__,
				     CONFIG_DECT_ICMP_PING_DEFAULT_IFACE);
	}
}

static char *net_utils_sckt_addr_ntop(const struct net_sockaddr *addr)
{
	static char buf[NET_IPV6_ADDR_LEN];

	if (addr->sa_family == AF_INET6) {
		return zsock_inet_ntop(AF_INET6, &net_sin6(addr)->sin6_addr, buf, sizeof(buf));
	}

	strcpy(buf, "Unknown AF");
	return buf;
}

static void dect_icmp_ping_argv_addrinfo_free(struct dect_icmp_ping_argv *ping_argv)
{
	if (ping_argv->dest != NULL) {
		zsock_freeaddrinfo(ping_argv->dest);
		ping_argv->dest = NULL;
	}
	if (ping_argv->src != NULL) {
		zsock_freeaddrinfo(ping_argv->src);
		ping_argv->src = NULL;
	}
}

static bool dect_icmp_ping_current_conn_info_set(struct dect_icmp_ping_argv *ping_args,
					       struct dect_icmp_ping_argv *ping_argv)
{
	int st = -1;
	struct zsock_addrinfo *res;
	char src_ipv_addr[NET_IPV6_ADDR_LEN];
	char *service = NULL;
	const struct in6_addr *src;
	struct zsock_addrinfo *old_src = ping_argv->src;
	struct zsock_addrinfo *old_dest = ping_argv->dest;

	memcpy(ping_argv, ping_args, sizeof(struct dect_icmp_ping_argv));
	ping_argv->src = NULL;
	ping_argv->dest = NULL;
	zsock_freeaddrinfo(old_dest);
	zsock_freeaddrinfo(old_src);

	dect_icmp_ping_print("Initiating ping to: %s", ping_argv->target_name);

	struct zsock_addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_flags = 0;

	res = NULL;

	st = zsock_getaddrinfo(ping_argv->target_name, service, &hints, &res);

	if (st != 0) {
		dect_icmp_ping_error("getaddrinfo(dest) error: %d (%s)",
			st, zsock_gai_strerror(st));
		goto exit;
	}
	ping_argv->dest = res;

	struct net_sockaddr *dst_sa;
	struct net_sockaddr *src_sa;

	dst_sa = ping_argv->dest->ai_addr;

	src = net_if_ipv6_select_src_addr(ping_argv->ping_iface, &net_sin6(dst_sa)->sin6_addr);
	if (!src) {
		dect_icmp_ping_error("No source address found for destination %s",
				     net_utils_sckt_addr_ntop(dst_sa));
		goto exit;
	}
	memcpy(&(ping_argv->current_addr6), src, sizeof(struct in6_addr));
	zsock_inet_ntop(AF_INET6, &(ping_argv->current_addr6), src_ipv_addr, sizeof(src_ipv_addr));

	st = zsock_getaddrinfo(src_ipv_addr, service, &hints, &res);
	if (st != 0) {
		dect_icmp_ping_error("%s: getaddrinfo(src) error: %d (%s)", __func__, st,
				     zsock_gai_strerror(st));
		goto exit;
	}
	ping_argv->src = res;
	src_sa = ping_argv->src->ai_addr;

	if (ping_argv->src->ai_family != ping_argv->dest->ai_family) {
		dect_icmp_ping_error("Source/Destination address family error");
		goto exit;
	}

	dect_icmp_ping_print("Source IP addr: %s", net_utils_sckt_addr_ntop(src_sa));
	dect_icmp_ping_print("Destination IP addr: %s", net_utils_sckt_addr_ntop(dst_sa));

	if (ping_argv->len > DECT_ICMP_IPV6_MAX_LEN) {
		dect_icmp_ping_warn("Payload size exceeds the link limits: "
				    "MTU %d - headers %d = %d bytes",
				    ping_argv->mtu, (DECT_ICMP_IPV6_HDR_LEN + DECT_ICMP_HDR_LEN),
				    DECT_ICMP_IPV6_MAX_LEN);
	}
	ping_argv->conn_info_read_uptime = k_uptime_get();
	return true;
exit:
	dect_icmp_ping_argv_addrinfo_free(ping_argv);
	return false;
}

#if defined(CONFIG_NET_CONNECTION_MANAGER)
#define L4_EVENT_MASK (NET_EVENT_L4_IPV6_CONNECTED)
static struct net_mgmt_event_callback l4_cb;

static void l4_event_handler(struct net_mgmt_event_callback *cb, uint64_t event,
			     struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	switch (event) {
	case NET_EVENT_L4_IPV6_CONNECTED:
		ipv6_connected_status_updated_uptime = k_uptime_get();
		break;
	default:
		break;
	}
}
#endif

static int dect_icmp_ping_sys_init(void)
{
	ipv6_connected_status_updated_uptime = k_uptime_get();
	k_sem_init(&icmp_sem, 0, 1);
#if defined(CONFIG_NET_CONNECTION_MANAGER)
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);
#endif
	return 0;
}

SYS_INIT(dect_icmp_ping_sys_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
