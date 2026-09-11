/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <net/dect/dect_net_l2.h>

#include "dect_tether_ipv6_int.h"

LOG_MODULE_REGISTER(dect_tether_ipv6_dhcpv6_srv, CONFIG_DECT_TETHER_IPV6_LOG_LEVEL);


/* All_DHCP_Relay_Agents_and_Servers — clients send Solicit / Information-request here
 * (RFC 8415 §7.1).
 */
#define DHCPV6_MCAST_ALL_SERVERS "ff02::1:2"


#define RECV_BUF_SIZE			1536
#define MAX_CLIENT_DUID_LEN		128
#define MAX_MSG				1400

#define DHCPV6_THREAD_PRIO K_PRIO_COOP(CONFIG_DECT_TETHER_IPV6_DHCPV6_THREAD_PRIO)

K_THREAD_STACK_DEFINE(dhcpv6_srv_stack, CONFIG_DECT_TETHER_IPV6_DHCPV6_THREAD_STACK_SIZE);

/* RX/TX buffers must not live on dhcp_thread stack (RECV_BUF_SIZE + MAX_MSG > default stack). */
static uint8_t dhcp_msg_rbuf[RECV_BUF_SIZE];
static uint8_t dhcp_msg_sbuf[MAX_MSG];

static struct {
	struct k_thread thread;
	k_tid_t tid;
	atomic_t stop;
	int sock; /* valid while thread poll loop runs; shutdown from deinit */
	uint8_t server_duid[10];
	uint8_t server_duid_len;
	struct k_mutex sock_mtx;
} dhcp;

struct dhcp_lease_saved {
	bool bound;
	struct sockaddr_in6 cli;
	uint8_t client_duid[MAX_CLIENT_DUID_LEN];
	uint16_t client_duid_len;
	uint32_t iaid;
	uint8_t tid[3];
	struct net_in6_addr ula, gua;
	bool have_ula, have_gua;
};

/* Single active lease slot (plus one stale generation for revoke on disconnect).
 * Only one tether host (DUID+IAID) may hold ULA/GUA at a time; see
 * dhcp_client_matches_lease().
 */
static struct dhcp_lease_saved dhcp_lease;
static struct dhcp_lease_saved dhcp_lease_stale;
static bool dhcp_lease_stale_valid;

static atomic_t dhcp_pending_revoke;

/* 1 while DECT uplink is present, 0 while it is absent. Initialised to 1
 * (the server starts only after the DECT stack is up).
 */
static atomic_t dhcp_uplink_present = ATOMIC_INIT(1);
/* Delayed-work item used to debounce the uplink-lost revoke. Brief DECT
 * re-associations (channel change, handover) must not trigger a lease revoke
 * that immediately deprecates the client's preferred address.
 */
static struct k_work_delayable dhcp_revoke_dwork;

static void dhcp_revoke_dwork_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	if (!atomic_get(&dhcp_uplink_present)) {
		if (dhcp_lease.bound || dhcp_lease_stale_valid) {
			atomic_set(&dhcp_pending_revoke, 1);
		}
	}
}

static bool dhcp_addr_same(const struct net_in6_addr *a, const struct net_in6_addr *b)
{
	return memcmp(a->s6_addr, b->s6_addr, 16) == 0;
}

static void dhcp_reply_revoke_plan(uint8_t mtype, const struct net_in6_addr *ula,
				   const struct net_in6_addr *gua, bool have_ula, bool have_gua,
				   struct net_in6_addr *revoke_ula, bool *do_revoke_ula,
				   struct net_in6_addr *revoke_gua, bool *do_revoke_gua)
{
	*do_revoke_ula = false;
	*do_revoke_gua = false;

	if (!dhcp_lease.bound ||
	    (mtype != DHCPV6_MSG_REQUEST && mtype != DHCPV6_MSG_RENEW &&
	     mtype != DHCPV6_MSG_REBIND)) {
		return;
	}

	if (dhcp_lease.have_ula && (!have_ula || !dhcp_addr_same(&dhcp_lease.ula, ula))) {
		*revoke_ula = dhcp_lease.ula;
		*do_revoke_ula = true;
	}
	if (dhcp_lease.have_gua && (!have_gua || !dhcp_addr_same(&dhcp_lease.gua, gua))) {
		*revoke_gua = dhcp_lease.gua;
		*do_revoke_gua = true;
	}
}

static int dhcpv6_join_all_servers_mcast(int fd, struct net_if *eth)
{
	struct net_ipv6_mreq mreq = { 0 };
	int ifx = net_if_get_by_iface(eth);

	if (ifx < 0) {
		return -ENODEV;
	}
	mreq.ipv6mr_ifindex = ifx;
	if (net_addr_pton(AF_INET6, DHCPV6_MCAST_ALL_SERVERS, &mreq.ipv6mr_multiaddr) != 0) {
		return -EINVAL;
	}
	if (zsock_setsockopt(fd, NET_IPPROTO_IPV6, ZSOCK_IPV6_JOIN_GROUP, &mreq,
			     sizeof(mreq)) != 0) {
		return -errno;
	}
	return 0;
}

