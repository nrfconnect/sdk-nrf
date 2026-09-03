/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/init.h>

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT)
#include <zephyr/net/ethernet.h>
#endif
#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT) && defined(CONFIG_NET_IPV6_MLD)
#include <zephyr/net/mld.h>
#endif
#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NS)
#include <zephyr/net/icmp.h>
#endif
#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NS_UNICAST_INTERCEPT)
#include <zephyr/net/net_pkt_filter.h>
#endif

#include "ipv6.h"
#include "nbr.h"
#if defined(CONFIG_NET_L2_DECT_BR_IPV6_SINK_ROUTE96)
#include "route_ipv6.h"
#endif

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NS)
#define ICMPV6_NS_TYPE 135
#endif

#include <net/dect/dect_net_l2.h>
#include <net/dect/dect_net_l2_mgmt.h>
#include <net/dect/dect_utils.h>

#include "dect_net_l2_ipv6.h"
#include "dect_net_l2_sink.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_l2_dect, CONFIG_NET_L2_DECT_LOG_LEVEL);

#include "net_private.h"

#define IPV6_LINK_LOCAL_PREFIX_BE32  0xfe800000

/** On-link ULA on DECT: common /64 from Kconfig + 32-bit peer RD id (bits 64–95), /96. */
#define DECT_L2_ULA_DECT_ONLINK_PLEN_BITS 96

/** On-DECT delegated /64 prefix length in struct dect_net_ipv6_prefix_config::prefix (bytes). */
#define DECT_IPV6_PREFIX_LEN_64_BYTES 8
/** On-DECT /96-style prefix length (delegated /64 + 32-bit sink scope) in bytes. */
#define DECT_IPV6_PREFIX_LEN_96_BYTES 12
/** Last bytes of a 128-bit address after the /96 prefix (IID fragment from link-local). */
#define DECT_IPV6_IID_TAIL_BYTES (NET_IPV6_ADDR_SIZE - DECT_IPV6_PREFIX_LEN_96_BYTES)

#if defined(CONFIG_NET_L2_DECT_ULA)
static void dect_net_l2_ipv6_ula_remove(struct net_if *iface, struct dect_net_l2_context *ctx)
{
	if (!ctx->ula_ipv6_configured) {
		return;
	}

	(void)net_if_ipv6_addr_rm(iface, &ctx->ula_ipv6_addr);
	if (ctx->ula_iface_plen_bits > 0U) {
		(void)net_if_ipv6_prefix_rm(
			iface, &ctx->ula_iface_prefix, ctx->ula_iface_plen_bits);
	}
	ctx->ula_ipv6_configured = false;
	ctx->ula_iface_plen_bits = 0U;
	memset(&ctx->ula_ipv6_addr, 0, sizeof(ctx->ula_ipv6_addr));
	memset(&ctx->ula_iface_prefix, 0, sizeof(ctx->ula_iface_prefix));
}
#endif

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_SINK_ROUTE96)
static void dect_net_l2_ipv6_sink_route96_normalize(const struct dect_net_ipv6_prefix_config *cfg,
						    struct net_in6_addr *out)
{
	memcpy(out->s6_addr, cfg->prefix.s6_addr, DECT_IPV6_PREFIX_LEN_96_BYTES);
	memset(out->s6_addr + DECT_IPV6_PREFIX_LEN_96_BYTES, 0, DECT_IPV6_IID_TAIL_BYTES);
}

static void dect_net_l2_ipv6_sink_route96_remove(struct net_if *dect_iface,
						 const struct dect_net_ipv6_prefix_config *cfg)
{
	struct net_in6_addr p96;
	struct net_route_entry *re;

	if (cfg->prefix_len != DECT_IPV6_PREFIX_LEN_96_BYTES) {
		return;
	}

	dect_net_l2_ipv6_sink_route96_normalize(cfg, &p96);

	re = net_route_ipv6_lookup(dect_iface, &p96);
	if (re != NULL) {
		(void)net_route_ipv6_del(re);
	}

	(void)net_ipv6_nbr_rm(dect_iface, &p96);
}

