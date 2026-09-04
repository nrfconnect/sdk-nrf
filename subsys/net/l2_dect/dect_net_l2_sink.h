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

#endif /* DECT_NET_L2_SINK_H */
