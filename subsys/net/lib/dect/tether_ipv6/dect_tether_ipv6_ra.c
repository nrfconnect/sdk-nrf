/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <sys/types.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/icmp.h>
#if defined(CONFIG_NET_L2_ETHERNET)
#include <zephyr/net/ethernet.h>
#endif

#include <net/dect/dect_net_l2.h>

#include "dect_tether_ipv6_int.h"

#if defined(CONFIG_DECT_TETHER_IPV6_ETH_UNSOLICITED_NA)
#include "ipv6.h"
#endif

LOG_MODULE_REGISTER(dect_tether_ipv6_ra, CONFIG_DECT_TETHER_IPV6_LOG_LEVEL);

#define IPV6_OFF_SRC		8
#define IPV6_OFF_DST		24
#define IPV6_ADDR_LEN		16
#define IPV6_HDR_SIZE		40
#define ICMPV6_RA_HOP_LIMIT	255

#if defined(CONFIG_DECT_TETHER_IPV6_RA_RDNSS)
#define RA_RDNSS_EXTRA (8U + 2U * IPV6_ADDR_LEN)
#else
#define RA_RDNSS_EXTRA 0U
#endif

/** RFC 4861 §4.6.2: PIO option size is always 32 bytes (len field = 4). */
#define RA_PIO_SIZE		32U

#if defined(CONFIG_DECT_TETHER_IPV6_RA_PREFIX_INFO)
#define RA_PIO_EXTRA		(2U * RA_PIO_SIZE)
#else
#define RA_PIO_EXTRA		0U
#endif

struct ra_hdr {
	uint8_t cur_hop_limit;
	uint8_t flags;
	uint16_t router_lifetime;
	uint32_t reachable_time;
	uint32_t retrans_timer;
} __packed;

struct ra_opt_lla_eth {
	uint8_t type;
	uint8_t len;
	uint8_t mac[6];
} __packed;

struct ra_opt_mtu {
	uint8_t type;
	uint8_t len;
	uint16_t reserved;
	uint32_t mtu;
} __packed;

struct ra_opt_pio {
	uint8_t  type;
	uint8_t  len;
	uint8_t  prefix_len;
	uint8_t  flags;
	uint32_t valid_lifetime;
	uint32_t preferred_lifetime;
	uint32_t reserved;
	uint8_t  prefix[IPV6_ADDR_LEN];
} __packed;

