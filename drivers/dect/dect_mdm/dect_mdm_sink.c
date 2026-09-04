/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>

#include <net/dect/dect_net_l2_mgmt.h>

#include <net/dect/dect_utils.h>

#include "dect_mdm_ctrl.h"
#include "dect_mdm_sink.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dect_mdm, CONFIG_DECT_MDM_LOG_LEVEL);

#include "net_private.h" /* For net_sprint_ipv6_addr */

struct dect_mdm_sink_context {
	struct net_if *iface_for_dect;

	bool global_prefix_addr_set;
	/** Prefix length in bits last seen on DECT iface (64 or 96). */
	uint8_t iface_prefix_len_bits;
	struct in6_addr global_prefix_addr;
};

static struct dect_mdm_sink_context dect_mdm_sink_context_data;

static struct net_mgmt_event_callback dect_mdm_net_mgmt_ipv6_event_cb;

bool dect_mdm_sink_ipv6_prefix_get(struct dect_mdm_ipv6_prefix *prefix_out)
{
	struct dect_mdm_sink_context *ctx = &dect_mdm_sink_context_data;

	if (ctx->global_prefix_addr_set == false || ctx->iface_for_dect == NULL) {
		return false;
	}
	/* nrf_modem cluster PREFIX is 8 bytes: delegated /64 only */
	prefix_out->len = 8;
	memcpy(prefix_out->prefix.s6_addr, ctx->global_prefix_addr.s6_addr, 8);
	memset(&prefix_out->prefix.s6_addr[8], 0, 8);

	return true;
}

static void dect_mdm_sink_net_mgmt_ipv6_event_handler(struct net_mgmt_event_callback *cb,
						   uint64_t mgmt_event, struct net_if *iface)
{
	char ipv6_addr_str[NET_IPV6_ADDR_LEN];
	struct dect_mdm_sink_context *ctx = &dect_mdm_sink_context_data;

	if (iface != ctx->iface_for_dect) {
		LOG_DBG("Ignoring event for iface %p", iface);
		return;
	}
	if (mgmt_event == NET_EVENT_IPV6_PREFIX_ADD) {
		struct net_event_ipv6_prefix *ipv6_prefix =
			(struct net_event_ipv6_prefix *)cb->info;

		if (!net_ipv6_is_global_addr((struct in6_addr *)&ipv6_prefix->addr)) {
			/* Ignore non-global prefixes */
			LOG_INF("NET_EVENT_IPV6_PREFIX_ADD: prefix %s/%d is not a global address",
				net_addr_ntop(AF_INET6, (struct in6_addr *)&ipv6_prefix->addr,
				      ipv6_addr_str, NET_IPV6_ADDR_LEN),
				ipv6_prefix->len);
			return;
		}

		LOG_DBG("NET_EVENT_IPV6_PREFIX_ADD: iface %p, prefix %s/%d", iface,
			net_addr_ntop(AF_INET6, (struct in6_addr *)&ipv6_prefix->addr,
				      ipv6_addr_str, NET_IPV6_ADDR_LEN),
			ipv6_prefix->len);
		/* Save our used global prefix (/64 or /96 on DECT netiface) */
		if (!ctx->global_prefix_addr_set &&
		    (ipv6_prefix->len == 64 || ipv6_prefix->len == 96)) {
			ctx->global_prefix_addr_set = true;
			ctx->iface_prefix_len_bits = ipv6_prefix->len;
			net_ipaddr_copy(&ctx->global_prefix_addr, &ipv6_prefix->addr);

			LOG_INF("NET_EVENT_IPV6_PREFIX_ADD: our global prefix set to %s/%u",
				net_sprint_ipv6_addr(&ctx->global_prefix_addr),
				ipv6_prefix->len);
			/* Inform modem by starting reconfigure */
			dect_mdm_ctrl_api_cluster_reconfig_start_for_ipv6_prefix_cfg_changed();
		} else if (ctx->global_prefix_addr_set &&
			   (ipv6_prefix->len == 64 || ipv6_prefix->len == 96) &&
			   (ctx->iface_prefix_len_bits != ipv6_prefix->len ||
			    !net_ipv6_is_prefix(ctx->global_prefix_addr.s6_addr,
						ipv6_prefix->addr.s6_addr,
						ipv6_prefix->len))) {
			/* Prefix was already set and now it changed */
			LOG_WRN("NET_EVENT_IPV6_PREFIX_ADD: our global prefix changed "
				"from %s/%u to %s/%u",
				net_sprint_ipv6_addr(&ctx->global_prefix_addr),
				ctx->iface_prefix_len_bits,
				net_sprint_ipv6_addr((struct in6_addr *)&ipv6_prefix->addr),
				ipv6_prefix->len);

			net_ipaddr_copy(&ctx->global_prefix_addr, &ipv6_prefix->addr);
			ctx->iface_prefix_len_bits = ipv6_prefix->len;

			/* Inform modem by starting reconfigure */
			dect_mdm_ctrl_api_cluster_reconfig_start_for_ipv6_prefix_cfg_changed();
		}
	} else if (mgmt_event == NET_EVENT_IPV6_PREFIX_DEL) {
		struct net_event_ipv6_prefix *ipv6_prefix =
			(struct net_event_ipv6_prefix *)cb->info;

		LOG_DBG("NET_EVENT_IPV6_PREFIX_DEL: iface %p, prefix %s/%d", iface,
			net_addr_ntop(AF_INET6, (struct in6_addr *)&ipv6_prefix->addr,
				      ipv6_addr_str, NET_IPV6_ADDR_LEN),
			ipv6_prefix->len);

		/* Check if this is the same as was selected as our used prefix */
		if (ctx->global_prefix_addr_set &&
		    ipv6_prefix->len == ctx->iface_prefix_len_bits &&
		    net_ipv6_is_prefix(ctx->global_prefix_addr.s6_addr,
				       ipv6_prefix->addr.s6_addr,
				       ipv6_prefix->len)) {
			ctx->global_prefix_addr_set = false;
			ctx->iface_prefix_len_bits = 0;
			memset(&ctx->global_prefix_addr, 0, sizeof(ctx->global_prefix_addr));
			LOG_INF("NET_EVENT_IPV6_PREFIX_DEL: our global prefix was removed");

			/* Inform modem by starting reconfigure */
			dect_mdm_ctrl_api_cluster_reconfig_start_for_ipv6_prefix_cfg_changed();
		}
	}
}

static int dect_mdm_sink_init(void)
{
	struct dect_mdm_sink_context *ctx = &dect_mdm_sink_context_data;

	LOG_DBG("dect_mdm_sink_init");

	memset(ctx, 0, sizeof(*ctx));

	ctx->iface_for_dect = net_if_get_by_index(
		net_if_get_by_name(CONFIG_DECT_MDM_DEVICE_NAME));

	if (!ctx->iface_for_dect) {
		LOG_ERR("%s: interface %s not found", (__func__),
			CONFIG_DECT_MDM_DEVICE_NAME);
	}

	net_mgmt_init_event_callback(&dect_mdm_net_mgmt_ipv6_event_cb,
				     dect_mdm_sink_net_mgmt_ipv6_event_handler,
				     (NET_EVENT_IPV6_PREFIX_ADD | NET_EVENT_IPV6_PREFIX_DEL));
	net_mgmt_add_event_callback(&dect_mdm_net_mgmt_ipv6_event_cb);

	return 0;
}

SYS_INIT(dect_mdm_sink_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
