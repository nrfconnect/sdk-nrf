/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>

#include "ipv6.h"

#include <net/dect/dect_net_l2.h>
#include <net/dect/dect_net_l2_mgmt.h>
#include <net/dect/dect_utils.h>

#include "dect_net_l2_ipv6.h"
#include "dect_net_l2_sink.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_l2_dect, CONFIG_NET_L2_DECT_LOG_LEVEL);

#include "net_private.h" /* For net_sprint_ipv6_addr */

#define IPV6_LINK_LOCAL_PREFIX_BE32  0xfe800000
#define DECT_IPV6_PREFIX_LEN_BYTES   8
#define IPV6_PREFIX_COMPARISON_BITS  64

#if defined(CONFIG_NET_IPV6_NBR_CACHE)
static void dect_net_l2_ipv6_util_global_nbr_add(
	struct net_if *iface, struct dect_net_ipv6_prefix_config *ipv6_prefix_cfg,
	uint32_t sink_long_rd_id, uint32_t nbr_long_rd_id,
	bool *nbr_global_addr_was_set, struct in6_addr *nbr_global_ipv6_addr_out)
{
	bool nbr_addr_generated;
	struct in6_addr nbr_addr = {};

	__ASSERT_NO_MSG(ipv6_prefix_cfg != NULL);
	__ASSERT_NO_MSG(nbr_global_addr_was_set != NULL);
	__ASSERT_NO_MSG(nbr_global_ipv6_addr_out != NULL);

	if (ipv6_prefix_cfg->prefix_len > 0) {
		/* Add global addr as a neighbor */
		nbr_addr_generated =
			dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(
				ipv6_prefix_cfg->prefix, sink_long_rd_id, nbr_long_rd_id,
				&nbr_addr);
		if (nbr_addr_generated) {
			/* global: add a parent as a neighbor to dect iface */
			if (!net_ipv6_nbr_add(iface, &nbr_addr, net_if_get_link_addr(iface), false,
					      NET_IPV6_NBR_STATE_REACHABLE)) {
				LOG_ERR("(%s): cannot add parents global addr as nbr to dect iface",
					(__func__));
			} else {
				*nbr_global_addr_was_set = true;
				*nbr_global_ipv6_addr_out = nbr_addr;
				LOG_DBG("(%s): global addr %s (link addr %s) added "
					"as a neighbor to dect iface",
					(__func__), net_sprint_ipv6_addr(&nbr_addr),
					net_sprint_ll_addr(net_if_get_link_addr(iface)->addr, 8));
			}
		}
	}
}

static void dect_net_l2_ipv6_util_nbr_add(
	struct net_if *iface, struct dect_net_ipv6_prefix_config *ipv6_prefix_cfg,
	uint32_t sink_long_rd_id, uint32_t nbr_long_rd_id,
	bool *nbr_local_addr_was_set, struct in6_addr *nbr_local_ipv6_addr_out,
	bool *nbr_global_addr_was_set, struct in6_addr *nbr_global_ipv6_addr_out)
{
	bool nbr_addr_generated;
	struct in6_addr nbr_addr = {};
	struct in6_addr prefix = {};

	__ASSERT_NO_MSG(nbr_local_addr_was_set != NULL && nbr_global_addr_was_set != NULL);
	__ASSERT_NO_MSG(nbr_local_ipv6_addr_out != NULL && nbr_global_ipv6_addr_out != NULL);

	UNALIGNED_PUT(htonl(IPV6_LINK_LOCAL_PREFIX_BE32), &prefix.s6_addr32[0]);

	/* Add local addr as a neighbor */
	nbr_addr_generated = dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(
		prefix, sink_long_rd_id, nbr_long_rd_id,
		&nbr_addr);
	if (nbr_addr_generated) {
		/* local: add a parent as a neighbor to dect iface */
		if (!net_ipv6_nbr_add(iface, &nbr_addr, net_if_get_link_addr(iface), false,
				      NET_IPV6_NBR_STATE_REACHABLE)) {
			LOG_ERR("(%s): cannot add parents local addr (%s) as nbr to dect iface",
				(__func__), net_sprint_ipv6_addr(&nbr_addr));
		} else {
			*nbr_local_addr_was_set = true;
			*nbr_local_ipv6_addr_out = nbr_addr;
			LOG_DBG("(%s): long RD ID %u, local addr %s (link addr %s) "
				"added as a neighbor to dect iface %p",
				(__func__), nbr_long_rd_id, net_sprint_ipv6_addr(&nbr_addr),
				net_sprint_ll_addr(net_if_get_link_addr(iface)->addr, 8), iface);
		}
	} else {
		LOG_ERR("(%s): cannot create parents local addr as nbr to dect iface",
			(__func__));
	}
	dect_net_l2_ipv6_util_global_nbr_add(
		iface, ipv6_prefix_cfg, sink_long_rd_id, nbr_long_rd_id,
		nbr_global_addr_was_set, nbr_global_ipv6_addr_out);
}