static const uint8_t ipv6_allnodes_ll[16] = {
	0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

static const struct net_in6_addr ipv6_mcast_all_routers = {
	.s6_addr = { 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 }
};

static struct {
	struct k_work ra_work;
	struct k_work_delayable ra_periodic;
#if defined(CONFIG_DECT_TETHER_IPV6_ETH_UNSOLICITED_NA)
	struct k_work_delayable na_unsol_periodic;
#endif
	struct net_icmp_ctx rs_ctx;
	struct net_mgmt_event_callback mgmt_cb_if;
	struct net_mgmt_event_callback mgmt_cb_ipv6;
	/* true while the DECT uplink is known to be present. Set on
	 * DECT_ASSOCIATION_CREATED (parent) and cleared on
	 * DECT_ASSOCIATION_RELEASED (parent). Used to guard the periodic RA
	 * from advertising router_lifetime=0 due to a transient false return
	 * from dect_net_l2_parent_ipv6_addr_get() at the 600-second tick.
	 */
	atomic_t uplink_present;
	bool inited;
} bh_ra;

#if defined(CONFIG_DECT_TETHER_IPV6_RA_RDNSS)
static struct net_in6_addr bh_ra_rdnss[2];
static uint8_t bh_ra_rdnss_cnt;

static void bh_ra_rdnss_cfg_load(void)
{
	struct net_in6_addr a2;

	bh_ra_rdnss_cnt = 0U;
	if (sizeof(CONFIG_DECT_TETHER_IPV6_RA_RDNSS_ADDR1) > 1U &&
	    CONFIG_DECT_TETHER_IPV6_RA_RDNSS_ADDR1[0] != '\0') {
		if (net_addr_pton(AF_INET6, CONFIG_DECT_TETHER_IPV6_RA_RDNSS_ADDR1,
				  &bh_ra_rdnss[0]) != 0) {
			LOG_WRN("RDNSS_ADDR1 parse failed");
		} else {
			bh_ra_rdnss_cnt = 1U;
		}
	}
	if (sizeof(CONFIG_DECT_TETHER_IPV6_RA_RDNSS_ADDR2) > 1U &&
	    CONFIG_DECT_TETHER_IPV6_RA_RDNSS_ADDR2[0] != '\0') {
		if (net_addr_pton(AF_INET6, CONFIG_DECT_TETHER_IPV6_RA_RDNSS_ADDR2,
				  &a2) != 0) {
			LOG_WRN("RDNSS_ADDR2 parse failed");
		} else if (bh_ra_rdnss_cnt == 0U) {
			bh_ra_rdnss[0] = a2;
			bh_ra_rdnss_cnt = 1U;
		} else if (memcmp(bh_ra_rdnss[0].s6_addr, a2.s6_addr, IPV6_ADDR_LEN) != 0) {
			bh_ra_rdnss[1] = a2;
			bh_ra_rdnss_cnt = 2U;
		}
	}
	if (bh_ra_rdnss_cnt == 0U) {
		LOG_WRN("RDNSS enabled but no valid DNS addresses");
	}
}
#endif

#if defined(CONFIG_DECT_TETHER_IPV6_ETH_UNSOLICITED_NA)
static void host_eth_send_unsolicited_na(struct net_if *iface)
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

		/* ULA/GUA on this iface are for the tethered host (DHCPv6); only LL is ours. */
		if (!net_ipv6_is_ll_addr(addr)) {
			continue;
		}

		if (net_ipv6_send_na(iface, addr, &allnodes, addr,
				     NET_ICMPV6_NA_FLAG_ROUTER |
					     NET_ICMPV6_NA_FLAG_OVERRIDE) < 0) {
			char abuf[NET_IPV6_ADDR_LEN];

			(void)net_addr_ntop(AF_INET6, addr, abuf, sizeof(abuf));
			LOG_WRN("host NA: unsolicited send failed for %s", abuf);
		} else {
			LOG_DBG("host NA: unsolicited if=%d", net_if_get_by_iface(iface));
		}
	}
}

static void na_unsol_periodic_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	host_eth_send_unsolicited_na(dect_tether_ipv6_host_eth_iface());
	(void)k_work_reschedule(
		&bh_ra.na_unsol_periodic,
		K_MSEC(CONFIG_DECT_TETHER_IPV6_ETH_UNSOLICITED_NA_INTERVAL_MS));
}

static void na_unsol_schedule(struct net_if *iface)
{
	host_eth_send_unsolicited_na(iface);
	(void)k_work_reschedule(
		&bh_ra.na_unsol_periodic,
		K_MSEC(CONFIG_DECT_TETHER_IPV6_ETH_UNSOLICITED_NA_INTERVAL_MS));
}

static void na_unsol_cancel(void)
{
	k_work_cancel_delayable(&bh_ra.na_unsol_periodic);
}
#endif /* CONFIG_DECT_TETHER_IPV6_ETH_UNSOLICITED_NA */

static uint16_t ra_icmpv6_checksum(const uint8_t *src, const uint8_t *dst, const uint8_t *icmp_msg,
				   size_t icmp_len)
{
	uint32_t sum = 0;

	for (size_t i = 0; i < 16; i += 2) {
		sum += (uint32_t)((src[i] << 8) | src[i + 1]);
		sum += (uint32_t)((dst[i] << 8) | dst[i + 1]);
	}
	sum += (uint32_t)((icmp_len >> 16) & 0xffffU);
	sum += (uint32_t)(icmp_len & 0xffffU);
	sum += 58;
	for (size_t i = 0; i < icmp_len; i += 2) {
		if (i == 2) {
			continue;
		}
		if (i + 1 < icmp_len) {
			sum += (uint32_t)((icmp_msg[i] << 8) | icmp_msg[i + 1]);
		} else {
			sum += (uint32_t)(icmp_msg[i] << 8);
		}
	}
	while (sum >> 16) {
		sum = (sum & 0xffffU) + (sum >> 16);
	}
	return (uint16_t)~sum;
}

