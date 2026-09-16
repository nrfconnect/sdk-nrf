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

/* Poll timeout (ms): shorter while a lease is active so uplink-lost revoke is not
 * delayed by a full second; longer otherwise, to poll less aggressively when idle.
 */
#define DHCPV6_POLL_TIMEOUT_LEASE_MS	250
#define DHCPV6_POLL_TIMEOUT_IDLE_MS	1000

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

/* Current addresses offered in this Reply (in), used to detect an address change
 * against the saved lease below.
 */
struct dhcp_reply_addrs {
	const struct net_in6_addr *ula;
	const struct net_in6_addr *gua;
	bool have_ula;
	bool have_gua;
};

/* Previous-lease addresses to revoke (lifetime 0) because they changed (out). */
struct dhcp_revoke_plan {
	struct net_in6_addr ula;
	struct net_in6_addr gua;
	bool do_ula;
	bool do_gua;
};

static void dhcp_reply_revoke_plan(uint8_t mtype, const struct dhcp_reply_addrs *addrs,
				   struct dhcp_revoke_plan *revoke)
{
	revoke->do_ula = false;
	revoke->do_gua = false;

	if (!dhcp_lease.bound ||
	    (mtype != DHCPV6_MSG_REQUEST && mtype != DHCPV6_MSG_RENEW &&
	     mtype != DHCPV6_MSG_REBIND)) {
		return;
	}

