/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_MDM_SINK_H
#define DECT_MDM_SINK_H

#include <zephyr/net/net_ip.h>

struct dect_mdm_ipv6_prefix {

	/** IPv6 prefix */
	struct in6_addr prefix;

	/** Prefix length in bytes */
	uint8_t len;
};

#if defined(CONFIG_NET_L2_DECT_BR)
bool dect_mdm_sink_ipv6_prefix_get(struct dect_mdm_ipv6_prefix *prefix_out);
#else
static inline bool dect_mdm_sink_ipv6_prefix_get(struct dect_mdm_ipv6_prefix *prefix_out)
{
	return false;
}
#endif /* CONFIG_NET_L2_DECT_BR */

#endif /* DECT_MDM_SINK_H */
