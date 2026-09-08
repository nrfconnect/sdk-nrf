/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Ethernet-sink-specific integration tests (eth_sink.conf).
 */

#include "unity.h"
#include "mock_nrf_modem_dect_mac.h"
#include "mock_eth_net_if.h"
#include "test_dect_utils.h"
#include "test_dect_sink_uplink.h"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/icmp.h>
#include <icmpv6.h>
#include <route_ipv6.h>
#include <ipv6.h>
#include <nbr.h>
#include <net/dect/dect_net_l2_mgmt.h>
#include <net/dect/dect_utils.h>

LOG_MODULE_DECLARE(test_dect_integration, CONFIG_TEST_DECT_INTEGRATION_LOG_LEVEL);

#define TEST_BEACON_SHORT_RD_ID 0x1234

#if defined(CONFIG_NET_L2_ETHERNET) && !defined(CONFIG_MODEM_CELLULAR)

/* Fresh PT for eth ND proxy pt_add (must differ from cluster tests' 0xCAFEBABE). */
#define TEST_ETH_ND_PROXY_PT_SHORT_RD_ID 0x5678
#define TEST_ETH_ND_PROXY_PT_LONG_RD_ID  0xB00B1E5U

static bool test_eth_wait_child_gua(struct net_if *dect_if, uint32_t long_rd_id,
				     struct in6_addr *gua_out, int timeout_ms)
{
	for (int elapsed = 0; elapsed < timeout_ms; elapsed += 100) {
		struct dect_status_info status = {0};

		if (test_dect_status_info_get(dect_if, &status) != 0) {
			k_sleep(K_MSEC(100));
			continue;
		}

		for (uint8_t i = 0; i < status.child_count; i++) {
			if (status.child_associations[i].long_rd_id == long_rd_id &&
			    status.child_associations[i].global_ipv6_addr_set) {
				if (gua_out != NULL) {
					*gua_out = status.child_associations[i].global_ipv6_addr;
				}
				return true;
			}
		}

		k_sleep(K_MSEC(100));
	}

	return false;
}

static void test_eth_assert_addr_eq(const struct in6_addr *expected, const struct in6_addr *actual,
				    const char *label)
{
	TEST_ASSERT_TRUE_MESSAGE(net_ipv6_addr_cmp(expected, actual),
				 label ? label : "IPv6 address should match");
}

static void test_eth_assert_pt_nd_proxy_ns_prime(const struct in6_addr *pt_global,
						 const struct in6_addr *router)
{
	const struct dect_test_eth_nd_ns_record *ns;

	TEST_ASSERT_EQUAL_MESSAGE(1, dect_test_mock_eth_ns_record_count(),
				  "One NS record should be captured on eth");
	ns = dect_test_mock_eth_ns_record_get(0);
	TEST_ASSERT_NOT_NULL_MESSAGE(ns, "NS prime record should exist");
	test_eth_assert_addr_eq(pt_global, &ns->src,
				"NS prime src should be PT GUA");
	test_eth_assert_addr_eq(router, &ns->dst,
				"NS prime dst should be default router (unicast)");
	test_eth_assert_addr_eq(router, &ns->tgt,
				"NS prime target should be default router");
	TEST_ASSERT_FALSE_MESSAGE(ns->is_my_address,
				  "NS prime should use PT GUA as src (is_my_address=false)");
}

static void test_eth_assert_pt_nd_proxy_na_unicast(const struct in6_addr *pt_global,
						   const struct in6_addr *router, int index)
{
	const struct dect_test_eth_nd_na_record *na;

	na = dect_test_mock_eth_na_record_get(index);
	TEST_ASSERT_NOT_NULL_MESSAGE(na, "Unicast NA record should exist");
	test_eth_assert_addr_eq(pt_global, &na->src,
				"Unicast NA src should be PT GUA");
	test_eth_assert_addr_eq(router, &na->dst,
				"Unicast NA dst should be default router");
	test_eth_assert_addr_eq(pt_global, &na->tgt,
				"Unicast NA target should be PT GUA");
	TEST_ASSERT_BITS_MESSAGE(NET_ICMPV6_NA_FLAG_OVERRIDE, NET_ICMPV6_NA_FLAG_OVERRIDE,
				 na->flags, "Unicast NA should set Override flag");
	TEST_ASSERT_BITS_MESSAGE(0, NET_ICMPV6_NA_FLAG_SOLICITED, na->flags,
				 "Unicast NA (initial) should not be solicited");
}

static void test_eth_assert_pt_nd_proxy_na_unsolicited(const struct in6_addr *pt_global,
						       int index)
{
	const struct dect_test_eth_nd_na_record *na;
	struct in6_addr allnodes;

	net_ipv6_addr_create_ll_allnodes_mcast(&allnodes);

	na = dect_test_mock_eth_na_record_get(index);
	TEST_ASSERT_NOT_NULL_MESSAGE(na, "Unsolicited NA record should exist");
	test_eth_assert_addr_eq(pt_global, &na->src,
				"Unsolicited NA src should be PT GUA");
	test_eth_assert_addr_eq(&allnodes, &na->dst,
				"Unsolicited NA dst should be ff02::1 (all-nodes)");
	test_eth_assert_addr_eq(pt_global, &na->tgt,
				"Unsolicited NA target should be PT GUA");
	TEST_ASSERT_BITS_MESSAGE(NET_ICMPV6_NA_FLAG_OVERRIDE, NET_ICMPV6_NA_FLAG_OVERRIDE,
				 na->flags, "Unsolicited NA should set Override flag");
	TEST_ASSERT_BITS_MESSAGE(0, NET_ICMPV6_NA_FLAG_SOLICITED, na->flags,
				 "Unsolicited NA should not be solicited");
}

