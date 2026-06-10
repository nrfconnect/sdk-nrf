/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file dect_net_l2_sink.c
 * @brief DECT NR+ L2 Sink Layer
 *
 * Implements DECT NR+-aware sink layer that integrates with Zephyr networking stack.
 * Provides global IPv6 prefix for DECT NR+ network.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <net/dect/dect_net_l2.h>
#include <net/dect/dect_net_l2_mgmt.h>

#if defined(CONFIG_MODEM_CELLULAR)
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#endif

#include "route_ipv6.h"
#include "ipv6.h"

#if defined(CONFIG_NET_L2_DECT_BR_UNSOLICITED_NA)
#include "icmpv6.h" /* NET_ICMPV6_NA_FLAG_OVERRIDE */
#endif

#if defined(CONFIG_NET_DHCPV6)
#include <zephyr/net/dhcpv6.h>
#endif

#include <net/dect/dect_utils.h>

#include "dect_net_l2_internal.h"
#include "dect_net_l2_ipv6.h"
#include "dect_net_l2_sink.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_l2_dect_br, CONFIG_NET_L2_DECT_BR_LOG_LEVEL);

#include "net_private.h"

static struct net_if *iface_for_prefix;
static struct net_if *iface_for_dect;

struct in6_addr sink_prefix_addr;
static atomic_t sink_prefix_addr_set;
/** Bytes of on-DECT prefix: 8 (/64) or 12 (/96 with transmitter long RD after delegated /64). */
static uint8_t sink_dect_prefix_len_bytes;

static struct in6_addr ipv6_router_addr;

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_UPSTREAM_PREFIX_ROUTE)
/* Static upstream /N route on Ethernet (RA prefix, nexthop = default router LL). */
static struct net_route_entry *eth_upstream_prefix_route;
#endif

/**
 * After learning delegated /64 from uplink, set DECT netiface prefix to that /64 plus
 * this device's DECT transmitter long RD ID in the next 32 bits (/96), when TX RD != 0.
 */
static void sink_apply_learned_delegated_prefix(const struct in6_addr *delegated_ra64)
{
	struct dect_net_l2_context *ctx;
	uint32_t tx_rd;

	if (iface_for_dect == NULL || delegated_ra64 == NULL) {
		return;
	}

	ctx = net_if_l2_data(iface_for_dect);
	tx_rd = ctx->transmitter_long_rd_id;

	memcpy(sink_prefix_addr.s6_addr, delegated_ra64->s6_addr,
	       DECT_NET_L2_SINK_IPV6_PREFIX_LEN_BYTES);
	if (IS_ENABLED(CONFIG_NET_L2_DECT_BR_IPV6_SINK_PREFIX96) &&
	    tx_rd != DECT_NET_L2_LONG_RD_ID_NOT_SET) {
		sink_prefix_addr.s6_addr32[2] = htonl(tx_rd);
		sink_prefix_addr.s6_addr32[3] = 0;
		sink_dect_prefix_len_bytes = DECT_NET_L2_SINK_IPV6_PREFIX_LEN_BYTES + 4;
	} else {
		memset(&sink_prefix_addr.s6_addr[8], 0, 8);
		sink_dect_prefix_len_bytes = DECT_NET_L2_SINK_IPV6_PREFIX_LEN_BYTES;
	}
	atomic_set(&sink_prefix_addr_set, 1);
}

void dect_net_l2_sink_reapply_prefix_for_tx_rd(struct net_if *dect_iface)
{
	struct dect_net_ipv6_prefix_config new_prefix;
	struct in6_addr delegated_ra64;

	if (dect_iface == NULL || dect_iface != iface_for_dect ||
	    !atomic_get(&sink_prefix_addr_set)) {
		return;
	}

	memcpy(delegated_ra64.s6_addr, sink_prefix_addr.s6_addr, 8);
	sink_apply_learned_delegated_prefix(&delegated_ra64);
	new_prefix.prefix = sink_prefix_addr;
	new_prefix.prefix_len = sink_dect_prefix_len_bytes;
	dect_net_l2_sink_ipv6_config_changed(dect_iface, &new_prefix);
}

static void sink_clear_prefix(void)
{
	atomic_set(&sink_prefix_addr_set, 0);
	sink_dect_prefix_len_bytes = 0;
	memset(&sink_prefix_addr, 0, sizeof(sink_prefix_addr));
}

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_UPSTREAM_PREFIX_ROUTE)
/* Install upstream /N route: RA prefix via default router link-local nexthop. */
static void sink_install_eth_upstream_prefix_route(void)
{
	struct net_if_router *router;
	struct in6_addr upstream_prefix;
	struct in6_addr router_addr;

	if (iface_for_prefix == NULL || !atomic_get(&sink_prefix_addr_set)) {
		return;
	}

	/* Default router LL from live router list (handles late iface_for_prefix init). */
	router = net_if_ipv6_router_find_default(iface_for_prefix, NULL);
	if (router == NULL) {
		LOG_WRN("SINK: no default router found for iface %p", iface_for_prefix);
		return;
	}
	net_ipv6_addr_copy_raw(router_addr.s6_addr, router->address.in6_addr.s6_addr);

	memset(&upstream_prefix, 0, sizeof(upstream_prefix));
	memcpy(upstream_prefix.s6_addr, sink_prefix_addr.s6_addr,
	       DECT_NET_L2_SINK_IPV6_PREFIX_LEN_BYTES);

	if (eth_upstream_prefix_route != NULL) {
		(void)net_route_ipv6_del(eth_upstream_prefix_route);
		eth_upstream_prefix_route = NULL;
	}

	eth_upstream_prefix_route = net_route_ipv6_add(iface_for_prefix,
		&upstream_prefix,
		DECT_NET_L2_SINK_IPV6_PREFIX_LEN_BYTES * 8U,
		&router_addr,
		NET_IPV6_ND_INFINITE_LIFETIME,
		NET_ROUTE_PREFERENCE_HIGH);
	if (eth_upstream_prefix_route == NULL) {
		LOG_ERR("SINK: failed to install upstream /%u route via router LL",
			(unsigned int)DECT_NET_L2_SINK_IPV6_PREFIX_LEN_BYTES * 8U);
	} else {
		LOG_INF("SINK: upstream /%u route installed via router LL %s",
			(unsigned int)DECT_NET_L2_SINK_IPV6_PREFIX_LEN_BYTES * 8U,
			net_sprint_ipv6_addr(&router_addr));
	}
}

