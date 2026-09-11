/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "dect_tether_ipv6.h"
#include "dect_tether_ipv6_int.h"

LOG_MODULE_REGISTER(dect_tether_ipv6, CONFIG_DECT_TETHER_IPV6_LOG_LEVEL);

static bool inited;

int dect_tether_ipv6_init(void)
{
	int err = 0;

	if (inited) {
		return -EALREADY;
	}

#if defined(CONFIG_DECT_TETHER_IPV6_RA)
	err = dect_tether_ipv6_ra_init();
	if (err < 0) {
		return err;
	}
#endif
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_SERVER)
	err = dect_tether_ipv6_dhcpv6_srv_init();
	if (err < 0) {
#if defined(CONFIG_DECT_TETHER_IPV6_RA)
		dect_tether_ipv6_ra_deinit();
#endif
		return err;
	}
#endif
	err = dect_tether_ipv6_fwd_init();
	if (err < 0) {
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_SERVER)
		dect_tether_ipv6_dhcpv6_srv_deinit();
#endif
#if defined(CONFIG_DECT_TETHER_IPV6_RA)
		dect_tether_ipv6_ra_deinit();
#endif
		return err;
	}
	err = dect_tether_ipv6_uplink_init();
	if (err < 0) {
		dect_tether_ipv6_fwd_deinit();
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_SERVER)
		dect_tether_ipv6_dhcpv6_srv_deinit();
#endif
#if defined(CONFIG_DECT_TETHER_IPV6_RA)
		dect_tether_ipv6_ra_deinit();
#endif
		return err;
	}
#if defined(CONFIG_DECT_TETHER_IPV6_MDNS_FORWARD)
	if (dect_tether_ipv6_mdns_fwd_start() != 0) {
		LOG_WRN("dect_tether_ipv6: IPv6 LL mDNS tap not started");
	}
#endif

	inited = true;
	LOG_INF("dect_tether_ipv6 started");
	return err;
}

void dect_tether_ipv6_deinit(void)
{
	if (!inited) {
		return;
	}
#if defined(CONFIG_DECT_TETHER_IPV6_MDNS_FORWARD)
	dect_tether_ipv6_mdns_fwd_stop();
#endif
	dect_tether_ipv6_uplink_deinit();
	dect_tether_ipv6_fwd_deinit();
#if defined(CONFIG_DECT_TETHER_IPV6_DHCPV6_SERVER)
	dect_tether_ipv6_dhcpv6_srv_deinit();
#endif
#if defined(CONFIG_DECT_TETHER_IPV6_RA)
	dect_tether_ipv6_ra_deinit();
#endif
	inited = false;
}