static void test_eth_assert_pt_nd_proxy_na_solicited(const struct in6_addr *pt_global,
						       const struct in6_addr *ns_src, int index)
{
	const struct dect_test_eth_nd_na_record *na;

	na = dect_test_mock_eth_na_record_get(index);
	TEST_ASSERT_NOT_NULL_MESSAGE(na, "Solicited NA record should exist");
	test_eth_assert_addr_eq(pt_global, &na->src,
				"Solicited NA src should be PT GUA (RFC 4861 7.2.4)");
	test_eth_assert_addr_eq(ns_src, &na->dst,
				"Solicited NA dst should be NS source (router)");
	test_eth_assert_addr_eq(pt_global, &na->tgt,
				"Solicited NA target should be PT GUA");
	TEST_ASSERT_BITS_MESSAGE(NET_ICMPV6_NA_FLAG_SOLICITED | NET_ICMPV6_NA_FLAG_OVERRIDE,
				 NET_ICMPV6_NA_FLAG_SOLICITED | NET_ICMPV6_NA_FLAG_OVERRIDE,
				 na->flags, "Solicited NA should set Solicited and Override");
}

extern struct net_if *dect_test_get_mock_eth_net_if(void);
extern int dect_test_mock_eth_inject_ns(struct net_if *iface, const struct in6_addr *src,
					const struct in6_addr *dst, const struct in6_addr *target,
					bool unicast_dst);

static const uint8_t test_ipv6_prefix_sink_64[8] = {0x20, 0x01, 0x0d, 0xb8,
						    0x00, 0x00, 0x00, 0x00};
static const uint8_t test_ipv6_addr_sink_router[16] = {
	0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};
static const uint8_t test_eth_router_lladdr[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

static uint16_t test_eth_icmpv6_checksum(const uint8_t *src_addr, const uint8_t *dst_addr,
					 const uint8_t *payload, size_t payload_len)
{
	uint32_t sum = 0;
	uint16_t odd;

	for (int i = 0; i < 16; i += 2) {
		sum += (uint16_t)(src_addr[i] << 8 | src_addr[i + 1]);
	}
	for (int i = 0; i < 16; i += 2) {
		sum += (uint16_t)(dst_addr[i] << 8 | dst_addr[i + 1]);
	}
	sum += payload_len >> 8;
	sum += payload_len & 0xff;
	sum += 0;
	sum += IPPROTO_ICMPV6;

	for (size_t i = 0; i + 1 < payload_len; i += 2) {
		sum += (uint16_t)(payload[i] << 8 | payload[i + 1]);
	}
	if (payload_len & 1) {
		odd = payload[payload_len - 1];
		sum += odd << 8;
	}

	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}

	return (uint16_t)~sum;
}

void test_dect_ft_eth_sink_init(void)
{
	struct net_if *eth_if = dect_test_get_mock_eth_net_if();

	TEST_ASSERT_NOT_NULL_MESSAGE(eth_if, "Fake Ethernet net_if should exist at boot");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(&NET_L2_GET_NAME(ETHERNET), net_if_l2(eth_if),
				      "Sink uplink should be Ethernet L2");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(eth_if, test_sink_uplink_if(),
				      "test_sink_uplink_if should return fake Ethernet");
}

