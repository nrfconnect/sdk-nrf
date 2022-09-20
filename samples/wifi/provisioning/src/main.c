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
#include <zephyr/zephyr.h>

#include "wifi_provisioning.h"

void main(void)
{
	/** Sleep 10 seconds to allow initialization of wifi driver. */
	k_sleep(K_SECONDS(10));
	int rc;

	rc = wifi_config_init();
	if (rc != 0) {
		printk("Initializing config module failed, err = %d", rc);
	}

	rc = wifi_prov_init();

	if (rc == 0) {
		printk("Wi-Fi provisioning service starts successfully.\n");
	} else {
		printk("Error occurs when initializing Wi-Fi provisioning service.\n");
		return;
	}

	if (wifi_has_config() == true) {
		struct wifi_config_t config;

		rc = wifi_get_config(&config);
		if (rc == 0) {
			printk("Configuration found. Try to apply.\n");
			struct net_if *iface = net_if_get_default();

			struct wifi_connect_req_params cnx_params = { 0 };

			cnx_params.ssid = config.ssid;
			cnx_params.ssid_length = config.ssid_len;
			cnx_params.security = config.auth_type;

			cnx_params.psk = NULL;
			cnx_params.psk_length = 0;
			cnx_params.sae_password = NULL;
			cnx_params.sae_password_length = 0;

			if (config.auth_type != WIFI_SECURITY_TYPE_NONE) {
				cnx_params.psk = config.password;
				cnx_params.psk_length = config.password_len;
			}

			cnx_params.channel = config.channel;
			cnx_params.band = config.band;
			cnx_params.mfp = WIFI_MFP_OPTIONAL;
			rc = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
			&cnx_params, sizeof(struct wifi_connect_req_params));
		}

	}

	while (true) {
		k_sleep(K_SECONDS(1));
	}
}
