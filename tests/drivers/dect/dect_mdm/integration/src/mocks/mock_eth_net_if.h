/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOCK_ETH_NET_IF_H_
#define MOCK_ETH_NET_IF_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/net/net_ip.h>

#if defined(CONFIG_NET_L2_ETHERNET) && !defined(CONFIG_MODEM_CELLULAR)

struct net_if;

/** One captured net_ipv6_send_ns() on the fake Ethernet iface. */
struct dect_test_eth_nd_ns_record {
	struct in6_addr src;
	struct in6_addr dst;
	struct in6_addr tgt;
	bool is_my_address;
};

/** One captured net_ipv6_send_na() on the fake Ethernet iface. */
struct dect_test_eth_nd_na_record {
	struct in6_addr src;
	struct in6_addr dst;
	struct in6_addr tgt;
	uint8_t flags;
};

struct net_if *dect_test_get_mock_eth_net_if(void);

void dect_test_mock_eth_reset_tx_counters(void);

int dect_test_mock_eth_tx_total(void);
int dect_test_mock_eth_na_tx_count(void);
int dect_test_mock_eth_ns_tx_count(void);

int dect_test_mock_eth_ns_record_count(void);
const struct dect_test_eth_nd_ns_record *dect_test_mock_eth_ns_record_get(int index);

int dect_test_mock_eth_na_record_count(void);
const struct dect_test_eth_nd_na_record *dect_test_mock_eth_na_record_get(int index);

#if defined(CONFIG_NET_IPV6)
int dect_test_mock_eth_ipv6_unicast_used_count(void);
void dect_test_mock_eth_restore_ipv6_unicast(void);

int dect_test_mock_eth_inject_ns(struct net_if *iface, const struct in6_addr *src,
				 const struct in6_addr *dst, const struct in6_addr *target,
				 bool unicast_dst);

/** Capture link-layer source on the next fake-Ethernet TX (cross-L2 forward tests). */
void dect_test_mock_eth_tx_ll_capture_enable(bool enable);
bool dect_test_mock_eth_tx_ll_capture_done(void);
struct net_if *dect_test_mock_eth_tx_ll_capture_iface(void);
uint8_t dect_test_mock_eth_tx_ll_capture_src_len(void);
const uint8_t *dect_test_mock_eth_tx_ll_capture_src(void);
#endif /* CONFIG_NET_IPV6 */

#endif /* CONFIG_NET_L2_ETHERNET && !CONFIG_MODEM_CELLULAR */

#endif /* MOCK_ETH_NET_IF_H_ */
