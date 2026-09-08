/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Fake Ethernet device for DECT L2 sink tests when CONFIG_NET_L2_ETHERNET is
 * enabled and CONFIG_MODEM_CELLULAR is disabled. Based on Zephyr eth_fake
 * (tests/net/iface). TX frames are captured for ND proxy / unsolicited NA tests;
 * RX injection supports Neighbor Solicitation tests.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <icmpv6.h>
#include <ipv6.h>
#include <zephyr/net/icmp.h>
#include <zephyr/sys/atomic.h>

#include "mock_eth_net_if.h"

#if defined(CONFIG_NET_L2_ETHERNET) && !defined(CONFIG_MODEM_CELLULAR)

#define DECT_TEST_ETH_MTU 1500
#define DECT_TEST_ETH_ND_RECORD_MAX 8

struct dect_test_eth_context {
	struct net_if *iface;
	uint8_t mac_address[6];
	bool promisc_mode;
};

static struct dect_test_eth_context dect_test_eth_data;

static int dect_eth_tx_total;
static int dect_eth_na_tx_count;
static int dect_eth_ns_tx_count;

static bool dect_eth_tx_ll_capture_active;
static bool dect_eth_tx_ll_capture_done;
static struct net_if *dect_eth_tx_ll_capture_iface;
static uint8_t dect_eth_tx_ll_capture_src_len;
static uint8_t dect_eth_tx_ll_capture_src[8];

static struct dect_test_eth_nd_ns_record dect_eth_ns_records[DECT_TEST_ETH_ND_RECORD_MAX];
static int dect_eth_ns_record_count;
static struct dect_test_eth_nd_na_record dect_eth_na_records[DECT_TEST_ETH_ND_RECORD_MAX];
static int dect_eth_na_record_count;

static void dect_eth_record_ns(const struct net_in6_addr *src, const struct net_in6_addr *dst,
			       const struct net_in6_addr *tgt, bool is_my_address)
{
	struct dect_test_eth_nd_ns_record *rec;

	if (dect_eth_ns_record_count >= DECT_TEST_ETH_ND_RECORD_MAX || src == NULL ||
	    dst == NULL || tgt == NULL) {
		return;
	}

	rec = &dect_eth_ns_records[dect_eth_ns_record_count++];
	rec->src = *src;
	rec->dst = *dst;
	rec->tgt = *tgt;
	rec->is_my_address = is_my_address;
}

static void dect_eth_record_na(const struct net_in6_addr *src, const struct net_in6_addr *dst,
			       const struct net_in6_addr *tgt, uint8_t flags)
{
	struct dect_test_eth_nd_na_record *rec;

	if (dect_eth_na_record_count >= DECT_TEST_ETH_ND_RECORD_MAX || src == NULL ||
	    dst == NULL || tgt == NULL) {
		return;
	}

	rec = &dect_eth_na_records[dect_eth_na_record_count++];
	rec->src = *src;
	rec->dst = *dst;
	rec->tgt = *tgt;
	rec->flags = flags;
}

static const uint8_t dect_test_eth_router_addr[16] = {
	0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

static void dect_eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct dect_test_eth_context *ctx = dev->data;

	ctx->iface = iface;
	ctx->mac_address[0] = 0x02;
	ctx->mac_address[1] = 0xde;
	ctx->mac_address[2] = 0xad;
	ctx->mac_address[3] = 0xbe;
	ctx->mac_address[4] = 0xef;
	ctx->mac_address[5] = 0x00;

	net_if_set_link_addr(iface, ctx->mac_address, sizeof(ctx->mac_address),
			     NET_LINK_ETHERNET);
	ethernet_init(iface);

	/* L3 address provisioning is the caller's job. Mirror the upstream
	 * zephyr/tests/net/iface eth_fake_iface_init: set link addr +
	 * ethernet_init() only. Tests add IPv6 addresses on demand via
	 * dect_test_mock_eth_restore_ipv6_unicast().
	 */
}

static int dect_eth_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);

	if (dect_eth_tx_ll_capture_active && pkt != NULL) {
		dect_eth_tx_ll_capture_done = true;
		dect_eth_tx_ll_capture_iface = net_pkt_iface(pkt);
		dect_eth_tx_ll_capture_src_len = net_pkt_lladdr_src(pkt)->len;
		memcpy(dect_eth_tx_ll_capture_src, net_pkt_lladdr_src(pkt)->addr,
		       MIN(net_pkt_lladdr_src(pkt)->len, sizeof(dect_eth_tx_ll_capture_src)));
	}

	dect_eth_tx_total++;

	return 0;
}

void dect_test_mock_eth_tx_ll_capture_enable(bool enable)
{
	if (enable) {
		dect_eth_tx_ll_capture_done = false;
		dect_eth_tx_ll_capture_iface = NULL;
		dect_eth_tx_ll_capture_src_len = 0;
		memset(dect_eth_tx_ll_capture_src, 0, sizeof(dect_eth_tx_ll_capture_src));
	}

	dect_eth_tx_ll_capture_active = enable;
}