static void dhcpv6_leave_all_servers_mcast(int fd, struct net_if *eth)
{
	struct net_ipv6_mreq mreq = { 0 };
	int ifx;

	if (fd < 0 || eth == NULL) {
		return;
	}
	ifx = net_if_get_by_iface(eth);
	if (ifx < 0) {
		return;
	}
	mreq.ipv6mr_ifindex = ifx;
	if (net_addr_pton(AF_INET6, DHCPV6_MCAST_ALL_SERVERS, &mreq.ipv6mr_multiaddr) != 0) {
		return;
	}
	(void)zsock_setsockopt(fd, NET_IPPROTO_IPV6, ZSOCK_IPV6_LEAVE_GROUP, &mreq, sizeof(mreq));
}

static void put_be16(uint8_t *p, uint16_t v)
{
	sys_put_be16(v, p);
}

static void put_be32(uint8_t *p, uint32_t v)
{
	sys_put_be32(v, p);
}

static uint16_t get_be16(const uint8_t *p)
{
	return sys_get_be16(p);
}

static uint32_t get_be32(const uint8_t *p)
{
	return sys_get_be32(p);
}

static int server_duid_build(struct net_if *eth)
{
	const struct net_linkaddr *hw = net_if_get_link_addr(eth);

	if (hw == NULL || hw->len != 6U) {
		return -EINVAL;
	}
	dhcp.server_duid[0] = 0;
	dhcp.server_duid[1] = DHCPV6_DUID_LL;
	dhcp.server_duid[2] = 0;
	dhcp.server_duid[3] = DHCPV6_HW_ETHERNET;
	memcpy(dhcp.server_duid + 4, hw->addr, 6);
	dhcp.server_duid_len = 10U;
	return 0;
}

static bool server_id_matches(const uint8_t *opt_data, uint16_t opt_len)
{
	if (opt_len != dhcp.server_duid_len) {
		return false;
	}
	return memcmp(opt_data, dhcp.server_duid, dhcp.server_duid_len) == 0;
}

struct parse_out {
	uint8_t client_duid[MAX_CLIENT_DUID_LEN];
	uint16_t client_duid_len;
	uint32_t iaid;
	bool have_clientid;
	bool have_iaid;
};

static bool dhcp_client_matches_lease(const struct parse_out *po)
{
	if (!dhcp_lease.bound) {
		return true;
	}
	if (!po->have_clientid || po->client_duid_len == 0U ||
	    po->client_duid_len != dhcp_lease.client_duid_len) {
		return false;
	}
	return memcmp(po->client_duid, dhcp_lease.client_duid, po->client_duid_len) == 0 &&
	       po->iaid == dhcp_lease.iaid;
}

static void parse_client_message(const uint8_t *buf, size_t len, struct parse_out *o)
{
	size_t pos = 4U;

	memset(o, 0, sizeof(*o));
	o->iaid = (uint32_t)CONFIG_DECT_TETHER_IPV6_DHCPV6_IAID_DEFAULT;

	while (pos + 4U <= len) {
		uint16_t code = get_be16(buf + pos);
		uint16_t olen = get_be16(buf + pos + 2);
		size_t dpos = pos + 4U;

		pos += 4U;
		if ((size_t)olen + dpos > len) {
			break;
		}
		if (code == DHCPV6_OPT_CLIENTID && olen > 0U &&
		    olen <= MAX_CLIENT_DUID_LEN) {
			memcpy(o->client_duid, buf + dpos, olen);
			o->client_duid_len = olen;
			o->have_clientid = true;
		} else if (code == DHCPV6_OPT_IA_NA && olen >= 12U) {
			o->iaid = get_be32(buf + dpos);
			o->have_iaid = true;
		}
		pos = dpos + (size_t)olen;
	}
}

static size_t append_opt_raw(uint8_t *wp, uint16_t code, const void *data, uint16_t len)
{
	put_be16(wp, code);
	put_be16(wp + 2, len);
	memcpy(wp + 4, data, len);
	return 4U + (size_t)len;
}

static size_t append_iaaddr(uint8_t *wp, const struct net_in6_addr *addr,
			    uint32_t preferred_lifetime, uint32_t valid_lifetime)
{
	uint8_t body[24];

	memcpy(body, addr->s6_addr, 16);
	put_be32(body + 16, preferred_lifetime);
	put_be32(body + 20, valid_lifetime);
	return append_opt_raw(wp, DHCPV6_OPT_IAADDR, body, 24);
}