static void sink_remove_eth_upstream_prefix_route(void)
{
	if (eth_upstream_prefix_route != NULL) {
		(void)net_route_ipv6_del(eth_upstream_prefix_route);
		eth_upstream_prefix_route = NULL;
		LOG_INF("SINK: upstream prefix route removed");
	}
}

#endif /* CONFIG_NET_L2_DECT_BR_IPV6_ETH_UPSTREAM_PREFIX_ROUTE */

#if defined(CONFIG_MODEM_CELLULAR)
const struct device *modem = DEVICE_DT_GET(DT_ALIAS(modem));
#endif

#if defined(CONFIG_MODEM_CELLULAR) || defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_UPSTREAM_PREFIX_ROUTE)
/* NUD can drop the router's neighbor cache entry (e.g. no reply while in
 * PROBE state) even though the router itself is still valid in the router
 * list. net_ipv6_nbr_rm() then silently deletes any route using it as
 * nexthop (see net_route_ipv6_del_by_nexthop() in ipv6_nbr.c), taking our
 * router-dependent route(s) down with it. This work re-adds what NUD
 * removed, shared by both the LTE and the Ethernet sink uplink.
 */
static struct k_work_delayable sink_router_nbr_deleted_work;

#endif
static struct k_work_delayable dect_sink_rs_work;
/* The sink solicits the upstream router (RS) to learn the delegated prefix.
 * A whole RS burst can be lost - e.g. the SPI-attached W5500 throws
 * TX-semaphore timeouts for the first seconds after link up, so the RS never
 * leaves the driver, no RA arrives, and no prefix is learned. Zephyr's own
 * RS retransmit (RS_COUNT bursts, one per CONFIG_NET_IPV6_RS_TIMEOUT) all
 * fire within a few seconds of a single net_if_start_rs() call, i.e. still
 * inside that failing window. Retry net_if_start_rs() at a coarser interval
 * until a prefix is set or the attempt budget is exhausted.
 */
#define SINK_RS_RETRY_INTERVAL K_SECONDS(5)
#define SINK_RS_MAX_ATTEMPTS   24
static uint8_t sink_rs_attempts;
#if defined(CONFIG_NET_DHCPV6)
/* Delay DHCPv6 start so IPv6 link-local DAD completes before the first Solicit. */
#define SINK_DHCPV6_START_DELAY_MS 5000
static struct k_work_delayable sink_dhcpv6_start_work;
#endif
#if defined(CONFIG_NET_L2_DECT_BR_UNSOLICITED_NA)
/** After ADDR_ADD, wait so IPv6 addr state and related events can propagate before NA. */
#define SINK_ETH_UNSOL_NA_ADDR_ADD_DELAY_MS 100
static struct k_work_delayable sink_eth_unsol_na_work;
#endif
static struct net_mgmt_event_callback dect_net_l2_net_mgmt_ipv6_event_cb;