void test_dect_ft_eth_upstream_prefix_route(void)
{
	struct net_if *eth_if = dect_test_get_mock_eth_net_if();
	struct net_route_entry *re = NULL;
	struct net_in6_addr *nexthop = NULL;
	struct in6_addr upstream_prefix = {0};
	struct in6_addr in_prefix_dst = {0};
	struct in6_addr expected_router = {0};
	bool found;

	TEST_ASSERT_NOT_NULL_MESSAGE(eth_if, "Fake Ethernet net_if required");

	test_eth_sink_bring_connected(eth_if);

	memcpy(upstream_prefix.s6_addr, test_ipv6_prefix_sink_64, 8);
	memcpy(in_prefix_dst.s6_addr, test_ipv6_addr_sink_router, sizeof(in_prefix_dst.s6_addr));
	in_prefix_dst.s6_addr[15] = 0x02; /* 2001:db8::2 — in-prefix, not the router nbr */

	memcpy(expected_router.s6_addr, test_ipv6_addr_sink_router,
	       sizeof(expected_router.s6_addr));

	/*
	 * Query 2001:db8:: (prefix network address), not ::1: net_route_get_info()
	 * checks the neighbor table first; the router is a nbr and would return
	 * route=NULL with nexthop=dst, masking the static /64 route.
	 */
	found = net_route_ipv6_get_info(eth_if, (struct net_in6_addr *)&upstream_prefix, &re,
					&nexthop);
	TEST_ASSERT_TRUE_MESSAGE(found, "Upstream /64 route should exist on Ethernet sink iface");
	TEST_ASSERT_NOT_NULL_MESSAGE(re,
				     "Route entry pointer should be set (not nbr shortcut)");
	TEST_ASSERT_NOT_NULL_MESSAGE(nexthop, "Route nexthop should be set");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(eth_if, re->iface, "Route should be on Ethernet sink iface");
	TEST_ASSERT_EQUAL_MESSAGE(64, re->prefix_len, "Upstream route should be /64");
	TEST_ASSERT_EQUAL_MESSAGE(NET_ROUTE_PREFERENCE_HIGH, re->preference,
				  "Upstream route preference should be HIGH");
	TEST_ASSERT_TRUE_MESSAGE(re->is_infinite, "Upstream route lifetime should be infinite");
	TEST_ASSERT_TRUE_MESSAGE(
		net_ipv6_is_prefix(re->addr.in6_addr.s6_addr, upstream_prefix.s6_addr, 64),
		"Route prefix should match delegated upstream /64");
	TEST_ASSERT_TRUE_MESSAGE(
		net_ipv6_addr_cmp(nexthop, (struct net_in6_addr *)&expected_router),
		"Route nexthop should be default router on Ethernet");

	/* In-prefix host should resolve via the same static route
	 * (sink_install_eth_upstream_prefix_route).
	 */
	re = NULL;
	nexthop = NULL;
	found = net_route_ipv6_get_info(eth_if, (struct net_in6_addr *)&in_prefix_dst, &re,
					&nexthop);
	TEST_ASSERT_TRUE_MESSAGE(found, "In-prefix dst should resolve on Ethernet sink iface");
	TEST_ASSERT_NOT_NULL_MESSAGE(re, "In-prefix dst should use route table entry");
	TEST_ASSERT_TRUE_MESSAGE(
		net_ipv6_addr_cmp(nexthop, (struct net_in6_addr *)&expected_router),
		"In-prefix dst nexthop should be upstream router");

	/* Nexthop neighbor: sink_install_eth_upstream_prefix_route() requires
	 * router in nbr cache.
	 */
	{
		struct net_nbr *router_nbr;
		struct net_ipv6_nbr_data *router_nbr_data;
		struct net_linkaddr *router_ll;
		struct net_route_entry *router_re = NULL;
		struct net_in6_addr *router_nexthop = NULL;
		bool router_via_nbr;

		router_nbr = net_ipv6_nbr_lookup(eth_if, (struct net_in6_addr *)&expected_router);
		TEST_ASSERT_NOT_NULL_MESSAGE(router_nbr,
					     "Upstream router neighbor should exist on Ethernet");
		router_nbr_data = net_ipv6_nbr_data(router_nbr);
		TEST_ASSERT_NOT_NULL_MESSAGE(router_nbr_data, "Router neighbor data should be set");
		TEST_ASSERT_EQUAL_PTR_MESSAGE(eth_if, router_nbr->iface,
					      "Router neighbor should be on Ethernet sink iface");
		TEST_ASSERT_EQUAL_MESSAGE(NET_IPV6_NBR_STATE_REACHABLE, router_nbr_data->state,
					  "Router neighbor should be REACHABLE");
		TEST_ASSERT_TRUE_MESSAGE(router_nbr_data->is_router,
					 "Router neighbor should be flagged as router");
		TEST_ASSERT_NOT_EQUAL_MESSAGE(NET_NBR_LLADDR_UNKNOWN, router_nbr->idx,
					      "Router neighbor should have link-layer address");

		router_ll = net_nbr_get_lladdr(router_nbr->idx);
		TEST_ASSERT_NOT_NULL_MESSAGE(router_ll, "Router link-layer address should exist");
		TEST_ASSERT_EQUAL_MESSAGE(NET_LINK_ETHERNET, router_ll->type,
					  "Router link-layer type should be Ethernet");
		TEST_ASSERT_EQUAL_MESSAGE(6, router_ll->len,
					  "Router link-layer length should be 6");
		TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(test_eth_router_lladdr, router_ll->addr, 6,
						      "Router link-layer address should match RA SLLAO fixture");

		/* Direct lookup of router uses nbr table (route=NULL), not the /64 route entry. */
		router_via_nbr = net_route_ipv6_get_info(eth_if,
							 (struct net_in6_addr *)&expected_router,
							 &router_re, &router_nexthop);
		TEST_ASSERT_TRUE_MESSAGE(router_via_nbr,
					 "Router address should resolve via neighbor cache");
		TEST_ASSERT_NULL_MESSAGE(router_re,
					 "Router lookup should not return route-table entry");
		TEST_ASSERT_EQUAL_PTR_MESSAGE((struct net_in6_addr *)&expected_router,
					      router_nexthop,
					      "Router lookup nexthop should be router itself");
	}
}

