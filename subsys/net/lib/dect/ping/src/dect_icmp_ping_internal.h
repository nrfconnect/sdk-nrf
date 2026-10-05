/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_ICMP_PING_INTERNAL_H__
#define DECT_ICMP_PING_INTERNAL_H__

#include <zephyr/types.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>

#define DECT_ICMP_IPV6_HDR_LEN 40

#define DECT_ICMP_MAX_ADDR	      128
#define DECT_ICMP_DEFAULT_LINK_MTU 1500
#define DECT_ICMP_HDR_LEN	      8

#define DECT_ICMP_IPV6_MAX_LEN \
	(DECT_ICMP_DEFAULT_LINK_MTU - DECT_ICMP_IPV6_HDR_LEN - DECT_ICMP_HDR_LEN)

#define DECT_ICMP_PARAM_LENGTH_DEFAULT   0
#define DECT_ICMP_PARAM_COUNT_DEFAULT    4
#define DECT_ICMP_PARAM_TIMEOUT_DEFAULT  5000
#define DECT_ICMP_PARAM_INTERVAL_DEFAULT 1000

struct shell;
struct k_poll_signal;
struct net_if;

struct dect_icmp_ping_argv {
	struct shell *shell;
	struct k_poll_signal *kill_signal;

	char target_name[DECT_ICMP_MAX_ADDR + 1];
	struct zsock_addrinfo *src;
	struct zsock_addrinfo *dest;
	struct in6_addr current_addr6;

	uint32_t mtu;
	uint32_t len;
	uint32_t timeout;
	uint32_t count;
	uint32_t interval;
	struct net_if *ping_iface;

	int64_t conn_info_read_uptime;
};

int dect_icmp_ping_start(struct dect_icmp_ping_argv *ping_args);
void dect_icmp_ping_cmd_defaults_set(struct dect_icmp_ping_argv *ping_args);

#endif /* DECT_ICMP_PING_INTERNAL_H__ */