	if (dhcp_lease.have_ula &&
	    (!addrs->have_ula || !dhcp_addr_same(&dhcp_lease.ula, addrs->ula))) {
		revoke->ula = dhcp_lease.ula;
		revoke->do_ula = true;
	}
	if (dhcp_lease.have_gua &&
	    (!addrs->have_gua || !dhcp_addr_same(&dhcp_lease.gua, addrs->gua))) {
		revoke->gua = dhcp_lease.gua;
		revoke->do_gua = true;
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
		} else {
			/* Other option codes (Server ID, Elapsed Time, ORO,
			 * ...), or a too-short IA_NA, carry no information
			 * this server needs; skip over via pos update below.
			 */
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

static size_t append_ia_na(uint8_t *wp, uint32_t iaid, const struct dhcp_reply_addrs *addrs,
			    const struct dhcp_revoke_plan *revoke)
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
	if (revoke->do_ula) {
		inner += append_iaaddr(inner_wp + inner, &revoke->ula, 0U, 0U);
	}
	if (revoke->do_gua) {
		inner += append_iaaddr(inner_wp + inner, &revoke->gua, 0U, 0U);
	}

	if (addrs->have_ula) {
		inner += append_iaaddr(inner_wp + inner, addrs->ula,
				       CONFIG_DECT_TETHER_IPV6_DHCPV6_ADDR_PREFERRED_LIFETIME,
				       CONFIG_DECT_TETHER_IPV6_DHCPV6_ADDR_VALID_LIFETIME);
	}
	if (addrs->have_gua) {
		inner += append_iaaddr(inner_wp + inner, addrs->gua,
				       CONFIG_DECT_TETHER_IPV6_DHCPV6_ADDR_PREFERRED_LIFETIME,
				       CONFIG_DECT_TETHER_IPV6_DHCPV6_ADDR_VALID_LIFETIME);
	}
	if (!addrs->have_ula && !addrs->have_gua && inner == 0U) {
		uint8_t stdata[2];

		put_be16(stdata, DHCPV6_STATUS_NOADDRSAVAIL);
		inner += append_opt_raw(inner_wp + inner, DHCPV6_OPT_STATUS_CODE, stdata, 2);
	}

	put_be16(hdr, DHCPV6_OPT_IA_NA);
	put_be16(hdr + 2, (uint16_t)(12U + inner));
	return 4U + 12U + inner;
}

/* Output buffer descriptor, to keep build_reply()'s parameter count down. */
struct dhcp_reply_out {
	uint8_t *buf;
	size_t cap;
};

static int build_reply(struct dhcp_reply_out *out, uint8_t msg_type_reply, const uint8_t *req,
		       const struct parse_out *po, const struct dhcp_reply_addrs *addrs,
		       bool include_ia, const struct dhcp_revoke_plan *revoke)
{
	size_t w = 4U;
	uint8_t pref1[1] = { 255 };

	if (out->cap < 128U) {
		return -EINVAL;
	}
	out->buf[0] = msg_type_reply;
	out->buf[1] = req[1];
	out->buf[2] = req[2];
	out->buf[3] = req[3];

	if (po->have_clientid) {
		if (w + 4U + po->client_duid_len > out->cap) {
			return -ENOSPC;
		}
		w += append_opt_raw(out->buf + w, DHCPV6_OPT_CLIENTID, po->client_duid,
				    po->client_duid_len);
	}
	if (w + 4U + dhcp.server_duid_len > out->cap) {
		return -ENOSPC;
	}
	w += append_opt_raw(out->buf + w, DHCPV6_OPT_SERVERID, dhcp.server_duid,
			    dhcp.server_duid_len);

	if (msg_type_reply == DHCPV6_MSG_ADVERTISE) {
		if (w + 4U + 1U > out->cap) {
			return -ENOSPC;
		}
		w += append_opt_raw(out->buf + w, DHCPV6_OPT_PREFERENCE, pref1, 1);
	}

	if (include_ia &&
	    (msg_type_reply == DHCPV6_MSG_ADVERTISE || msg_type_reply == DHCPV6_MSG_REPLY)) {
		size_t need = 4U + 12U + (addrs->have_ula ? 28U : 0U) +
			      (addrs->have_gua ? 28U : 0U) + (revoke->do_ula ? 28U : 0U) +
			      (revoke->do_gua ? 28U : 0U) +
			      ((!addrs->have_ula && !addrs->have_gua && !revoke->do_ula &&
				!revoke->do_gua) ?
				       (4U + 6U) :
				       0U);

		if (w + need > out->cap) {
			return -ENOSPC;
		}
		w += append_ia_na(out->buf + w, po->iaid, addrs, revoke);
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

static int build_lease_revoke(struct dhcp_reply_out *out, const struct dhcp_lease_saved *ls)
{
	size_t w = 4U;
	size_t na;

	if (out->cap < 128U || ls->client_duid_len == 0U) {
		return -EINVAL;
	}

	out->buf[0] = DHCPV6_MSG_REPLY;
	out->buf[1] = ls->tid[0];
	out->buf[2] = ls->tid[1];
	out->buf[3] = ls->tid[2];

	w += append_opt_raw(out->buf + w, DHCPV6_OPT_CLIENTID, ls->client_duid,
			    ls->client_duid_len);
	if (w + 4U + dhcp.server_duid_len > out->cap) {
		return -ENOSPC;
	}
	w += append_opt_raw(out->buf + w, DHCPV6_OPT_SERVERID, dhcp.server_duid,
			    dhcp.server_duid_len);

	na = append_ia_na_revoke(out->buf + w, ls->iaid, &ls->ula, &ls->gua, ls->have_ula,
				 ls->have_gua);
	if (na == 0U || w + na > out->cap) {
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
	struct dhcp_reply_out reply_out = { .buf = dhcp_msg_sbuf, .cap = sizeof(dhcp_msg_sbuf) };
	int rv;

	c.sin6_port = htons(DHCPV6_CLIENT_PORT);
	rv = build_lease_revoke(&reply_out, ls);
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

/* Waits for the host Ethernet interface to be ready, then opens, binds, and joins
 * the DHCPv6 multicast group on a fresh UDP socket. Publishes dhcp.sock on success.
 *
 * @retval fd (>= 0) on success; *eth_out is set to the interface it is bound to.
 * @retval negative on failure. Caller should check dhcp.stop to tell a shutdown
 *         request apart from a transient failure that is worth retrying.
 */
static int dhcp_open_bound_socket(struct net_if **eth_out)
{
	struct net_if *eth = dect_tether_ipv6_host_eth_iface();
	int fd;

	/* Poll often: hosts send DHCPv6 (e.g. Information-request) as soon as the link is
	 * up; a 1 s sleep here can miss the first exchange before the socket is bound.
	 */
	while (!atomic_get(&dhcp.stop) && !dect_tether_ipv6_host_eth_ready(eth)) {
		k_sleep(K_MSEC(100));
		eth = dect_tether_ipv6_host_eth_iface();
	}
	*eth_out = eth;
	if (atomic_get(&dhcp.stop)) {
		return -ECANCELED;
	}
	if (server_duid_build(eth) < 0) {
		LOG_ERR("DHCPv6: no Ethernet MAC for Server ID");
		k_sleep(K_SECONDS(2));
		return -ENODEV;
	}

	fd = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		LOG_ERR("DHCPv6 socket failed %d", errno);
		k_sleep(K_SECONDS(2));
		return -errno;
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
			return -errno;
		}
	}

	if (dhcpv6_join_all_servers_mcast(fd, eth) != 0) {
		LOG_ERR("DHCPv6 join %s failed %d (need IPv6 MLD / ND)",
			DHCPV6_MCAST_ALL_SERVERS, errno);
		zsock_close(fd);
		k_sleep(K_SECONDS(2));
		return -errno;
	}

	LOG_INF("DHCPv6 server UDP %u + joined %s", DHCPV6_SERVER_PORT,
		DHCPV6_MCAST_ALL_SERVERS);

	k_mutex_lock(&dhcp.sock_mtx, K_FOREVER);
	dhcp.sock = fd;
	k_mutex_unlock(&dhcp.sock_mtx);

	return fd;
}

/* Handles one round of pending lease revokes (uplink lost). */
static void dhcp_process_pending_revoke(int fd)
{
	bool pending_done = true;
	const struct dhcp_lease_saved *slots[2];
	unsigned int nslots = 0U;
	unsigned int si;

	/* Older generation first (Windows often keeps both until IAADDR=0). */
	if (dhcp_lease_stale_valid) {
		slots[nslots++] = &dhcp_lease_stale;
	}
	if (dhcp_lease.bound) {
		slots[nslots++] = &dhcp_lease;
	}

	if (nslots == 0U) {
		atomic_set(&dhcp_pending_revoke, 0);
		return;
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
			(void)memset(&dhcp_lease_stale, 0, sizeof(dhcp_lease_stale));
		}
	}

	if (pending_done) {
		atomic_set(&dhcp_pending_revoke, 0);
	}
}

/* Resolves the ULA/GUA to offer in the Reply for this message, applying the
 * Renew/Rebind cached-lease fallback and the single-lease-slot "other client"
 * policy.
 *
 * @retval true if the message must be dropped (caller should skip it).
 */
static bool dhcp_resolve_addrs_for_msg(uint8_t mtype, const struct parse_out *po,
				       struct net_in6_addr *ula, struct net_in6_addr *gua,
				       bool *have_ula, bool *have_gua)
{
	struct net_if *dect = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT));