void test_dect_ft_eth_cross_l2_route_packet_ll_src(void)
{
	struct net_if *eth_if = dect_test_get_mock_eth_net_if();
	struct dect_status_info status_info = {0};
	struct in6_addr pt_global = {0};
	struct in6_addr upstream_dst = {0};
	const uint32_t pt_long_rd_id = TEST_ETH_ND_PROXY_PT_LONG_RD_ID;
	const struct net_linkaddr *eth_ll;
	int eth_tx_before;
	bool found_child = false;
	uint8_t ipv6_pkt[64];
	size_t pkt_len = 0;

	TEST_ASSERT_NOT_NULL_MESSAGE(eth_if, "Fake Ethernet net_if required");
	TEST_ASSERT_NOT_NULL_MESSAGE(mock_ntf_callbacks.dlc_data_rx_ntf,
				     "DLC data RX notification callback required");

	/* Reuse PT associated in test_dect_ft_eth_nd_proxy_pt_add (runs before this). */
	TEST_ASSERT_EQUAL_MESSAGE(0, test_dect_status_info_get(test_iface, &status_info),
				  "DECT status info get should succeed");
	for (uint8_t i = 0; i < status_info.child_count; i++) {
		if (status_info.child_associations[i].long_rd_id == pt_long_rd_id &&
		    status_info.child_associations[i].global_ipv6_addr_set) {
			pt_global = status_info.child_associations[i].global_ipv6_addr;
			found_child = true;
			break;
		}
	}
	TEST_ASSERT_TRUE_MESSAGE(found_child,
				 "PT from prior pt_add test should have global IPv6 address");

	eth_ll = net_if_get_link_addr(eth_if);
	TEST_ASSERT_NOT_NULL_MESSAGE(eth_ll, "Ethernet iface link address should exist");
	TEST_ASSERT_EQUAL_MESSAGE(6, eth_ll->len, "Ethernet link-layer address should be 6 bytes");

	/* Upstream router GUA (2001:db8::1): nbr-table hit, route=NULL.
	 * ipv6_route_packet() then leaves pkt on dect0; net_route_packet()
	 * must switch to eth0 before copying LL src (8398122). In-prefix ::2
	 * uses the /64 route entry and pre-switches iface in ipv6.c, masking
	 * the bug.
	 */
	memcpy(upstream_dst.s6_addr, test_ipv6_addr_sink_router, sizeof(upstream_dst.s6_addr));

	/*
	 * PT → upstream host over DECT (mock libmodem RX) → sink forwards on eth0.
	 * sink_install_eth_upstream_prefix_route() installs the /64 via the router
	 * neighbor that bring_connected() already seeded on eth0.
	 */
	{
		struct {
			uint8_t vtc_flow[4];
			uint16_t payload_len;
			uint8_t next_header;
			uint8_t hop_limit;
			uint8_t src_addr[16];
			uint8_t dst_addr[16];
		} __packed ipv6_hdr;
		struct {
			uint8_t type;
			uint8_t code;
			uint16_t checksum;
			uint16_t identifier;
			uint16_t sequence;
		} __packed icmpv6_hdr;
		uint8_t echo_payload[] = {0x01, 0x02, 0x03, 0x04};
		uint8_t icmpv6_msg[sizeof(icmpv6_hdr) + sizeof(echo_payload)];

		memset(&ipv6_hdr, 0, sizeof(ipv6_hdr));
		ipv6_hdr.vtc_flow[0] = 0x60;
		ipv6_hdr.payload_len = htons(sizeof(icmpv6_hdr) + sizeof(echo_payload));
		ipv6_hdr.next_header = IPPROTO_ICMPV6;
		ipv6_hdr.hop_limit = 64;
		memcpy(ipv6_hdr.src_addr, &pt_global, sizeof(ipv6_hdr.src_addr));
		memcpy(ipv6_hdr.dst_addr, &upstream_dst, sizeof(ipv6_hdr.dst_addr));

		memset(&icmpv6_hdr, 0, sizeof(icmpv6_hdr));
		icmpv6_hdr.type = NET_ICMPV6_ECHO_REQUEST;
		icmpv6_hdr.identifier = htons(0x1234);
		icmpv6_hdr.sequence = htons(1);

		memcpy(icmpv6_msg, &icmpv6_hdr, sizeof(icmpv6_hdr));
		memcpy(icmpv6_msg + sizeof(icmpv6_hdr), echo_payload, sizeof(echo_payload));
		icmpv6_hdr.checksum = htons(test_eth_icmpv6_checksum(
			ipv6_hdr.src_addr, ipv6_hdr.dst_addr, icmpv6_msg, sizeof(icmpv6_msg)));

		memcpy(ipv6_pkt, &ipv6_hdr, sizeof(ipv6_hdr));
		pkt_len = sizeof(ipv6_hdr);
		memcpy(ipv6_pkt + pkt_len, &icmpv6_hdr, sizeof(icmpv6_hdr));
		pkt_len += sizeof(icmpv6_hdr);
		memcpy(ipv6_pkt + pkt_len, echo_payload, sizeof(echo_payload));
		pkt_len += sizeof(echo_payload);
	}

	eth_tx_before = dect_test_mock_eth_tx_total();
	dect_test_mock_eth_tx_ll_capture_enable(true);

	{
		struct nrf_modem_dect_dlc_data_rx_ntf_cb_params rx_params = {
			.flow_id = 0,
			.long_rd_id = pt_long_rd_id,
			.data = ipv6_pkt,
			.data_len = pkt_len,
		};

		mock_ntf_callbacks.dlc_data_rx_ntf(&rx_params);
	}

	TEST_ASSERT_TRUE_MESSAGE(test_eth_sink_wait_eth_tx(eth_tx_before, 3000),
				 "PT packet toward upstream should forward out fake Ethernet");

	TEST_ASSERT_TRUE_MESSAGE(dect_test_mock_eth_tx_ll_capture_done(),
				 "Forwarded packet should reach fake Ethernet driver send");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(eth_if, dect_test_mock_eth_tx_ll_capture_iface(),
				      "Egress iface should be eth0 after cross-L2 forward");
	TEST_ASSERT_EQUAL_MESSAGE(6, dect_test_mock_eth_tx_ll_capture_src_len(),
				  "LL src length should come from eth0 (6), not dect0 (8)");
	TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(eth_ll->addr, dect_test_mock_eth_tx_ll_capture_src(),
					      eth_ll->len,
					      "LL src should be fake Ethernet MAC on eth egress");

	dect_test_mock_eth_tx_ll_capture_enable(false);
}

