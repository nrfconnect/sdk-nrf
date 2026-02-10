/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include "vendor_specific.h"
#include "utils.h"
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/net_if.h>

void interfaces_init(void)
{
}
/* Be invoked when start controlApp */
void vendor_init(void)
{
}

/* Be invoked when terminate controlApp */
void vendor_deinit(void)
{
}

/* Called by reset_device_handler() */
void vendor_device_reset(void)
{
}

#ifdef CONFIG_AP
/* Called by configure_ap_handler() */
void configure_ap_enable_mbssid(void)
{
	/*TODO: Implement this for zephyr */
}

void configure_ap_radio_params(char *band, char *country, int channel, int chwidth)
{
	/*TODO: Implement this for zephyr */
}

/* void (*callback_fn)(void *), callback of active wlans iterator
 *
 * Called by start_ap_handler() after invoking hostapd
 */
void start_ap_set_wlan_params(void *if_info)
{
	/*TODO: Implement this for zephyr */
}
#endif /* End Of CONFIG_AP*/

#ifdef CONFIG_P2P
/* Return addr of P2P-device if there is no GO or client interface */
int get_p2p_mac_addr(char *mac_addr, size_t size)
{
	/*TODO: Implement this for zephyr */

	return 0;
}

/* Get the name of P2P Group(GO or Client) interface */
int get_p2p_group_if(char *if_name, size_t size)
{
	snprintf(if_name, size, "%s", get_wireless_interface());
	return 0;
}

int get_p2p_dev_if(char *if_name, size_t size)
{
	snprintf(if_name, size, "%s", get_wireless_interface());

	return 0;
}
#endif /* End Of CONFIG_P2P */

/* Append IP range config and start dhcpd */
void start_dhcp_server(char *if_name, char *ip_addr)
{
	struct net_if *iface;
	struct in_addr pool_start;
	int ret = -1;

	iface = net_if_get_first_wifi();
	if (!iface) {
		indigo_logger(LOG_LEVEL_ERROR, "Failed to get Wi-Fi interface");
		return;
	}

	if (net_addr_pton(AF_INET, CONFIG_WFA_QT_SAMPLE_DHCPV4_POOL_START, &pool_start.s_addr)) {
		indigo_logger(LOG_LEVEL_ERROR, "Invalid address: %s",
			      CONFIG_WFA_QT_SAMPLE_DHCPV4_POOL_START);
		return;
	}

	ret = net_dhcpv4_server_start(iface, &pool_start);
	if (ret == -EALREADY) {
		indigo_logger(LOG_LEVEL_ERROR, "DHCPv4 server already running on interface");
	} else if (ret < 0) {
		indigo_logger(LOG_LEVEL_ERROR,
			      "DHCPv4 server failed to start and returned %d error", ret);
	} else {
		indigo_logger(LOG_LEVEL_ERROR,
			      "DHCPv4 server started and pool address starts from %s",
			      CONFIG_WFA_QT_SAMPLE_DHCPV4_POOL_START);
	}

	return;
}

void stop_dhcp_server(void)
{
	/*TODO: Implement this for zephyr */
}

void start_dhcp_client(char *if_name)
{
	/*TODO: Implement this for zephyr */
}

void stop_dhcp_client(void)
{
	/*TODO: Implement this for zephyr */
}

#ifdef CONFIG_WPS
void save_wsc_setting(wps_setting *s, char *entry, int len)
{
	/*TODO: Implement this for zephyr */
}

wps_setting *__get_wps_setting(int len, char *buffer, enum wps_device_role role)
{
	/*TODO: Implement this for zephyr */

	return NULL;
}

wps_setting *get_vendor_wps_settings(enum wps_device_role role)
{
	/*TODO: Implement this for zephyr */

	return NULL;
}
#endif /* End Of CONFIG_WPS */
