/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <nrfx_clock.h>

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>

void test_change_mac(void)
{
	struct net_if *iface = net_if_get_default();
	char mac_addr_change[]={0xf4, 0xce, 0x36, 0x11, 0x10, 0xb4};

	struct ethernet_req_params params;
	int ret;

	memcpy(params.mac_address.addr, mac_addr_change, 6);

	net_if_down(iface);

	ret = net_mgmt(NET_REQUEST_ETHERNET_SET_MAC_ADDRESS, iface,
		       &params, sizeof(struct ethernet_req_params));

	if(ret != 0) {
		printk("unable to change mac address");
		return;
	}

	ret = memcmp(net_if_get_link_addr(iface)->addr, mac_addr_change,
		     sizeof(mac_addr_change));

	if(ret != 0) {
		printk("mac address change failed");
		return;
	}

	net_if_up(iface);
}

void main(void)
{
#ifdef CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT
	/* For now hardcode to 128MHz */
	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK,
			       NRF_CLOCK_HFCLK_DIV_1);
#endif
	printk("Starting %s with CPU frequency: %d MHz\n", CONFIG_BOARD, SystemCoreClock/MHZ(1));

	test_change_mac();
}
