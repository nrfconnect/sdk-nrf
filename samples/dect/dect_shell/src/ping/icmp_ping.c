/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>

#include <zephyr/shell/shell.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>

#include "desh_print.h"
#include "desh_defines.h"

#include "icmp_ping.h"

extern struct k_poll_signal desh_signal;

/* When IPv6 connection status was updated last time: used to detect when
 * iface addressing must be re-read before the next echo request.
 */
static int64_t ipv6_connected_status_updated_uptime;

static bool icmp_ping_current_conn_info_set(struct icmp_ping_shell_cmd_argv *ping_args,
					    struct icmp_ping_shell_cmd_argv *ping_argv);

/*
 * Same ICMP echo path as Zephyr "net ping" (see subsys/net/lib/shell/ping.c):
 * raw INET sockets do not reliably receive echo replies because delivery goes
 * through net_conn_raw_ip_input() clones while the stack still owns the packet
 * flow differently than the native ICMP API.
 */
static struct net_icmp_ctx desh_icmp_ctx;
static struct k_sem desh_icmp_sem;
static uint16_t desh_icmp_expect_id;
static uint16_t desh_icmp_expect_seq;
static uint32_t desh_icmp_rtt_ms;

static enum net_verdict desh_icmpv6_echo_reply_handler(struct net_icmp_ctx *ctx,
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

	if (id != desh_icmp_expect_id || seq != desh_icmp_expect_seq) {
		return NET_CONTINUE;
	}

	if (net_pkt_remaining_data(pkt) >= sizeof(uint32_t)) {
		if (net_pkt_read_be32(pkt, &sent_cycles)) {
			return NET_DROP;
		}

		desh_icmp_rtt_ms =
			(uint32_t)(k_cyc_to_ns_floor64(k_cycle_get_32() - sent_cycles) / 1000000U);
	} else {
		desh_icmp_rtt_ms = 0U;
	}

	k_sem_give(&desh_icmp_sem);
	return NET_OK;
}

static uint32_t send_ping_wait_reply(struct icmp_ping_shell_cmd_argv *ping_args)
{
	struct zsock_addrinfo *si = ping_args->src;
	struct net_icmp_ping_params params;
	static uint16_t echo_seq;
	int64_t start_t;
	int ret;

	if (si->ai_family != AF_INET6) {
		desh_error("ping: only IPv6 is supported");
		return 0U;
	}

	echo_seq++;
	if (echo_seq == 0) {
		echo_seq = 1;
	}

	memset(&params, 0, sizeof(params));
	desh_icmp_expect_id = (uint16_t)(sys_rand32_get() & 0xFFFFu);
	if (desh_icmp_expect_id == 0U) {
		desh_icmp_expect_id = 1U;
	}
	desh_icmp_expect_seq = echo_seq;

	params.identifier = desh_icmp_expect_id;
	params.sequence = desh_icmp_expect_seq;
	params.tc_tos = 0U;
	params.priority = -1;
	params.data = NULL;
	params.data_size = ping_args->len;

	k_sem_init(&desh_icmp_sem, 0, 1);

	start_t = k_uptime_get();
	ret = net_icmp_send_echo_request_no_wait(&desh_icmp_ctx, ping_args->ping_iface,
						 (struct net_sockaddr *)ping_args->dest->ai_addr,
						 &params, ping_args);
	if (ret < 0) {
		desh_error("ICMP send failed: %d", ret);
		return 0U;
	}

	ret = k_sem_take(&desh_icmp_sem, K_MSEC(ping_args->timeout));
	if (ret != 0) {
		int32_t after_tx = ping_args->timeout - (int32_t)k_uptime_delta(&start_t);

		desh_print("Pinging %s results: no response in given timeout %u msec "
			   "(timeout after TX %d)",
			   ping_args->target_name, ping_args->timeout, after_tx);
		return 0U;
	}

	uint32_t rtt_ms = desh_icmp_rtt_ms;

	if (rtt_ms == 0U) {
		rtt_ms = (uint32_t)k_uptime_delta(&start_t);
	}

	desh_print("Pinging %s results: time=%u.%03usecs, payload sent: %u, payload received %u",
		   ping_args->target_name, rtt_ms / 1000U, rtt_ms % 1000U, ping_args->len,
		   ping_args->len);

	return rtt_ms;
}

