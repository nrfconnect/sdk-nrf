/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

#include <net/wifi_prov_core/wifi_prov_core.h>

LOG_MODULE_REGISTER(wifi_prov_internal, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
	LOG_INF("WiFi Provisioning Internal Sample");
	LOG_INF("This sample demonstrates the core WiFi provisioning library functionality");

#if defined(CONFIG_WIFI_PROV_CONFIG)
	/* Initialize the WiFi provisioning service */
	int ret;

	ret = wifi_prov_init();
	if (ret < 0) {
		LOG_ERR("Failed to initialize WiFi provisioning service: %d", ret);
		return -1;
	}
	LOG_INF("WiFi provisioning service initialized successfully");
#endif

	return 0;
}
