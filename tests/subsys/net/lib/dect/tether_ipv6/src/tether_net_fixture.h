/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TETHER_NET_FIXTURE_H_
#define TETHER_NET_FIXTURE_H_

#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tether_net_fixture {
	struct net_if *dect;
	struct net_if *eth;
	struct in6_addr eth_ll;
	struct in6_addr server_ll;
	bool tether_started;
};

int tether_net_fixture_setup(struct tether_net_fixture *fx);
void tether_net_fixture_teardown(struct tether_net_fixture *fx);
int tether_net_fixture_parent_release(struct tether_net_fixture *fx);
int tether_net_fixture_change_gua_prefix(struct tether_net_fixture *fx);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_NET_FIXTURE_H_ */