void test_dect_ft_eth_cross_l2_forward_unknown_ll_nbr(void)
{
	struct net_if *eth_if = dect_test_get_mock_eth_net_if();
	struct dect_status_info status_info = {0};
	struct in6_addr pt_global = {0};
	struct in6_addr upstream_dst = {0};
	const uint32_t pt_long_rd_id = TEST_ETH_ND_PROXY_PT_LONG_RD_ID;
	int eth_tx_before;
	bool found_child = false;
	uint8_t ipv6_pkt[64];
	size_t pkt_len = 0;

	TEST_ASSERT_NOT_NULL_MESSAGE(eth_if, "Fake Ethernet net_if required");
	TEST_ASSERT_NOT_NULL_MESSAGE(mock_ntf_callbacks.dlc_data_rx_ntf,
				     "DLC data RX notification callback required");

	/* Reuse PT from test_dect_ft_eth_nd_proxy_pt_add (runs before this). */
	TEST_ASSERT_EQUAL_MESSAGE(0, test_dect_status_info_get(test_iface, &status_info),
				  "DECT status info get should succeed");
	for (uint8_t i = 0; i < status_info.child_count; i++) {
		if (status_info.child_associations[i].long_rd_id == pt_long_rd_id &&
		    status_info.child_associations[i].global_ipv6_addr_set) {
			pt_global = status_info.child_associations[i].global_ipv6_addr;
			found_child = true;
			break;
		}
	}
	TEST_ASSERT_TRUE_MESSAGE(found_child,
				 "PT from prior pt_add test should have global IPv6 address");

	/* In-prefix host on eth0 without resolved LL (ND incomplete). */
	memcpy(upstream_dst.s6_addr, test_ipv6_addr_sink_router, sizeof(upstream_dst.s6_addr));
	upstream_dst.s6_addr[15] = 0x02;

	TEST_ASSERT_TRUE_MESSAGE(test_sink_seed_incomplete_nbr(eth_if, &upstream_dst),
				 "Upstream host nbr on eth should stay INCOMPLETE with unknown LL");

	{
		struct {
			uint8_t vtc_flow[4];
			uint16_t payload_len;
			uint8_t next_header;
			uint8_t hop_limit;
			uint8_t src_addr[16];
			uint8_t dst_addr[16];
		} __packed ipv6_hdr;
		struct {
			uint8_t type;
			uint8_t code;
			uint16_t checksum;
			uint16_t identifier;
			uint16_t sequence;
		} __packed icmpv6_hdr;
		uint8_t echo_payload[] = {0x01, 0x02, 0x03, 0x04};
		uint8_t icmpv6_msg[sizeof(icmpv6_hdr) + sizeof(echo_payload)];

		memset(&ipv6_hdr, 0, sizeof(ipv6_hdr));
		ipv6_hdr.vtc_flow[0] = 0x60;
		ipv6_hdr.payload_len = htons(sizeof(icmpv6_hdr) + sizeof(echo_payload));
		ipv6_hdr.next_header = IPPROTO_ICMPV6;
		ipv6_hdr.hop_limit = 64;
		memcpy(ipv6_hdr.src_addr, &pt_global, sizeof(ipv6_hdr.src_addr));
		memcpy(ipv6_hdr.dst_addr, &upstream_dst, sizeof(ipv6_hdr.dst_addr));

		memset(&icmpv6_hdr, 0, sizeof(icmpv6_hdr));
		icmpv6_hdr.type = NET_ICMPV6_ECHO_REQUEST;
		icmpv6_hdr.identifier = htons(0x1234);
		icmpv6_hdr.sequence = htons(2);

		memcpy(icmpv6_msg, &icmpv6_hdr, sizeof(icmpv6_hdr));
		memcpy(icmpv6_msg + sizeof(icmpv6_hdr), echo_payload, sizeof(echo_payload));
		icmpv6_hdr.checksum = htons(test_eth_icmpv6_checksum(
			ipv6_hdr.src_addr, ipv6_hdr.dst_addr, icmpv6_msg, sizeof(icmpv6_msg)));

		memcpy(ipv6_pkt, &ipv6_hdr, sizeof(ipv6_hdr));
		pkt_len = sizeof(ipv6_hdr);
		memcpy(ipv6_pkt + pkt_len, &icmpv6_hdr, sizeof(icmpv6_hdr));
		pkt_len += sizeof(icmpv6_hdr);
		memcpy(ipv6_pkt + pkt_len, echo_payload, sizeof(echo_payload));
		pkt_len += sizeof(echo_payload);
	}

	eth_tx_before = dect_test_mock_eth_tx_total();

	{
		struct nrf_modem_dect_dlc_data_rx_ntf_cb_params rx_params = {
			.flow_id = 0,
			.long_rd_id = pt_long_rd_id,
			.data = ipv6_pkt,
			.data_len = pkt_len,
		};

		mock_ntf_callbacks.dlc_data_rx_ntf(&rx_params);
	}

	k_sleep(K_MSEC(500));

	TEST_ASSERT_EQUAL_MESSAGE(eth_tx_before, dect_test_mock_eth_tx_total(),
				  "Forward via unknown-LL nbr should drop without eth TX");

	(void)net_ipv6_nbr_rm(eth_if, &upstream_dst);
}