static size_t append_ia_na(uint8_t *wp, uint32_t iaid, const struct net_in6_addr *ula,
			    const struct net_in6_addr *gua, bool have_ula, bool have_gua,
			    const struct net_in6_addr *revoke_ula, bool do_revoke_ula,
			    const struct net_in6_addr *revoke_gua, bool do_revoke_gua)
{
	uint8_t *hdr = wp;
	size_t inner = 0U;
	uint8_t *inner_wp;

	wp += 4;
	put_be32(wp, iaid);
	wp += 4;
	put_be32(wp, CONFIG_DECT_TETHER_IPV6_DHCPV6_T1);
	wp += 4;
	put_be32(wp, CONFIG_DECT_TETHER_IPV6_DHCPV6_T2);
	wp += 4;
	inner_wp = wp;

	/* Windows keeps prior IAADDR bindings until the server returns the old
	 * address with preferred and valid lifetimes 0 in the same Reply as any
	 * new IAADDR (see doc/dect_tether_ipv6_analysis.md §6).
	 */
	if (do_revoke_ula && revoke_ula != NULL) {
		inner += append_iaaddr(inner_wp + inner, revoke_ula, 0U, 0U);
	}
	if (do_revoke_gua && revoke_gua != NULL) {
		inner += append_iaaddr(inner_wp + inner, revoke_gua, 0U, 0U);
	}

	if (have_ula) {
		inner += append_iaaddr(inner_wp + inner, ula,
				       CONFIG_DECT_TETHER_IPV6_DHCPV6_ADDR_PREFERRED_LIFETIME,
				       CONFIG_DECT_TETHER_IPV6_DHCPV6_ADDR_VALID_LIFETIME);
	}
	if (have_gua) {
		inner += append_iaaddr(inner_wp + inner, gua,
				       CONFIG_DECT_TETHER_IPV6_DHCPV6_ADDR_PREFERRED_LIFETIME,
				       CONFIG_DECT_TETHER_IPV6_DHCPV6_ADDR_VALID_LIFETIME);
	}
	if (!have_ula && !have_gua && inner == 0U) {
		uint8_t stdata[2];

		put_be16(stdata, DHCPV6_STATUS_NOADDRSAVAIL);
		inner += append_opt_raw(inner_wp + inner, DHCPV6_OPT_STATUS_CODE, stdata, 2);
	}

	put_be16(hdr, DHCPV6_OPT_IA_NA);
	put_be16(hdr + 2, (uint16_t)(12U + inner));
	return 4U + 12U + inner;
}

static int build_reply(uint8_t *out, size_t out_cap, uint8_t msg_type_reply, const uint8_t *req,
		       size_t req_len, const struct parse_out *po, const struct net_in6_addr *ula,
		       const struct net_in6_addr *gua, bool have_ula, bool have_gua,
		       bool include_ia, const struct net_in6_addr *revoke_ula, bool do_revoke_ula,
		       const struct net_in6_addr *revoke_gua, bool do_revoke_gua)
{
	size_t w = 4U;
	uint8_t pref1[1] = { 255 };

	if (out_cap < 128U) {
		return -EINVAL;
	}
	out[0] = msg_type_reply;
	out[1] = req[1];
	out[2] = req[2];
	out[3] = req[3];

	if (po->have_clientid) {
		if (w + 4U + po->client_duid_len > out_cap) {
			return -ENOSPC;
		}
		w += append_opt_raw(out + w, DHCPV6_OPT_CLIENTID, po->client_duid,
				    po->client_duid_len);
	}
	if (w + 4U + dhcp.server_duid_len > out_cap) {
		return -ENOSPC;
	}
	w += append_opt_raw(out + w, DHCPV6_OPT_SERVERID, dhcp.server_duid, dhcp.server_duid_len);

	if (msg_type_reply == DHCPV6_MSG_ADVERTISE) {
		if (w + 4U + 1U > out_cap) {
			return -ENOSPC;
		}
		w += append_opt_raw(out + w, DHCPV6_OPT_PREFERENCE, pref1, 1);
	}

	if (include_ia &&
	    (msg_type_reply == DHCPV6_MSG_ADVERTISE || msg_type_reply == DHCPV6_MSG_REPLY)) {
		size_t need = 4U + 12U + (have_ula ? 28U : 0U) + (have_gua ? 28U : 0U) +
			      (do_revoke_ula ? 28U : 0U) + (do_revoke_gua ? 28U : 0U) +
			      ((!have_ula && !have_gua && !do_revoke_ula && !do_revoke_gua) ?
				       (4U + 6U) :
				       0U);

		if (w + need > out_cap) {
			return -ENOSPC;
		}
		w += append_ia_na(out + w, po->iaid, ula, gua, have_ula, have_gua, revoke_ula,
				  do_revoke_ula, revoke_gua, do_revoke_gua);
	}

	return (int)w;
}

