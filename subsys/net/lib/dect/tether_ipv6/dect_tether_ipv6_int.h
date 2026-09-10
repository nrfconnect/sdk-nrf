/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_TETHER_IPV6_INT_H__
#define DECT_TETHER_IPV6_INT_H__

#include <stdbool.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>

struct sockaddr_in6;

/* DHCPv6 ports (RFC 8415 §7.2) and message types (§7.3), shared with the whitebox
 * test's host simulator.
 */
#define DHCPV6_CLIENT_PORT		546
#define DHCPV6_SERVER_PORT		547

#define DHCPV6_MSG_SOLICIT		1
#define DHCPV6_MSG_ADVERTISE		2
#define DHCPV6_MSG_REQUEST		3
#define DHCPV6_MSG_CONFIRM		4
#define DHCPV6_MSG_RENEW		5
#define DHCPV6_MSG_REBIND		6
#define DHCPV6_MSG_REPLY		7
#define DHCPV6_MSG_INFO_REQUEST		11

#define DHCPV6_OPT_CLIENTID		1
#define DHCPV6_OPT_SERVERID		2
#define DHCPV6_OPT_IA_NA		3
#define DHCPV6_OPT_IAADDR		5
#define DHCPV6_OPT_PREFERENCE		7
#define DHCPV6_OPT_STATUS_CODE		13

#define DHCPV6_STATUS_SUCCESS		0
#define DHCPV6_STATUS_NOADDRSAVAIL	2

#define DHCPV6_DUID_LL			3
#define DHCPV6_HW_ETHERNET		1

/* RA (RFC 4861) type/flag/option codes, shared with the whitebox test's host simulator. */
#define ICMPV6_RS_TYPE			133
#define ICMPV6_RA_TYPE			134
#define ICMPV6_NS_TYPE			135
#define RA_FLAG_MANAGED			0x80U
#define RA_OPT_SRC_LL_ADDR		1U
#define RA_OPT_PREFIX_INFO		3U
#define RA_OPT_MTU			5U
#define RA_OPT_RDNSS			25U

/** Host Ethernet leg (CONFIG_DECT_TETHER_IPV6_HOST_ETH_IFACE or first Ethernet). */
struct net_if *dect_tether_ipv6_host_eth_iface(void);
/** True when the host Ethernet iface is up with a preferred link-local address. */
bool dect_tether_ipv6_host_eth_ready(struct net_if *eth);

bool dect_tether_ipv6_dect_addrs_get(struct net_if *dect_iface, struct net_in6_addr *ula_out,
					 struct net_in6_addr *gua_out, bool *have_ula,
					 bool *have_gua);

/** Extract Ethernet MAC from DHCPv6 Client Identifier (DUID-LL or DUID-LLT). */
bool dect_tether_ipv6_duid_eth_mac_get(const uint8_t *duid, uint16_t dlen, uint8_t mac[6]);

int dect_tether_ipv6_ra_init(void);
void dect_tether_ipv6_ra_deinit(void);
#if defined(CONFIG_DECT_TETHER_IPV6_RA)
void dect_tether_ipv6_ra_kick(void);
#endif
/* Called when the DECT parent association is created / released so RA, DHCP, and
 * routing helpers can react. Wired from dect_tether_ipv6_uplink.c.
 */
void dect_tether_ipv6_ra_set_uplink(bool present);

int dect_tether_ipv6_uplink_init(void);
void dect_tether_ipv6_uplink_deinit(void);

int dect_tether_ipv6_dhcpv6_srv_init(void);
void dect_tether_ipv6_dhcpv6_srv_deinit(void);
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_SERVER)
bool dect_tether_ipv6_dhcpv6_srv_lease_bound(void);
/** Bound lease ULA/GUA for RA PIO (false if no addresses on lease). */
bool dect_tether_ipv6_dhcpv6_srv_tether_pio_addrs(struct net_in6_addr *ula_out,
						      struct net_in6_addr *gua_out,
						      bool *have_ula_out, bool *have_gua_out);
void dect_tether_ipv6_dhcpv6_srv_uplink_lost(void);
void dect_tether_ipv6_dhcpv6_srv_uplink_restored(void);
#endif

int dect_tether_ipv6_fwd_init(void);
void dect_tether_ipv6_fwd_deinit(void);
void dect_tether_ipv6_fwd_parent_assoc_created(struct net_if *dect);
void dect_tether_ipv6_fwd_parent_assoc_released(void);
void dect_tether_ipv6_fwd_tether_update(struct net_if *eth,
					      const struct sockaddr_in6 *cli,
					      const uint8_t *client_duid, uint16_t client_duid_len,
					      const struct net_in6_addr *ula,
					      const struct net_in6_addr *gua, bool have_ula,
					      bool have_gua);

#if defined(CONFIG_DECT_TETHER_IPV6_MDNS_FORWARD)
int dect_tether_ipv6_mdns_fwd_start(void);
void dect_tether_ipv6_mdns_fwd_stop(void);
#endif

#endif /* DECT_TETHER_IPV6_INT_H__ */