static int host_ra_tx_netif(struct net_if *iface, const uint8_t *buf, size_t len,
			    const uint8_t *dst_mac_override)
{
	struct net_pkt *pkt;
	int tx_ret;

	if (iface == NULL || len == 0U) {
		return -EINVAL;
	}

	pkt = net_pkt_alloc_with_buffer(iface, len, NET_AF_INET6, NET_IPPROTO_RAW, K_MSEC(100));
	if (pkt == NULL) {
		LOG_WRN("RA TX alloc failed if=%d", net_if_get_by_iface(iface));
		return -ENOMEM;
	}
	if (net_pkt_write(pkt, buf, len) < 0) {
		net_pkt_unref(pkt);
		return -EIO;
	}
#if defined(CONFIG_NET_L2_ETHERNET)
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		const struct net_in6_addr *ip6_dst =
			(const struct net_in6_addr *)&buf[IPV6_OFF_DST];
		struct net_eth_addr dst_mac;

		net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_IPV6);
		if (dst_mac_override != NULL) {
			memcpy(dst_mac.addr, dst_mac_override, sizeof(dst_mac.addr));
		} else {
			net_eth_ipv6_mcast_to_mac_addr(ip6_dst, &dst_mac);
		}
		(void)net_linkaddr_set(net_pkt_lladdr_dst(pkt), dst_mac.addr, sizeof(dst_mac.addr));
		(void)net_linkaddr_copy(net_pkt_lladdr_src(pkt), net_if_get_link_addr(iface));
	}
#endif
	net_pkt_cursor_init(pkt);
	tx_ret = net_try_send_data(pkt, K_NO_WAIT);
	if (tx_ret < 0) {
		LOG_WRN("RA net_try_send_data failed %d", tx_ret);
		net_pkt_unref(pkt);
		return -EIO;
	}
	return 0;
}