static void dect_net_l2_ipv6_sink_route96_add(struct net_if *dect_iface,
					      const struct dect_net_ipv6_prefix_config *cfg)
{
	struct net_in6_addr p96;
	struct net_route_entry *re;

	if (cfg->prefix_len != DECT_IPV6_PREFIX_LEN_96_BYTES) {
		return;
	}

	dect_net_l2_ipv6_sink_route96_normalize(cfg, &p96);

	if (!net_ipv6_nbr_add(dect_iface, &p96, net_if_get_link_addr(dect_iface), false,
			      NET_IPV6_NBR_STATE_REACHABLE)) {
		LOG_ERR("(%s): cannot add nbr for %s (/96 sink route nexthop)",
			__func__, net_sprint_ipv6_addr(&p96));
		return;
	}

	/*
	 * nexthop == p96 on purpose (standard Zephyr "connected route" idiom, cf.
	 * add_route() in ipv6.c): only wins longest-prefix-match onto DECT. The
	 * resolved lladdr is never used to reach a PT - dect_net_l2_send() always
	 * re-derives the real target long RD ID from the packet's IPv6 dst addr.
	 */
	re = net_route_ipv6_add(dect_iface, &p96, 96, &p96, NET_IPV6_ND_INFINITE_LIFETIME,
		NET_ROUTE_PREFERENCE_HIGH);
	if (re == NULL) {
		LOG_ERR("(%s): failed to add /96 route for %s/96",
			__func__, net_sprint_ipv6_addr(&p96));
		(void)net_ipv6_nbr_rm(dect_iface, &p96);
		return;
	}

	LOG_INF("(%s): /96 route %s/96 on iface %p",
		__func__, net_sprint_ipv6_addr(&p96), dect_iface);
}
#endif /* CONFIG_NET_L2_DECT_BR_IPV6_SINK_ROUTE96 */

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT)
static struct net_if *dect_net_l2_ipv6_eth_iface_first(void)
{
	return net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
}

/*
 * Register a PT GUA on the Ethernet sink uplink for ND proxy (FT border router):
 * join the solicited-node multicast group (MLD) and send optional NS/NA priming
 * and unsolicited NA toward upstream LAN peers (Kconfig under NET_L2_ETHERNET).
 */
static void dect_net_l2_ipv6_pt_eth_nd_proxy_add(const struct in6_addr *pt_global)
{
	struct net_if *eth;
	const struct net_in6_addr *pt = (const struct net_in6_addr *)pt_global;

	if (!net_ipv6_is_global_addr(pt)) {
		return;
	}

	eth = dect_net_l2_ipv6_eth_iface_first();
	if (eth == NULL) {
		LOG_DBG("(%s): no Ethernet iface", __func__);
		return;
	}

	LOG_DBG("(%s): register PT %s on eth ND proxy", __func__, net_sprint_ipv6_addr(pt));

#if defined(CONFIG_NET_IPV6_MLD)
	{
		struct net_in6_addr mcast;
		int ret;

		/* RFC 4861 solicited-node multicast (MLD join for upstream NS). */
		net_ipv6_addr_create_solicited_node(pt, &mcast);
		ret = net_ipv6_mld_join(eth, &mcast);
		if (ret < 0 && ret != -EALREADY) {
			LOG_ERR("(%s): MLD join %s for PT %s failed (%d)", __func__,
				net_sprint_ipv6_addr(&mcast), net_sprint_ipv6_addr(pt), ret);
		} else {
			LOG_DBG("(%s): MLD join %s on eth for PT %s", __func__,
				net_sprint_ipv6_addr(&mcast), net_sprint_ipv6_addr(pt));
		}
	}
#endif

	/*
	 * RFC 4861 7.2.3 NS prime, 7.2.4 unicast NA, 7.2.6 multicast NA (initial PT publish).
	 */
#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NS_PRIME)
	dect_net_l2_sink_eth_pt_nd_proxy_ns_prime(pt_global, "initial");
	if (CONFIG_NET_L2_DECT_BR_IPV6_ETH_TX_PACING_MS > 0) {
		k_msleep(CONFIG_NET_L2_DECT_BR_IPV6_ETH_TX_PACING_MS);
	}
#endif

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NA_UNICAST_REFRESH)
	dect_net_l2_sink_eth_pt_nd_proxy_na_unicast(pt_global, "initial");
	if (CONFIG_NET_L2_DECT_BR_IPV6_ETH_TX_PACING_MS > 0) {
		k_msleep(CONFIG_NET_L2_DECT_BR_IPV6_ETH_TX_PACING_MS);
	}