static size_t append_ia_na_revoke(uint8_t *wp, uint32_t iaid, const struct net_in6_addr *ula,
				  const struct net_in6_addr *gua, bool have_ula, bool have_gua)
{
	uint8_t *hdr = wp;
	size_t inner = 0U;
	uint8_t *inner_wp;

	wp += 4;
	put_be32(wp, iaid);
	wp += 4;
	put_be32(wp, 0U);
	wp += 4;
	put_be32(wp, 0U);
	wp += 4;
	inner_wp = wp;

	if (have_ula) {
		inner += append_iaaddr(inner_wp + inner, ula, 0U, 0U);
	}
	if (have_gua) {
		inner += append_iaaddr(inner_wp + inner, gua, 0U, 0U);
	}
	if (inner == 0U) {
		return 0U;
	}

	put_be16(hdr, DHCPV6_OPT_IA_NA);
	put_be16(hdr + 2, (uint16_t)(12U + inner));
	return 4U + 12U + inner;
}

static int build_lease_revoke(uint8_t *out, size_t out_cap, const uint8_t *client_duid,
			      uint16_t client_duid_len, const uint8_t *tid, uint32_t iaid,
			      const struct net_in6_addr *ula, const struct net_in6_addr *gua,
			      bool have_ula, bool have_gua)
{
	size_t w = 4U;
	size_t na;

	if (out_cap < 128U || client_duid_len == 0U) {
		return -EINVAL;
	}

	out[0] = DHCPV6_MSG_REPLY;
	out[1] = tid[0];
	out[2] = tid[1];
	out[3] = tid[2];

	w += append_opt_raw(out + w, DHCPV6_OPT_CLIENTID, client_duid, client_duid_len);
	if (w + 4U + dhcp.server_duid_len > out_cap) {
		return -ENOSPC;
	}
	w += append_opt_raw(out + w, DHCPV6_OPT_SERVERID, dhcp.server_duid, dhcp.server_duid_len);

	na = append_ia_na_revoke(out + w, iaid, ula, gua, have_ula, have_gua);
	if (na == 0U || w + na > out_cap) {
		return -ENOSPC;
	}
	w += na;

	return (int)w;
}

static void dhcp_lease_save(const struct sockaddr_in6 *cli, const struct parse_out *po,
			    const uint8_t *req, const struct net_in6_addr *ula,
			    const struct net_in6_addr *gua, bool have_ula, bool have_gua)
{
	bool same_client;
	bool addr_changed;

	same_client = dhcp_client_matches_lease(po);
	if (dhcp_lease.bound && !same_client) {
		return;
	}

	addr_changed = (have_ula != dhcp_lease.have_ula) || (have_gua != dhcp_lease.have_gua) ||
		       (have_ula && memcmp(ula->s6_addr, dhcp_lease.ula.s6_addr, 16) != 0) ||
		       (have_gua && memcmp(gua->s6_addr, dhcp_lease.gua.s6_addr, 16) != 0);

	if (dhcp_lease.bound && same_client && addr_changed) {
		memcpy(&dhcp_lease_stale, &dhcp_lease, sizeof(dhcp_lease));
		dhcp_lease_stale_valid = true;
	}

	memcpy(&dhcp_lease.cli, cli, sizeof(*cli));
	memcpy(dhcp_lease.client_duid, po->client_duid, po->client_duid_len);
	dhcp_lease.client_duid_len = po->client_duid_len;
	dhcp_lease.iaid = po->iaid;
	dhcp_lease.tid[0] = req[1];
	dhcp_lease.tid[1] = req[2];
	dhcp_lease.tid[2] = req[3];
	if (have_ula) {
		memcpy(&dhcp_lease.ula, ula, sizeof(*ula));
	}
	if (have_gua) {
		memcpy(&dhcp_lease.gua, gua, sizeof(*gua));
	}
	dhcp_lease.have_ula = have_ula;
	dhcp_lease.have_gua = have_gua;
	dhcp_lease.bound = true;
}

bool dect_tether_ipv6_dhcpv6_srv_lease_bound(void)
{
	return dhcp_lease.bound;
}

bool dect_tether_ipv6_dhcpv6_srv_tether_pio_addrs(struct net_in6_addr *ula_out,
						      struct net_in6_addr *gua_out,
						      bool *have_ula_out, bool *have_gua_out)
{
	if (!dhcp_lease.bound) {
		return false;
	}

	if (have_ula_out != NULL) {
		*have_ula_out = dhcp_lease.have_ula;
	}
	if (have_gua_out != NULL) {
		*have_gua_out = dhcp_lease.have_gua;
	}
	if (dhcp_lease.have_ula && ula_out != NULL) {
		*ula_out = dhcp_lease.ula;
	}
	if (dhcp_lease.have_gua && gua_out != NULL) {
		*gua_out = dhcp_lease.gua;
	}

	return dhcp_lease.have_ula || dhcp_lease.have_gua;
}

