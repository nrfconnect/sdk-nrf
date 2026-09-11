/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

#include "dect_tether_ipv6_int.h"

LOG_MODULE_REGISTER(dect_tether_ipv6_host, CONFIG_DECT_TETHER_IPV6_LOG_LEVEL);

struct net_if *dect_tether_ipv6_host_eth_iface(void)
{
#if defined(CONFIG_NET_L2_ETHERNET)
#if defined(CONFIG_NET_INTERFACE_NAME)
	const char *name = CONFIG_DECT_TETHER_IPV6_HOST_ETH_IFACE;

	if (name[0] != '\0') {
		int idx = net_if_get_by_name(name);

		if (idx >= 0) {
			return net_if_get_by_index(idx);
		}
		LOG_WRN("host Ethernet iface %s not found", name);
	}
#endif
	return net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
#else
	return NULL;
#endif
}

bool dect_tether_ipv6_host_eth_ready(struct net_if *eth)
{
	const struct net_in6_addr *ll;

	if (eth == NULL || !net_if_is_up(eth)) {
		return false;
	}
	ll = net_if_ipv6_get_ll(eth, NET_ADDR_PREFERRED);
	return ll != NULL;
}
