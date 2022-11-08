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

#include "wifi_provisioning.h"

void main(void)
{
	int rc;
	struct net_if *iface = net_if_get_default();
	struct wifi_connect_req_params cnx_params = { 0 };
	struct wifi_credentials_personal creds = { 0 };

	/* Sleep 1 seconds to allow initialization of wifi driver. */
	k_sleep(K_SECONDS(1));

	rc = wifi_prov_init();
	if (rc == 0) {
		printk("Wi-Fi provisioning service starts successfully.\n");
	} else {
		printk("Error occurs when initializing Wi-Fi provisioning service.\n");
		return;
	}

	wifi_credentials_get_first(&creds);

	if (creds.header.ssid_len != 0) {
		cnx_params.ssid = creds.header.ssid;
		cnx_params.ssid_length = creds.header.ssid_len;
		cnx_params.security = creds.header.type;
		cnx_params.psk = NULL;
		cnx_params.psk_length = 0;
		cnx_params.sae_password = NULL;
		cnx_params.sae_password_length = 0;
		if (cnx_params.security == WIFI_SECURITY_TYPE_PSK ||
		cnx_params.security == WIFI_SECURITY_TYPE_PSK_SHA256) {
			cnx_params.psk = creds.password;
			cnx_params.psk_length = creds.password_len;
		}
		if (cnx_params.security == WIFI_SECURITY_TYPE_SAE) {
			cnx_params.sae_password = creds.password;
			cnx_params.sae_password_length = creds.password_len;
		}

		cnx_params.channel = WIFI_CHANNEL_ANY;
		cnx_params.band = 255;
		cnx_params.mfp = WIFI_MFP_OPTIONAL;

		rc = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
			&cnx_params, sizeof(struct wifi_connect_req_params));
		if (rc < 0) {
			printk("Cannot connect to saved network, err = %d.\n", rc);
		} else {
			printk("Configuration applied.\n");
		}
	} else {
		printk("Device is unprovisioned.\n");
	}
}