static void dect_net_l2_net_mgmt_ipv6_event_handler(struct net_mgmt_event_callback *cb,
						   uint64_t mgmt_event, struct net_if *iface)
{
	char ipv6_addr_str[NET_IPV6_ADDR_LEN];

	if (iface && iface != iface_for_prefix) {
		/* With NULL interface we should continue */
		return;
	}

	switch (mgmt_event) {
	case NET_EVENT_IPV6_PREFIX_ADD: {
		struct net_event_ipv6_prefix *ipv6_prefix =
			(struct net_event_ipv6_prefix *)cb->info;

		LOG_DBG("NET_EVENT_IPV6_PREFIX_ADD: iface %p, prefix %s/%d", iface,
			net_addr_ntop(AF_INET6, (struct in6_addr *)&ipv6_prefix->addr,
				      ipv6_addr_str, NET_IPV6_ADDR_LEN),
			ipv6_prefix->len);
		break;
	}
	case NET_EVENT_IPV6_PREFIX_DEL: {
		struct net_event_ipv6_prefix *ipv6_prefix =
			(struct net_event_ipv6_prefix *)cb->info;

		LOG_DBG("NET_EVENT_IPV6_PREFIX_DEL: iface %p, prefix %s/%d", iface,
			net_addr_ntop(AF_INET6, (struct in6_addr *)&ipv6_prefix->addr,
				      ipv6_addr_str, NET_IPV6_ADDR_LEN),
			ipv6_prefix->len);
		break;
	}
	case NET_EVENT_IPV6_ROUTER_DEL: {
		struct in6_addr *router_addr = (struct in6_addr *)cb->info;

		LOG_DBG("NET_EVENT_IPV6_ROUTER_DEL: iface %p, router %s", iface,
			net_addr_ntop(AF_INET6, router_addr, ipv6_addr_str, NET_IPV6_ADDR_LEN));
		if (net_if_is_up(iface_for_prefix) &&
		    net_ipv6_addr_cmp(&ipv6_router_addr, router_addr)) {
			struct dect_sink_status_evt sink_status_data = {
				.sink_status = DECT_SINK_STATUS_DISCONNECTED,
				.br_iface = iface_for_prefix,
			};
			struct dect_net_ipv6_prefix_config empty_prefix;

			memset(&empty_prefix, 0, sizeof(empty_prefix));
			LOG_INF("SINK: Router with IPv6 addr %s deleted from sink iface %p",
				net_addr_ntop(AF_INET6, router_addr, ipv6_addr_str,
					      NET_IPV6_ADDR_LEN),
				iface_for_prefix);

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_UPSTREAM_PREFIX_ROUTE)
			sink_remove_eth_upstream_prefix_route();
#endif
			sink_clear_prefix();
			dect_net_l2_sink_ipv6_config_changed(
				iface_for_dect,
				&empty_prefix);
			dect_mgmt_sink_status_evt(iface_for_dect, sink_status_data);

			LOG_INF("SINK: starting router solicitation for iface %p",
				iface_for_prefix);
			/* Reset the retry budget and let the RS work drive the
			 * (re)solicitation so a dropped RS here is also retried.
			 */
			sink_rs_attempts = 0;
			(void)k_work_reschedule(&dect_sink_rs_work, K_NO_WAIT);
		}
		break;
	}
	case NET_EVENT_IPV6_ROUTER_ADD: {
		struct in6_addr *router_addr = (struct in6_addr *)cb->info;
		bool prefix_found = false;
		struct in6_addr delegated_ra64 = {};

		LOG_DBG("NET_EVENT_IPV6_ROUTER_ADD: iface %p, router %s", iface,
			net_addr_ntop(AF_INET6, router_addr, ipv6_addr_str, NET_IPV6_ADDR_LEN));

		ipv6_router_addr = *router_addr;

		/* Check that one of the iface public ipv6 addresses is having still
		 * the same prefix
		 */
		struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;

		ARRAY_FOR_EACH(ipv6->unicast, i)
		{
			struct in6_addr *ipv6_addr = &ipv6->unicast[i].address.in6_addr;

			if (!ipv6->unicast[i].is_used ||
			    ipv6->unicast[i].address.family != AF_INET6) {
				continue;
			}

			if (!net_ipv6_is_global_addr(ipv6_addr)) {
				continue;
			}

			if (atomic_get(&sink_prefix_addr_set)) {
				if (net_ipv6_is_prefix(
					ipv6_addr->s6_addr,
					sink_prefix_addr.s6_addr,
					DECT_NET_L2_SINK_IPV6_PREFIX_LEN_BYTES * 8U)) {
					prefix_found = true;
					break;
				}
			} else {
				memcpy(delegated_ra64.s6_addr, ipv6_addr->s6_addr,
				       DECT_NET_L2_SINK_IPV6_PREFIX_LEN_BYTES);
				prefix_found = true;
				break;
			}
		}
		if (prefix_found == false) {
			LOG_WRN("SINK: Router with IPv6 addr %s added to sink iface %p, but no "
				"existing public address with our prefix found - "
				"sink prefix needs to be re-created and we wait for "
				"NET_EVENT_IPV6_ADDR_ADD",
				net_addr_ntop(AF_INET6, router_addr, ipv6_addr_str,
					      NET_IPV6_ADDR_LEN),
				iface_for_prefix);
		} else {
			if (!atomic_get(&sink_prefix_addr_set)) {
				struct dect_sink_status_evt sink_status_data = {
					.sink_status = DECT_SINK_STATUS_CONNECTED,
					.br_iface = iface_for_prefix,
				};
				struct dect_net_ipv6_prefix_config new_prefix;

				sink_apply_learned_delegated_prefix(&delegated_ra64);
				new_prefix.prefix = sink_prefix_addr;
				new_prefix.prefix_len = sink_dect_prefix_len_bytes;

				dect_net_l2_sink_ipv6_config_changed(
					iface_for_dect,
					&new_prefix);
				dect_mgmt_sink_status_evt(iface_for_dect, sink_status_data);
			}
#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_UPSTREAM_PREFIX_ROUTE)
			/* Install upstream /N route now that prefix is known. */
			sink_install_eth_upstream_prefix_route();
#endif
		}
		break;
	}
	case NET_EVENT_IPV6_ADDR_ADD: {
		struct net_event_ipv6_addr *evt_ipv6_addr = (struct net_event_ipv6_addr *)cb->info;
		struct in6_addr *ipv6_addr = &evt_ipv6_addr->addr;

		LOG_DBG("NET_EVENT_IPV6_ADDR_ADD: iface %p, addr %s", iface,
			net_addr_ntop(AF_INET6, ipv6_addr, ipv6_addr_str, NET_IPV6_ADDR_LEN));

		/* This is the trick: we get the 8 bytes as a prefix for
		 * dect nr+ network usage from 1st added public address.
		 */
		if (!atomic_get(&sink_prefix_addr_set) && net_ipv6_is_global_addr(ipv6_addr)) {
			struct dect_sink_status_evt sink_status_data = {
				.sink_status = DECT_SINK_STATUS_CONNECTED,
				.br_iface = iface_for_prefix,
			};
			struct dect_net_ipv6_prefix_config new_prefix;
			struct in6_addr delegated_ra64;

			memcpy(delegated_ra64.s6_addr, ipv6_addr->s6_addr,
			       DECT_NET_L2_SINK_IPV6_PREFIX_LEN_BYTES);
			sink_apply_learned_delegated_prefix(&delegated_ra64);
			new_prefix.prefix = sink_prefix_addr;
			new_prefix.prefix_len = sink_dect_prefix_len_bytes;

			LOG_DBG("SINK: IPv6 addr %s added for dect nr+ prefix usage (DECT /%u)",
				net_sprint_ipv6_addr(ipv6_addr),
				(unsigned int)new_prefix.prefix_len * 8U);

			dect_net_l2_sink_ipv6_config_changed(
				iface_for_dect,
				&new_prefix);

			dect_mgmt_sink_status_evt(iface_for_dect, sink_status_data);

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_UPSTREAM_PREFIX_ROUTE)
			/* Retry upstream /N route if ROUTER_ADD ran before iface was ready. */
			sink_install_eth_upstream_prefix_route();
#endif
		}

#if defined(CONFIG_NET_L2_DECT_BR_UNSOLICITED_NA)
		/* Schedule uplink unsolicited NA after global ADDR_ADD. */
		if (iface && net_if_is_up(iface) &&
		    net_ipv6_is_global_addr((struct net_in6_addr *)ipv6_addr)) {
			(void)k_work_reschedule(&sink_eth_unsol_na_work,
					       K_MSEC(SINK_ETH_UNSOL_NA_ADDR_ADD_DELAY_MS));
		}
#endif
		break;
	}
	case NET_EVENT_IPV6_ADDR_DEL: {
		struct net_event_ipv6_addr *evt_ipv6_addr = (struct net_event_ipv6_addr *)cb->info;
		struct in6_addr *ipv6_addr = &evt_ipv6_addr->addr;

		LOG_DBG("NET_EVENT_IPV6_ADDR_DEL: iface %p, addr %s", iface,
			net_addr_ntop(AF_INET6, ipv6_addr, ipv6_addr_str, NET_IPV6_ADDR_LEN));

		if (atomic_get(&sink_prefix_addr_set) &&
		    net_ipv6_is_prefix(ipv6_addr->s6_addr, sink_prefix_addr.s6_addr, 64)) {
			struct dect_sink_status_evt sink_status_data = {
				.sink_status = DECT_SINK_STATUS_DISCONNECTED,
				.br_iface = iface_for_prefix,
			};
			struct dect_net_ipv6_prefix_config empty_prefix;

			memset(&empty_prefix, 0, sizeof(empty_prefix));

			LOG_WRN("SINK: IPv6 addr with our prefix %s/%d removed from iface %p",
				net_sprint_ipv6_addr(ipv6_addr), sizeof(struct in6_addr) / 2,
				iface);
			sink_clear_prefix();

			dect_net_l2_sink_ipv6_config_changed(
				iface_for_dect,
				&empty_prefix);
			dect_mgmt_sink_status_evt(iface_for_dect, sink_status_data);
		}
		break;
	}
	case NET_EVENT_IPV6_NBR_ADD: {
		struct net_event_ipv6_nbr *ipv6_nbr = (struct net_event_ipv6_nbr *)cb->info;

		LOG_DBG("NET_EVENT_IPV6_NBR_ADD: iface %p, nbr %s", iface,
			net_addr_ntop(AF_INET6, (struct in6_addr *)&ipv6_nbr->addr, ipv6_addr_str,
				      NET_IPV6_ADDR_LEN));
		break;
	}

	case NET_EVENT_IPV6_NBR_DEL: {
		struct net_event_ipv6_nbr *ipv6_nbr = (struct net_event_ipv6_nbr *)cb->info;

		LOG_DBG("NET_EVENT_IPV6_NBR_DEL: iface %p, nbr %s", iface,
			net_addr_ntop(AF_INET6, (struct in6_addr *)&ipv6_nbr->addr, ipv6_addr_str,
				      NET_IPV6_ADDR_LEN));
#if defined(CONFIG_MODEM_CELLULAR) || defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_UPSTREAM_PREFIX_ROUTE)
		/* The sink's upstream router can be dropped from the nbr table by NUD
		 * (LTE and Ethernet uplinks both observed this) - let's add it back.
		 */
		if (net_if_is_up(iface_for_prefix) &&
		    net_ipv6_addr_cmp(&ipv6_router_addr, (struct in6_addr *)&ipv6_nbr->addr)) {
			LOG_INF("NET_EVENT_IPV6_NBR_DEL: Sink upstream router removed "
				"as nbr - let's add it back");

			/* Submit a work to get it back (system queue) */
			k_work_reschedule(&sink_router_nbr_deleted_work, K_MSEC(100));
		}
#endif
		break;
	}
	case NET_EVENT_IPV6_ROUTE_ADD:
		LOG_DBG("NET_EVENT_IPV6_ROUTE_ADD: iface %p", iface);
		break;
	case NET_EVENT_IPV6_ROUTE_DEL:
		LOG_DBG("NET_EVENT_IPV6_ROUTE_DEL: iface %p", iface);
		break;