static ssize_t ra_build_minimal(uint8_t *buf, size_t buf_cap, struct net_if *host,
				const struct net_in6_addr *src_ll,
				const struct net_in6_addr *ip_dst,
				uint16_t router_lifetime, uint32_t rdnss_lifetime, uint8_t ra_flags,
				const struct net_in6_addr *pio_pfx, uint8_t pio_cnt,
				uint32_t pio_valid_lt, uint32_t pio_preferred_lt)
{
	size_t icmp_payload = 4U + sizeof(struct ra_hdr);
	size_t lla_sz = 0U;
#if defined(CONFIG_NET_L2_ETHERNET)
	const struct net_linkaddr *hw;

	if (host != NULL && net_if_l2(host) == &NET_L2_GET_NAME(ETHERNET)) {
		hw = net_if_get_link_addr(host);
		if (hw != NULL && hw->len == 6U) {
			lla_sz = sizeof(struct ra_opt_lla_eth);
			icmp_payload += lla_sz;
		}
	}
#endif
#if defined(CONFIG_DECT_TETHER_IPV6_RA_MTU_OPTION)
	icmp_payload += sizeof(struct ra_opt_mtu);
#endif
#if defined(CONFIG_DECT_TETHER_IPV6_RA_RDNSS)
	if (bh_ra_rdnss_cnt > 0U) {
		icmp_payload += 8U + (size_t)bh_ra_rdnss_cnt * IPV6_ADDR_LEN;
	}
#endif
	icmp_payload += (size_t)pio_cnt * RA_PIO_SIZE;

	if (buf_cap < IPV6_HDR_SIZE + icmp_payload) {
		return -1;
	}

	memset(buf, 0, IPV6_HDR_SIZE);
	buf[0] = 0x60;
	buf[4] = (uint8_t)((icmp_payload >> 8) & 0xff);
	buf[5] = (uint8_t)(icmp_payload & 0xff);
	buf[6] = NET_IPPROTO_ICMPV6;
	buf[7] = ICMPV6_RA_HOP_LIMIT;
	memcpy(&buf[IPV6_OFF_SRC], src_ll->s6_addr, IPV6_ADDR_LEN);
	if (ip_dst != NULL) {
		memcpy(&buf[IPV6_OFF_DST], ip_dst->s6_addr, IPV6_ADDR_LEN);
	} else {
		memcpy(&buf[IPV6_OFF_DST], ipv6_allnodes_ll, IPV6_ADDR_LEN);
	}

	{
		uint8_t *icmp = &buf[IPV6_HDR_SIZE];
		struct ra_hdr *rah;
		uint8_t *wp;
		uint16_t chksum;
		uint16_t rl;

		icmp[0] = ICMPV6_RA_TYPE;
		icmp[1] = 0;
		icmp[2] = 0;
		icmp[3] = 0;

		rah = (struct ra_hdr *)(icmp + 4);
		rah->cur_hop_limit = 64;
		rah->flags = ra_flags;
		rl = router_lifetime;
		rah->router_lifetime = net_htons(rl);
#if CONFIG_DECT_TETHER_IPV6_RA_REACHABLE_TIME_MS > 0
		rah->reachable_time =
			net_htonl((uint32_t)CONFIG_DECT_TETHER_IPV6_RA_REACHABLE_TIME_MS);
#else
		rah->reachable_time = 0;
#endif
#if CONFIG_DECT_TETHER_IPV6_RA_RETRANS_TIMER_MS > 0
		rah->retrans_timer =
			net_htonl((uint32_t)CONFIG_DECT_TETHER_IPV6_RA_RETRANS_TIMER_MS);
#else
		rah->retrans_timer = 0;
#endif

		wp = (uint8_t *)rah + sizeof(struct ra_hdr);

#if defined(CONFIG_NET_L2_ETHERNET)
		if (lla_sz != 0U) {
			struct ra_opt_lla_eth *lla = (struct ra_opt_lla_eth *)wp;
			const struct net_linkaddr *hw = net_if_get_link_addr(host);

			lla->type = (uint8_t)RA_OPT_SRC_LL_ADDR;
			lla->len = 1U;
			memcpy(lla->mac, hw->addr, 6);
			wp += sizeof(struct ra_opt_lla_eth);
		}
#endif

#if defined(CONFIG_DECT_TETHER_IPV6_RA_MTU_OPTION)
		{
			struct ra_opt_mtu *mo = (struct ra_opt_mtu *)wp;

			mo->type = (uint8_t)RA_OPT_MTU;
			mo->len = 1U;
			mo->reserved = 0;
			mo->mtu = net_htonl(CONFIG_NET_L2_DECT_MTU);
			wp += sizeof(struct ra_opt_mtu);
		}
#endif

#if defined(CONFIG_DECT_TETHER_IPV6_RA_RDNSS)
		if (bh_ra_rdnss_cnt > 0U) {
			const size_t rdnss_len = 8U + (size_t)bh_ra_rdnss_cnt * IPV6_ADDR_LEN;
			const uint8_t rdnss_len_units = (uint8_t)(rdnss_len / 8U);

			wp[0] = (uint8_t)RA_OPT_RDNSS;
			wp[1] = rdnss_len_units;
			wp[2] = 0;
			wp[3] = 0;
			{
				uint32_t lt = net_htonl(rdnss_lifetime);

				memcpy(&wp[4], &lt, sizeof(lt));
			}
			wp += 8;
			for (size_t ri = 0; ri < (size_t)bh_ra_rdnss_cnt; ri++) {
				memcpy(wp, bh_ra_rdnss[ri].s6_addr, IPV6_ADDR_LEN);
				wp += IPV6_ADDR_LEN;
			}
		}
#endif

		/* PIO: L=0, A=0 — prefix hint only (not on-link, no SLAAC); see Kconfig help. */
		for (uint8_t pi = 0U; pi < pio_cnt && pio_pfx != NULL; pi++) {
			struct ra_opt_pio *pio = (struct ra_opt_pio *)wp;

			pio->type             = (uint8_t)RA_OPT_PREFIX_INFO;
			pio->len              = (uint8_t)(RA_PIO_SIZE / 8U);
			pio->prefix_len       = 64U;
			pio->flags            = 0U; /* L=0 (not on-link), A=0 (no SLAAC) */
			pio->valid_lifetime     = net_htonl(pio_valid_lt);
			pio->preferred_lifetime = net_htonl(pio_preferred_lt);
			pio->reserved           = 0U;
			/* Copy first 64 bits; host part is already zeroed by caller. */
			memcpy(pio->prefix, pio_pfx[pi].s6_addr, IPV6_ADDR_LEN);
			wp += RA_PIO_SIZE;
		}

		chksum = ra_icmpv6_checksum(&buf[IPV6_OFF_SRC], &buf[IPV6_OFF_DST], icmp,
					   icmp_payload);
		icmp[2] = (uint8_t)((chksum >> 8) & 0xff);
		icmp[3] = (uint8_t)(chksum & 0xff);
	}

	return (ssize_t)(IPV6_HDR_SIZE + icmp_payload);
}

