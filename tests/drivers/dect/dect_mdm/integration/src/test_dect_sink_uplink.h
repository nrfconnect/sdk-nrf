/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Shared sink uplink helpers for PPP (cellular mock) and Ethernet mock variants.
 */

#ifndef TEST_DECT_SINK_UPLINK_H_
#define TEST_DECT_SINK_UPLINK_H_

#include <zephyr/net/net_if.h>
#include <zephyr/sys/util.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_NET_IPV6),
	     "DECT integration tests require CONFIG_NET_IPV6=y");

#if defined(CONFIG_MODEM_CELLULAR) || defined(CONFIG_NET_L2_ETHERNET)
#define DECT_TEST_HAVE_SINK_UPLINK 1
#endif

extern int test_net_if_start_rs_call_count;

/* Shared "primary DECT test iface" pointer, set by main test suite and used by
 * eth-only tests too. Keep declaration here so a rename/type change breaks all
 * users at compile time.
 */
extern struct net_if *test_iface;

struct net_if *test_sink_uplink_if(void);

void test_sink_uplink_set_up(struct net_if *uplink);
void test_mock_sink_notify_prefix_router_nbr(struct net_if *uplink);
void test_mock_sink_notify_prefix_router_nbr_nd_pending(struct net_if *uplink);

bool test_sink_seed_incomplete_nbr(struct net_if *iface, const struct in6_addr *addr);

int test_sink_uplink_ipv6_unicast_used_count(struct net_if *uplink);
void test_sink_uplink_restore_ipv6_unicast(struct net_if *uplink);

#if defined(CONFIG_NET_L2_ETHERNET) && !defined(CONFIG_MODEM_CELLULAR)
extern int dect_test_mock_eth_ns_tx_count(void);
void test_eth_sink_bring_connected(struct net_if *eth_if);
bool test_eth_sink_reset_router_nd_pending(struct net_if *eth_if);
bool test_eth_sink_wait_na_tx(int baseline, int timeout_ms);
bool test_eth_sink_wait_eth_tx(int baseline, int timeout_ms);
bool test_eth_sink_wait_na_tx_count(int min_count, int timeout_ms);
bool test_eth_sink_wait_pt_add_nd_proxy_burst(int timeout_ms);
#endif

#endif /* TEST_DECT_SINK_UPLINK_H_ */