#if defined(CONFIG_NET_DHCPV6)
	case NET_EVENT_IPV6_DAD_SUCCEED: {
		struct net_event_ipv6_addr *evt = (struct net_event_ipv6_addr *)cb->info;

		/* Start DHCPv6 only once the link-local address has passed DAD —
		 * that is the source address DHCPv6 needs for its exchanges.
		 */
		if (evt && net_ipv6_is_ll_addr((struct net_in6_addr *)&evt->addr)) {
			LOG_DBG("NET_EVENT_IPV6_DAD_SUCCEED: link-local DAD done on iface %p, "
				"scheduling DHCPv6 start", iface);
			k_work_reschedule(&sink_dhcpv6_start_work,
					  K_MSEC(SINK_DHCPV6_START_DELAY_MS));
		}
		break;
	}
#endif

	default:
		LOG_WRN("Unknown event %llu", mgmt_event);
		break;
	}
}

bool dect_net_l2_sink_ipv6_prefix_get(struct dect_net_l2_sink_ipv6_prefix *prefix_out)
{
	if (!atomic_get(&sink_prefix_addr_set) || iface_for_prefix == NULL) {
		return false;
	}
	prefix_out->len = sink_dect_prefix_len_bytes;
	net_ipaddr_copy(&prefix_out->prefix, &sink_prefix_addr);
	prefix_out->iface = iface_for_prefix;

	return true;
}

