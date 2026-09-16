/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TETHER_TEST_ETH_H_
#define TETHER_TEST_ETH_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TETHER_TX_CAPTURE_MAX 32
#define TETHER_TX_CAPTURE_SIZE 1600

struct tether_tx_frame {
	size_t len;
	uint8_t data[TETHER_TX_CAPTURE_SIZE];
};

extern struct tether_tx_frame tether_tx_frames[TETHER_TX_CAPTURE_MAX];
extern int tether_tx_capture_count;

struct net_if *tether_test_eth_iface(void);
void tether_test_eth_tx_reset(void);
int tether_test_eth_inject_rs(struct net_if *iface, const struct in6_addr *src,
			      const uint8_t src_mac[6]);
int tether_test_eth_inject_udp(struct net_if *iface, const struct in6_addr *src,
			       const struct in6_addr *dst, uint16_t sport, uint16_t dport,
			       const uint8_t *payload, size_t payload_len,
			       const uint8_t src_mac[6]);
int tether_test_eth_wait_ll(struct net_if *iface, struct in6_addr *ll_out, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_TEST_ETH_H_ */