static bool msg_has_matching_serverid(const uint8_t *buf, size_t len)
{
	size_t pos = 4U;

	while (pos + 4U <= len) {
		uint16_t code = get_be16(buf + pos);
		uint16_t olen = get_be16(buf + pos + 2);
		size_t dpos = pos + 4U;

		pos += 4U;
		if ((size_t)olen + dpos > len) {
			break;
		}
		if (code == DHCPV6_OPT_SERVERID && server_id_matches(buf + dpos, olen)) {
			return true;
		}
		pos = dpos + (size_t)olen;
	}
	return false;
}

/**
 * Send one lease revoke datagram.
 *
 * @return False if @c zsock_sendto failed (caller should retry revoke later).
 */
static bool dhcp_send_lease_revoke_pkt(int fd, const struct dhcp_lease_saved *ls, unsigned int si)
{
	struct sockaddr_in6 c = ls->cli;
	int rv;

	c.sin6_port = htons(DHCPV6_CLIENT_PORT);
	rv = build_lease_revoke(dhcp_msg_sbuf, sizeof(dhcp_msg_sbuf), ls->client_duid,
				ls->client_duid_len, ls->tid, ls->iaid, &ls->ula, &ls->gua,
				ls->have_ula, ls->have_gua);
	if (rv <= 0) {
		LOG_WRN("DHCPv6 lease revoke build failed: %d", rv);
		return true;
	}

	{
		ssize_t sent = zsock_sendto(fd, dhcp_msg_sbuf, (size_t)rv, 0,
					    (struct sockaddr *)&c, sizeof(c));

		if (sent != (ssize_t)rv) {
			LOG_WRN("DHCPv6 lease revoke send failed");
			return false;
		}
	}

	LOG_INF("DHCPv6 lease revoke sent (DECT uplink lost, slot %u)", si);
	return true;
}

static void dhcp_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (!atomic_get(&dhcp.stop)) {
		struct net_if *eth = dect_tether_ipv6_host_eth_iface();
		int fd = -1;
		struct zsock_pollfd pfd;
		int pr;

		/* Poll often: hosts send DHCPv6 (e.g. Information-request) as soon as the link is
		 * up; a 1 s sleep here can miss the first exchange before the socket is bound.
		 */
		while (!atomic_get(&dhcp.stop) && !dect_tether_ipv6_host_eth_ready(eth)) {
			k_sleep(K_MSEC(100));
			eth = dect_tether_ipv6_host_eth_iface();
		}
		if (atomic_get(&dhcp.stop)) {
			break;
		}
		if (server_duid_build(eth) < 0) {
			LOG_ERR("DHCPv6: no Ethernet MAC for Server ID");
			k_sleep(K_SECONDS(2));
			continue;
		}

		fd = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (fd < 0) {
			LOG_ERR("DHCPv6 socket failed %d", errno);
			k_sleep(K_SECONDS(2));
			continue;
		}

#if defined(CONFIG_NET_INTERFACE_NAME)
		{
			struct net_ifreq ifr = {0};
			char ifname[CONFIG_NET_INTERFACE_NAME_LEN + 1];

			(void)net_if_get_name(eth, ifname, sizeof(ifname));
			memcpy(ifr.ifr_name, ifname,
			       MIN(sizeof(ifname) - 1, sizeof(ifr.ifr_name) - 1));
			if (zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE, &ifr,
					     sizeof(ifr)) != 0) {
				LOG_WRN("SO_BINDTODEVICE failed %d", errno);
			}
		}