static struct net_mgmt_event_callback net_if_cb;

static void dect_net_l2_sink_net_if_mgmt_event_handler(struct net_mgmt_event_callback *cb,
						      uint64_t event, struct net_if *iface)
{
	if (iface != iface_for_prefix) {
		return;
	}

	switch (event) {
	case NET_EVENT_IF_UP: {
		struct dect_sink_status_evt sink_status_data = {
			.sink_status = DECT_SINK_STATUS_DISCONNECTED,
			.br_iface = iface_for_prefix,
		};

		LOG_INF("NET_EVENT_IF_UP: Sink networking iface (%p) is up", iface_for_prefix);
		dect_mgmt_sink_status_evt(iface_for_dect, sink_status_data);

		/* Schedule router solicitation work to be run in a while.
		 * We wait 20 seconds to have a chance to get RS initiated otherwise.
		 * Reset the retry budget for this fresh up cycle; the work then
		 * retries on its own until a prefix is learned (see handler).
		 */
		sink_rs_attempts = 0;
		k_work_reschedule(&dect_sink_rs_work, K_SECONDS(20));
		break;
	}
	case NET_EVENT_IF_DOWN:
		struct dect_sink_status_evt sink_status_data = {
			.sink_status = DECT_SINK_STATUS_DISCONNECTED,
			.br_iface = iface_for_prefix,
		};
		struct dect_net_ipv6_prefix_config empty_prefix;

		memset(&empty_prefix, 0, sizeof(empty_prefix));

		LOG_WRN("NET_EVENT_IF_DOWN: Sink networking iface (%p) is down", iface_for_prefix);
		dect_mgmt_sink_status_evt(iface_for_dect, sink_status_data);
		sink_clear_prefix();

		/* Stop any pending RS retries; a later IF_UP restarts the cycle. */
		(void)k_work_cancel_delayable(&dect_sink_rs_work);
		sink_rs_attempts = 0;

#if defined(CONFIG_NET_DHCPV6)
		(void)k_work_cancel_delayable(&sink_dhcpv6_start_work);
		net_dhcpv6_stop(iface);
		LOG_INF("DHCPv6 client stopped on iface %p", iface);
#endif

#if defined(CONFIG_NET_L2_DECT_BR_UNSOLICITED_NA)
		(void)k_work_cancel_delayable(&sink_eth_unsol_na_work);
#endif

		/* Update our addressing */
		dect_net_l2_sink_ipv6_config_changed(
			iface_for_dect,
			&empty_prefix);

#if defined(CONFIG_MODEM_CELLULAR)
		struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
		struct net_if_router *router;

		/* Work around: flush addresses and router as this is not done by cellular modem. */
		ARRAY_FOR_EACH(ipv6->unicast, i)
		{
			net_if_ipv6_addr_rm(iface, &ipv6->unicast[i].address.in6_addr);
		}
		net_ipv6_nbr_rm(iface, &ipv6_router_addr);
		router = net_if_ipv6_router_find_default(iface, NULL);
		if (router) {
			net_if_ipv6_router_rm(router);
		}
		memset(&ipv6_router_addr, 0, sizeof(ipv6_router_addr));
#endif
		break;
	default:
		break;
	}
}