#endif

#if defined(CONFIG_NET_L2_DECT_BR_UNSOLICITED_NA)
	dect_net_l2_sink_eth_unsol_na_pt_nd_proxy(pt_global);
#endif
}

/* Leave PT GUA solicited-node group on the Ethernet uplink when PT is removed. */
static void dect_net_l2_ipv6_pt_eth_nd_proxy_remove(const struct in6_addr *pt_global)
{
	struct net_if *eth;
	const struct net_in6_addr *pt = (const struct net_in6_addr *)pt_global;

	if (!net_ipv6_is_global_addr(pt)) {
		return;
	}

	eth = dect_net_l2_ipv6_eth_iface_first();
	if (eth == NULL) {
		return;
	}

#if defined(CONFIG_NET_IPV6_MLD)
	{
		struct net_in6_addr mcast;
		int ret;

		net_ipv6_addr_create_solicited_node(pt, &mcast);
		ret = net_ipv6_mld_leave(eth, &mcast);
		if (ret < 0 && ret != -ENOENT) {
			LOG_DBG("(%s): MLD leave %s (%d)", __func__, net_sprint_ipv6_addr(&mcast),
				ret);
		}
	}
#endif
}

#endif /* CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT */

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NS)

/*
 * ICMPv6 NS handler for PT GUAs on the Ethernet uplink: queue RFC 4861 7.2.4
 * solicited NA when the target matches a DECT child.
 */
static struct net_icmp_ctx dect_pt_nd_proxy_ns_ctx;

/* Queue solicited NA on sysworkq (Ethernet driver RX thread must not TX inline). */
#define DECT_PT_ND_PROXY_NA_QUEUE_DEPTH 8

struct dect_pt_nd_proxy_na_req {
	bool in_use;
	struct net_if *iface;
	struct net_in6_addr src;    /* = NS target = PT GUA. */
	struct net_in6_addr na_dst; /* = NS source (router) or all-nodes (DAD). */
};

static struct dect_pt_nd_proxy_na_req dect_pt_nd_proxy_na_queue[DECT_PT_ND_PROXY_NA_QUEUE_DEPTH];
static struct k_spinlock dect_pt_nd_proxy_na_lock;

static void dect_pt_nd_proxy_na_work_handler(struct k_work *work);
static K_WORK_DEFINE(dect_pt_nd_proxy_na_work, dect_pt_nd_proxy_na_work_handler);

static int dect_pt_nd_proxy_na_enqueue(struct net_if *iface,
				    const struct net_in6_addr *src,
				    const struct net_in6_addr *na_dst)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&dect_pt_nd_proxy_na_lock);
	for (int i = 0; i < ARRAY_SIZE(dect_pt_nd_proxy_na_queue); i++) {
		struct dect_pt_nd_proxy_na_req *r = &dect_pt_nd_proxy_na_queue[i];

		if (!r->in_use) {
			r->iface = iface;
			r->src = *src;
			r->na_dst = *na_dst;
			r->in_use = true;
			k_spin_unlock(&dect_pt_nd_proxy_na_lock, key);
			return 0;
		}
	}
	k_spin_unlock(&dect_pt_nd_proxy_na_lock, key);
	return -ENOSPC;
}

