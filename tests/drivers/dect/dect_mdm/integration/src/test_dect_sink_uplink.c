/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "test_dect_sink_uplink.h"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <route_ipv6.h>
#include <ipv6.h>
#include <nbr.h>
#include <net/dect/dect_net_l2.h>

#if defined(CONFIG_MODEM_CELLULAR)
extern struct net_if *dect_test_get_mock_ppp_net_if(void);
extern int dect_test_mock_ppp_ipv6_unicast_used_count(void);
extern void dect_test_mock_ppp_restore_ipv6_unicast(void);
#endif

#if defined(CONFIG_NET_L2_ETHERNET) && !defined(CONFIG_MODEM_CELLULAR)
extern struct net_if *dect_test_get_mock_eth_net_if(void);
extern int dect_test_mock_eth_na_tx_count(void);
extern int dect_test_mock_eth_ns_tx_count(void);
extern int dect_test_mock_eth_tx_total(void);
extern int dect_test_mock_eth_ipv6_unicast_used_count(void);
extern void dect_test_mock_eth_restore_ipv6_unicast(void);
extern bool dect_sink_status_received;
extern struct dect_sink_status_evt received_sink_status_data;
#endif

/* Sink router / prefix fixtures (same as test_dect_integration.c). */
static const uint8_t test_ipv6_addr_sink_router[16] = {
	0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

struct net_if *test_sink_uplink_if(void)
{
#if defined(CONFIG_MODEM_CELLULAR)
	return dect_test_get_mock_ppp_net_if();
#elif defined(CONFIG_NET_L2_ETHERNET)
	return dect_test_get_mock_eth_net_if();
#else
	return NULL;
#endif
}

void test_sink_uplink_set_up(struct net_if *uplink)
{
	if (uplink == NULL) {
		return;
	}

	net_if_flag_set(uplink, NET_IF_UP);
	net_if_flag_set(uplink, NET_IF_RUNNING);
#if defined(CONFIG_NET_L2_ETHERNET) && !defined(CONFIG_MODEM_CELLULAR)
	net_if_flag_set(uplink, NET_IF_LOWER_UP);
#endif
}

void test_mock_sink_notify_prefix_router_nbr(struct net_if *uplink)
{
	test_mock_sink_notify_prefix_router_nbr_nd_pending(uplink);

#if defined(CONFIG_NET_L2_ETHERNET) && !defined(CONFIG_MODEM_CELLULAR)
	if (uplink != NULL) {
		struct in6_addr router_addr;
		struct net_linkaddr ll = {
			.type = NET_LINK_ETHERNET,
			.len = 6,
			.addr = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01},
		};

		memcpy(router_addr.s6_addr, test_ipv6_addr_sink_router,
		       sizeof(router_addr.s6_addr));
		if (net_if_ipv6_router_find_default(uplink, NULL) == NULL) {
			(void)net_if_ipv6_router_add(uplink, &router_addr, true, 7200);
		}
		(void)net_ipv6_nbr_add(uplink, &router_addr, &ll, true,
				       NET_IPV6_NBR_STATE_REACHABLE);
	}
#endif
}

void test_mock_sink_notify_prefix_router_nbr_nd_pending(struct net_if *uplink)
{
	struct net_event_ipv6_prefix prefix_evt = {
		.len = 64,
		.lifetime = 7200,
	};
	struct in6_addr router_addr;
	struct net_event_ipv6_nbr nbr_evt = { .idx = 0 };

	if (uplink == NULL) {
		return;
	}

#if defined(CONFIG_NET_L2_ETHERNET) && !defined(CONFIG_MODEM_CELLULAR)
	/* Real eth sink ifaces need the delegated GUA in the stack unicast table
	 * (PPP mock seeds mock_ppp_ipv6 manually). Mgmt events alone are not enough
	 * for ROUTER_ADD to match prefix and install the upstream /64 route.
	 */
	dect_test_mock_eth_restore_ipv6_unicast();
#endif

	memcpy(prefix_evt.addr.s6_addr, test_ipv6_addr_sink_router,
	       sizeof(test_ipv6_addr_sink_router));
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_PREFIX_ADD, uplink, &prefix_evt,
					sizeof(prefix_evt));
	k_sleep(K_MSEC(50));

	memcpy(router_addr.s6_addr, test_ipv6_addr_sink_router, sizeof(test_ipv6_addr_sink_router));
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ROUTER_ADD, uplink, &router_addr,
					sizeof(router_addr));
	k_sleep(K_MSEC(50));

#if defined(CONFIG_NET_L2_ETHERNET) && !defined(CONFIG_MODEM_CELLULAR)
	if (net_if_ipv6_router_find_default(uplink, NULL) == NULL) {
		(void)net_if_ipv6_router_add(uplink, (struct net_in6_addr *)&router_addr, true,
					     7200);
	}
#endif

	memcpy(nbr_evt.addr.s6_addr, test_ipv6_addr_sink_router,
	       sizeof(test_ipv6_addr_sink_router));
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_NBR_ADD, uplink, &nbr_evt, sizeof(nbr_evt));
	k_sleep(K_MSEC(50));
}

bool test_sink_seed_incomplete_nbr(struct net_if *iface, const struct in6_addr *addr)
{
	struct net_nbr *nbr;

	if (iface == NULL || addr == NULL) {
		return false;
	}

	nbr = net_ipv6_nbr_add(iface, addr, NULL, false, NET_IPV6_NBR_STATE_INCOMPLETE);

	return nbr != NULL && nbr->idx == NET_NBR_LLADDR_UNKNOWN;
}