#if defined(CONFIG_MODEM_CELLULAR) || defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_UPSTREAM_PREFIX_ROUTE)
static void sink_router_nbr_deleted_work_handler(struct k_work *work_item)
{
	ARG_UNUSED(work_item);

#if defined(CONFIG_MODEM_CELLULAR)
	struct net_route_entry *route;
	struct net_linkaddr lte_if_mac_addr;

	memset(lte_if_mac_addr.addr, 0, 6);
	lte_if_mac_addr.len = 6;
	lte_if_mac_addr.type = NET_LINK_UNKNOWN;

	/* Let's add router back */
	if (!net_ipv6_nbr_add(iface_for_prefix, &ipv6_router_addr, &lte_if_mac_addr, true,
			      NET_IPV6_NBR_STATE_REACHABLE)) {
		LOG_ERR("(%s): Cannot add IPv6 router as a nbr to sink prefix iface",
			(__func__));
	} else {
		LOG_DBG("(%s): sink iface IPv6 router added as a nbr to sink prefix iface",
			(__func__));
	}
	route = net_route_ipv6_add(iface_for_prefix, &ipv6_router_addr, 128,
				   &ipv6_router_addr, NET_IPV6_ND_INFINITE_LIFETIME,
				   NET_ROUTE_PREFERENCE_HIGH);
	if (!route) {
		LOG_ERR("Cannot add sink ipv6 router as a route");
	}
#endif /* CONFIG_MODEM_CELLULAR */

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_UPSTREAM_PREFIX_ROUTE)
	/* Unlike the LTE PPP link, this is real Ethernet: no need to re-add the
	 * nbr by hand, ND will re-resolve it on next send. The route reinstalls
	 * right away since the router list entry itself is untouched.
	 */
	LOG_INF("SINK: reinstalling upstream prefix route after router nbr removal");
	sink_install_eth_upstream_prefix_route();
#endif /* CONFIG_NET_L2_DECT_BR_IPV6_ETH_UPSTREAM_PREFIX_ROUTE */
}
#endif

#if defined(CONFIG_NET_L2_DECT_BR_UNSOLICITED_NA)
/**
 * RFC 4861 7.2.6 unsolicited NA (ff02::1) for each usable uplink unicast.
 */