/*****************************************************************************/

int icmp_ping_start(struct icmp_ping_shell_cmd_argv *ping_args)
{
	struct icmp_ping_shell_cmd_argv current_ping_args;
	uint32_t sum = 0;
	uint32_t count = 0;
	uint32_t rtt_min = 0xFFFFFFFF;
	uint32_t rtt_max = 0;
	int set, res;
	uint32_t ping_t;
	int ret = 0;

	/* All good for ping_args, get the current connection info and start the ping */
	if (!icmp_ping_current_conn_info_set(ping_args, &current_ping_args)) {
		return -1;
	}

	if (current_ping_args.dest->ai_family != AF_INET6) {
		desh_error("ping: only IPv6 destination is supported");
		zsock_freeaddrinfo(current_ping_args.src);
		zsock_freeaddrinfo(current_ping_args.dest);
		current_ping_args.src = NULL;
		current_ping_args.dest = NULL;
		return -1;
	}

	ret = net_icmp_init_ctx(&desh_icmp_ctx, NET_AF_INET6, NET_ICMPV6_ECHO_REPLY, 0,
				desh_icmpv6_echo_reply_handler);
	if (ret < 0) {
		desh_error("net_icmp_init_ctx failed: %d", ret);
		zsock_freeaddrinfo(current_ping_args.src);
		zsock_freeaddrinfo(current_ping_args.dest);
		current_ping_args.src = NULL;
		current_ping_args.dest = NULL;
		return -1;
	}

	for (int i = 0; i < current_ping_args.count; i++) {
		if (current_ping_args.conn_info_read_uptime <
			ipv6_connected_status_updated_uptime) {
			/* Connection status has changed since last read,
			 * re-read the connection info:
			 */
			desh_print("Re-reading conn info...");
			if (current_ping_args.dest) {
				zsock_freeaddrinfo(current_ping_args.dest);
			}
			current_ping_args.dest = NULL;
			if (current_ping_args.src) {
				zsock_freeaddrinfo(current_ping_args.src);
			}
			current_ping_args.src = NULL;
			if (!icmp_ping_current_conn_info_set(ping_args, &current_ping_args)) {
				desh_warn("Failed to re-read conn info - continue");
				k_sleep(K_MSEC(current_ping_args.interval));
				continue;
			}
		}
		ping_t = send_ping_wait_reply(&current_ping_args);

		k_poll_signal_check(current_ping_args.kill_signal, &set, &res);
		if (set) {
			k_poll_signal_reset(current_ping_args.kill_signal);
			desh_error("KILL signal received - exiting");
			break;
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

	desh_print("Ping statistics for %s:", current_ping_args.target_name);
	desh_print("    Packets: Sent = %d, Received = %d, Lost = %d (%d%% loss)",
		   current_ping_args.count, count, lost, lost * 100 / current_ping_args.count);

	if (count > 0) {
		desh_print("Approximate round trip times in milli-seconds:");
		desh_print("    Minimum = %dms, Maximum = %dms, Average = %dms", rtt_min, rtt_max,
			   sum / count);
	}

	(void)net_icmp_cleanup_ctx(&desh_icmp_ctx);

	zsock_freeaddrinfo(current_ping_args.src);
	current_ping_args.src = NULL;
	zsock_freeaddrinfo(current_ping_args.dest);
	current_ping_args.dest = NULL;

	desh_print("Pinging DONE");

	return ret;
}

/*****************************************************************************/

void icmp_ping_cmd_defaults_set(struct icmp_ping_shell_cmd_argv *ping_args)
{
	memset(ping_args, 0, sizeof(struct icmp_ping_shell_cmd_argv));
	/* ping_args->dest = NULL; */
	ping_args->kill_signal = &desh_signal;
	ping_args->count = ICMP_PARAM_COUNT_DEFAULT;
	ping_args->interval = ICMP_PARAM_INTERVAL_DEFAULT;
	ping_args->timeout = ICMP_PARAM_TIMEOUT_DEFAULT;
	ping_args->len = ICMP_PARAM_LENGTH_DEFAULT;
	ping_args->mtu = ICMP_DEFAULT_LINK_MTU;

	ping_args->ping_iface = net_if_get_by_index(
		net_if_get_by_name(CONFIG_DECT_MDM_DEVICE_NAME));
	if (!ping_args->ping_iface) {
		desh_error("%s: Interface %s not found", (__func__),
			CONFIG_DECT_MDM_DEVICE_NAME);
	}
}

/*****************************************************************************/

/**
 * @brief Read and set current connection parameters.
 *
 * @param ping_args In: Current ping parameters.
 * @param ping_argv Out: Ping parameters with updated connection parameters.
 *
 */
char *net_utils_sckt_addr_ntop(const struct net_sockaddr *addr)
{
	static char buf[NET_IPV6_ADDR_LEN];

	if (addr->sa_family == AF_INET6) {
		return zsock_inet_ntop(AF_INET6, &net_sin6(addr)->sin6_addr, buf, sizeof(buf));
	}

	strcpy(buf, "Unknown AF");
	return buf;
}

static bool icmp_ping_current_conn_info_set(struct icmp_ping_shell_cmd_argv *ping_args,
					    struct icmp_ping_shell_cmd_argv *ping_argv)
{
	int st = -1;
	struct zsock_addrinfo *res;
	char src_ipv_addr[NET_IPV6_ADDR_LEN];
	char *service = NULL;
	const struct in6_addr *src;

	/* Finally copy args in local storage here and start pinging */
	memcpy(ping_argv, ping_args, sizeof(struct icmp_ping_shell_cmd_argv));

	desh_print("Initiating ping to: %s", ping_argv->target_name);

	/* Sets getaddrinfo hints by using current host address(es): */
	struct zsock_addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_flags = 0;

	/* Get destination */
	res = NULL;

	st = zsock_getaddrinfo(ping_argv->target_name, service, &hints, &res);

	if (st != 0) {
		desh_error("getaddrinfo(dest) error: %d", st);
		desh_error("Cannot resolve remote host\r\n");
		zsock_freeaddrinfo(ping_argv->src);
		ping_argv->src = NULL;
		goto exit;
	}
	ping_argv->dest = res;

	struct net_sockaddr *dst_sa;
	struct net_sockaddr *src_sa;

	dst_sa = ping_argv->dest->ai_addr;

	src = net_if_ipv6_select_src_addr(ping_argv->ping_iface, &net_sin6(dst_sa)->sin6_addr);
	if (!src) {
		desh_error("No source address found for destination %s",
			   net_utils_sckt_addr_ntop(dst_sa));
		goto exit;
	}
	memcpy(&(ping_argv->current_addr6), src, sizeof(struct in6_addr));
	zsock_inet_ntop(AF_INET6, &(ping_argv->current_addr6), src_ipv_addr, sizeof(src_ipv_addr));

	st = zsock_getaddrinfo(src_ipv_addr, service, &hints, &res);
	if (st != 0) {
		desh_error("%s: getaddrinfo(src) error: %d", (__func__), st);
		goto exit;
	}
	ping_argv->src = res;
	src_sa = ping_argv->src->ai_addr;

	if (ping_argv->src->ai_family != ping_argv->dest->ai_family) {
		desh_error("Source/Destination address family error");
		zsock_freeaddrinfo(ping_argv->dest);
		ping_argv->dest = NULL;
		zsock_freeaddrinfo(ping_argv->src);
		ping_argv->src = NULL;
		goto exit;
	}

	desh_print("Source IP addr: %s", net_utils_sckt_addr_ntop(src_sa));
	desh_print("Destination IP addr: %s", net_utils_sckt_addr_ntop(dst_sa));

	if (ping_argv->len > ICMP_IPV6_MAX_LEN) {
		desh_warn("Payload size exceeds the link limits: MTU %d - headers %d = %d ",
			  ping_argv->mtu, (ICMP_IPV6_HDR_LEN + ICMP_HDR_LEN), ICMP_IPV6_MAX_LEN);
	}
	ping_argv->conn_info_read_uptime = k_uptime_get();
	return true;
exit:
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

static int icmp_ping_init(void)
{
	ipv6_connected_status_updated_uptime = k_uptime_get();
	k_sem_init(&desh_icmp_sem, 0, 1);
#if defined(CONFIG_NET_CONNECTION_MANAGER)
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);
#endif
	return 0;
}

SYS_INIT(icmp_ping_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