/* forced_rl < 0: derive from uplink; else use forced_rl (0 = withdraw default route). */
/* ip_dst NULL → multicast to ff02::1; else unicast.  dst_mac_override only when unicast. */
static void host_ra_emit_forced(const struct net_in6_addr *ip_dst,
				const uint8_t *dst_mac_override, int16_t forced_rl)
{
	struct net_if *host = dect_tether_ipv6_host_eth_iface();
	const struct net_in6_addr *src_ll;
	uint8_t buf[IPV6_HDR_SIZE + 4 + sizeof(struct ra_hdr) + sizeof(struct ra_opt_lla_eth) +
		     sizeof(struct ra_opt_mtu) + RA_RDNSS_EXTRA + RA_PIO_EXTRA];
	ssize_t pkt_len;
	int send_res;
	uint16_t rl = 0U;
	uint8_t flags = 0U;
#if defined(CONFIG_DECT_TETHER_IPV6_RA_PREFIX_INFO)
	struct net_in6_addr pio_pfx[2];
	uint8_t pio_cnt = 0U;
#endif

	if (!dect_tether_ipv6_host_eth_ready(host)) {
		LOG_DBG("RA skip (Ethernet not ready)");
		return;
	}
	src_ll = net_if_ipv6_get_ll(host, NET_ADDR_PREFERRED);
	if (src_ll == NULL) {
		LOG_WRN("RA skip (no link-local on host iface)");
		return;
	}

	{
		bool uplink;
		struct net_in6_addr parent_dummy;
		uint32_t rdl;

		uplink = dect_net_l2_parent_ipv6_addr_get(&parent_dummy);

		/* Use the cached uplink_present flag as a tie-breaker: if the
		 * DECT stack has not yet signalled a parent-lost event but the
		 * query above returned false (transient during re-association),
		 * keep advertising a valid router lifetime so the tethered PC
		 * does not lose its default route at the periodic 600-second tick.
		 */
		if (!uplink && atomic_get(&bh_ra.uplink_present)) {
			uplink = true;
		}

		if (forced_rl >= 0) {
			rl = (uint16_t)forced_rl;
#if defined(CONFIG_DECT_TETHER_IPV6_RA_RDNSS)
			rdl = (forced_rl > 0) ?
				      (uint32_t)CONFIG_DECT_TETHER_IPV6_RA_RDNSS_LIFETIME :
				      0U;
#else
			rdl = 0U;
#endif
			flags = (forced_rl > 0 &&
				 IS_ENABLED(CONFIG_DECT_TETHER_IPV6_RA_MANAGED)) ?
					RA_FLAG_MANAGED :
					0;
		} else if (uplink) {
			rl = (uint16_t)MIN(65535, CONFIG_DECT_TETHER_IPV6_RA_ROUTER_LIFETIME);
#if defined(CONFIG_DECT_TETHER_IPV6_RA_RDNSS)
			rdl = (uint32_t)CONFIG_DECT_TETHER_IPV6_RA_RDNSS_LIFETIME;
#else
			rdl = 0U;
#endif
			flags = IS_ENABLED(CONFIG_DECT_TETHER_IPV6_RA_MANAGED) ?
					RA_FLAG_MANAGED : 0;
		} else {
			rl = 0U;
			rdl = 0U;
			flags = 0U;
		}

#if defined(CONFIG_DECT_TETHER_IPV6_RA_PREFIX_INFO)
		if (uplink && (forced_rl < 0 || forced_rl > 0)) {
			struct net_in6_addr ula, gua;
			bool have_ula, have_gua;

			have_ula = false;
			have_gua = false;
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_SERVER)
			if (!dect_tether_ipv6_dhcpv6_srv_tether_pio_addrs(&ula, &gua, &have_ula,
									  &have_gua))
#endif
			{
				struct net_if *dect =
					net_if_get_first_by_type(&NET_L2_GET_NAME(DECT));

				(void)dect_tether_ipv6_dect_addrs_get(dect, &ula, &gua, &have_ula,
								      &have_gua);
			}

			if (have_gua) {
				pio_pfx[pio_cnt] = gua;
				memset(&pio_pfx[pio_cnt].s6_addr[8], 0, 8);
				pio_cnt++;
			}
			if (have_ula) {
				pio_pfx[pio_cnt] = ula;
				memset(&pio_pfx[pio_cnt].s6_addr[8], 0, 8);
				pio_cnt++;
			}
		}
#endif

		pkt_len = ra_build_minimal(buf, sizeof(buf), host, src_ll, ip_dst, rl, rdl, flags,
#if defined(CONFIG_DECT_TETHER_IPV6_RA_PREFIX_INFO)
					   pio_cnt > 0U ? pio_pfx : NULL, pio_cnt,
					   CONFIG_DECT_TETHER_IPV6_RA_PIO_VALID_LIFETIME,
					   CONFIG_DECT_TETHER_IPV6_RA_PIO_PREFERRED_LIFETIME
#else
					   NULL, 0U, 0U, 0U
#endif
					   );
	}
	if (pkt_len <= 0) {
		return;
	}
	send_res = host_ra_tx_netif(host, buf, (size_t)pkt_len, dst_mac_override);
	if (ip_dst != NULL) {
		char dst_abuf[NET_IPV6_ADDR_LEN];

		(void)net_addr_ntop(AF_INET6, ip_dst, dst_abuf, sizeof(dst_abuf));
		LOG_INF("RA unicast ifindex=%d ret=%d dst=%s "
			"router_lifetime=%us reachable_time=%ums "
			"flags=0x%02x(M=%d) PIO=%u",
			net_if_get_by_iface(host), send_res, dst_abuf,
			(unsigned int)rl,
			(unsigned int)CONFIG_DECT_TETHER_IPV6_RA_REACHABLE_TIME_MS,
			(unsigned int)flags,
			(flags & RA_FLAG_MANAGED) ? 1 : 0,
#if defined(CONFIG_DECT_TETHER_IPV6_RA_PREFIX_INFO)
			(unsigned int)pio_cnt
#else
			0U
#endif
			);
	} else {
		LOG_INF("RA ifindex=%d ret=%d "
			"router_lifetime=%us reachable_time=%ums retrans=%ums "
			"flags=0x%02x(M=%d) "
			"opts: SLLAO=1 MTU=%s(%u) RDNSS=%s PIO=%u",
			net_if_get_by_iface(host), send_res,
			(unsigned int)rl,
			(unsigned int)CONFIG_DECT_TETHER_IPV6_RA_REACHABLE_TIME_MS,
			(unsigned int)CONFIG_DECT_TETHER_IPV6_RA_RETRANS_TIMER_MS,
			(unsigned int)flags,
			(flags & RA_FLAG_MANAGED) ? 1 : 0,
			IS_ENABLED(CONFIG_DECT_TETHER_IPV6_RA_MTU_OPTION) ? "y" : "n",
			(unsigned int)(IS_ENABLED(CONFIG_DECT_TETHER_IPV6_RA_MTU_OPTION)
				? CONFIG_NET_L2_DECT_MTU : 0U),
			IS_ENABLED(CONFIG_DECT_TETHER_IPV6_RA_RDNSS) ? "y" : "n",
#if defined(CONFIG_DECT_TETHER_IPV6_RA_PREFIX_INFO)
			(unsigned int)pio_cnt
#else
			0U
#endif
			);
	}
}