static void dect_net_l2_ipv6_util_nbr_remove(struct net_if *iface,
					     struct dect_net_l2_association_data *ass_list_item)
{
	if (ass_list_item->local_ipv6_addr_set) {
		if (!net_ipv6_nbr_rm(iface, &ass_list_item->local_ipv6_addr)) {
			LOG_WRN("Failed to remove local IPv6 neighbor %s on iface %p",
				net_sprint_ipv6_addr(&ass_list_item->local_ipv6_addr), iface);
		}
		ass_list_item->local_ipv6_addr_set = false;
	}
	if (ass_list_item->global_ipv6_addr_set) {
		if (!net_ipv6_nbr_rm(iface, &ass_list_item->global_ipv6_addr)) {
			LOG_ERR("Failed to remove global IPv6 neighbor %s on iface %p",
				net_sprint_ipv6_addr(&ass_list_item->global_ipv6_addr), iface);
		}
		ass_list_item->global_ipv6_addr_set = false;
	}
}
#endif

static bool dect_net_l2_ipv6_util_link_local_addr_create_add(struct net_if *iface,
							     struct in6_addr *link_local_addr_out)
{
	struct net_if_addr *ifaddr;
	struct in6_addr iid;

	dect_utils_lib_net_ipv6_addr_create_iid(&iid, net_if_get_link_addr(iface));
	ifaddr = net_if_ipv6_addr_add(iface, &iid, NET_ADDR_AUTOCONF, 0);
	if (!ifaddr) {
		LOG_WRN("%s: cannot add link address to interface %p", (__func__), iface);
		return false;
	}
	LOG_DBG("Link local IPv6 address %s added to interface %p",
		net_sprint_ipv6_addr(&iid), iface);
	*link_local_addr_out = iid;
	return true;
}

static bool dect_net_l2_ipv6_util_global_addr_create_add(struct net_if *iface,
	struct dect_net_ipv6_prefix_config *ipv6_prefix_config,
	struct in6_addr *global_ipv6_addr_out)
{
	struct in6_addr global_addr = {};
	struct net_if_addr *ifaddr;
	bool added = false;

	/* Set ipv6 addr based on given info from peer FT device */
	if (ipv6_prefix_config->prefix_len == 0) {
		LOG_WRN("No global IPv6 address to set - using link local only");
	} else {
		/* Create our own IPv6 address using the given prefix and iid. We first
		 * setup link local address, and then copy prefix over first 16/8
		 * bytes of that address.
		 */
		dect_utils_lib_net_ipv6_addr_create_iid(
			&global_addr, net_if_get_link_addr(iface));
		memcpy(&global_addr.s6_addr,
		       ipv6_prefix_config->prefix.s6_addr, ipv6_prefix_config->prefix_len);

		ifaddr = net_if_ipv6_addr_lookup(&global_addr, NULL);
		if (ifaddr) {
			LOG_WRN("IPv6 address %s already exists - continue",
				net_sprint_ipv6_addr(&global_addr));
			net_if_addr_set_lf(ifaddr, true);
		} else {
			ifaddr = net_if_ipv6_addr_add(iface, &global_addr, NET_ADDR_AUTOCONF, 0);
			if (!ifaddr) {
				LOG_WRN("%s: cannot add address (%s) to interface %p", (__func__),
					net_sprint_ipv6_addr(&global_addr), iface);
			} else {
				added = true;
				*global_ipv6_addr_out = global_addr;
				LOG_DBG("Global IPv6 address %s added to interface %p",
					net_sprint_ipv6_addr(&global_addr), iface);
			}
		}
	}
	return added;
}

