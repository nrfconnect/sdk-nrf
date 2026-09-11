/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/byteorder.h>

#include "dect_tether_ipv6_int.h"

#if defined(CONFIG_NET_L2_DECT_IPV6_IFACE_UNICAST_SKIP)
#include <net/dect/dect_net_l2.h>
#endif

#if !defined(CONFIG_NET_L2_DECT_IPV6_IFACE_UNICAST_SKIP)
struct addr_pick_ctx {
	struct net_in6_addr *out;
	bool found;
	enum { PICK_ULA, PICK_GLOBAL } which;
};

static void addr_pick_cb(struct net_if *iface, struct net_if_addr *ifa, void *user_data)
{
	struct addr_pick_ctx *c = user_data;
	struct net_in6_addr *a;

	ARG_UNUSED(iface);
	if (c->found || !ifa->is_used || ifa->address.family != AF_INET6) {
		return;
	}
	if (ifa->addr_state != NET_ADDR_PREFERRED) {
		return;
	}
	a = &ifa->address.in6_addr;
	if (c->which == PICK_ULA) {
		if (net_ipv6_is_ula_addr(a)) {
			net_ipv6_addr_copy_raw(c->out->s6_addr, a->s6_addr);
			c->found = true;
		}
	} else {
		if (net_ipv6_is_global_addr(a)) {
			net_ipv6_addr_copy_raw(c->out->s6_addr, a->s6_addr);
			c->found = true;
		}
	}
}
#endif

#define DUID_TYPE_LLT 1
#define DUID_TYPE_LL  3
#define HW_TYPE_ETH   1

bool dect_tether_ipv6_duid_eth_mac_get(const uint8_t *duid, uint16_t dlen, uint8_t mac[6])
{
	if (duid == NULL || mac == NULL) {
		return false;
	}
	/* DUID-LL: type 3, 16-bit hardware type, link-layer address */
	if (dlen >= 10U && sys_get_be16(duid) == DUID_TYPE_LL &&
	    sys_get_be16(duid + 2) == HW_TYPE_ETH) {
		memcpy(mac, duid + 4, 6U);
		return true;
	}
	/* DUID-LLT: type 1, 16-bit hardware type, 32-bit time, link-layer address */
	if (dlen >= 14U && sys_get_be16(duid) == DUID_TYPE_LLT &&
	    sys_get_be16(duid + 2) == HW_TYPE_ETH) {
		memcpy(mac, duid + 8, 6U);
		return true;
	}
	return false;
}

bool dect_tether_ipv6_dect_addrs_get(struct net_if *dect_iface, struct net_in6_addr *ula_out,
					 struct net_in6_addr *gua_out, bool *have_ula,
					 bool *have_gua)
{
#if !IS_ENABLED(CONFIG_NET_L2_DECT_IPV6_IFACE_UNICAST_SKIP)
	struct addr_pick_ctx ula_ctx = {
		.out = ula_out,
		.found = false,
		.which = PICK_ULA,
	};
	struct addr_pick_ctx gua_ctx = {
		.out = gua_out,
		.found = false,
		.which = PICK_GLOBAL,
	};
#endif

	if (dect_iface == NULL || ula_out == NULL || gua_out == NULL || have_ula == NULL ||
	    have_gua == NULL) {
		return false;
	}

#if defined(CONFIG_NET_L2_DECT_IPV6_IFACE_UNICAST_SKIP)
	dect_net_l2_ipv6_off_iface_unicast_get(dect_iface, ula_out, gua_out, have_ula, have_gua);
	return *have_ula || *have_gua;
#else
	net_if_ipv6_addr_foreach(dect_iface, addr_pick_cb, &ula_ctx);
	net_if_ipv6_addr_foreach(dect_iface, addr_pick_cb, &gua_ctx);

	*have_ula = ula_ctx.found;
	*have_gua = gua_ctx.found;

	return ula_ctx.found || gua_ctx.found;
#endif
}