	(void)dect_tether_ipv6_dect_addrs_get(dect, ula, gua, have_ula, have_gua);

	/* Renew/Rebind: if the DECT uplink is momentarily down (brief
	 * re-association between recvfrom and addrs_get), fall back to
	 * the cached lease addresses instead of replying NoAddrsAvail,
	 * which would deprecate the client's preferred addresses.
	 */
	if (!*have_ula && !*have_gua && dhcp_lease.bound &&
	    (mtype == DHCPV6_MSG_RENEW || mtype == DHCPV6_MSG_REBIND)) {
		if (dhcp_lease.have_ula) {
			*ula = dhcp_lease.ula;
			*have_ula = true;
		}
		if (dhcp_lease.have_gua) {
			*gua = dhcp_lease.gua;
			*have_gua = true;
		}
	}

	/* Single lease slot: once bound, only the same DUID+IAID gets
	 * ULA/GUA. Other hosts get NoAddrsAvail (Solicit) or silence
	 * (Request/Renew/Rebind/Confirm).
	 */
	if (dhcp_lease.bound && !dhcp_client_matches_lease(po)) {
		if (mtype == DHCPV6_MSG_SOLICIT) {
			*have_ula = false;
			*have_gua = false;
		} else if (mtype != DHCPV6_MSG_INFO_REQUEST) {
			if (IS_ENABLED(CONFIG_DECT_TETHER_IPV6_DHCPV6_TRACE)) {
				LOG_DBG("DHCPv6 drop: other client (bound lease "
					"active, type=%u)",
					(unsigned int)mtype);
			}
			return true;
		}
		/* Else: Info-Request never carries/returns IA_NA, so a
		 * different client's Info-Request is unaffected by the
		 * bound lease; proceed normally below.
		 */
	}

	return false;
}

/* Determines the Reply message type and whether to include an IA_NA option,
 * per RFC 8415 §18.3, for one incoming DHCPv6 client message.
 *
 * @retval true if the message must be dropped/ignored (caller should skip it).
 */