static void host_ra_emit(const struct net_in6_addr *ip_dst, const uint8_t *dst_mac_override)
{
	host_ra_emit_forced(ip_dst, dst_mac_override, -1);
}

static void ra_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	host_ra_emit(NULL, NULL);
}

static void ra_periodic_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	k_work_submit(&bh_ra.ra_work);
#if CONFIG_DECT_TETHER_IPV6_RA_UNSOLICIT_MS > 0
	(void)k_work_reschedule(
		&bh_ra.ra_periodic,
		K_MSEC(CONFIG_DECT_TETHER_IPV6_RA_UNSOLICIT_MS));
#endif
}

static void host_leg_join_all_routers_mcast(struct net_if *iface)
{
	struct net_if *ifp = iface;
	struct net_if_mcast_addr *m;

	if (iface == NULL || net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}
	m = net_if_ipv6_maddr_lookup(&ipv6_mcast_all_routers, &ifp);
	if (m == NULL) {
		m = net_if_ipv6_maddr_add(iface, &ipv6_mcast_all_routers);
	}
	if (m != NULL) {
		net_if_ipv6_maddr_join(iface, m);
	}
}

static void host_leg_leave_all_routers_mcast(struct net_if *iface)
{
	struct net_if *ifp = iface;
	struct net_if_mcast_addr *m;

	if (iface == NULL) {
		return;
	}
	m = net_if_ipv6_maddr_lookup(&ipv6_mcast_all_routers, &ifp);
	if (m != NULL) {
		net_if_ipv6_maddr_leave(iface, m);
		(void)net_if_ipv6_maddr_rm(iface, &ipv6_mcast_all_routers);
	}
}