bool dect_test_mock_eth_tx_ll_capture_done(void)
{
	return dect_eth_tx_ll_capture_done;
}

struct net_if *dect_test_mock_eth_tx_ll_capture_iface(void)
{
	return dect_eth_tx_ll_capture_iface;
}

uint8_t dect_test_mock_eth_tx_ll_capture_src_len(void)
{
	return dect_eth_tx_ll_capture_src_len;
}

const uint8_t *dect_test_mock_eth_tx_ll_capture_src(void)
{
	return dect_eth_tx_ll_capture_src;
}

static enum ethernet_hw_caps dect_eth_get_capabilities(const struct device *dev,
						       struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return ETHERNET_PROMISC_MODE;
}

static int dect_eth_set_config(const struct device *dev, struct net_if *iface,
			       enum ethernet_config_type type,
			       const struct ethernet_config *config)
{
	struct dect_test_eth_context *ctx = dev->data;

	ARG_UNUSED(iface);

	switch (type) {
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (config->promisc_mode == ctx->promisc_mode) {
			return -EALREADY;
		}
		ctx->promisc_mode = config->promisc_mode;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct ethernet_api dect_eth_api = {
	.iface_api.init = dect_eth_iface_init,
	.get_capabilities = dect_eth_get_capabilities,
	.set_config = dect_eth_set_config,
	.send = dect_eth_send,
};

static int dect_eth_init(const struct device *dev)
{
	struct dect_test_eth_context *ctx = dev->data;

	ctx->promisc_mode = false;
	return 0;
}

ETH_NET_DEVICE_INIT(dect_test_eth, "dect_test_eth", dect_eth_init, NULL,
		    &dect_test_eth_data, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &dect_eth_api, DECT_TEST_ETH_MTU);

struct net_if *dect_test_get_mock_eth_net_if(void)
{
	if (dect_test_eth_data.iface == NULL) {
		return NULL;
	}

	return dect_test_eth_data.iface;
}

void dect_test_mock_eth_reset_tx_counters(void)
{
	dect_eth_tx_total = 0;
	dect_eth_na_tx_count = 0;
	dect_eth_ns_tx_count = 0;
	dect_eth_ns_record_count = 0;
	dect_eth_na_record_count = 0;
}

int dect_test_mock_eth_tx_total(void)
{
	return dect_eth_tx_total;
}

int dect_test_mock_eth_na_tx_count(void)
{
	return dect_eth_na_tx_count;
}

int dect_test_mock_eth_ns_tx_count(void)
{
	return dect_eth_ns_tx_count;
}

int dect_test_mock_eth_ns_record_count(void)
{
	return dect_eth_ns_record_count;
}

const struct dect_test_eth_nd_ns_record *dect_test_mock_eth_ns_record_get(int index)
{
	if (index < 0 || index >= dect_eth_ns_record_count) {
		return NULL;
	}

	return &dect_eth_ns_records[index];
}

int dect_test_mock_eth_na_record_count(void)
{
	return dect_eth_na_record_count;
}

const struct dect_test_eth_nd_na_record *dect_test_mock_eth_na_record_get(int index)
{
	if (index < 0 || index >= dect_eth_na_record_count) {
		return NULL;
	}

	return &dect_eth_na_records[index];
}

#if defined(CONFIG_NET_IPV6)
int dect_test_mock_eth_ipv6_unicast_used_count(void)
{
	struct net_if *iface = dect_test_get_mock_eth_net_if();
	struct net_if_ipv6 *ipv6;
	int n = 0;

	if (iface == NULL || iface->config.ip.ipv6 == NULL) {
		return 0;
	}

	ipv6 = iface->config.ip.ipv6;
	for (int i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (ipv6->unicast[i].is_used &&
		    ipv6->unicast[i].address.family == NET_AF_INET6) {
			n++;
		}
	}

	return n;
}

void dect_test_mock_eth_restore_ipv6_unicast(void)
{
	struct net_if *iface = dect_test_eth_data.iface;
	struct in6_addr global;
	struct net_if_addr *ifaddr;

	if (iface == NULL) {
		return;
	}

	memcpy(global.s6_addr, dect_test_eth_router_addr, sizeof(global.s6_addr));
	ifaddr = net_if_ipv6_addr_lookup_by_iface(iface, &global);
	if (ifaddr != NULL) {
		return;
	}

	ifaddr = net_if_ipv6_addr_add(iface, &global, NET_ADDR_AUTOCONF, 0);
	if (ifaddr != NULL) {
		ifaddr->addr_state = NET_ADDR_PREFERRED;
	}
}

int dect_test_mock_eth_inject_ns(struct net_if *iface,
				 const struct in6_addr *src,
				 const struct in6_addr *dst,
				 const struct in6_addr *target,
				 bool unicast_dst)
{
	struct net_eth_hdr eth_hdr;
	struct net_pkt *pkt;
	struct net_icmpv6_ns_hdr ns_hdr = { .reserved = 0 };

	if (iface == NULL || src == NULL || dst == NULL || target == NULL) {
		return -EINVAL;
	}

	memcpy(ns_hdr.tgt, target->s6_addr, sizeof(ns_hdr.tgt));

	pkt = net_pkt_alloc_with_buffer(iface, 128, NET_AF_INET6, NET_IPPROTO_ICMPV6, K_NO_WAIT);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	net_pkt_set_ipv6_hop_limit(pkt, NET_IPV6_ND_HOP_LIMIT);

	eth_hdr.type = net_htons(NET_ETH_PTYPE_IPV6);
	memcpy(eth_hdr.src.addr, dect_test_eth_data.mac_address, sizeof(eth_hdr.src.addr));
	if (unicast_dst) {
		/* Unicast NS (NUD): L2 dest = sink MAC. */
		memcpy(eth_hdr.dst.addr, dect_test_eth_data.mac_address, sizeof(eth_hdr.dst.addr));
	} else {
		struct net_in6_addr mcast;

		net_ipv6_addr_create_solicited_node(target, &mcast);
		eth_hdr.dst.addr[0] = 0x33;
		eth_hdr.dst.addr[1] = 0x33;
		memcpy(&eth_hdr.dst.addr[2], &mcast.s6_addr[11], 4);
	}

	net_buf_reserve(pkt->frags, sizeof(struct net_eth_hdr));
	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, false);

	if (net_ipv6_create(pkt, src, dst) < 0 ||
	    net_icmpv6_create(pkt, NET_ICMPV6_NS, 0) < 0 ||
	    net_pkt_write(pkt, &ns_hdr, sizeof(ns_hdr)) < 0) {
		net_pkt_unref(pkt);
		return -EIO;
	}

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, NET_IPPROTO_ICMPV6);
	net_buf_push_mem(pkt->frags, &eth_hdr, sizeof(eth_hdr));
	net_pkt_cursor_init(pkt);
	net_pkt_set_forwarding(pkt, true);

	return net_recv_data(iface, pkt);
}

