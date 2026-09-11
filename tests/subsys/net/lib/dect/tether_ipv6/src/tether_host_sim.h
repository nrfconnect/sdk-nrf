/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TETHER_HOST_SIM_H_
#define TETHER_HOST_SIM_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/net/net_ip.h>

#include "dect_tether_ipv6_int.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TETHER_HOST_MAC_LEN 6

struct tether_host_ctx {
	uint8_t mac[TETHER_HOST_MAC_LEN];
	struct in6_addr ll;
	uint32_t iaid;
	uint8_t tid[3];
	uint8_t server_duid[32];
	uint16_t server_duid_len;
};

struct tether_ra_parse {
	uint16_t router_lifetime;
	uint8_t flags;
	bool managed;
	bool has_pio;
	bool has_rdnss;
};

struct tether_dhcp_parse {
	uint8_t msg_type;
	bool has_iaaddr;
	struct in6_addr ula;
	struct in6_addr gua;
	bool have_ula;
	bool have_gua;
};

void tether_host_ctx_init(struct tether_host_ctx *host);
int tether_host_send_rs(struct net_if *eth, struct tether_host_ctx *host);
int tether_host_send_solicit(struct net_if *eth, struct tether_host_ctx *host);
int tether_host_send_solicit_to(struct net_if *eth, struct tether_host_ctx *host,
			      const struct in6_addr *server_ll);
int tether_host_send_request(struct net_if *eth, struct tether_host_ctx *host,
			     const struct in6_addr *server_ll);
int tether_host_send_renew(struct net_if *eth, struct tether_host_ctx *host,
			   const struct in6_addr *server_ll);

int tether_host_wait_tx_growth(int baseline, int timeout_ms);
bool tether_host_find_ra(int baseline, struct tether_ra_parse *out);
bool tether_host_find_dhcp(int baseline, uint8_t want_type, struct tether_dhcp_parse *out,
			   struct tether_host_ctx *host);
bool tether_host_dhcp_has_revoked_iaaddr(int baseline, uint8_t want_type);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_HOST_SIM_H_ */