void dect_net_l2_ipv6_addressing_parent_changed_handle(
	struct dect_net_l2_association_data *list_item,
	struct net_if *iface, uint32_t parent_long_rd_id,
	struct dect_net_ipv6_prefix_config *ipv6_prefix_config)
{
	struct dect_net_l2_context *ctx = net_if_l2_data(iface);

	/* Check what has changed in parent IPv6 addressing */
	if (ctx->ipv6_prefix_cfg.prefix_len == 0 &&
	    ipv6_prefix_config->prefix_len > 0) {
		/* Prefix added -> we got Internet connectivity */
		LOG_INF("Parent IPv6 prefix added as %s/%d",
			net_sprint_ipv6_addr(&ipv6_prefix_config->prefix),
			ipv6_prefix_config->prefix_len * 8);
		dect_net_l2_ipv6_addressing_parent_added_handle(
			list_item, iface, parent_long_rd_id, ipv6_prefix_config);
	} else if (ctx->ipv6_prefix_cfg.prefix_len > 0 &&
		   ipv6_prefix_config->prefix_len == 0) {
		/* Prefix removed */
		LOG_WRN("Parent IPv6 prefix removed from %s/%d",
			net_sprint_ipv6_addr(&ctx->ipv6_prefix_cfg.prefix),
			ctx->ipv6_prefix_cfg.prefix_len * 8);
		if (ctx->global_ipv6_addr_set) {
			net_if_ipv6_addr_rm(iface, &ctx->global_ipv6_addr);
		}
		ctx->global_ipv6_addr_set = false;
		ctx->ipv6_prefix_cfg.prefix_len = 0;
		memset(&ctx->ipv6_prefix_cfg.prefix, 0, sizeof(ctx->ipv6_prefix_cfg.prefix));

#if defined(CONFIG_NET_IPV6_NBR_CACHE)
		if (list_item->global_ipv6_addr_set) {
			if (!net_ipv6_nbr_rm(iface, &list_item->global_ipv6_addr)) {
				LOG_ERR("%s: failed to remove global IPv6 neighbor %s on iface %p",
					(__func__),
					net_sprint_ipv6_addr(&list_item->global_ipv6_addr), iface);
			}
			list_item->global_ipv6_addr_set = false;
		}
#endif
	} else if (ctx->ipv6_prefix_cfg.prefix_len > 0 &&
		   ipv6_prefix_config->prefix_len > 0 &&
		   !net_ipv6_is_prefix(ctx->ipv6_prefix_cfg.prefix.s6_addr,
				       ipv6_prefix_config->prefix.s6_addr,
				       IPV6_PREFIX_COMPARISON_BITS)) {
		/* Prefix changed */
		LOG_WRN("Parent IPv6 prefix changed from %s/%d to %s/%d",
			net_sprint_ipv6_addr(&ctx->ipv6_prefix_cfg.prefix),
			ctx->ipv6_prefix_cfg.prefix_len * 8,
			net_sprint_ipv6_addr(&ipv6_prefix_config->prefix),
			ipv6_prefix_config->prefix_len * 8);
		/* Remove our global addr */
		if (ctx->global_ipv6_addr_set) {
			net_if_ipv6_addr_rm(iface, &ctx->global_ipv6_addr);
		}
		ctx->global_ipv6_addr_set = false;

		/* Add new prefix */
		dect_net_l2_ipv6_addressing_parent_added_handle(
			list_item, iface, parent_long_rd_id, ipv6_prefix_config);
	} else {
		/* No change */
		LOG_WRN("No change in parent IPv6 prefix - ignore");
		return;
	}
}

void dect_net_l2_ipv6_addressing_parent_added_handle(
	struct dect_net_l2_association_data *list_item,
	struct net_if *iface, uint32_t parent_long_rd_id,
	struct dect_net_ipv6_prefix_config *ipv6_prefix_config)
{
	bool removed = false;
	struct dect_net_l2_context *ctx = net_if_l2_data(iface);

	/* Store prefix config */
	ctx->ipv6_prefix_cfg = *ipv6_prefix_config;

	/* Parent added: remove/update our link local addr and ipv6 IID */
	removed = net_if_ipv6_addr_rm(iface, &ctx->local_ipv6_addr);
	if (!removed) {
		LOG_ERR("Failed to remove local IPv6 address %s on iface %p",
			net_sprint_ipv6_addr(&ctx->local_ipv6_addr), iface);
	}