static bool dhcp_classify_msg(uint8_t mtype, const uint8_t *req, ssize_t n, uint8_t *rtype,
			      bool *include_ia)
{
	if (mtype == DHCPV6_MSG_SOLICIT) {
		*rtype = DHCPV6_MSG_ADVERTISE;
	} else if (mtype == DHCPV6_MSG_REQUEST || mtype == DHCPV6_MSG_RENEW) {
		if (!msg_has_matching_serverid(req, (size_t)n)) {
			if (IS_ENABLED(CONFIG_DECT_TETHER_IPV6_DHCPV6_TRACE)) {
				LOG_WRN("DHCPv6: Server ID mismatch or missing "
					"(msg type=%u)",
					(unsigned int)mtype);
			} else {
				LOG_DBG("DHCPv6: Server ID mismatch or missing");
			}
			return true;
		}
		*rtype = DHCPV6_MSG_REPLY;
	} else if (mtype == DHCPV6_MSG_REBIND) {
		/* RFC 8415 §18.3.5: Rebind contains no Server ID.
		 * Only reply if we have an active binding for this
		 * client; otherwise stay silent.
		 */
		if (!dhcp_lease.bound) {
			return true;
		}
		*rtype = DHCPV6_MSG_REPLY;
	} else if (mtype == DHCPV6_MSG_CONFIRM) {
		/* RFC 8415 §18.3.3: Confirm contains no Server ID.
		 * Reply Status=Success only if we have an active binding
		 * (cannot otherwise vouch the addresses fit this link).
		 * No IA options in the Reply, only Status Code.
		 */
		if (!dhcp_lease.bound) {
			return true;
		}
		*rtype = DHCPV6_MSG_REPLY;
		*include_ia = false;
	} else if (mtype == DHCPV6_MSG_INFO_REQUEST) {
		*rtype = DHCPV6_MSG_REPLY;
		*include_ia = false;
	} else {
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_TRACE)
		LOG_WRN("DHCPv6: ignore msg type=%u len=%zd "
			"(not Solicit/Request/Renew/Rebind/Confirm/Info-req)",
			(unsigned int)mtype, n);
#endif
		return true;
	}

	return false;
}

/* Appends the Confirm status code if needed, refreshes routing/neighbor state,
 * sends the Reply datagram, and saves the lease on a successful bind exchange.
 */
static void dhcp_send_and_finish(int fd, struct net_if *eth, struct sockaddr_in6 *cli,
				 const struct parse_out *po, uint8_t mtype, uint8_t rtype,
				 bool include_ia, struct net_in6_addr *ula,
				 struct net_in6_addr *gua, bool have_ula, bool have_gua, int slen)
{
	ssize_t sent;
	bool is_bind_reply;

	/* RFC 8415 §18.3.3: Reply to Confirm MUST carry Status=Success. */
	if (mtype == DHCPV6_MSG_CONFIRM) {
		uint8_t stdata[2] = {0, DHCPV6_STATUS_SUCCESS};

		if ((size_t)slen + 4U + sizeof(stdata) <= sizeof(dhcp_msg_sbuf)) {
			slen += (int)append_opt_raw(dhcp_msg_sbuf + slen, DHCPV6_OPT_STATUS_CODE,
						    stdata, sizeof(stdata));
		}
	}
	cli->sin6_port = htons(DHCPV6_CLIENT_PORT);
	/* Install/refresh the /128 route and neighbor entry for the
	 * PC's GUA/ULA BEFORE sending the Reply so that Zephyr has a
	 * valid next-hop when it forwards the unicast Reply back to the
	 * client. On a Renew at T1 (1 h), the neighbor entry from the
	 * initial exchange may have been removed by NUD failure, causing
	 * the Reply to be silently dropped if tether_update runs after
	 * sendto (as it did previously).
	 */
	if (include_ia && (have_ula || have_gua)) {
		dect_tether_ipv6_fwd_tether_update(eth, cli, po->client_duid,
						   po->client_duid_len, ula, gua, have_ula,
						   have_gua);
	}

	sent = zsock_sendto(fd, dhcp_msg_sbuf, (size_t)slen, 0, (struct sockaddr *)cli,
			    sizeof(*cli));

	is_bind_reply = (rtype == DHCPV6_MSG_REPLY &&
			 (mtype == DHCPV6_MSG_REQUEST || mtype == DHCPV6_MSG_RENEW ||
			  mtype == DHCPV6_MSG_REBIND));