int test_sink_uplink_ipv6_unicast_used_count(struct net_if *uplink)
{
#if defined(CONFIG_MODEM_CELLULAR)
	ARG_UNUSED(uplink);
	return dect_test_mock_ppp_ipv6_unicast_used_count();
#elif defined(CONFIG_NET_L2_ETHERNET)
	return dect_test_mock_eth_ipv6_unicast_used_count();
#else
	ARG_UNUSED(uplink);
	return 0;
#endif
}

void test_sink_uplink_restore_ipv6_unicast(struct net_if *uplink)
{
#if defined(CONFIG_MODEM_CELLULAR)
	ARG_UNUSED(uplink);
	dect_test_mock_ppp_restore_ipv6_unicast();
#elif defined(CONFIG_NET_L2_ETHERNET)
	ARG_UNUSED(uplink);
	dect_test_mock_eth_restore_ipv6_unicast();
#else
	ARG_UNUSED(uplink);
#endif
}

#if defined(CONFIG_NET_L2_ETHERNET) && !defined(CONFIG_MODEM_CELLULAR)

void test_eth_sink_bring_connected(struct net_if *eth_if)
{
	struct net_event_ipv6_addr addr_evt;
	bool need_if_up;

	if (eth_if == NULL) {
		return;
	}

	/*
	 * NET_EVENT_IF_UP on an already-up iface makes the sink emit a spurious
	 * DISCONNECTED status and reschedule RS work; skip when flags are set.
	 */
	need_if_up = !net_if_flag_is_set(eth_if, NET_IF_UP);
	if (need_if_up) {
		net_if_flag_set(eth_if, NET_IF_UP);
		net_if_flag_set(eth_if, NET_IF_RUNNING);
		net_mgmt_event_notify(NET_EVENT_IF_UP, eth_if);
		k_sleep(K_MSEC(100));
	} else {
		test_sink_uplink_set_up(eth_if);
	}

	memcpy(addr_evt.addr.s6_addr, test_ipv6_addr_sink_router,
	       sizeof(test_ipv6_addr_sink_router));
	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ADDR_ADD, eth_if, &addr_evt,
					sizeof(addr_evt));
	k_sleep(K_MSEC(150));
	test_mock_sink_notify_prefix_router_nbr(eth_if);

	/* Allow deferred sysworkq: upstream route, unsolicited NA, ND proxy. */
	k_sleep(K_MSEC(500));
}

bool test_eth_sink_reset_router_nd_pending(struct net_if *eth_if)
{
	struct in6_addr router_addr;
	struct net_nbr *nbr;
	struct net_ipv6_nbr_data *nbr_data;

	if (eth_if == NULL) {
		return false;
	}

	memcpy(router_addr.s6_addr, test_ipv6_addr_sink_router, sizeof(router_addr.s6_addr));
	(void)net_ipv6_nbr_rm(eth_if, &router_addr);
	test_mock_sink_notify_prefix_router_nbr_nd_pending(eth_if);

	nbr = net_ipv6_nbr_lookup(eth_if, &router_addr);
	if (nbr == NULL) {
		/* net_mgmt events alone do not fill nbr cache; RA without SLLAO does. */
		nbr = net_ipv6_nbr_add(eth_if, &router_addr, NULL, true,
				       NET_IPV6_NBR_STATE_INCOMPLETE);
	}
	if (nbr == NULL) {
		return false;
	}

	nbr_data = net_ipv6_nbr_data(nbr);

	return nbr->idx == NET_NBR_LLADDR_UNKNOWN && nbr_data != NULL &&
	       nbr_data->state == NET_IPV6_NBR_STATE_INCOMPLETE;
}

bool test_eth_sink_wait_na_tx(int baseline, int timeout_ms)
{
	for (int elapsed = 0; elapsed < timeout_ms; elapsed += 50) {
		if (dect_test_mock_eth_na_tx_count() > baseline) {
			return true;
		}
		k_sleep(K_MSEC(50));
	}

	return dect_test_mock_eth_na_tx_count() > baseline;
}

bool test_eth_sink_wait_eth_tx(int baseline, int timeout_ms)
{
	for (int elapsed = 0; elapsed < timeout_ms; elapsed += 50) {
		if (dect_test_mock_eth_tx_total() > baseline) {
			return true;
		}
		k_sleep(K_MSEC(50));
	}

	return dect_test_mock_eth_tx_total() > baseline;
}

bool test_eth_sink_wait_na_tx_count(int min_count, int timeout_ms)
{
	for (int elapsed = 0; elapsed < timeout_ms; elapsed += 50) {
		if (dect_test_mock_eth_na_tx_count() >= min_count) {
			return true;
		}
		k_sleep(K_MSEC(50));
	}

	return dect_test_mock_eth_na_tx_count() >= min_count;
}

/*
 * PT association ND proxy cold-start burst (dect_net_l2_ipv6_pt_eth_nd_proxy_add):
 *   1x NS prime, 1x unicast NA (initial), 1x unsolicited NA (multicast).
 */
bool test_eth_sink_wait_pt_add_nd_proxy_burst(int timeout_ms)
{
	for (int elapsed = 0; elapsed < timeout_ms; elapsed += 50) {
		if (dect_test_mock_eth_ns_tx_count() >= 1 &&
		    dect_test_mock_eth_na_tx_count() >= 2 &&
		    dect_test_mock_eth_tx_total() >= 3) {
			return true;
		}
		k_sleep(K_MSEC(50));
	}

	return dect_test_mock_eth_ns_tx_count() >= 1 &&
	       dect_test_mock_eth_na_tx_count() >= 2 &&
	       dect_test_mock_eth_tx_total() >= 3;
}

#endif /* CONFIG_NET_L2_ETHERNET && !CONFIG_MODEM_CELLULAR */