static void sink_eth_send_unsolicited_na(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6;
	struct net_in6_addr allnodes;

	if (iface == NULL || !net_if_is_up(iface)) {
		return;
	}

	if (net_if_flag_is_set(iface, NET_IF_IPV6_NO_ND)) {
		return;
	}

	ipv6 = iface->config.ip.ipv6;
	if (ipv6 == NULL) {
		return;
	}

	net_ipv6_addr_create_ll_allnodes_mcast(&allnodes);

	ARRAY_FOR_EACH(ipv6->unicast, i)
	{
		struct net_if_addr *ifa = &ipv6->unicast[i];
		const struct net_in6_addr *addr = &ifa->address.in6_addr;

		if (!ifa->is_used || ifa->address.family != AF_INET6) {
			continue;
		}

		if (ifa->addr_state == NET_ADDR_TENTATIVE) {
			continue;
		}

		if (net_ipv6_is_addr_unspecified(addr) || net_ipv6_is_addr_mcast(addr)) {
			continue;
		}

		if (net_ipv6_send_na(iface, addr, &allnodes, addr,
				     NET_ICMPV6_NA_FLAG_OVERRIDE) < 0) {
			LOG_WRN("SINK: unsolicited NA failed for %s",
				net_sprint_ipv6_addr(addr));
		} else {
			LOG_INF("SINK: unsolicited NA sent for %s (iface %d)",
				net_sprint_ipv6_addr(addr),
				net_if_get_by_iface(iface));
		}
	}
}
#endif /* CONFIG_NET_L2_DECT_BR_UNSOLICITED_NA */

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NS_PRIME)
void dect_net_l2_sink_eth_pt_nd_proxy_ns_prime(const struct in6_addr *pt_global,
					    const char *ctx)
{
	const struct net_in6_addr *src = (const struct net_in6_addr *)pt_global;
	struct net_if_router *router;
	struct net_in6_addr router_addr;
	bool router_known;
	const char *ctx_tag = (ctx != NULL) ? ctx : "?";
	int ret;

	if (iface_for_prefix == NULL || !net_if_is_up(iface_for_prefix)) {
		return;
	}

	if (net_if_flag_is_set(iface_for_prefix, NET_IF_IPV6_NO_ND)) {
		return;
	}

	if (pt_global == NULL || net_ipv6_is_addr_unspecified(src) ||
	    net_ipv6_is_addr_mcast(src) || !net_ipv6_is_global_addr(src)) {
		return;
	}

	router = net_if_ipv6_router_find_default(iface_for_prefix, NULL);
	router_known = (router != NULL);
	if (router_known) {
		net_ipv6_addr_copy_raw(router_addr.s6_addr,
				       router->address.in6_addr.s6_addr);
	}

	/*
	 * RFC 4861 7.2.3: NS from PT GUA, SLLAO = Ethernet MAC; seeds router NCE.
	 */
	if (router_known) {
		ret = net_ipv6_send_ns(iface_for_prefix, NULL, src,
				       &router_addr,
				       (const struct net_in6_addr *)&router_addr,
				       false);
		if (ret < 0) {
			LOG_WRN("SINK: PT ND proxy NS prime (target=router, %s) "
				"failed for %s (ret=%d)",
				ctx_tag, net_sprint_ipv6_addr(src), ret);
		} else {
			LOG_INF("SINK: PT ND proxy NS prime (target=router, %s) "
				"for %s -> %s (iface %d)",
				ctx_tag, net_sprint_ipv6_addr(src),
				net_sprint_ipv6_addr(&router_addr),
				net_if_get_by_iface(iface_for_prefix));
		}
	}
}
#endif /* CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NS_PRIME */

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NA_UNICAST_REFRESH)
void dect_net_l2_sink_eth_pt_nd_proxy_na_unicast(const struct in6_addr *pt_global,
					      const char *ctx)
{
	const struct net_in6_addr *tgt = (const struct net_in6_addr *)pt_global;
	struct net_if_router *router;
	struct net_in6_addr router_addr;
	const char *ctx_tag = (ctx != NULL) ? ctx : "?";

	if (iface_for_prefix == NULL || !net_if_is_up(iface_for_prefix)) {
		return;
	}

	if (net_if_flag_is_set(iface_for_prefix, NET_IF_IPV6_NO_ND)) {
		return;
	}

	if (pt_global == NULL || net_ipv6_is_addr_unspecified(tgt) ||
	    net_ipv6_is_addr_mcast(tgt) || !net_ipv6_is_global_addr(tgt)) {
		return;
	}

	router = net_if_ipv6_router_find_default(iface_for_prefix, NULL);
	if (router == NULL) {
		return;
	}

	net_ipv6_addr_copy_raw(router_addr.s6_addr, router->address.in6_addr.s6_addr);

	/* RFC 4861 7.2.4: unicast NA to router, source and target = PT GUA. */
	if (net_ipv6_send_na(iface_for_prefix, tgt, &router_addr, tgt,
			     NET_ICMPV6_NA_FLAG_OVERRIDE) < 0) {
		LOG_WRN("SINK: PT ND proxy unicast NA (%s) failed for %s",
			ctx_tag, net_sprint_ipv6_addr(tgt));
	} else {
		LOG_INF("SINK: PT ND proxy unicast NA (%s) for %s -> %s (iface %d)",
			ctx_tag,
			net_sprint_ipv6_addr(tgt),
			net_sprint_ipv6_addr(&router_addr),
			net_if_get_by_iface(iface_for_prefix));
	}
}
#endif /* CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NA_UNICAST_REFRESH */

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT)
void dect_net_l2_sink_eth_unsol_na_pt_nd_proxy(const struct in6_addr *pt_global)
{
	const struct net_in6_addr *tgt = (const struct net_in6_addr *)pt_global;
	struct net_in6_addr allnodes;

	if (iface_for_prefix == NULL || !net_if_is_up(iface_for_prefix)) {
		return;
	}

	if (net_if_flag_is_set(iface_for_prefix, NET_IF_IPV6_NO_ND)) {
		return;
	}

	if (pt_global == NULL || net_ipv6_is_addr_unspecified(tgt) ||
	    net_ipv6_is_addr_mcast(tgt) || !net_ipv6_is_global_addr(tgt)) {
		return;
	}

	net_ipv6_addr_create_ll_allnodes_mcast(&allnodes);

	/* RFC 4861 7.2.6: multicast unsolicited NA, src = target = PT GUA, Override. */
	if (net_ipv6_send_na(iface_for_prefix, tgt, &allnodes, tgt,
			     NET_ICMPV6_NA_FLAG_OVERRIDE) < 0) {
		LOG_WRN("SINK: PT ND proxy unsolicited NA failed for %s",
			net_sprint_ipv6_addr(tgt));
	} else {
		LOG_INF("SINK: PT ND proxy unsolicited NA for %s (iface %d)",
			net_sprint_ipv6_addr(tgt),
			net_if_get_by_iface(iface_for_prefix));
	}
}
#endif /* CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT */

#if defined(CONFIG_NET_L2_DECT_BR_UNSOLICITED_NA)
static void sink_eth_unsol_na_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (iface_for_prefix == NULL || !net_if_is_up(iface_for_prefix)) {
		return;
	}

	LOG_INF("SINK: unsolicited NA work for iface %p", iface_for_prefix);
	sink_eth_send_unsolicited_na(iface_for_prefix);
}
#endif /* CONFIG_NET_L2_DECT_BR_UNSOLICITED_NA */

#if defined(CONFIG_NET_DHCPV6)
static void sink_dhcpv6_start_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!iface_for_prefix || !net_if_is_up(iface_for_prefix)) {
		return;
	}

	if (atomic_get(&sink_prefix_addr_set)) {
		LOG_INF("DHCPv6 client: prefix already set, skipping start");
		return;
	}

	struct net_dhcpv6_params dhcpv6_params = {
		.request_addr = true,
		.request_prefix = false,
	};

	net_dhcpv6_start(iface_for_prefix, &dhcpv6_params);
	LOG_INF("DHCPv6 client started on iface %p", iface_for_prefix);
}
#endif