void test_dect_ft_eth_cross_l2_forward_router_nd_pending(void)
{
	struct net_if *eth_if = dect_test_get_mock_eth_net_if();
	struct dect_status_info status_info = {0};
	struct in6_addr pt_global = {0};
	struct in6_addr router_dst = {0};
	const uint32_t pt_long_rd_id = TEST_ETH_ND_PROXY_PT_LONG_RD_ID;
	int eth_tx_before;
	bool found_child = false;
	uint8_t ipv6_pkt[64];
	size_t pkt_len = 0;

	TEST_ASSERT_NOT_NULL_MESSAGE(eth_if, "Fake Ethernet net_if required");
	TEST_ASSERT_NOT_NULL_MESSAGE(mock_ntf_callbacks.dlc_data_rx_ntf,
				     "DLC data RX notification callback required");

	/* Reuse PT from test_dect_ft_eth_nd_proxy_pt_add (runs before this). */
	TEST_ASSERT_EQUAL_MESSAGE(0, test_dect_status_info_get(test_iface, &status_info),
				  "DECT status info get should succeed");
	for (uint8_t i = 0; i < status_info.child_count; i++) {
		if (status_info.child_associations[i].long_rd_id == pt_long_rd_id &&
		    status_info.child_associations[i].global_ipv6_addr_set) {
			pt_global = status_info.child_associations[i].global_ipv6_addr;
			found_child = true;
			break;
		}
	}
	TEST_ASSERT_TRUE_MESSAGE(found_child,
				 "PT from prior pt_add test should have global IPv6 address");

	/* Cold-start: prefix/router up but default router LL not yet resolved (ceb390e). */
	TEST_ASSERT_TRUE_MESSAGE(test_eth_sink_reset_router_nd_pending(eth_if),
				 "Default router should be INCOMPLETE with unknown LL after reset");

	memcpy(router_dst.s6_addr, test_ipv6_addr_sink_router, sizeof(router_dst.s6_addr));

	{
		struct {
			uint8_t vtc_flow[4];
			uint16_t payload_len;
			uint8_t next_header;
			uint8_t hop_limit;
			uint8_t src_addr[16];
			uint8_t dst_addr[16];
		} __packed ipv6_hdr;
		struct {
			uint8_t type;
			uint8_t code;
			uint16_t checksum;
			uint16_t identifier;
			uint16_t sequence;
		} __packed icmpv6_hdr;
		uint8_t echo_payload[] = {0x01, 0x02, 0x03, 0x04};
		uint8_t icmpv6_msg[sizeof(icmpv6_hdr) + sizeof(echo_payload)];

		memset(&ipv6_hdr, 0, sizeof(ipv6_hdr));
		ipv6_hdr.vtc_flow[0] = 0x60;
		ipv6_hdr.payload_len = htons(sizeof(icmpv6_hdr) + sizeof(echo_payload));
		ipv6_hdr.next_header = IPPROTO_ICMPV6;
		ipv6_hdr.hop_limit = 64;
		memcpy(ipv6_hdr.src_addr, &pt_global, sizeof(ipv6_hdr.src_addr));
		memcpy(ipv6_hdr.dst_addr, &router_dst, sizeof(ipv6_hdr.dst_addr));

		memset(&icmpv6_hdr, 0, sizeof(icmpv6_hdr));
		icmpv6_hdr.type = NET_ICMPV6_ECHO_REQUEST;
		icmpv6_hdr.identifier = htons(0x1234);
		icmpv6_hdr.sequence = htons(3);

		memcpy(icmpv6_msg, &icmpv6_hdr, sizeof(icmpv6_hdr));
		memcpy(icmpv6_msg + sizeof(icmpv6_hdr), echo_payload, sizeof(echo_payload));
		icmpv6_hdr.checksum = htons(test_eth_icmpv6_checksum(
			ipv6_hdr.src_addr, ipv6_hdr.dst_addr, icmpv6_msg, sizeof(icmpv6_msg)));

		memcpy(ipv6_pkt, &ipv6_hdr, sizeof(ipv6_hdr));
		pkt_len = sizeof(ipv6_hdr);
		memcpy(ipv6_pkt + pkt_len, &icmpv6_hdr, sizeof(icmpv6_hdr));
		pkt_len += sizeof(icmpv6_hdr);
		memcpy(ipv6_pkt + pkt_len, echo_payload, sizeof(echo_payload));
		pkt_len += sizeof(echo_payload);
	}

	eth_tx_before = dect_test_mock_eth_tx_total();

	{
		struct nrf_modem_dect_dlc_data_rx_ntf_cb_params rx_params = {
			.flow_id = 0,
			.long_rd_id = pt_long_rd_id,
			.data = ipv6_pkt,
			.data_len = pkt_len,
		};

		mock_ntf_callbacks.dlc_data_rx_ntf(&rx_params);
	}

	k_sleep(K_MSEC(500));

	TEST_ASSERT_EQUAL_MESSAGE(eth_tx_before, dect_test_mock_eth_tx_total(),
				  "Forward to router with pending ND should drop without eth TX");

	/* Restore REACHABLE router nbr for following ND proxy tests. */
	test_mock_sink_notify_prefix_router_nbr(eth_if);
}

