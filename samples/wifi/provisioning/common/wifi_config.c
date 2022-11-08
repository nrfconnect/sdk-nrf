/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "wifi_provisioning.h"
#include "wifi_prov_internal.h"

LOG_MODULE_DECLARE(wifi_prov, CONFIG_WIFI_PROVISIONING_LOG_LEVEL);

static void wifi_credentials_for_each_ssid_cb(void *cb_arg, const char *ssid, size_t ssid_len)
{
	struct wifi_credentials_personal *creds = cb_arg;

	if (creds->header.ssid_len != 0) {
		return;
	}

	int ret = wifi_credentials_get_by_ssid_personal_struct(ssid, ssid_len, creds);

	if (ret) {
		LOG_ERR("Failed to load WiFi credentials (err %d).", ret);
	}
}

void wifi_credentials_get_first(struct wifi_credentials_personal *creds)
{
	wifi_credentials_for_each_ssid(wifi_credentials_for_each_ssid_cb, creds);
}

bool wifi_has_config(void)
{
	struct wifi_credentials_personal creds = { 0 };

	wifi_credentials_get_first(&creds);

	return creds.header.ssid_len != 0;
}

int wifi_remove_config(void)
{
	struct wifi_credentials_personal creds = { 0 };

	wifi_credentials_get_first(&creds);
	if (creds.header.ssid_len == 0) {
		return 0;
	}
	return wifi_credentials_delete_by_ssid(creds.header.ssid, creds.header.ssid_len);
}
