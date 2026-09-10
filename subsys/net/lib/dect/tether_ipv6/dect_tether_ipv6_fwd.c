/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

#include <net/dect/dect_net_l2.h>

LOG_MODULE_REGISTER(dect_tether_ipv6_fwd, CONFIG_DECT_TETHER_IPV6_LOG_LEVEL);

#include "dect_tether_ipv6_int.h"

#include "ipv6.h"
#include "nbr.h"
#include "net_private.h"
#include "route_ipv6.h"

static struct net_route_entry *rt_ula;
static struct net_route_entry *rt_gua;
static struct net_if_router *dect_def_router;
static struct k_mutex tether_mu;
static struct net_in6_addr tether_nbr_ll;
static struct net_in6_addr tether_nbr_ula;
static struct net_in6_addr tether_nbr_gua;
static bool tether_nbr_have_ula;
static bool tether_nbr_have_gua;
static bool tether_nbr_valid;

static struct net_if *iface_dect(void)
{
	return net_if_get_first_by_type(&NET_L2_GET_NAME(DECT));
}

static void tether_routes_remove_locked(void)
{
	if (rt_ula != NULL) {
		(void)net_route_ipv6_del(rt_ula);
		rt_ula = NULL;
	}
	if (rt_gua != NULL) {
		(void)net_route_ipv6_del(rt_gua);
		rt_gua = NULL;
	}
}

static void tether_neighbors_remove_locked(struct net_if *eth)
{
	if (!tether_nbr_valid || eth == NULL) {
		return;
	}

	(void)net_ipv6_nbr_rm(eth, &tether_nbr_ll);
	if (tether_nbr_have_ula) {
		(void)net_ipv6_nbr_rm(eth, &tether_nbr_ula);
	}
	if (tether_nbr_have_gua) {
		(void)net_ipv6_nbr_rm(eth, &tether_nbr_gua);
	}
	tether_nbr_valid = false;
}

static void tether_teardown_locked(struct net_if *eth)
{
	tether_routes_remove_locked();
	tether_neighbors_remove_locked(eth);
}

static void dect_parent_router_clear(void)
{
	if (dect_def_router != NULL) {
		(void)net_if_ipv6_router_rm(dect_def_router);
		dect_def_router = NULL;
	}
}

static void dect_parent_router_refresh(struct net_if *dect)
{
	struct net_in6_addr parent;

	dect_parent_router_clear();

	if (dect == NULL) {
		return;
	}
	if (!dect_net_l2_parent_ipv6_addr_get(&parent)) {
		return;
	}

	dect_def_router = net_if_ipv6_router_add(dect, &parent, true, 65535U);
	if (dect_def_router == NULL) {
		LOG_WRN("fwd: net_if_ipv6_router_add failed for %s",
			net_sprint_ipv6_addr((const struct in6_addr *)&parent));
	} else {
		LOG_INF("fwd: default IPv6 router on DECT via parent %s",
			net_sprint_ipv6_addr((const struct in6_addr *)&parent));
	}
}

static bool eth_peer_ll_lookup(struct net_if *eth, const struct sockaddr_in6 *cli,
			       const uint8_t *duid, uint16_t dlen, struct net_linkaddr *lla)
{
	struct net_nbr *nbr;
	const struct net_linkaddr *ll;

	if (eth == NULL || cli == NULL || lla == NULL) {
		return false;
	}

	if (net_ipv6_is_ll_addr((struct net_in6_addr *)&cli->sin6_addr)) {
		nbr = net_ipv6_nbr_lookup(eth, (struct net_in6_addr *)&cli->sin6_addr);
		if (nbr != NULL && nbr->idx != NET_NBR_LLADDR_UNKNOWN) {
			ll = net_nbr_get_lladdr(nbr->idx);
			if (ll != NULL && ll->len == 6U) {
				lla->len = 6U;
				lla->type = NET_LINK_ETHERNET;
				memcpy(lla->addr, ll->addr, 6U);
				return true;
			}
		}
	}

	if (dect_tether_ipv6_duid_eth_mac_get(duid, dlen, lla->addr)) {
		lla->len = 6U;
		lla->type = NET_LINK_ETHERNET;
		return true;
	}
	return false;
}

static int tether_one_addr(struct net_if *eth, const struct net_in6_addr *addr,
			   const struct net_linkaddr *lla)
{
	struct net_nbr *nbr;

	if (addr == NULL || lla == NULL) {
		return -EINVAL;
	}

	/* STATIC avoids REACHABLE->STALE->PROBE->delete decay after ~30 s idle.
	 * Deleting the neighbor also removes its /128 route (Zephyr deletes
	 * routes by nexthop), dropping inbound traffic to the DECT default
	 * route until the next DHCPv6 Renew. Remove first: net_ipv6_nbr_add()
	 * keeps existing state on update, so a fresh STATIC entry needs an
	 * explicit rm beforehand.
	 */
	(void)net_ipv6_nbr_rm(eth, (struct net_in6_addr *)addr);
	nbr = net_ipv6_nbr_add(eth, addr, lla, false, NET_IPV6_NBR_STATE_STATIC);
	if (nbr == NULL) {
		LOG_WRN("fwd: net_ipv6_nbr_add(STATIC) failed for %s",
			net_sprint_ipv6_addr((const struct in6_addr *)addr));
		return -EIO;
	}

	return 0;
}