	if (sent != (ssize_t)slen) {
		LOG_WRN("DHCPv6: sendto failed ret=%zd expect=%d errno=%d "
			"req=%u rply=%u",
			sent, slen, errno, (unsigned int)mtype, (unsigned int)rtype);
	} else if (is_bind_reply) {
		LOG_INF("DHCPv6 Reply TX req=%u len=%d", (unsigned int)mtype, slen);
	} else if (IS_ENABLED(CONFIG_DECT_TETHER_IPV6_DHCPV6_TRACE)) {
		LOG_INF("DHCPv6 TX ok req_type=%u reply_type=%u len=%d",
			(unsigned int)mtype, (unsigned int)rtype, slen);
	} else {
		/* Non-bind reply (e.g. Advertise, Confirm, Info-Request
		 * Reply) sent successfully; nothing to log outside of
		 * verbose tracing to avoid log spam on regular traffic.
		 */
	}

	if (include_ia && (have_ula || have_gua) && is_bind_reply) {
		dhcp_lease_save(cli, po, dhcp_msg_rbuf, ula, gua, have_ula, have_gua);
	}
}

/* Handles one received DHCPv6 client datagram already in dhcp_msg_rbuf (n bytes). */
static void dhcp_handle_message(int fd, struct net_if *eth, struct sockaddr_in6 *cli, ssize_t n)
{
	uint8_t mtype = dhcp_msg_rbuf[0];
	struct parse_out po;
	struct net_in6_addr ula, gua;
	bool have_ula, have_gua;
	uint8_t rtype = DHCPV6_MSG_REPLY;
	bool include_ia = true;
	int slen;

#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_RX_LOG)
	{
		char abuf[NET_IPV6_ADDR_LEN];

		(void)net_addr_ntop(AF_INET6, &cli->sin6_addr, abuf, sizeof(abuf));
		LOG_INF("DHCPv6 RX from %s type=%u len=%zd", abuf, (unsigned int)mtype, n);
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
		return;
	}

	if (dhcp_resolve_addrs_for_msg(mtype, &po, &ula, &gua, &have_ula, &have_gua)) {
		return;
	}

	if (dhcp_classify_msg(mtype, dhcp_msg_rbuf, n, &rtype, &include_ia)) {
		return;
	}

	{
		struct dhcp_reply_addrs addrs = {
			.ula = &ula, .gua = &gua, .have_ula = have_ula, .have_gua = have_gua,
		};
		struct dhcp_revoke_plan revoke = { 0 };
		struct dhcp_reply_out reply_out = {
			.buf = dhcp_msg_sbuf, .cap = sizeof(dhcp_msg_sbuf),
		};

		dhcp_reply_revoke_plan(mtype, &addrs, &revoke);

		slen = build_reply(&reply_out, rtype, dhcp_msg_rbuf, &po, &addrs, include_ia,
				   &revoke);
		if (slen < 0) {
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_TRACE)
			LOG_WRN("DHCPv6: build_reply failed ret=%d "
				"(req type=%u rply type=%u)",
				slen, (unsigned int)mtype, (unsigned int)rtype);
#endif
			return;
		}
	}

	dhcp_send_and_finish(fd, eth, cli, &po, mtype, rtype, include_ia, &ula, &gua, have_ula,
			     have_gua, slen);
}

static void dhcp_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (!atomic_get(&dhcp.stop)) {
		struct net_if *eth;
		int fd = dhcp_open_bound_socket(&eth);

		if (fd < 0) {
			if (atomic_get(&dhcp.stop)) {
				break;
			}
			continue;
		}

		while (!atomic_get(&dhcp.stop) && eth != NULL && net_if_is_up(eth)) {
			struct sockaddr_in6 cli = {0};
			socklen_t clen = sizeof(cli);
			struct zsock_pollfd pfd;
			ssize_t n;
			int pr;
			int poll_timeout_ms;

			if (atomic_get(&dhcp_pending_revoke)) {
				dhcp_process_pending_revoke(fd);
				continue;
			}

			poll_timeout_ms = dhcp_lease.bound ? DHCPV6_POLL_TIMEOUT_LEASE_MS
							    : DHCPV6_POLL_TIMEOUT_IDLE_MS;
			pfd.fd = fd;
			pfd.events = ZSOCK_POLLIN;
			pr = zsock_poll(&pfd, 1, poll_timeout_ms);
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

			dhcp_handle_message(fd, eth, &cli, n);
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
