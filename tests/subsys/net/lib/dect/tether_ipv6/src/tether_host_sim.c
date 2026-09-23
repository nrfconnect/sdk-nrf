/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/byteorder.h>

#include "tether_host_sim.h"
#include "tether_test_eth.h"

#include <ipv6.h>


static const struct in6_addr dhcp_all_servers = {
	.s6_addr = { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0, 0x02 }
};

static void host_ll_from_mac(const uint8_t mac[6], struct in6_addr *ll)
{
	ll->s6_addr[0] = 0xfe;
	ll->s6_addr[1] = 0x80;
	memset(&ll->s6_addr[2], 0, 6);
	ll->s6_addr[8] = mac[0];
	ll->s6_addr[9] = mac[1];
	ll->s6_addr[10] = mac[2];
	ll->s6_addr[11] = 0xff;
	ll->s6_addr[12] = 0xfe;
	ll->s6_addr[13] = mac[3];
	ll->s6_addr[14] = mac[4];
	ll->s6_addr[15] = mac[5];
}

static size_t append_opt(uint8_t *wp, uint16_t code, const void *data, uint16_t dlen)
{
	sys_put_be16(code, wp);
	sys_put_be16(dlen, wp + 2);
	if (dlen > 0U && data != NULL) {
		memcpy(wp + 4, data, dlen);
	}
	return 4U + dlen;
}

static size_t append_client_duid(uint8_t *wp, const uint8_t mac[6])
{
	uint8_t duid[10];
	uint16_t t = sys_cpu_to_be16(DHCPV6_DUID_LL);

	memcpy(duid, &t, 2);
	t = sys_cpu_to_be16(1);
	memcpy(duid + 2, &t, 2);
	memcpy(duid + 4, mac, 6);
	return append_opt(wp, DHCPV6_OPT_CLIENTID, duid, sizeof(duid));
}

static size_t append_ia_na(uint8_t *wp, uint32_t iaid)
{
	uint8_t inner[12];

	sys_put_be32(iaid, inner);
	sys_put_be32(0, inner + 4);
	sys_put_be32(0, inner + 8);
	return append_opt(wp, DHCPV6_OPT_IA_NA, inner, sizeof(inner));
}

static int send_dhcp(struct net_if *eth, struct tether_host_ctx *host, const struct in6_addr *dst,
		     uint8_t mtype, const uint8_t *extra, size_t extra_len)
{
	uint8_t msg[256];
	size_t len = 4U + extra_len;

	msg[0] = mtype;
	memcpy(msg + 1, host->tid, 3);
	if (extra != NULL && extra_len > 0U) {
		memcpy(msg + 4, extra, extra_len);
	}

	return tether_test_eth_inject_udp(eth, &host->ll, dst, htons(DHCPV6_CLIENT_PORT),
					  htons(DHCPV6_SERVER_PORT), msg, len, host->mac);
}

static void host_ctx_init_common(struct tether_host_ctx *host, uint8_t mac_last_octet,
				 uint32_t iaid, const uint8_t tid[3])
{
	static const uint8_t def_mac[6] = { 0x02, 0x11, 0x22, 0x33, 0x44, 0x55 };

	memcpy(host->mac, def_mac, sizeof(def_mac));
	host->mac[5] = mac_last_octet;
	host_ll_from_mac(host->mac, &host->ll);
	host->iaid = iaid;
	memcpy(host->tid, tid, 3U);
	host->server_duid_len = 0U;
}

void tether_host_ctx_init(struct tether_host_ctx *host)
{
	static const uint8_t tid[3] = { 0x12, 0x34, 0x56 };

	host_ctx_init_common(host, 0x55, 1U, tid);
}

/* Second, independent client identity (distinct DUID-LL and link-local
 * address) used to exercise the DHCPv6 server's single-lease-slot
 * "other client" policy while a lease is bound to the primary host.
 */
void tether_host_ctx_init_other(struct tether_host_ctx *host, uint8_t mac_last_octet)
{
	static const uint8_t tid[3] = { 0x22, 0x33, 0x44 };

	host_ctx_init_common(host, mac_last_octet, 2U, tid);
}