extern int __real_net_ipv6_send_na(struct net_if *iface, const struct net_in6_addr *src,
				   const struct net_in6_addr *dst, const struct net_in6_addr *tgt,
				   uint8_t flags);

int __wrap_net_ipv6_send_na(struct net_if *iface, const struct net_in6_addr *src,
			    const struct net_in6_addr *dst, const struct net_in6_addr *tgt,
			    uint8_t flags)
{
	if (iface == dect_test_eth_data.iface) {
		dect_eth_na_tx_count++;
		dect_eth_record_na(src, dst, tgt, flags);
	}

	return __real_net_ipv6_send_na(iface, src, dst, tgt, flags);
}

extern int __real_net_ipv6_send_ns(struct net_if *iface, struct net_pkt *pending,
				   const struct net_in6_addr *src, const struct net_in6_addr *dst,
				   const struct net_in6_addr *tgt, bool is_my_address);

int __wrap_net_ipv6_send_ns(struct net_if *iface, struct net_pkt *pending,
			    const struct net_in6_addr *src, const struct net_in6_addr *dst,
			    const struct net_in6_addr *tgt, bool is_my_address)
{
	if (iface == dect_test_eth_data.iface) {
		dect_eth_ns_tx_count++;
		dect_eth_record_ns(src, dst, tgt, is_my_address);
	}

	return __real_net_ipv6_send_ns(iface, pending, src, dst, tgt, is_my_address);
}
#endif /* CONFIG_NET_IPV6 */

#endif /* CONFIG_NET_L2_ETHERNET && !CONFIG_MODEM_CELLULAR */

/* net_if_start_rs wrap: shared between PPP (cellular) and fake-eth variants. */
#if defined(CONFIG_MODEM_CELLULAR) || defined(CONFIG_NET_L2_ETHERNET)
extern void __real_net_if_start_rs(struct net_if *iface);

int test_net_if_start_rs_call_count;

void __wrap_net_if_start_rs(struct net_if *iface)
{
	test_net_if_start_rs_call_count++;
	__real_net_if_start_rs(iface);
}
#endif

#if defined(CONFIG_MODEM_CELLULAR)
extern struct net_if *__real_net_if_get_first_by_type(const struct net_l2 *l2);

struct net_if *__wrap_net_if_get_first_by_type(const struct net_l2 *l2)
{
	extern const struct net_l2 _net_l2_PPP;

	if (l2 == &_net_l2_PPP) {
		return dect_test_get_mock_ppp_net_if();
	}

	return __real_net_if_get_first_by_type(l2);
}
#endif