	/* Link level addr has been set by the driver according
	 * to parent/sink long rd id + our long rd id.
	 * Let's continue from that on IPv6 level.
	 * Let's add our link local addr and possible global address to net iface
	 * and also add parent as a neighbor.
	 */
	if (!dect_net_l2_ipv6_util_link_local_addr_create_add(iface, &ctx->local_ipv6_addr)) {
		LOG_WRN("%s: cannot add our link local address to interface %p",
			(__func__), iface);
	}

	ctx->global_ipv6_addr_set = dect_net_l2_ipv6_util_global_addr_create_add(
		iface, ipv6_prefix_config,
		&ctx->global_ipv6_addr);

	/* Add parent as a neighbor and also in association list as nbr */
#if defined(CONFIG_NET_IPV6_NBR_CACHE)
	dect_net_l2_ipv6_util_nbr_add(iface, ipv6_prefix_config,
					  parent_long_rd_id,
					  parent_long_rd_id,
					  &list_item->local_ipv6_addr_set,
					  &list_item->local_ipv6_addr,
					  &list_item->global_ipv6_addr_set,
					  &list_item->global_ipv6_addr);
#endif
}

void dect_net_l2_ipv6_addressing_child_added_handle(
	struct dect_net_l2_association_data *ass_list_item, struct net_if *iface,
	uint32_t child_long_rd_id, bool first_child)
{
	struct dect_net_l2_context *ctx = net_if_l2_data(iface);

	/* Add child as a neighbor, both local and global */
	if (first_child) {
		/* But 1st,
		 * update ipv6 prefix info, and in case if there was related settings changes,
		 * so update also ipv6 addressing also for this device
		 * when 1st association is created.
		 */
		bool done = net_if_ipv6_addr_rm(iface, &ctx->local_ipv6_addr);

		if (!done) {
			LOG_WRN("%s: cannot remove our local address %s from interface %p",
				(__func__), net_sprint_ipv6_addr(&ctx->local_ipv6_addr), iface);
		}
		if (!dect_net_l2_ipv6_util_link_local_addr_create_add(iface,
								      &ctx->local_ipv6_addr)) {
			LOG_WRN("%s: cannot add our link local address to interface %p", (__func__),
				iface);
		}
		/* Update also our global address */
		dect_net_l2_ipv6_global_addressing_replace(iface);
	}

	/* Add child as a neighbor and also in association list */
#if defined(CONFIG_NET_IPV6_NBR_CACHE)
	dect_net_l2_ipv6_util_nbr_add(
		iface, &ctx->ipv6_prefix_cfg, ctx->transmitter_long_rd_id, child_long_rd_id,
		&ass_list_item->local_ipv6_addr_set, &ass_list_item->local_ipv6_addr,
		&ass_list_item->global_ipv6_addr_set, &ass_list_item->global_ipv6_addr);
#endif
}

void dect_net_l2_ipv6_addressing_child_removed_handle(
	struct dect_net_l2_association_data *ass_list_item,
	struct net_if *iface, uint32_t child_long_rd_id)
{
	if (ass_list_item == NULL) {
		return;
	}
#if defined(CONFIG_NET_IPV6_NBR_CACHE)
	dect_net_l2_ipv6_util_nbr_remove(
		iface,
		ass_list_item);
#endif
	ass_list_item->global_ipv6_addr_set = false;
	ass_list_item->local_ipv6_addr_set = false;
}

void dect_net_l2_ipv6_global_addressing_child_removed_handle(
	struct dect_net_l2_association_data *ass_list_item, struct net_if *iface)
{
	if (ass_list_item == NULL) {
		return;
	}
#if defined(CONFIG_NET_IPV6_NBR_CACHE)
	if (ass_list_item->global_ipv6_addr_set) {
		if (!net_ipv6_nbr_rm(iface, &ass_list_item->global_ipv6_addr)) {
			LOG_ERR("Failed to remove global IPv6 neighbor %s on iface %p",
				net_sprint_ipv6_addr(&ass_list_item->global_ipv6_addr), iface);
		}
	}
#endif
	ass_list_item->global_ipv6_addr_set = false;
}