int tether_host_install_neighbor(struct net_if *eth, struct tether_host_ctx *host)
{
	struct net_linkaddr lla = {
		.len = TETHER_HOST_MAC_LEN,
		.type = NET_LINK_ETHERNET,
	};
	struct net_nbr *nbr;

	memcpy(lla.addr, host->mac, TETHER_HOST_MAC_LEN);

	/* The fake Ethernet driver never answers real Neighbor Discovery, so
	 * unlike the production code path (which installs a neighbor entry
	 * itself once it grants an address, see dect_tether_ipv6_fwd.c),
	 * tests that expect a reply routed back to a client that was NOT
	 * granted an address (e.g. NoAddrsAvail) must pre-seed the neighbor
	 * cache themselves, or the reply gets stuck waiting on ND forever.
	 */
	(void)net_ipv6_nbr_rm(eth, &host->ll);
	nbr = net_ipv6_nbr_add(eth, &host->ll, &lla, false, NET_IPV6_NBR_STATE_STATIC);
	return (nbr != NULL) ? 0 : -EIO;
}

int tether_host_send_rs(struct net_if *eth, struct tether_host_ctx *host)
{
	return tether_test_eth_inject_rs(eth, &host->ll, host->mac);
}

int tether_host_send_solicit(struct net_if *eth, struct tether_host_ctx *host)
{
	return tether_host_send_solicit_to(eth, host, &dhcp_all_servers);
}

int tether_host_send_solicit_to(struct net_if *eth, struct tether_host_ctx *host,
				const struct in6_addr *server_ll)
{
	uint8_t opts[64];
	size_t olen = 0U;

	olen += append_client_duid(opts + olen, host->mac);
	olen += append_ia_na(opts + olen, host->iaid);
	return send_dhcp(eth, host, server_ll, DHCPV6_MSG_SOLICIT, opts, olen);
}

int tether_host_send_request(struct net_if *eth, struct tether_host_ctx *host,
			     const struct in6_addr *server_ll)
{
	uint8_t opts[96];
	size_t olen = 0U;

	olen += append_client_duid(opts + olen, host->mac);
	if (host->server_duid_len > 0U) {
		olen += append_opt(opts + olen, DHCPV6_OPT_SERVERID, host->server_duid,
				   host->server_duid_len);
	}
	olen += append_ia_na(opts + olen, host->iaid);
	return send_dhcp(eth, host, server_ll, DHCPV6_MSG_REQUEST, opts, olen);
}

int tether_host_send_renew(struct net_if *eth, struct tether_host_ctx *host,
			   const struct in6_addr *server_ll)
{
	uint8_t opts[96];
	size_t olen = 0U;

	olen += append_client_duid(opts + olen, host->mac);
	if (host->server_duid_len > 0U) {
		olen += append_opt(opts + olen, DHCPV6_OPT_SERVERID, host->server_duid,
				   host->server_duid_len);
	}
	olen += append_ia_na(opts + olen, host->iaid);
	return send_dhcp(eth, host, server_ll, DHCPV6_MSG_RENEW, opts, olen);
}

int tether_host_send_rebind(struct net_if *eth, struct tether_host_ctx *host,
			    const struct in6_addr *server_ll)
{
	uint8_t opts[96];
	size_t olen = 0U;

	/* RFC 8415 §18.3.5: Rebind MUST NOT include a Server ID option. */
	olen += append_client_duid(opts + olen, host->mac);
	olen += append_ia_na(opts + olen, host->iaid);
	return send_dhcp(eth, host, server_ll, DHCPV6_MSG_REBIND, opts, olen);
}

int tether_host_send_confirm(struct net_if *eth, struct tether_host_ctx *host,
			     const struct in6_addr *server_ll)
{
	uint8_t opts[96];
	size_t olen = 0U;

	/* RFC 8415 §18.3.3: Confirm MUST NOT include a Server ID option. */
	olen += append_client_duid(opts + olen, host->mac);
	olen += append_ia_na(opts + olen, host->iaid);
	return send_dhcp(eth, host, server_ll, DHCPV6_MSG_CONFIRM, opts, olen);
}

int tether_host_send_info_request(struct net_if *eth, struct tether_host_ctx *host,
				  const struct in6_addr *server_ll, bool with_clientid)
{
	uint8_t opts[32];
	size_t olen = 0U;

	/* RFC 8415 §18.2.6: Info-Request carries no IA_NA; Client ID is only
	 * RECOMMENDED, not required, so callers can test either variant.
	 */
	if (with_clientid) {
		olen += append_client_duid(opts + olen, host->mac);
	}
	return send_dhcp(eth, host, server_ll, DHCPV6_MSG_INFO_REQUEST, opts, olen);
}

int tether_host_send_solicit_no_clientid(struct net_if *eth, struct tether_host_ctx *host,
					 const struct in6_addr *server_ll)
{
	uint8_t opts[32];
	size_t olen = 0U;

	/* Deliberately omit the Client ID: the server must silently drop any
	 * non-Info-Request message that lacks one.
	 */
	olen += append_ia_na(opts + olen, host->iaid);
	return send_dhcp(eth, host, server_ll, DHCPV6_MSG_SOLICIT, opts, olen);
}