static void dect_net_l2_sink_rs_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (iface_for_prefix == NULL || !net_if_is_up(iface_for_prefix)) {
		return;
	}

	if (!atomic_get(&sink_prefix_addr_set)) {
		if (sink_rs_attempts < SINK_RS_MAX_ATTEMPTS) {
			sink_rs_attempts++;
			LOG_INF("SINK: RS work: attempt %u/%u for iface %p",
				sink_rs_attempts, SINK_RS_MAX_ATTEMPTS, iface_for_prefix);
#if CONFIG_NET_L2_DECT_BR_IPV6_ETH_TX_PACING_MS > 0
			k_msleep(CONFIG_NET_L2_DECT_BR_IPV6_ETH_TX_PACING_MS);
#endif
			net_if_start_rs(iface_for_prefix);

			/* The RA may still be missing if this burst was dropped
			 * (e.g. driver TX timeout). Reschedule; a later run is a
			 * no-op once the prefix is set (NET_EVENT_IPV6_ROUTER_ADD).
			 */
			(void)k_work_reschedule(&dect_sink_rs_work, SINK_RS_RETRY_INTERVAL);
		} else {
			LOG_WRN("SINK: RS work: no prefix after %u attempts, giving up "
				"for iface %p", sink_rs_attempts, iface_for_prefix);
		}
	} else {
		/* Prefix learned: nothing left to solicit, stop retrying. */
		sink_rs_attempts = 0;
	}
#if defined(CONFIG_NET_L2_DECT_BR_UNSOLICITED_NA)
	if (iface_for_prefix && net_if_is_up(iface_for_prefix)) {
		if (atomic_get(&sink_prefix_addr_set)) {
			LOG_INF("SINK: RS work: prefix set, schedule unsolicited NA work "
				"for iface %p", iface_for_prefix);
			(void)k_work_reschedule(&sink_eth_unsol_na_work, K_NO_WAIT);
		} else {
			LOG_INF("SINK: RS work: no prefix set, schedule unsolicited NA work "
				"for iface %p", iface_for_prefix);
			(void)k_work_reschedule(&sink_eth_unsol_na_work, K_SECONDS(20));
		}
	}
#endif
}

#define NET_IF_EVENT_MASK (NET_EVENT_IF_UP | NET_EVENT_IF_DOWN)

#if defined(CONFIG_NET_DHCPV6)
#define IPV6_LAYER_EVENT_MASK_DHCPV6 (NET_EVENT_IPV6_DAD_SUCCEED)
#else
#define IPV6_LAYER_EVENT_MASK_DHCPV6 0
#endif

#define IPV6_LAYER_EVENT_MASK							\
	(NET_EVENT_IPV6_PREFIX_ADD | NET_EVENT_IPV6_PREFIX_DEL |		\
	 NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_ADDR_DEL |			\
	 NET_EVENT_IPV6_ROUTER_ADD | NET_EVENT_IPV6_ROUTER_DEL |		\
	 NET_EVENT_IPV6_NBR_ADD | NET_EVENT_IPV6_NBR_DEL |			\
	 NET_EVENT_IPV6_ROUTE_ADD | NET_EVENT_IPV6_ROUTE_DEL |			\
	 IPV6_LAYER_EVENT_MASK_DHCPV6)

static int dect_net_l2_sink_init(void)
{
	iface_for_prefix = NULL;
	iface_for_dect = NULL;

	net_mgmt_init_event_callback(&dect_net_l2_net_mgmt_ipv6_event_cb,
				     dect_net_l2_net_mgmt_ipv6_event_handler,
				     IPV6_LAYER_EVENT_MASK);
	net_mgmt_init_event_callback(&net_if_cb, dect_net_l2_sink_net_if_mgmt_event_handler,
				     NET_IF_EVENT_MASK);
	net_mgmt_add_event_callback(&dect_net_l2_net_mgmt_ipv6_event_cb);
	net_mgmt_add_event_callback(&net_if_cb);
#if defined(CONFIG_NET_L2_ETHERNET)
	iface_for_prefix = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	if (!iface_for_prefix) {
		LOG_ERR("No Ethernet interface found for sink");
		return -ENOENT;
	}
	LOG_INF("Ethernet interface found for sink");
#endif
	iface_for_dect = net_if_get_by_index(
		net_if_get_by_name(CONFIG_DECT_MDM_DEVICE_NAME));
	if (!iface_for_dect) {
		LOG_ERR("%s: interface %s not found", (__func__),
			CONFIG_DECT_MDM_DEVICE_NAME);
	}

#if defined(CONFIG_MODEM_CELLULAR)
	struct net_if *const modem_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));
	int ret;

	pm_device_action_run(modem, PM_DEVICE_ACTION_RESUME);

	net_if_flag_set(modem_iface, NET_IF_POINTOPOINT);

	ret = net_if_up(modem_iface);
	if (ret < 0) {
		LOG_ERR("Failed to bring up modem interface");
		return -1;
	}
	iface_for_prefix = modem_iface;
#endif
	k_work_init_delayable(&dect_sink_rs_work, dect_net_l2_sink_rs_work_handler);
#if defined(CONFIG_MODEM_CELLULAR) || defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_UPSTREAM_PREFIX_ROUTE)
	k_work_init_delayable(&sink_router_nbr_deleted_work,
			      sink_router_nbr_deleted_work_handler);
#endif
#if defined(CONFIG_NET_DHCPV6)
	k_work_init_delayable(&sink_dhcpv6_start_work, sink_dhcpv6_start_work_handler);
#endif
#if defined(CONFIG_NET_L2_DECT_BR_UNSOLICITED_NA)
	k_work_init_delayable(&sink_eth_unsol_na_work, sink_eth_unsol_na_work_handler);
#endif

	return 0;
}

SYS_INIT(dect_net_l2_sink_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