static void dect_pt_nd_proxy_na_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	for (int i = 0; i < ARRAY_SIZE(dect_pt_nd_proxy_na_queue); i++) {
		struct net_if *iface = NULL;
		struct net_in6_addr src;
		struct net_in6_addr na_dst;
		k_spinlock_key_t key;
		bool valid = false;
		int ret;

		key = k_spin_lock(&dect_pt_nd_proxy_na_lock);
		if (dect_pt_nd_proxy_na_queue[i].in_use) {
			iface = dect_pt_nd_proxy_na_queue[i].iface;
			src = dect_pt_nd_proxy_na_queue[i].src;
			na_dst = dect_pt_nd_proxy_na_queue[i].na_dst;
			dect_pt_nd_proxy_na_queue[i].in_use = false;
			valid = true;
		}
		k_spin_unlock(&dect_pt_nd_proxy_na_lock, key);

		if (!valid) {
			continue;
		}

		if (CONFIG_NET_L2_DECT_BR_IPV6_ETH_TX_PACING_MS > 0) {
			k_msleep(CONFIG_NET_L2_DECT_BR_IPV6_ETH_TX_PACING_MS);
		}

		/* RFC 4861 7.2.4 solicited NA: source and target = PT GUA, TLLAO = Ethernet MAC. */
		ret = net_ipv6_send_na(iface, &src, &na_dst, &src,
				       NET_ICMPV6_NA_FLAG_SOLICITED |
				       NET_ICMPV6_NA_FLAG_OVERRIDE);
		if (ret < 0) {
			LOG_WRN("SINK: ND proxy solicited NA failed for PT %s (ret=%d)",
				net_sprint_ipv6_addr(&src), ret);
		} else {
			LOG_INF("SINK: ND proxy solicited NA sent for PT %s -> %s",
				net_sprint_ipv6_addr(&src),
				net_sprint_ipv6_addr(&na_dst));
		}
	}
}

/* Queue solicited NA for a matching PT GUA (ICMP handler or unicast intercept). */
static bool dect_pt_nd_proxy_ns_schedule(struct net_if *rx, const struct net_in6_addr *tgt,
				      const struct net_in6_addr *ns_src, const char *via)
{
	struct net_in6_addr na_dst;
	int qret;

	if (!net_ipv6_is_global_addr(tgt)) {
		return false;
	}

	LOG_INF("SINK: solicited NS for PT %s from %s, scheduling ND proxy NA (%s)",
		net_sprint_ipv6_addr(tgt), net_sprint_ipv6_addr(ns_src), via);

	if (net_ipv6_is_addr_unspecified(ns_src)) {
		net_ipv6_addr_create_ll_allnodes_mcast(&na_dst);
	} else {
		na_dst = *ns_src;
	}

	qret = dect_pt_nd_proxy_na_enqueue(rx, tgt, &na_dst);
	if (qret < 0) {
		LOG_WRN("SINK: ND proxy NA enqueue full for PT %s (depth=%d), dropping",
			net_sprint_ipv6_addr(tgt),
			(int)ARRAY_SIZE(dect_pt_nd_proxy_na_queue));
		return false;
	}

	(void)k_work_submit(&dect_pt_nd_proxy_na_work);
	return true;
}

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NS_UNICAST_INTERCEPT)

static bool dect_pt_nd_proxy_ns_parse(struct net_pkt *pkt, struct net_in6_addr *tgt_out)
{
	struct net_ipv6_hdr *hdr = NET_IPV6_HDR(pkt);
	struct net_pkt_cursor backup;
	struct net_icmp_hdr icmp_hdr;
	uint32_t reserved;

	if (hdr->nexthdr != IPPROTO_ICMPV6) {
		return false;
	}

	net_pkt_cursor_init(pkt);
	net_pkt_cursor_backup(pkt, &backup);

	if (net_pkt_skip(pkt, sizeof(struct net_ipv6_hdr)) != 0) {
		goto restore;
	}

	if (net_pkt_read(pkt, &icmp_hdr, sizeof(icmp_hdr)) < 0) {
		goto restore;
	}

	if (icmp_hdr.type != ICMPV6_NS_TYPE) {
		goto restore;
	}

	if (net_pkt_read(pkt, &reserved, sizeof(reserved)) < 0) {
		goto restore;
	}

	if (net_pkt_read(pkt, tgt_out->s6_addr, sizeof(tgt_out->s6_addr)) < 0) {
		goto restore;
	}

	net_pkt_cursor_restore(pkt, &backup);
	return true;

restore:
	net_pkt_cursor_restore(pkt, &backup);
	return false;
}