int tether_host_send_request_bad_serverid(struct net_if *eth, struct tether_host_ctx *host,
					  const struct in6_addr *server_ll)
{
	static const uint8_t bogus_duid[10] = { 0xff, 0xff, 0xff, 0xff, 0xff,
						 0xff, 0xff, 0xff, 0xff, 0xff };
	uint8_t opts[96];
	size_t olen = 0U;

	/* Server ID deliberately does not match the server's own DUID: the
	 * server must silently drop the Request.
	 */
	olen += append_client_duid(opts + olen, host->mac);
	olen += append_opt(opts + olen, DHCPV6_OPT_SERVERID, bogus_duid, sizeof(bogus_duid));
	olen += append_ia_na(opts + olen, host->iaid);
	return send_dhcp(eth, host, server_ll, DHCPV6_MSG_REQUEST, opts, olen);
}

int tether_host_send_unknown_type(struct net_if *eth, struct tether_host_ctx *host,
				  const struct in6_addr *server_ll)
{
	/* RFC 8415 §7.3: message type 9 is Decline, which this server does not
	 * implement/handle; it must be silently ignored.
	 */
	const uint8_t unhandled_msg_type = 9U;
	uint8_t opts[96];
	size_t olen = 0U;

	olen += append_client_duid(opts + olen, host->mac);
	olen += append_ia_na(opts + olen, host->iaid);
	return send_dhcp(eth, host, server_ll, unhandled_msg_type, opts, olen);
}

int tether_host_wait_tx_growth(int baseline, int timeout_ms)
{
	int64_t end = k_uptime_get() + timeout_ms;

	while (k_uptime_get() < end) {
		if (tether_tx_capture_count > baseline) {
			return tether_tx_capture_count;
		}
		k_msleep(20);
	}

	return -ETIMEDOUT;
}

static bool parse_frame_ipv6(const struct tether_tx_frame *f, const uint8_t **l3, size_t *l3_len,
			     uint8_t *nexthdr)
{
	if (f->len < 14U + 40U) {
		return false;
	}

	if (f->data[12] != 0x86 || f->data[13] != 0xdd) {
		return false;
	}

	*l3 = f->data + 14U;
	*l3_len = f->len - 14U;
	*nexthdr = (*l3)[6];
	return true;
}

bool tether_host_find_ra(int baseline, struct tether_ra_parse *out)
{
	for (int i = baseline; i < tether_tx_capture_count; i++) {
		const uint8_t *l3;
		size_t l3_len;
		uint8_t nh;

		if (!parse_frame_ipv6(&tether_tx_frames[i], &l3, &l3_len, &nh)) {
			continue;
		}
		if (nh != IPPROTO_ICMPV6 || l3_len < 48U) {
			continue;
		}
		if (l3[40] != ICMPV6_RA_TYPE) {
			continue;
		}

		out->flags = l3[45];
		out->router_lifetime = sys_get_be16(l3 + 46);
		out->managed = (out->flags & RA_FLAG_MANAGED) != 0U;
		out->has_pio = false;
		out->has_rdnss = false;

		size_t off = 56U;

		while (off + 2U <= l3_len) {
			uint8_t otype = l3[off];
			uint8_t olen = l3[off + 1];

			if (olen == 0U) {
				break;
			}
			if (off + (size_t)olen * 8U > l3_len) {
				break;
			}
			if (otype == RA_OPT_PREFIX_INFO) {
				out->has_pio = true;
			} else if (otype == RA_OPT_RDNSS) {
				out->has_rdnss = true;
			}
			off += (size_t)olen * 8U;
		}
		return true;
	}

	return false;
}

static bool dhcp_opt_find(const uint8_t *opts, size_t opts_len, uint16_t code,
			  const uint8_t **val, uint16_t *vlen)
{
	size_t off = 0U;

	while (off + 4U <= opts_len) {
		uint16_t oc = sys_get_be16(opts + off);
		uint16_t ol = sys_get_be16(opts + off + 2);

		off += 4U;
		if (off + ol > opts_len) {
			break;
		}
		if (oc == code) {
			*val = opts + off;
			*vlen = ol;
			return true;
		}
		off += ol;
	}

	return false;
}

