/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>

#include <modem/nrf_modem_lib.h>
#include <nrf_modem.h>

#include <net/dect/dect_net_l2.h>

#include <dect_tether_ipv6.h>

#if defined(CONFIG_DK_LIBRARY)
#include <dk_buttons_and_leds.h>
#endif

LOG_MODULE_REGISTER(dect_tether_ipv6_sample, CONFIG_LOG_DEFAULT_LEVEL);

BUILD_ASSERT(IS_ENABLED(CONFIG_DECT_DEFAULT_DEV_TYPE_PT),
	     "dect_tether_ipv6 is a PT-only sample");

void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{
	LOG_ERR("Modem fault: reason=%d, program_counter=0x%x", fault_info->reason,
		fault_info->program_counter);
	__ASSERT(false, "Modem crash detected, halting application");
}

int main(void)
{
	int err;
	struct net_if *dect;

	LOG_INF("=== DECT host-leg IPv6 sample (device type: PT) ===");

	dect = net_if_get_first_by_type(&NET_L2_GET_NAME(DECT));
	if (dect == NULL) {
		LOG_ERR("No DECT interface found");
		return -ENODEV;
	}

	err = nrf_modem_lib_init();
	if (err != 0) {
		LOG_ERR("nrf_modem_lib_init failed: %d", err);
		return err;
	}

#if defined(CONFIG_DK_LIBRARY)
	err = dk_leds_init();
	if (err != 0) {
		LOG_WRN("dk_leds_init failed: %d", err);
	}
#endif

	err = dect_tether_ipv6_init();
	if (err != 0) {
		LOG_ERR("dect_tether_ipv6_init failed: %d", err);
		return err;
	}

	LOG_INF("dect_tether_ipv6 active (RA + DHCPv6 per Kconfig)");
	return 0;
}