static bool dect_pt_nd_proxy_unicast_ns_npf_test(struct npf_test *test, struct net_pkt *pkt)
{
	struct net_if *rx;
	struct net_ipv6_hdr *hdr;
	struct net_in6_addr tgt;

	ARG_UNUSED(test);

	rx = net_pkt_iface(pkt);
	if (rx == NULL || net_if_l2(rx) != &NET_L2_GET_NAME(ETHERNET)) {
		return false;
	}

	hdr = NET_IPV6_HDR(pkt);
	if (net_ipv6_is_addr_mcast_raw(hdr->dst)) {
		return false;
	}

	if (!dect_pt_nd_proxy_ns_parse(pkt, &tgt)) {
		return false;
	}

	/* npf rule callback context: lock-free, best-effort match. */
	if (!dect_net_l2_npf_child_global_ipv6_match((const struct in6_addr *)&tgt)) {
		return false;
	}

	return dect_pt_nd_proxy_ns_schedule(rx, &tgt,
					 (const struct net_in6_addr *)&hdr->src,
					 "unicast intercept");
}

static struct npf_test dect_pt_nd_proxy_unicast_ns_npf_test_inst = {
	.fn = dect_pt_nd_proxy_unicast_ns_npf_test,
};

static struct npf_rule dect_pt_nd_proxy_unicast_ns_intercept = {
	.result = NET_DROP,
	.nb_tests = 1,
	.tests = { &dect_pt_nd_proxy_unicast_ns_npf_test_inst },
};

static int dect_net_l2_ipv6_pt_nd_proxy_unicast_intercept_init(void)
{
	/* Intercept rule first, then npf_default_ok for other IPv6 traffic. */
	npf_insert_ipv6_recv_rule(&dect_pt_nd_proxy_unicast_ns_intercept);
	npf_append_ipv6_recv_rule(&npf_default_ok);
	LOG_INF("PT ND proxy unicast NS intercept rule registered on IPv6 recv");
	return 0;
}

SYS_INIT(dect_net_l2_ipv6_pt_nd_proxy_unicast_intercept_init, APPLICATION, 1);

#endif /* CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NS_UNICAST_INTERCEPT */

static enum net_verdict dect_net_l2_ipv6_pt_nd_proxy_ns_handler(struct net_icmp_ctx *icmp_ctx,
							      struct net_pkt *pkt,
							      struct net_icmp_ip_hdr *ip_hdr,
							      struct net_icmp_hdr *icmp_hdr,
							      void *user_data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ns_access, struct net_icmpv6_ns_hdr);
	struct net_icmpv6_ns_hdr *ns_hdr;
	struct net_if *rx;
	struct net_in6_addr tgt;

	ARG_UNUSED(icmp_ctx);
	ARG_UNUSED(icmp_hdr);
	ARG_UNUSED(user_data);

	/*
	 * Return NET_CONTINUE for every "not my NS" early exit so the
	 * dispatch loop continues to Zephyr's own NS handler.
	 */
	rx = net_pkt_iface(pkt);
	if (rx == NULL || net_if_l2(rx) != &NET_L2_GET_NAME(ETHERNET)) {
		return NET_CONTINUE;
	}

	ns_hdr = (struct net_icmpv6_ns_hdr *)net_pkt_get_data(pkt, &ns_access);
	if (ns_hdr == NULL) {
		return NET_CONTINUE;
	}

	net_ipv6_addr_copy_raw(tgt.s6_addr, ns_hdr->tgt);

	/* ICMP handler runs in regular thread context: take the mutex-guarded
	 * variant so the membership decision is exact.
	 */
	if (!dect_net_l2_child_global_ipv6_match((const struct in6_addr *)&tgt)) {
		return NET_CONTINUE;
	}

	(void)dect_pt_nd_proxy_ns_schedule(rx, &tgt,
					(const struct net_in6_addr *)&ip_hdr->ipv6->src,
					"icmp");

	return NET_CONTINUE;
}

static int dect_net_l2_ipv6_pt_nd_proxy_ns_sys_init(void)
{
	int ret;

	ret = net_icmp_init_ctx(&dect_pt_nd_proxy_ns_ctx, NET_AF_INET6, ICMPV6_NS_TYPE, 0,
				dect_net_l2_ipv6_pt_nd_proxy_ns_handler);
	LOG_INF("PT ND proxy NS handler registered (ret=%d)", ret);
	return 0;
}
SYS_INIT(dect_net_l2_ipv6_pt_nd_proxy_ns_sys_init, APPLICATION, 0);