#endif

		{
			struct sockaddr_in6 a = {0};

			a.sin6_family = AF_INET6;
			a.sin6_port = htons(DHCPV6_SERVER_PORT);
			if (zsock_bind(fd, (struct sockaddr *)&a, sizeof(a)) != 0) {
				LOG_ERR("DHCPv6 bind failed %d", errno);
				zsock_close(fd);
				k_sleep(K_SECONDS(2));
				continue;
			}
		}

		if (dhcpv6_join_all_servers_mcast(fd, eth) != 0) {
			LOG_ERR("DHCPv6 join %s failed %d (need IPv6 MLD / ND)",
				DHCPV6_MCAST_ALL_SERVERS, errno);
			zsock_close(fd);
			k_sleep(K_SECONDS(2));
			continue;
		}

		LOG_INF("DHCPv6 server UDP %u + joined %s", DHCPV6_SERVER_PORT,
			DHCPV6_MCAST_ALL_SERVERS);

		k_mutex_lock(&dhcp.sock_mtx, K_FOREVER);
		dhcp.sock = fd;
		k_mutex_unlock(&dhcp.sock_mtx);

		while (!atomic_get(&dhcp.stop) && eth != NULL && net_if_is_up(eth)) {
			struct sockaddr_in6 cli = {0};
			socklen_t clen = sizeof(cli);
			ssize_t n;
			uint8_t mtype;
			struct parse_out po;
			struct net_in6_addr ula, gua;
			bool have_ula, have_gua;
			struct net_if *dect = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT));
			int slen;
			uint8_t rtype = DHCPV6_MSG_REPLY;
			bool include_ia = true;

			if (atomic_get(&dhcp_pending_revoke)) {
				bool pending_done = true;
				const struct dhcp_lease_saved *slots[2];
				unsigned int nslots = 0U;
				unsigned int si;

				/* Older generation first (Windows often keeps both until
				 * IAADDR=0).
				 */
				if (dhcp_lease_stale_valid) {
					slots[nslots++] = &dhcp_lease_stale;
				}
				if (dhcp_lease.bound) {
					slots[nslots++] = &dhcp_lease;
				}

				if (nslots == 0U) {
					atomic_set(&dhcp_pending_revoke, 0);
					continue;
				}

				for (si = 0U; si < nslots; si++) {
					const struct dhcp_lease_saved *ls = slots[si];

					if (!dhcp_send_lease_revoke_pkt(fd, ls, si)) {
						pending_done = false;
						break;
					}

					if (ls == &dhcp_lease) {
						dhcp_lease.bound = false;
					} else {
						dhcp_lease_stale_valid = false;
						(void)memset(&dhcp_lease_stale, 0,
							     sizeof(dhcp_lease_stale));
					}
				}

				if (pending_done) {
					atomic_set(&dhcp_pending_revoke, 0);
				}
				continue;
			}

			pfd.fd = fd;
			pfd.events = ZSOCK_POLLIN;
			/* Shorter poll while a lease is active so uplink-lost revoke is not delayed
			 * by a full second.
			 */
			pr = zsock_poll(&pfd, 1, dhcp_lease.bound ? 250 : 1000);
			if (pr < 0) {
				LOG_ERR("DHCPv6: poll failed errno=%d", errno);
				break;
			}
			if (pr == 0) {
				continue;
			}
			n = zsock_recvfrom(fd, dhcp_msg_rbuf, sizeof(dhcp_msg_rbuf), 0,
					   (struct sockaddr *)&cli, &clen);
			if (n < 0) {
				LOG_ERR("DHCPv6: recvfrom failed errno=%d", errno);
				continue;
			}
			if (n < 4) {
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_TRACE)
				LOG_WRN("DHCPv6: recvfrom short len=%zd (need >= 4)", n);
#endif
				continue;
			}
			mtype = dhcp_msg_rbuf[0];
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_RX_LOG)
			{
				char abuf[NET_IPV6_ADDR_LEN];

				(void)net_addr_ntop(AF_INET6, &cli.sin6_addr, abuf, sizeof(abuf));
				LOG_INF("DHCPv6 RX from %s type=%u len=%zd", abuf,
					(unsigned int)mtype, n);
			}