void test_dect_ft_eth_unsol_na(void)
{
	struct net_if *eth_if = dect_test_get_mock_eth_net_if();

	TEST_ASSERT_NOT_NULL_MESSAGE(eth_if, "Fake Ethernet net_if required");

	dect_test_mock_eth_reset_tx_counters();
	test_eth_sink_bring_connected(eth_if);

	TEST_ASSERT_TRUE_MESSAGE(test_eth_sink_wait_na_tx(0, 1500),
				 "Unsolicited NA should be sent on Ethernet after prefix set");
}

void test_dect_ft_eth_nd_proxy_pt_add(void)
{
	struct net_if *eth_if = dect_test_get_mock_eth_net_if();
	struct in6_addr pt_global = {0};
	struct in6_addr router = {0};
	const uint32_t pt_long_rd_id = TEST_ETH_ND_PROXY_PT_LONG_RD_ID;

	TEST_ASSERT_NOT_NULL_MESSAGE(eth_if, "Fake Ethernet net_if required");
	TEST_ASSERT_NOT_NULL_MESSAGE(mock_ntf_callbacks.association_ntf,
				     "Association notification callback required");

	memcpy(router.s6_addr, test_ipv6_addr_sink_router, sizeof(router.s6_addr));

	test_eth_sink_bring_connected(eth_if);
	/* Router/prefix already configured by bring_connected; drain deferred NA work. */
	k_sleep(K_MSEC(500));
	dect_test_mock_eth_reset_tx_counters();

	struct nrf_modem_dect_mac_association_ntf_cb_params assoc_ntf_params = {
		.status = NRF_MODEM_DECT_MAC_ASSOCIATION_INDICATION_STATUS_SUCCESS,
		.tx_method = NRF_MODEM_DECT_MAC_COMMUNICATION_METHOD_RACH,
		.rx_signal_info = {.rssi_2 = -50, .snr = 20},
		.short_rd_id = TEST_ETH_ND_PROXY_PT_SHORT_RD_ID,
		.long_rd_id = pt_long_rd_id,
		.number_of_ies = 0,
	};

	mock_ntf_callbacks.association_ntf(&assoc_ntf_params);

	TEST_ASSERT_TRUE_MESSAGE(
		test_eth_wait_child_gua(test_iface, pt_long_rd_id, &pt_global, 3000),
		"PT should get global IPv6 after association");

	/* ND proxy burst is synchronous once GUA nbr is added; short yield for pacing. */
	k_sleep(K_MSEC(50));

	TEST_ASSERT_EQUAL_MESSAGE(1, dect_test_mock_eth_ns_tx_count(),
				"PT ND proxy NS prime (target=router, initial) should be sent once");
	TEST_ASSERT_EQUAL_MESSAGE(2, dect_test_mock_eth_na_tx_count(),
				"PT add should send unicast NA (initial) and unsolicited NA");
	TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(2, dect_test_mock_eth_tx_total(),
					     "PT add ND proxy burst should produce eth TX frames");

	test_eth_assert_pt_nd_proxy_ns_prime(&pt_global, &router);
	test_eth_assert_pt_nd_proxy_na_unicast(&pt_global, &router, 0);
	test_eth_assert_pt_nd_proxy_na_unsolicited(&pt_global, 1);
}