void dect_net_l2_ipv6_global_addressing_child_changed_handle(struct dect_net_l2_context *ctx,
	struct dect_net_l2_association_data *ass_list_item, struct net_if *iface)
{
	if (ass_list_item == NULL) {
		return;
	}

#if defined(CONFIG_NET_IPV6_NBR_CACHE)
	dect_net_l2_ipv6_util_global_nbr_add(
		iface, &ctx->ipv6_prefix_cfg, ctx->transmitter_long_rd_id,
		ass_list_item->target_long_rd_id,
		&ass_list_item->global_ipv6_addr_set, &ass_list_item->global_ipv6_addr);
#endif
}

void dect_net_l2_ipv6_parent_addressing_removed_handle(
	struct dect_net_l2_association_data *ass_list_item, struct net_if *iface,
	uint32_t parent_long_rd_id)
{
	struct dect_net_l2_context *ctx = net_if_l2_data(iface);
	bool removed;

#if defined(CONFIG_NET_IPV6_NBR_CACHE)
	dect_net_l2_ipv6_util_nbr_remove(
		iface,
		ass_list_item);
#endif
	removed = net_if_ipv6_addr_rm(iface, &ctx->local_ipv6_addr);
	if (!removed) {
		LOG_WRN("%s: cannot remove our local address %s from interface %p",
			(__func__), net_sprint_ipv6_addr(&ctx->local_ipv6_addr), iface);
	}

	/* Set link local addr back as original */
	if (!dect_net_l2_ipv6_util_link_local_addr_create_add(iface, &ctx->local_ipv6_addr)) {
		LOG_WRN("%s: cannot add our orig link local address to interface %p",
			(__func__), iface);
	}

	/* Our local address was already updated, now remove our global IP address */
	if (ctx->global_ipv6_addr_set) {
		net_if_ipv6_addr_rm(iface, &ctx->global_ipv6_addr);
	}
	ctx->global_ipv6_addr_set = false;
	ass_list_item->global_ipv6_addr_set = false;
	ass_list_item->local_ipv6_addr_set = false;
}

void dect_net_l2_ipv6_global_addressing_replace(struct net_if *dect_iface)
{
	struct dect_net_l2_context *ctx = net_if_l2_data(dect_iface);
	struct net_if_ipv6 *dect_ipv6s = dect_iface->config.ip.ipv6;
	struct dect_net_l2_sink_ipv6_prefix sink_global_prefix;

	if (dect_net_l2_sink_ipv6_prefix_get(&sink_global_prefix)) {
		__ASSERT_NO_MSG(sink_global_prefix.len == 8);
		ctx->ipv6_prefix_cfg.prefix = sink_global_prefix.prefix;
		ctx->ipv6_prefix_cfg.prefix_len = sink_global_prefix.len;
	} else {
		ctx->ipv6_prefix_cfg.prefix_len = 0;
	}

	/* Remove all old global address from dect nr+ iface*/
	ARRAY_FOR_EACH(dect_ipv6s->unicast, i)
	{
		if (net_ipv6_is_global_addr(&dect_ipv6s->unicast[i].address.in6_addr)) {
			LOG_DBG("Removing old global address %s",
				net_sprint_ipv6_addr(&dect_ipv6s->unicast[i].address.in6_addr));
			net_if_ipv6_addr_rm(dect_iface,
					    &dect_ipv6s->unicast[i].address.in6_addr);
		}
	}
	/* ...and finally set new global address */
	ctx->global_ipv6_addr_set = dect_net_l2_ipv6_util_global_addr_create_add(
		dect_iface, &ctx->ipv6_prefix_cfg,
		&ctx->global_ipv6_addr);
}

void dect_net_l2_addr_util_prefix_replace(struct dect_net_l2_context *ctx,
	struct net_if *dect_iface, struct dect_net_ipv6_prefix_config *new_ipv6_prefix_config)
{
	struct net_if_ipv6 *dect_ipv6s = dect_iface->config.ip.ipv6;

	__ASSERT_NO_MSG(ctx != NULL);
	__ASSERT_NO_MSG(dect_iface != NULL);
	__ASSERT_NO_MSG(new_ipv6_prefix_config != NULL);

	ctx->ipv6_prefix_cfg = *new_ipv6_prefix_config;

