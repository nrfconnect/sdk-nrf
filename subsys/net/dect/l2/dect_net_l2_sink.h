/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_NET_L2_SINK_H
#define DECT_NET_L2_SINK_H

#include <zephyr/net/net_ip.h>

struct net_if;

/* Define for the prefix length that we use from BR iface global prefix */
#define DECT_NET_L2_SINK_IPV6_PREFIX_LEN_BYTES 8

struct dect_net_l2_sink_ipv6_prefix {

	/** IPv6 prefix */
	struct in6_addr prefix;

	/** Backpointer to network interface where this prefix is used */
	struct net_if *iface;

	/** Prefix length in bytes */
	uint8_t len;
};

#if defined(CONFIG_NET_L2_DECT_BR)
bool dect_net_l2_sink_ipv6_prefix_get(struct dect_net_l2_sink_ipv6_prefix *prefix_out);

/** Rebuild learned sink prefix from uplink /64 +
 *  current L2 transmitter_long_rd_id (e.g. after settings).
 */
void dect_net_l2_sink_reapply_prefix_for_tx_rd(struct net_if *dect_iface);
#else
static inline bool dect_net_l2_sink_ipv6_prefix_get(struct dect_net_l2_sink_ipv6_prefix *prefix_out)
{
	return false;
}

static inline void dect_net_l2_sink_reapply_prefix_for_tx_rd(struct net_if *dect_iface)
{
	(void)dect_iface;
}
#endif /* CONFIG_NET_L2_DECT_BR */

#if defined(CONFIG_NET_L2_DECT_BR_UNSOLICITED_NA) && \
	defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT)
/** RFC 4861 7.2.6 multicast unsolicited NA on Ethernet for PT GUA (ND proxy). */
void dect_net_l2_sink_eth_unsol_na_pt_nd_proxy(const struct in6_addr *tgt);
#endif

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NS_PRIME)
/** RFC 4861 7.2.3 NS from PT GUA toward default router (ND cache prime). @p ctx tags logs. */
void dect_net_l2_sink_eth_pt_nd_proxy_ns_prime(const struct in6_addr *pt_global,
					    const char *ctx);
#endif

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NA_UNICAST_REFRESH)
/** RFC 4861 7.2.4 unicast NA to default router for PT GUA. @p ctx tags logs. */
void dect_net_l2_sink_eth_pt_nd_proxy_na_unicast(const struct in6_addr *pt_global,
					      const char *ctx);
#endif

#endif /* DECT_NET_L2_SINK_H */
