/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>

#include <net/dect/dect_net_l2.h>
#include <net/dect/dect_net_l2_mgmt.h>

#include "dect_tether_ipv6_int.h"

LOG_MODULE_REGISTER(dect_tether_ipv6_uplink, CONFIG_DECT_TETHER_IPV6_LOG_LEVEL);

#if defined(CONFIG_NET_L2_DECT_MGMT) && \
	(defined(CONFIG_DECT_TETHER_IPV6_RA) || defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_SERVER))

static struct net_mgmt_event_callback uplink_mgmt_cb;
static bool uplink_inited;

static struct net_if *iface_dect(void)
{
	return net_if_get_first_by_type(&NET_L2_GET_NAME(DECT));
}

static void uplink_parent_created(struct net_if *dect)
{
	dect_tether_ipv6_fwd_parent_assoc_created(dect);
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_SERVER)
	dect_tether_ipv6_dhcpv6_srv_uplink_restored();
#endif
	dect_tether_ipv6_ra_set_uplink(true);
#if defined(CONFIG_DECT_TETHER_IPV6_RA)
	dect_tether_ipv6_ra_kick();
#endif
}

static void uplink_parent_released(void)
{
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_SERVER)
	dect_tether_ipv6_dhcpv6_srv_uplink_lost();
#endif
	dect_tether_ipv6_ra_set_uplink(false);
	dect_tether_ipv6_fwd_parent_assoc_released();
#if defined(CONFIG_DECT_TETHER_IPV6_RA)
	dect_tether_ipv6_ra_kick();
#endif
}

static void uplink_mgmt_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				struct net_if *iface)
{
	struct net_if *dect = iface_dect();

	if (mgmt_event != NET_EVENT_DECT_ASSOCIATION_CHANGED || iface != dect || iface == NULL) {
		return;
	}

#if !IS_ENABLED(CONFIG_NET_MGMT_EVENT_INFO)
	ARG_UNUSED(cb);
	return;
#else
	{
		const struct dect_association_changed_evt *evt;

		if (cb->info == NULL ||
		    cb->info_length < sizeof(struct dect_association_changed_evt)) {
			return;
		}
		evt = (const struct dect_association_changed_evt *)cb->info;

		if (evt->neighbor_role != DECT_NEIGHBOR_ROLE_PARENT) {
			return;
		}

		if (evt->association_change_type == DECT_ASSOCIATION_CREATED) {
			uplink_parent_created(dect);
		} else if (evt->association_change_type == DECT_ASSOCIATION_RELEASED) {
			uplink_parent_released();
		}
	}
#endif
}

int dect_tether_ipv6_uplink_init(void)
{
	if (uplink_inited) {
		return 0;
	}

	net_mgmt_init_event_callback(&uplink_mgmt_cb, uplink_mgmt_handler,
				    NET_EVENT_DECT_ASSOCIATION_CHANGED);
	net_mgmt_add_event_callback(&uplink_mgmt_cb);
	uplink_inited = true;
	LOG_INF("uplink: DECT parent association -> RA/DHCP/routing hooks");
	return 0;
}

void dect_tether_ipv6_uplink_deinit(void)
{
	if (!uplink_inited) {
		return;
	}

	net_mgmt_del_event_callback(&uplink_mgmt_cb);
	uplink_inited = false;
}

#else /* no uplink subscribers or no DECT mgmt */

int dect_tether_ipv6_uplink_init(void)
{
	return 0;
}

void dect_tether_ipv6_uplink_deinit(void)
{
}

#endif