#endif /* CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT_NS */

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
				LOG_ERR("(%s): cannot add global addr (%s) as nbr to dect iface "
					"(sink long RD ID %u, nbr long RD ID %u)",
					(__func__), net_sprint_ipv6_addr(&nbr_addr),
					sink_long_rd_id, nbr_long_rd_id);
			} else {
				*nbr_global_addr_was_set = true;
				*nbr_global_ipv6_addr_out = nbr_addr;
				LOG_DBG("(%s): global addr %s (link addr %s) added "
					"as a neighbor to dect iface",
					(__func__), net_sprint_ipv6_addr(&nbr_addr),
					net_sprint_ll_addr(net_if_get_link_addr(iface)->addr, 8));

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT)
				{
					struct dect_net_l2_context *l2_ctx = net_if_l2_data(iface);

					if ((l2_ctx->device_type & DECT_DEVICE_TYPE_FT) != 0) {
						dect_net_l2_ipv6_pt_eth_nd_proxy_add(&nbr_addr);
					}
				}
#endif
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
			LOG_ERR("(%s): cannot add local addr (%s) as nbr to dect iface",
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
		LOG_ERR("(%s): cannot create local addr as nbr to dect iface for long RD ID %u",
			(__func__), nbr_long_rd_id);
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
#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT)
		{
			struct dect_net_l2_context *l2_ctx = net_if_l2_data(iface);

			if ((l2_ctx->device_type & DECT_DEVICE_TYPE_FT) != 0) {
				dect_net_l2_ipv6_pt_eth_nd_proxy_remove(
					&ass_list_item->global_ipv6_addr);
			}
		}
#endif
		if (!net_ipv6_nbr_rm(iface, &ass_list_item->global_ipv6_addr)) {
			LOG_ERR("Failed to remove global IPv6 neighbor %s on iface %p",
				net_sprint_ipv6_addr(&ass_list_item->global_ipv6_addr), iface);
		}
		ass_list_item->global_ipv6_addr_set = false;
	}
}
#endif

#if defined(CONFIG_NET_L2_DECT_ULA)
static bool dect_net_l2_ipv6_ula_parse_base_prefix(struct net_in6_addr *base_pfx)
{
	if (sizeof(CONFIG_NET_L2_DECT_ULA_PREFIX) <= 1U ||
	    CONFIG_NET_L2_DECT_ULA_PREFIX[0] == '\0') {
		return false;
	}

	if (net_addr_pton(AF_INET6, CONFIG_NET_L2_DECT_ULA_PREFIX, base_pfx) != 0) {
		LOG_WRN("NET_L2_DECT_ULA_PREFIX: parse failed");
		return false;
	}

	if (!net_ipv6_is_ula_addr(base_pfx)) {
		LOG_WRN("NET_L2_DECT_ULA_PREFIX: not a ULA (fc00::/7)");
		return false;
	}

	memset(base_pfx->s6_addr + 8, 0, 8);
	return true;
}
#endif