#endif
			parse_client_message(dhcp_msg_rbuf, (size_t)n, &po);
			if (!po.have_clientid && mtype != DHCPV6_MSG_INFO_REQUEST) {
				if (IS_ENABLED(CONFIG_DECT_TETHER_IPV6_DHCPV6_TRACE)) {
					LOG_WRN("DHCPv6 drop: no Client ID (msg type=%u len=%zd)",
						(unsigned int)mtype, n);
				} else {
					LOG_DBG("DHCPv6 drop: no Client ID");
				}
				continue;
			}

			(void)dect_tether_ipv6_dect_addrs_get(dect, &ula, &gua, &have_ula,
								   &have_gua);

			/* Renew/Rebind: if the DECT uplink is momentarily down (brief
			 * re-association between recvfrom and addrs_get), fall back to
			 * the cached lease addresses instead of replying NoAddrsAvail,
			 * which would deprecate the client's preferred addresses.
			 */
			if (!have_ula && !have_gua && dhcp_lease.bound &&
			    (mtype == DHCPV6_MSG_RENEW || mtype == DHCPV6_MSG_REBIND)) {
				if (dhcp_lease.have_ula) {
					ula = dhcp_lease.ula;
					have_ula = true;
				}
				if (dhcp_lease.have_gua) {
					gua = dhcp_lease.gua;
					have_gua = true;
				}
			}

			/* Single lease slot: once bound, only the same DUID+IAID gets
			 * ULA/GUA. Other hosts get NoAddrsAvail (Solicit) or silence
			 * (Request/Renew/Rebind/Confirm).
			 */
			if (dhcp_lease.bound && !dhcp_client_matches_lease(&po)) {
				if (mtype == DHCPV6_MSG_SOLICIT) {
					have_ula = false;
					have_gua = false;
				} else if (mtype != DHCPV6_MSG_INFO_REQUEST) {
					if (IS_ENABLED(CONFIG_DECT_TETHER_IPV6_DHCPV6_TRACE)) {
						LOG_DBG("DHCPv6 drop: other client (bound lease "
							"active, type=%u)",
							(unsigned int)mtype);
					}
					continue;
				}
			}

			if (mtype == DHCPV6_MSG_SOLICIT) {
				rtype = DHCPV6_MSG_ADVERTISE;
			} else if (mtype == DHCPV6_MSG_REQUEST || mtype == DHCPV6_MSG_RENEW) {
				if (!msg_has_matching_serverid(dhcp_msg_rbuf, (size_t)n)) {
					if (IS_ENABLED(CONFIG_DECT_TETHER_IPV6_DHCPV6_TRACE)) {
						LOG_WRN("DHCPv6: Server ID mismatch or missing "
							"(msg type=%u)",
							(unsigned int)mtype);
					} else {
						LOG_DBG("DHCPv6: Server ID mismatch or missing");
					}
					continue;
				}
				rtype = DHCPV6_MSG_REPLY;
			} else if (mtype == DHCPV6_MSG_REBIND) {
				/* RFC 8415 §18.3.5: Rebind contains no Server ID.
				 * Only reply if we have an active binding for this
				 * client; otherwise stay silent.
				 */
				if (!dhcp_lease.bound) {
					continue;
				}
				rtype = DHCPV6_MSG_REPLY;
			} else if (mtype == DHCPV6_MSG_CONFIRM) {
				/* RFC 8415 §18.3.3: Confirm contains no Server ID.
				 * Reply Status=Success only if we have an active binding
				 * (cannot otherwise vouch the addresses fit this link).
				 * No IA options in the Reply, only Status Code.
				 */
				if (!dhcp_lease.bound) {
					continue;
				}
				rtype = DHCPV6_MSG_REPLY;
				include_ia = false;
			} else if (mtype == DHCPV6_MSG_INFO_REQUEST) {
				rtype = DHCPV6_MSG_REPLY;
				include_ia = false;
			} else {
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_TRACE)
				LOG_WRN("DHCPv6: ignore msg type=%u len=%zd "
					"(not Solicit/Request/Renew/Rebind/Confirm/Info-req)",
					(unsigned int)mtype, n);
#endif
				continue;
			}

			struct net_in6_addr revoke_ula, revoke_gua;
			bool do_revoke_ula = false;
			bool do_revoke_gua = false;

			dhcp_reply_revoke_plan(mtype, &ula, &gua, have_ula, have_gua, &revoke_ula,
					       &do_revoke_ula, &revoke_gua, &do_revoke_gua);

			slen = build_reply(dhcp_msg_sbuf, sizeof(dhcp_msg_sbuf),
					   rtype, dhcp_msg_rbuf,
					   (size_t)n, &po, &ula, &gua, have_ula, have_gua,
					   include_ia, &revoke_ula, do_revoke_ula, &revoke_gua,
					   do_revoke_gua);
			if (slen < 0) {
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_TRACE)
				LOG_WRN("DHCPv6: build_reply failed ret=%d "
					"(req type=%u rply type=%u)",
					slen, (unsigned int)mtype, (unsigned int)rtype);
#endif
				continue;
			}

			/* RFC 8415 §18.3.3: Reply to Confirm MUST carry Status=Success. */
			if (mtype == DHCPV6_MSG_CONFIRM) {
				uint8_t stdata[2] = {0, DHCPV6_STATUS_SUCCESS};

				if ((size_t)slen + 4U + sizeof(stdata) <= sizeof(dhcp_msg_sbuf)) {
					slen += (int)append_opt_raw(dhcp_msg_sbuf + slen,
								    DHCPV6_OPT_STATUS_CODE,
								    stdata, sizeof(stdata));
				}
			}
			cli.sin6_port = htons(DHCPV6_CLIENT_PORT);
			/* Install/refresh the /128 route and neighbor entry for the
			 * PC's GUA/ULA BEFORE sending the Reply so that Zephyr has a
			 * valid next-hop when it forwards the unicast Reply back to the
			 * client. On a Renew at T1 (1 h), the neighbor entry from the
			 * initial exchange may have been removed by NUD failure, causing
			 * the Reply to be silently dropped if tether_update runs after
			 * sendto (as it did previously).
			 */
			if (include_ia && (have_ula || have_gua)) {
				dect_tether_ipv6_fwd_tether_update(eth, &cli, po.client_duid,
									po.client_duid_len, &ula,
									&gua, have_ula, have_gua);
			}
			ssize_t sent = zsock_sendto(fd, dhcp_msg_sbuf, (size_t)slen, 0,
						    (struct sockaddr *)&cli, sizeof(cli));

			if (sent != (ssize_t)slen) {
				LOG_WRN("DHCPv6: sendto failed ret=%zd expect=%d errno=%d "
					"req=%u rply=%u",
					sent, slen, errno, (unsigned int)mtype,
					(unsigned int)rtype);
			} else if (rtype == DHCPV6_MSG_REPLY &&
				   (mtype == DHCPV6_MSG_REQUEST ||
				    mtype == DHCPV6_MSG_RENEW ||
				    mtype == DHCPV6_MSG_REBIND)) {
				LOG_INF("DHCPv6 Reply TX req=%u len=%d", (unsigned int)mtype, slen);
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_TRACE)
			} else {
				LOG_INF("DHCPv6 TX ok req_type=%u reply_type=%u len=%d",
					(unsigned int)mtype, (unsigned int)rtype, slen);
#endif
			}

			if (include_ia && (have_ula || have_gua) && rtype == DHCPV6_MSG_REPLY &&
			    (mtype == DHCPV6_MSG_REQUEST || mtype == DHCPV6_MSG_RENEW ||
			     mtype == DHCPV6_MSG_REBIND)) {
				dhcp_lease_save(&cli, &po, dhcp_msg_rbuf, &ula, &gua, have_ula,
						have_gua);
			}
		}