static void bh_ra_mgmt_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			       struct net_if *iface)
{
#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	const struct net_in6_addr *na = (const struct net_in6_addr *)cb->info;
#else
	ARG_UNUSED(cb);
#endif

	if (iface == NULL || net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (mgmt_event == NET_EVENT_IF_UP) {
		host_leg_join_all_routers_mcast(iface);
		k_work_submit(&bh_ra.ra_work);
#if CONFIG_DECT_TETHER_IPV6_RA_UNSOLICIT_MS > 0
		(void)k_work_reschedule(
			&bh_ra.ra_periodic,
			K_MSEC(CONFIG_DECT_TETHER_IPV6_RA_UNSOLICIT_MS));
#endif
#if defined(CONFIG_DECT_TETHER_IPV6_ETH_UNSOLICITED_NA)
		na_unsol_schedule(iface);
#endif
	} else if (mgmt_event == NET_EVENT_IF_DOWN) {
#if CONFIG_DECT_TETHER_IPV6_RA_UNSOLICIT_MS > 0
		k_work_cancel_delayable(&bh_ra.ra_periodic);
#endif
#if defined(CONFIG_DECT_TETHER_IPV6_ETH_UNSOLICITED_NA)
		na_unsol_cancel();
#endif
		host_leg_leave_all_routers_mcast(iface);
	} else if (mgmt_event == NET_EVENT_IPV6_DAD_SUCCEED) {
#if IS_ENABLED(CONFIG_NET_MGMT_EVENT_INFO)
		if (na != NULL && net_ipv6_is_ll_addr(na)) {
			k_work_submit(&bh_ra.ra_work);
		}
#endif
	} else if (mgmt_event == NET_EVENT_IPV6_ADDR_ADD) {
#if IS_ENABLED(CONFIG_NET_MGMT_EVENT_INFO)
		if (na != NULL && net_ipv6_is_ll_addr(na)) {
			k_work_submit(&bh_ra.ra_work);
		}
#endif
	}
}

static enum net_verdict on_router_solicitation(struct net_icmp_ctx *icmp_ctx, struct net_pkt *pkt,
					       struct net_icmp_ip_hdr *ip_hdr,
					       struct net_icmp_hdr *icmp_hdr, void *user_data)
{
	struct net_if *rx;

	ARG_UNUSED(icmp_ctx);
	ARG_UNUSED(icmp_hdr);
	ARG_UNUSED(user_data);

	rx = net_pkt_iface(pkt);
	if (rx == NULL || net_if_l2(rx) != &NET_L2_GET_NAME(ETHERNET)) {
		return NET_CONTINUE;
	}

#if IS_ENABLED(CONFIG_DECT_TETHER_IPV6_RA_RS_UNICAST)
	if (ip_hdr != NULL && ip_hdr->ipv6 != NULL) {
		struct net_in6_addr rs_src;
		struct net_linkaddr *lla;

		net_ipv6_addr_copy_raw(rs_src.s6_addr, ip_hdr->ipv6->src);

		if (!net_ipv6_is_addr_unspecified(&rs_src) &&
		    !net_ipv6_is_addr_mcast(&rs_src)) {
			lla = net_pkt_lladdr_src(pkt);
			if (lla != NULL && lla->len >= 6) {
				host_ra_emit(&rs_src, lla->addr);
			} else {
				host_ra_emit(&rs_src, NULL);
			}
		}
	}
#endif

	k_work_submit(&bh_ra.ra_work);
	return NET_OK;
}

int dect_tether_ipv6_ra_init(void)
{
	int ret;

	if (bh_ra.inited) {
		return 0;
	}

	memset(&bh_ra, 0, sizeof(bh_ra));
	k_work_init(&bh_ra.ra_work, ra_work_handler);
	k_work_init_delayable(&bh_ra.ra_periodic, ra_periodic_handler);
#if defined(CONFIG_DECT_TETHER_IPV6_ETH_UNSOLICITED_NA)
	k_work_init_delayable(&bh_ra.na_unsol_periodic, na_unsol_periodic_handler);
#endif
	ret = net_icmp_init_ctx(&bh_ra.rs_ctx, NET_AF_INET6, ICMPV6_RS_TYPE, 0,
				on_router_solicitation);
	if (ret < 0) {
		LOG_ERR("RS icmp ctx failed %d", ret);
		return ret;
	}

	net_mgmt_init_event_callback(&bh_ra.mgmt_cb_if, bh_ra_mgmt_handler,
				     NET_EVENT_IF_UP | NET_EVENT_IF_DOWN);
	net_mgmt_add_event_callback(&bh_ra.mgmt_cb_if);
	net_mgmt_init_event_callback(&bh_ra.mgmt_cb_ipv6, bh_ra_mgmt_handler,
				     NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_DAD_SUCCEED);
	net_mgmt_add_event_callback(&bh_ra.mgmt_cb_ipv6);

#if defined(CONFIG_DECT_TETHER_IPV6_RA_RDNSS)
	bh_ra_rdnss_cfg_load();
#endif

#if defined(CONFIG_NET_L2_ETHERNET)
	{
		struct net_if *eth = dect_tether_ipv6_host_eth_iface();

		if (eth != NULL && net_if_is_up(eth)) {
			host_leg_join_all_routers_mcast(eth);
			k_work_submit(&bh_ra.ra_work);
#if CONFIG_DECT_TETHER_IPV6_RA_UNSOLICIT_MS > 0
			(void)k_work_reschedule(
				&bh_ra.ra_periodic,
				K_MSEC(CONFIG_DECT_TETHER_IPV6_RA_UNSOLICIT_MS));
#endif
#if defined(CONFIG_DECT_TETHER_IPV6_ETH_UNSOLICITED_NA)
			na_unsol_schedule(eth);
#endif
		}
	}
#endif

	bh_ra.inited = true;
	return 0;
}

void dect_tether_ipv6_ra_deinit(void)
{
	if (!bh_ra.inited) {
		return;
	}
	net_mgmt_del_event_callback(&bh_ra.mgmt_cb_if);
	net_mgmt_del_event_callback(&bh_ra.mgmt_cb_ipv6);
	(void)net_icmp_cleanup_ctx(&bh_ra.rs_ctx);
	k_work_cancel_delayable(&bh_ra.ra_periodic);
#if defined(CONFIG_DECT_TETHER_IPV6_ETH_UNSOLICITED_NA)
	k_work_cancel_delayable(&bh_ra.na_unsol_periodic);
#endif
#if defined(CONFIG_NET_L2_ETHERNET)
	{
		struct net_if *eth = dect_tether_ipv6_host_eth_iface();

		if (eth != NULL) {
			host_leg_leave_all_routers_mcast(eth);
		}
	}
#endif
	memset(&bh_ra, 0, sizeof(bh_ra));
}

#if defined(CONFIG_DECT_TETHER_IPV6_RA)
void dect_tether_ipv6_ra_kick(void)
{
	if (bh_ra.inited) {
		k_work_submit(&bh_ra.ra_work);
	}
}
#endif

void dect_tether_ipv6_ra_set_uplink(bool present)
{
	atomic_set(&bh_ra.uplink_present, present ? 1 : 0);
}