void dect_net_l2_ipv6_ula_sync_for_peer(struct net_if *iface,
					const struct in6_addr *link_local_addr,
					uint32_t peer_long_rd_id)
{
#if !defined(CONFIG_NET_L2_DECT_ULA)
	ARG_UNUSED(iface);
	ARG_UNUSED(link_local_addr);
	ARG_UNUSED(peer_long_rd_id);
#else /* CONFIG_NET_L2_DECT_ULA */
	struct dect_net_l2_context *ctx = net_if_l2_data(iface);
	struct net_in6_addr base_pfx;
	struct net_in6_addr pfx;
	struct in6_addr ula;
	struct net_if_addr *ifaddr;
	uint32_t id = peer_long_rd_id;

	if (id == DECT_NET_L2_LONG_RD_ID_NOT_SET) {
		dect_net_l2_ipv6_ula_remove(iface, ctx);
		LOG_WRN("%s: RD id not known yet - not building a ULA", __func__);
		return;
	}

	/* No valid base prefix configured (parse_base_prefix already warns on
	 * misconfiguration): leave ULA removed.
	 */
	if (!dect_net_l2_ipv6_ula_parse_base_prefix(&base_pfx)) {
		dect_net_l2_ipv6_ula_remove(iface, ctx);
		return;
	}

	/* 96-bit ULA prefix: full common 64-bit prefix from Kconfig + BE32(peer)
	 * in bytes 8–11 (e.g. fdde:ad00:0000:0000:0000:0001::/96 when peer long RD id is 1).
	 */
	memcpy(pfx.s6_addr, base_pfx.s6_addr, DECT_IPV6_PREFIX_LEN_64_BYTES);
	sys_put_be32(id, pfx.s6_addr + DECT_IPV6_PREFIX_LEN_64_BYTES);
	memset(pfx.s6_addr + DECT_IPV6_PREFIX_LEN_96_BYTES, 0, DECT_IPV6_IID_TAIL_BYTES);

	memcpy(ula.s6_addr, pfx.s6_addr, DECT_IPV6_PREFIX_LEN_96_BYTES);
	memcpy(ula.s6_addr + DECT_IPV6_PREFIX_LEN_96_BYTES,
	       link_local_addr->s6_addr + DECT_IPV6_PREFIX_LEN_96_BYTES, DECT_IPV6_IID_TAIL_BYTES);

	/* Same prefix + host already configured (e.g. FT gaining another child, or a sink
	 * prefix replace that did not change the peer id): skip the remove/add flap.
	 */
	if (ctx->ula_ipv6_configured &&
	    ctx->ula_iface_plen_bits == DECT_L2_ULA_DECT_ONLINK_PLEN_BITS &&
	    memcmp(ctx->ula_iface_prefix.s6_addr, pfx.s6_addr, NET_IPV6_ADDR_SIZE) == 0 &&
	    memcmp(ctx->ula_ipv6_addr.s6_addr, ula.s6_addr, NET_IPV6_ADDR_SIZE) == 0) {
		return;
	}

	dect_net_l2_ipv6_ula_remove(iface, ctx);

	ifaddr = net_if_ipv6_addr_add(iface, &ula, NET_ADDR_AUTOCONF, 0);
	if (ifaddr == NULL) {
		LOG_WRN("%s: cannot add ULA %s on DECT", __func__,
			net_sprint_ipv6_addr(&ula));
		return;
	}
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	if (!net_if_ipv6_prefix_add(iface, &pfx, DECT_L2_ULA_DECT_ONLINK_PLEN_BITS,
				    NET_IPV6_ND_INFINITE_LIFETIME)) {
		LOG_WRN("%s: cannot add ULA /%u prefix on DECT", __func__,
			(unsigned int)DECT_L2_ULA_DECT_ONLINK_PLEN_BITS);
		(void)net_if_ipv6_addr_rm(iface, &ula);
		return;
	}
#if defined(CONFIG_NET_L2_DECT_BR)
	if (!net_if_ipv6_prefix_add(iface, &base_pfx, DECT_IPV6_PREFIX_LEN_64_BYTES * 8,
				    NET_IPV6_ND_INFINITE_LIFETIME)) {
		LOG_WRN("%s: cannot add ULA /64 prefix %s on DECT", __func__,
			net_sprint_ipv6_addr(&base_pfx));
		(void)net_if_ipv6_prefix_rm(iface, &pfx, DECT_L2_ULA_DECT_ONLINK_PLEN_BITS);
		(void)net_if_ipv6_addr_rm(iface, &ula);
		return;
	}
#endif
	ctx->ula_ipv6_configured = true;
	ctx->ula_ipv6_addr = ula;
	ctx->ula_iface_plen_bits = DECT_L2_ULA_DECT_ONLINK_PLEN_BITS;
	net_ipv6_addr_copy_raw(ctx->ula_iface_prefix.s6_addr, pfx.s6_addr);
	LOG_INF("DECT ULA %s (on-link /%u on iface, peer RD id %u)", net_sprint_ipv6_addr(&ula),
		(unsigned int)DECT_L2_ULA_DECT_ONLINK_PLEN_BITS, peer_long_rd_id);
#endif
}