#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_TRACE)
		{
			struct net_if *cur = dect_tether_ipv6_host_eth_iface();
			int if_up = -1;
			int carrier = -1;

			if (cur != NULL) {
				if_up = net_if_is_up(cur) ? 1 : 0;
				carrier = net_if_is_carrier_ok(cur) ? 1 : 0;
			}
			LOG_INF("DHCPv6: recv loop exit stop=%d eth=%p if_up=%d carrier=%d",
				(int)atomic_get(&dhcp.stop), (void *)cur, if_up, carrier);
		}
#endif
		k_mutex_lock(&dhcp.sock_mtx, K_FOREVER);
		dhcp.sock = -1;
		k_mutex_unlock(&dhcp.sock_mtx);
		dhcpv6_leave_all_servers_mcast(fd, eth);
		zsock_close(fd);
	}
}

int dect_tether_ipv6_dhcpv6_srv_init(void)
{
	if (dhcp.tid != NULL) {
		return 0;
	}

	k_work_init_delayable(&dhcp_revoke_dwork, dhcp_revoke_dwork_fn);
	k_mutex_init(&dhcp.sock_mtx);

	atomic_clear(&dhcp.stop);
	dhcp.sock = -1;

	dhcp.tid = k_thread_create(&dhcp.thread, dhcpv6_srv_stack,
				   K_THREAD_STACK_SIZEOF(dhcpv6_srv_stack), dhcp_thread, NULL, NULL,
				   NULL, DHCPV6_THREAD_PRIO, 0, K_NO_WAIT);
	if (dhcp.tid == NULL) {
		return -ENOMEM;
	}
	k_thread_name_set(dhcp.tid, "dect_dhcpv6");
	return 0;
}

void dect_tether_ipv6_dhcpv6_srv_deinit(void)
{
	atomic_set(&dhcp.stop, 1);
	k_mutex_lock(&dhcp.sock_mtx, K_FOREVER);
	if (dhcp.sock >= 0) {
		(void)zsock_shutdown(dhcp.sock, ZSOCK_SHUT_RDWR);
	}
	k_mutex_unlock(&dhcp.sock_mtx);
	if (dhcp.tid != NULL) {
		k_thread_join(dhcp.tid, K_SECONDS(5));
		dhcp.tid = NULL;
	}
	dhcp_lease.bound = false;
	dhcp_lease_stale_valid = false;
	memset(&dhcp_lease_stale, 0, sizeof(dhcp_lease_stale));
	atomic_set(&dhcp_pending_revoke, 0);
	k_work_cancel_delayable(&dhcp_revoke_dwork);
	atomic_set(&dhcp_uplink_present, 1);
}

void dect_tether_ipv6_dhcpv6_srv_uplink_lost(void)
{
	atomic_set(&dhcp_uplink_present, 0);
	/* 10 s debounce: brief DECT re-associations (channel change, handover)
	 * are common. If the parent comes back within the window the delayed
	 * work is cancelled in uplink_restored() and no revoke is ever sent,
	 * so the client's preferred addresses are never deprecated.
	 */
	k_work_schedule(&dhcp_revoke_dwork,
			K_SECONDS(CONFIG_DECT_TETHER_IPV6_DHCPV6_REVOKE_DEBOUNCE_SEC));
}

void dect_tether_ipv6_dhcpv6_srv_uplink_restored(void)
{
	atomic_set(&dhcp_uplink_present, 1);
	k_work_cancel_delayable(&dhcp_revoke_dwork);
}