	/* Remove all prefixes*/
	ARRAY_FOR_EACH(dect_ipv6s->prefix, i)
	{
		net_if_ipv6_prefix_rm(dect_iface,
			&dect_ipv6s->prefix[i].prefix,
			dect_ipv6s->prefix[i].len);
	}
	/* Add new prefix */
	if (ctx->ipv6_prefix_cfg.prefix_len > 0) {
		if (!net_if_ipv6_prefix_add(dect_iface,
			&ctx->ipv6_prefix_cfg.prefix,
			ctx->ipv6_prefix_cfg.prefix_len * 8,
			NET_IPV6_ND_INFINITE_LIFETIME)) {
			LOG_WRN("Failed to add IPv6 prefix %s/%d to dect nr+ iface %p",
				net_sprint_ipv6_addr(&ctx->ipv6_prefix_cfg.prefix),
				ctx->ipv6_prefix_cfg.prefix_len * 8, dect_iface);
		} else {
			LOG_INF("IPv6 prefix %s/%d added to dect nr+ iface %p",
				net_sprint_ipv6_addr(&ctx->ipv6_prefix_cfg.prefix),
				ctx->ipv6_prefix_cfg.prefix_len * 8, dect_iface);
		}
	}
}

bool dect_net_l2_ipv6_addressing_sink_changed_handle(struct net_if *iface,
	struct dect_net_ipv6_prefix_config *ipv6_prefix_config)
{
	struct dect_net_l2_context *ctx = net_if_l2_data(iface);
	bool global_address_changed = false;

	/* Check what has changed in sink IPv6 addressing and update our addressing accordingly */
	if (ctx->ipv6_prefix_cfg.prefix_len == 0 &&
	    ipv6_prefix_config->prefix_len > 0) {
		/* Prefix added -> we got Internet connectivity */
		LOG_DBG("Sink IPv6 prefix added as %s/%d",
			net_sprint_ipv6_addr(&ipv6_prefix_config->prefix),
			ipv6_prefix_config->prefix_len * 8);
		dect_net_l2_ipv6_global_addressing_replace(iface);
		dect_net_l2_addr_util_prefix_replace(ctx, iface, ipv6_prefix_config);
		global_address_changed = true;
	} else if (ctx->ipv6_prefix_cfg.prefix_len > 0 &&
		   ipv6_prefix_config->prefix_len == 0) {
		/* Prefix removed */
		LOG_WRN("Sink IPv6 prefix removed from %s/%d from dect nr+ iface %p",
			net_sprint_ipv6_addr(&ctx->ipv6_prefix_cfg.prefix),
			ctx->ipv6_prefix_cfg.prefix_len * 8, iface);

		/* Remove prefix */
		if (!net_if_ipv6_prefix_rm(
			iface, &ctx->ipv6_prefix_cfg.prefix, ctx->ipv6_prefix_cfg.prefix_len * 8)) {
			LOG_WRN("SINK: IPv6 prefix %s/64 removal failed from "
				"dect nr+ iface %p",
					net_sprint_ipv6_addr(&ctx->ipv6_prefix_cfg.prefix), iface);
		}
		if (ctx->global_ipv6_addr_set) {
			net_if_ipv6_addr_rm(iface, &ctx->global_ipv6_addr);
		}
		ctx->global_ipv6_addr_set = false;
		ctx->ipv6_prefix_cfg.prefix_len = 0;
		memset(&ctx->ipv6_prefix_cfg.prefix, 0, sizeof(ctx->ipv6_prefix_cfg.prefix));
	} else if (ctx->ipv6_prefix_cfg.prefix_len > 0 &&
		   ipv6_prefix_config->prefix_len > 0 &&
		   !net_ipv6_is_prefix(ctx->ipv6_prefix_cfg.prefix.s6_addr,
				       ipv6_prefix_config->prefix.s6_addr,
				       IPV6_PREFIX_COMPARISON_BITS)) {

		/* Prefix changed */
		LOG_WRN("Sink IPv6 prefix changed from %s/%d to %s/%d",
			net_sprint_ipv6_addr(&ctx->ipv6_prefix_cfg.prefix),
			ctx->ipv6_prefix_cfg.prefix_len * 8,
			net_sprint_ipv6_addr(&ipv6_prefix_config->prefix),
			ipv6_prefix_config->prefix_len * 8);
		dect_net_l2_ipv6_global_addressing_replace(iface);
		dect_net_l2_addr_util_prefix_replace(ctx, iface, ipv6_prefix_config);
		global_address_changed = true;
	} else {
		/* No change */
		LOG_WRN("No change in parent IPv6 prefix - ignore");
	}
	return global_address_changed;
}