void test_dect_ft_eth_nd_proxy_ns_reply(void)
{
	struct net_if *eth_if = dect_test_get_mock_eth_net_if();
	struct dect_status_info status_info = {0};
	struct in6_addr pt_global = {0};
	struct in6_addr router = {0};
	int na_before;
	int ret;
	bool found_child = false;
	const uint32_t pt_long_rd_id = TEST_ETH_ND_PROXY_PT_LONG_RD_ID;

	TEST_ASSERT_NOT_NULL_MESSAGE(eth_if, "Fake Ethernet net_if required");

	/* Reuse PT associated in test_dect_ft_eth_nd_proxy_pt_add (runs before this). */
	TEST_ASSERT_EQUAL_MESSAGE(0, test_dect_status_info_get(test_iface, &status_info),
				  "DECT status info get should succeed");
	for (uint8_t i = 0; i < status_info.child_count; i++) {
		if (status_info.child_associations[i].long_rd_id == pt_long_rd_id &&
		    status_info.child_associations[i].global_ipv6_addr_set) {
			pt_global = status_info.child_associations[i].global_ipv6_addr;
			found_child = true;
			break;
		}
	}
	TEST_ASSERT_TRUE_MESSAGE(found_child,
				 "PT from prior pt_add test should have global IPv6 address");

	memcpy(router.s6_addr, test_ipv6_addr_sink_router, sizeof(router.s6_addr));

	dect_test_mock_eth_reset_tx_counters();
	na_before = dect_test_mock_eth_na_tx_count();

	/*
	 * Fake eth cannot reliably inject solicited-node multicast NS into the
	 * ICMP handler path on native_sim. Use router-originated unicast NS
	 * (same deferred NA worker as multicast NS reply).
	 */
	ret = dect_test_mock_eth_inject_ns(eth_if, &router, &pt_global, &pt_global, true);
	TEST_ASSERT_EQUAL_MESSAGE(0, ret, "Router NS inject should succeed");

	TEST_ASSERT_TRUE_MESSAGE(test_eth_sink_wait_na_tx_count(1, 3000),
				 "Router NS for PT should schedule ND proxy solicited NA");

	/* Solicited NS reply: dect_pt_nd_proxy_na_work_handler → net_ipv6_send_na(). */
	TEST_ASSERT_EQUAL_MESSAGE(0, dect_test_mock_eth_ns_tx_count(),
				"ND proxy NS reply should not send NS from sink");
	TEST_ASSERT_EQUAL_MESSAGE(1, dect_test_mock_eth_na_tx_count() - na_before,
				"ND proxy solicited NA should be sent once for PT GUA");
	test_eth_assert_pt_nd_proxy_na_solicited(&pt_global, &router, 0);
}

void test_dect_ft_eth_nd_proxy_unicast_ns_intercept(void)
{
	struct net_if *eth_if = dect_test_get_mock_eth_net_if();
	struct dect_status_info status_info = {0};
	struct in6_addr pt_global = {0};
	struct in6_addr router = {0};
	int na_before;
	int ret;
	bool found_child = false;
	const uint32_t pt_long_rd_id = TEST_ETH_ND_PROXY_PT_LONG_RD_ID;

	TEST_ASSERT_NOT_NULL_MESSAGE(eth_if, "Fake Ethernet net_if required");

	TEST_ASSERT_EQUAL_MESSAGE(0, test_dect_status_info_get(test_iface, &status_info),
				  "DECT status info get should succeed");
	for (uint8_t i = 0; i < status_info.child_count; i++) {
		if (status_info.child_associations[i].long_rd_id == pt_long_rd_id &&
		    status_info.child_associations[i].global_ipv6_addr_set) {
			pt_global = status_info.child_associations[i].global_ipv6_addr;
			found_child = true;
			break;
		}
	}
	TEST_ASSERT_TRUE_MESSAGE(found_child,
				 "PT from prior pt_add test should have global IPv6 address");

	memcpy(router.s6_addr, test_ipv6_addr_sink_router, sizeof(router.s6_addr));

	dect_test_mock_eth_reset_tx_counters();
	na_before = dect_test_mock_eth_na_tx_count();

	/* Unicast NS with IPv6 dst = PT GUA (Huawei NUD style). */
	ret = dect_test_mock_eth_inject_ns(eth_if, &router, &pt_global, &pt_global, true);
	TEST_ASSERT_EQUAL_MESSAGE(0, ret, "Unicast NS inject should succeed");

	TEST_ASSERT_TRUE_MESSAGE(test_eth_sink_wait_na_tx_count(1, 3000),
				 "Unicast NS intercept should schedule ND proxy solicited NA");

	TEST_ASSERT_EQUAL_MESSAGE(0, dect_test_mock_eth_ns_tx_count(),
				"Unicast NS intercept should not send NS from sink");
	TEST_ASSERT_EQUAL_MESSAGE(1, dect_test_mock_eth_na_tx_count() - na_before,
				"Unicast NS intercept should send one solicited NA for PT GUA");
	test_eth_assert_pt_nd_proxy_na_solicited(&pt_global, &router, 0);
}

#else

void test_dect_ft_eth_sink_init(void)
{
	TEST_IGNORE_MESSAGE("Ethernet sink variant not enabled");
}

void test_dect_ft_eth_upstream_prefix_route(void)
{
	TEST_IGNORE_MESSAGE("Ethernet sink variant not enabled");
}

void test_dect_ft_eth_cross_l2_route_packet_ll_src(void)
{
	TEST_IGNORE_MESSAGE("Ethernet sink variant not enabled");
}

void test_dect_ft_eth_cross_l2_forward_unknown_ll_nbr(void)
{
	TEST_IGNORE_MESSAGE("Ethernet sink variant not enabled");
}

void test_dect_ft_eth_cross_l2_forward_router_nd_pending(void)
{
	TEST_IGNORE_MESSAGE("Ethernet sink variant not enabled");
}

void test_dect_ft_eth_unsol_na(void)
{
	TEST_IGNORE_MESSAGE("Ethernet sink variant not enabled");
}

void test_dect_ft_eth_nd_proxy_pt_add(void)
{
	TEST_IGNORE_MESSAGE("Ethernet sink variant not enabled");
}

void test_dect_ft_eth_nd_proxy_ns_reply(void)
{
	TEST_IGNORE_MESSAGE("Ethernet sink variant not enabled");
}

void test_dect_ft_eth_nd_proxy_unicast_ns_intercept(void)
{
	TEST_IGNORE_MESSAGE("Ethernet sink variant not enabled");
}

#endif
