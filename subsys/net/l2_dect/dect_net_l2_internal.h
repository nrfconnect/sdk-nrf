/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_NET_L2_INTERNAL_H_
#define DECT_NET_L2_INTERNAL_H_
#ifdef __cplusplus
extern "C" {
#endif

struct dect_net_l2_association_data {
	bool in_use;
	uint32_t target_long_rd_id;

	bool local_ipv6_addr_set;
	struct in6_addr local_ipv6_addr;
	bool global_ipv6_addr_set;
	struct in6_addr global_ipv6_addr;
};

void dect_net_l2_status_info_fill_association_data(struct net_if *iface,
						   struct dect_status_info *status_info_out);
void dect_net_l2_status_info_fill_sink_data(struct net_if *iface,
					    struct dect_status_info *status_info_out);

void dect_net_l2_sink_ipv6_config_changed(struct net_if *iface,
					  struct dect_net_ipv6_prefix_config *ipv6_prefix_config);
#ifdef __cplusplus
}
#endif

#endif /* DECT_NET_L2_INTERNAL_H_ */
