/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Fake Ethernet for tether integration tests: capture TX frames and inject RX.
 */

#include <string.h>

#include <zephyr/sys/util.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/udp.h>

#include <icmpv6.h>
#include <ipv6.h>
#include <udp_internal.h>

#include "tether_test_eth.h"

#if defined(CONFIG_NET_L2_ETHERNET) && !defined(CONFIG_MODEM_CELLULAR)

#define TETHER_ETH_MTU 1500

struct tether_eth_ctx {
	struct net_if *iface;
	uint8_t mac_address[6];
};

static struct tether_eth_ctx tether_eth_data;

static int tether_eth_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);

	if (pkt != NULL && tether_tx_capture_count < TETHER_TX_CAPTURE_MAX) {
		size_t len = net_pkt_get_len(pkt);
		struct tether_tx_frame *slot = &tether_tx_frames[tether_tx_capture_count];

		if (len <= sizeof(slot->data)) {
			net_pkt_cursor_init(pkt);
			if (net_pkt_read(pkt, slot->data, len) == 0) {
				slot->len = len;
				tether_tx_capture_count++;
			}
		}
	}

	return 0;
}

static enum ethernet_hw_caps tether_eth_caps(const struct device *dev, struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return ETHERNET_PROMISC_MODE;
}

static int tether_eth_set_config(const struct device *dev, struct net_if *iface,
				 enum ethernet_config_type type,
				 const struct ethernet_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);
	ARG_UNUSED(type);
	ARG_UNUSED(config);

	return -ENOTSUP;
}

static void tether_eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct tether_eth_ctx *ctx = dev->data;

	ctx->iface = iface;
	ctx->mac_address[0] = 0x02;
	ctx->mac_address[1] = 0xaa;
	ctx->mac_address[2] = 0xbb;
	ctx->mac_address[3] = 0xcc;
	ctx->mac_address[4] = 0xdd;
	ctx->mac_address[5] = 0x01;

	net_if_set_link_addr(iface, ctx->mac_address, sizeof(ctx->mac_address), NET_LINK_ETHERNET);
	ethernet_init(iface);
}

static int tether_eth_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static struct ethernet_api tether_eth_api = {
	.iface_api.init = tether_eth_iface_init,
	.get_capabilities = tether_eth_caps,
	.set_config = tether_eth_set_config,
	.send = tether_eth_send,
};

ETH_NET_DEVICE_INIT(tether_test_eth, "tether_test_eth", tether_eth_dev_init, NULL, &tether_eth_data,
		    NULL, CONFIG_ETH_INIT_PRIORITY, &tether_eth_api, TETHER_ETH_MTU);

struct tether_tx_frame tether_tx_frames[TETHER_TX_CAPTURE_MAX];
int tether_tx_capture_count;

struct net_if *tether_test_eth_iface(void)
{
	return tether_eth_data.iface;
}

void tether_test_eth_tx_reset(void)
{
	tether_tx_capture_count = 0;
	memset(tether_tx_frames, 0, sizeof(tether_tx_frames));
}

int tether_test_eth_inject_rs(struct net_if *iface, const struct in6_addr *src,
			      const uint8_t src_mac[6])
{
	struct in6_addr dst = { .s6_addr = { 0xff, 0x02, 0, 0, 0, 0, 0, 0,
					      0, 0, 0, 0, 0, 0, 0, 0x02 } };
	struct net_eth_hdr eth;
	struct net_pkt *pkt;
	uint32_t reserved = 0;

	pkt = net_pkt_alloc_with_buffer(iface, 128, AF_INET6, IPPROTO_ICMPV6, K_NO_WAIT);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	net_pkt_set_ipv6_hop_limit(pkt, 255);

	eth.type = net_htons(NET_ETH_PTYPE_IPV6);
	memcpy(eth.src.addr, src_mac, sizeof(eth.src.addr));
	net_eth_ipv6_mcast_to_mac_addr((struct net_in6_addr *)&dst,
				       (struct net_eth_addr *)&eth.dst);

	net_buf_reserve(pkt->frags, sizeof(eth));
	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, false);
	if (net_ipv6_create(pkt, (struct net_in6_addr *)src, &dst) < 0 ||
	    net_icmpv6_create(pkt, NET_ICMPV6_RS, 0) < 0 ||
	    net_pkt_write(pkt, &reserved, sizeof(reserved)) < 0) {
		net_pkt_unref(pkt);
		return -EIO;
	}

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, NET_IPPROTO_ICMPV6);
	net_buf_push_mem(pkt->frags, &eth, sizeof(eth));
	net_pkt_cursor_init(pkt);

	return net_recv_data(iface, pkt);
}

int tether_test_eth_inject_udp(struct net_if *iface, const struct in6_addr *src,
			       const struct in6_addr *dst, uint16_t sport, uint16_t dport,
			       const uint8_t *payload, size_t payload_len,
			       const uint8_t src_mac[6])
{
	struct net_pkt *pkt;

	ARG_UNUSED(src_mac);

	if (iface == NULL || src == NULL || dst == NULL || payload == NULL) {
		return -EINVAL;
	}

	pkt = net_pkt_alloc_with_buffer(iface, 40U + 8U + payload_len, AF_INET6, IPPROTO_UDP,
					K_NO_WAIT);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	net_pkt_set_ipv6_hop_limit(pkt, 64);
	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, false);
	if (net_ipv6_create(pkt, (struct net_in6_addr *)src, (struct net_in6_addr *)dst) < 0 ||
	    net_udp_create(pkt, sport, dport) < 0 ||
	    net_pkt_write(pkt, payload, payload_len) < 0) {
		net_pkt_unref(pkt);
		return -EIO;
	}

	net_pkt_cursor_init(pkt);
	if (net_ipv6_finalize(pkt, IPPROTO_UDP) < 0) {
		net_pkt_unref(pkt);
		return -EIO;
	}

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_l2_processed(pkt, true);
	net_pkt_cursor_init(pkt);

	return net_recv_data(iface, pkt);
}

int tether_test_eth_wait_ll(struct net_if *iface, struct in6_addr *ll_out, int timeout_ms)
{
	int64_t end = k_uptime_get() + timeout_ms;

	while (k_uptime_get() < end) {
		const struct in6_addr *ll = net_if_ipv6_get_ll(iface, NET_ADDR_PREFERRED);

		if (ll != NULL) {
			if (ll_out != NULL) {
				*ll_out = *ll;
			}
			return 0;
		}
		k_msleep(20);
	}

	return -ETIMEDOUT;
}

#endif /* CONFIG_NET_L2_ETHERNET && !CONFIG_MODEM_CELLULAR */
