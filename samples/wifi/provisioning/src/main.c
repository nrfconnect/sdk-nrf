/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <net/wifi_mgmt_ext.h>

#include "wifi_provisioning.h"

void main(void)
{
	int rc;
	struct net_if *iface = net_if_get_default();

	/* Sleep 1 seconds to allow initialization of wifi driver. */
	k_sleep(K_SECONDS(1));

	rc = wifi_prov_init();
	if (rc == 0) {
		printk("Wi-Fi provisioning service starts successfully.\n");
	} else {
		printk("Error occurred when initializing Wi-Fi provisioning service.\n");
		return;
	}

	rc = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0);
	if (rc) {
		printk("Error occurred when trying to auto-connect to a network. err: %d", rc);
	}
}