static int tether_route_install(struct net_if *eth, const struct net_in6_addr *addr,
				struct net_route_entry **slot)
{
	struct net_route_entry *re;

	if (eth == NULL || addr == NULL || slot == NULL) {
		return -EINVAL;
	}

	if (*slot != NULL) {
		(void)net_route_ipv6_del(*slot);
		*slot = NULL;
	}

	re = net_route_ipv6_add(eth, (struct net_in6_addr *)addr, 128,
				(struct net_in6_addr *)addr, NET_IPV6_ND_INFINITE_LIFETIME,
				NET_ROUTE_PREFERENCE_HIGH);
	if (re == NULL) {
		LOG_WRN("fwd: net_route_ipv6_add /128 failed for %s",
			net_sprint_ipv6_addr((const struct in6_addr *)addr));
		return -EIO;
	}

	*slot = re;
	LOG_INF("fwd: /128 route on Ethernet for %s",
		net_sprint_ipv6_addr((const struct in6_addr *)addr));
	return 0;
}

void dect_tether_ipv6_fwd_tether_update(struct net_if *eth,
					     const struct sockaddr_in6 *cli,
					     const uint8_t *client_duid, uint16_t client_duid_len,
					     const struct net_in6_addr *ula,
					     const struct net_in6_addr *gua, bool have_ula,
					     bool have_gua)
{
	struct net_linkaddr lla = { 0 };

	if (eth == NULL || cli == NULL || (!have_ula && !have_gua)) {
		return;
	}

	k_mutex_lock(&tether_mu, K_FOREVER);
	tether_routes_remove_locked();

	if (!eth_peer_ll_lookup(eth, cli, client_duid, client_duid_len, &lla)) {
		LOG_WRN("fwd: no Ethernet link-layer for tether (DUID-LL/LLT or neighbor on fe80)");
		k_mutex_unlock(&tether_mu);
		return;
	}

	/* Pin the PC's link-local neighbor STATIC: it is only ever used as the
	 * DHCPv6 unicast destination, so no data traffic keeps it reachable and
	 * it would otherwise decay REACHABLE->STALE->PROBE->FAIL between Renew
	 * cycles (T1, default 3600 s). STATIC is exempt from NUD state changes
	 * (ipv6_nbr_set_state() no-ops once STATIC), so the entry persists until
	 * reboot.
	 *
	 * Remove first: net_ipv6_nbr_add() keeps an existing entry's state
	 * unchanged, so a fresh STATIC entry needs an explicit rm beforehand.
	 * The PC's LL address is never a route nexthop, so the rm's internal
	 * route cleanup is a safe no-op here.
	 */
	(void)net_ipv6_nbr_rm(eth, (struct net_in6_addr *)&cli->sin6_addr);
	{
		struct net_nbr *ll_nbr;

		ll_nbr = net_ipv6_nbr_add(eth,
					  (const struct net_in6_addr *)&cli->sin6_addr,
					  &lla, false,
					  NET_IPV6_NBR_STATE_STATIC);
		if (ll_nbr == NULL) {
			LOG_WRN("fwd: net_ipv6_nbr_add(STATIC) failed for PC LL %s",
				net_sprint_ipv6_addr(&cli->sin6_addr));
		}
	}

	if (have_ula) {
		(void)tether_one_addr(eth, ula, &lla);
		(void)tether_route_install(eth, ula, &rt_ula);
	}
	if (have_gua) {
		(void)tether_one_addr(eth, gua, &lla);
		(void)tether_route_install(eth, gua, &rt_gua);
	}

	net_ipv6_addr_copy_raw(tether_nbr_ll.s6_addr,
			       ((const struct net_in6_addr *)&cli->sin6_addr)->s6_addr);
	tether_nbr_have_ula = have_ula;
	tether_nbr_have_gua = have_gua;
	if (have_ula) {
		tether_nbr_ula = *ula;
	}
	if (have_gua) {
		tether_nbr_gua = *gua;
	}
	tether_nbr_valid = true;

	k_mutex_unlock(&tether_mu);
}

void dect_tether_ipv6_fwd_parent_assoc_created(struct net_if *dect)
{
	dect_parent_router_refresh(dect);
}

void dect_tether_ipv6_fwd_parent_assoc_released(void)
{
	dect_parent_router_clear();
	k_mutex_lock(&tether_mu, K_FOREVER);
	tether_teardown_locked(dect_tether_ipv6_host_eth_iface());
	k_mutex_unlock(&tether_mu);
}

int dect_tether_ipv6_fwd_init(void)
{
	struct net_if *dect;

	k_mutex_init(&tether_mu);
	dect = iface_dect();
	dect_parent_router_refresh(dect);

	LOG_INF("fwd: DECT parent default router + DHCP-driven /128 + nbr on Ethernet");
	return 0;
}

void dect_tether_ipv6_fwd_deinit(void)
{
	dect_parent_router_clear();
	k_mutex_lock(&tether_mu, K_FOREVER);
	tether_teardown_locked(dect_tether_ipv6_host_eth_iface());
	k_mutex_unlock(&tether_mu);
}