static void dhcp_parse_iaaddr(const uint8_t *ia_na, uint16_t ia_len, struct tether_dhcp_parse *out)
{
	size_t off = 12U;

	while (off + 4U <= ia_len) {
		uint16_t oc = sys_get_be16(ia_na + off);
		uint16_t ol = sys_get_be16(ia_na + off + 2);
		const uint8_t *val;

		off += 4U;
		if (off + ol > ia_len) {
			break;
		}
		val = ia_na + off;
		if (oc == DHCPV6_OPT_IAADDR && ol >= 24U) {
			struct in6_addr a;

			memcpy(a.s6_addr, val, 16);
			if (net_ipv6_is_ula_addr(&a)) {
				out->ula = a;
				out->have_ula = true;
			} else if (net_ipv6_is_global_addr(&a)) {
				out->gua = a;
				out->have_gua = true;
			}
			out->has_iaaddr = true;
		}
		off += ol;
	}
}

bool tether_host_find_dhcp(int baseline, uint8_t want_type, struct tether_dhcp_parse *out,
			 struct tether_host_ctx *host)
{
	for (int i = baseline; i < tether_tx_capture_count; i++) {
		const uint8_t *l3;
		size_t l3_len;
		uint8_t nh;

		if (!parse_frame_ipv6(&tether_tx_frames[i], &l3, &l3_len, &nh)) {
			continue;
		}
		if (nh != IPPROTO_UDP || l3_len < 48U) {
			continue;
		}

		const uint8_t *udp = l3 + 40U;
		uint16_t sport = sys_get_be16(udp);
		uint16_t dport = sys_get_be16(udp + 2);

		if (sport != DHCPV6_SERVER_PORT && dport != DHCPV6_SERVER_PORT) {
			continue;
		}

		const uint8_t *dhcp = udp + 8U;
		size_t dhcp_len = l3_len - 40U - 8U;

		if (dhcp_len < 4U || dhcp[0] != want_type) {
			continue;
		}

		memset(out, 0, sizeof(*out));
		out->msg_type = dhcp[0];

		const uint8_t *opts = dhcp + 4U;
		size_t opts_len = dhcp_len - 4U;
		const uint8_t *val;
		uint16_t vlen;

		if (host != NULL &&
		    dhcp_opt_find(opts, opts_len, DHCPV6_OPT_SERVERID, &val, &vlen) &&
		    vlen <= sizeof(host->server_duid)) {
			memcpy(host->server_duid, val, vlen);
			host->server_duid_len = vlen;
		}

		if (dhcp_opt_find(opts, opts_len, DHCPV6_OPT_IA_NA, &val, &vlen)) {
			dhcp_parse_iaaddr(val, vlen, out);
		}

		return true;
	}

	return false;
}

static bool ia_na_has_revoked_iaaddr(const uint8_t *ia_na, uint16_t ia_len)
{
	size_t off = 12U;

	while (off + 4U <= ia_len) {
		uint16_t oc = sys_get_be16(ia_na + off);
		uint16_t ol = sys_get_be16(ia_na + off + 2);

		off += 4U;
		if (off + ol > ia_len) {
			break;
		}
		if (oc == DHCPV6_OPT_IAADDR && ol >= 24U) {
			uint32_t pref = sys_get_be32(ia_na + off + 16);
			uint32_t valid = sys_get_be32(ia_na + off + 20);

			if (pref == 0U && valid == 0U) {
				return true;
			}
		}
		off += ol;
	}

	return false;
}

bool tether_host_dhcp_has_revoked_iaaddr(int baseline, uint8_t want_type)
{
	for (int i = baseline; i < tether_tx_capture_count; i++) {
		const uint8_t *l3;
		size_t l3_len;
		uint8_t nh;

		if (!parse_frame_ipv6(&tether_tx_frames[i], &l3, &l3_len, &nh)) {
			continue;
		}
		if (nh != IPPROTO_UDP || l3_len < 48U) {
			continue;
		}

		const uint8_t *udp = l3 + 40U;
		const uint8_t *dhcp = udp + 8U;
		size_t dhcp_len = l3_len - 40U - 8U;

		if (dhcp_len < 4U || dhcp[0] != want_type) {
			continue;
		}

		const uint8_t *opts = dhcp + 4U;
		size_t opts_len = dhcp_len - 4U;
		size_t off = 0U;

		while (off + 4U <= opts_len) {
			uint16_t oc = sys_get_be16(opts + off);
			uint16_t ol = sys_get_be16(opts + off + 2);
			const uint8_t *val;

			off += 4U;
			if (off + ol > opts_len) {
				break;
			}
			val = opts + off;
			if (oc == DHCPV6_OPT_IA_NA && ia_na_has_revoked_iaaddr(val, ol)) {
				return true;
			}
			off += ol;
		}
	}

	return false;
}