#if defined(CONFIG_NET_L2_DECT_ULA)
bool dect_net_l2_ipv6_dect_ula_onlink_prefix_get(struct net_if *dect_iface,
						 struct net_in6_addr *pfx_out,
						 uint8_t *prefix_len_bits_out)
{
	struct dect_net_l2_context *ctx;

	if (dect_iface == NULL || pfx_out == NULL || prefix_len_bits_out == NULL) {
		return false;
	}

	ctx = net_if_l2_data(dect_iface);
	if (!ctx->ula_ipv6_configured || ctx->ula_iface_plen_bits == 0U) {
		return false;
	}

	net_ipv6_addr_copy_raw(pfx_out->s6_addr, ctx->ula_iface_prefix.s6_addr);
	memset(pfx_out->s6_addr + DECT_IPV6_PREFIX_LEN_96_BYTES, 0, DECT_IPV6_IID_TAIL_BYTES);
	*prefix_len_bits_out = ctx->ula_iface_plen_bits;
	return true;
}
#endif /* CONFIG_NET_L2_DECT_ULA */

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
#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT)
			{
				struct dect_net_l2_context *l2_ctx = net_if_l2_data(iface);

				if ((l2_ctx->device_type & DECT_DEVICE_TYPE_FT) != 0) {
					dect_net_l2_ipv6_pt_eth_nd_proxy_remove(
						&list_item->global_ipv6_addr);
				}
			}
#endif
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
				       MIN(ctx->ipv6_prefix_cfg.prefix_len,
					   ipv6_prefix_config->prefix_len) *
					       8U)) {
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

#if defined(CONFIG_NET_L2_DECT_ULA)
	dect_net_l2_ipv6_ula_sync_for_peer(iface, &ctx->local_ipv6_addr, parent_long_rd_id);
#endif

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
#if defined(CONFIG_NET_L2_DECT_ULA)
		/* FT: DECT ULA /64 uses common base + our long RD id (not the child's). */
		dect_net_l2_ipv6_ula_sync_for_peer(iface, &ctx->local_ipv6_addr,
						   ctx->transmitter_long_rd_id);
#endif
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
#if defined(CONFIG_NET_L2_DECT_BR_IPV6_ETH_ND_PROXY_PT)
		{
			struct dect_net_l2_context *l2_ctx = net_if_l2_data(iface);

			if ((l2_ctx->device_type & DECT_DEVICE_TYPE_FT) != 0) {
				dect_net_l2_ipv6_pt_eth_nd_proxy_remove(
					&ass_list_item->global_ipv6_addr);
			}
		}
#endif
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
#if defined(CONFIG_NET_L2_DECT_ULA)
	dect_net_l2_ipv6_ula_remove(iface, ctx);
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
		__ASSERT_NO_MSG(sink_global_prefix.len == DECT_IPV6_PREFIX_LEN_64_BYTES ||
				sink_global_prefix.len == DECT_IPV6_PREFIX_LEN_96_BYTES);
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

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_SINK_ROUTE96)
	if (ctx->ipv6_prefix_cfg.prefix_len == DECT_IPV6_PREFIX_LEN_96_BYTES) {
		dect_net_l2_ipv6_sink_route96_remove(dect_iface, &ctx->ipv6_prefix_cfg);
	}
#endif

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
#if defined(CONFIG_NET_L2_DECT_BR_IPV6_SINK_ROUTE96)
			if (ctx->ipv6_prefix_cfg.prefix_len == DECT_IPV6_PREFIX_LEN_96_BYTES) {
				dect_net_l2_ipv6_sink_route96_add(
					dect_iface, &ctx->ipv6_prefix_cfg);
			}
#endif
		}
	}
#if defined(CONFIG_NET_L2_DECT_ULA)
	/* Replacing sink prefixes removes every on-net prefix, including the ULA /64. */
	dect_net_l2_ipv6_ula_sync_for_peer(dect_iface, &ctx->local_ipv6_addr,
					   ctx->transmitter_long_rd_id);
#endif
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

#if defined(CONFIG_NET_L2_DECT_BR_IPV6_SINK_ROUTE96)
		if (ctx->ipv6_prefix_cfg.prefix_len == DECT_IPV6_PREFIX_LEN_96_BYTES) {
			dect_net_l2_ipv6_sink_route96_remove(iface, &ctx->ipv6_prefix_cfg);
		}
#endif

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
		   (ctx->ipv6_prefix_cfg.prefix_len != ipv6_prefix_config->prefix_len ||
		    !net_ipv6_is_prefix(ctx->ipv6_prefix_cfg.prefix.s6_addr,
					ipv6_prefix_config->prefix.s6_addr,
					MIN(ctx->ipv6_prefix_cfg.prefix_len,
					    ipv6_prefix_config->prefix_len) *
						8U))) {

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
