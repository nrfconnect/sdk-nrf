/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Internal net lib utils for nrf_cloud. */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <net/connect_by_name.h>

#include "nrf_cloud_dns.h"

LOG_MODULE_REGISTER(nrf_cloud_dns, CONFIG_NRF_CLOUD_LOG_LEVEL);

int nrf_cloud_connect_host(struct connect_by_name *ctx, struct connect_by_name_cfg *cfg)
{
	int sock;

#if defined(CONFIG_NRF_CLOUD_STATIC_IPV4)
	/* Do not fall back to DNS if a static IP address is set */
	cfg->host = CONFIG_NRF_CLOUD_STATIC_IPV4_ADDR;
	cfg->family = NET_AF_INET;
#else
	cfg->family = IS_ENABLED(CONFIG_NRF_CLOUD_IPV6) ? NET_AF_INET6 : NET_AF_UNSPEC;
#endif

	sock = connect_by_name_init(ctx, cfg);
	if (sock) {
		return sock;
	}

	sock = connect_by_name(ctx);
	if (sock < 0) {
		connect_by_name_deinit(ctx);
		LOG_WRN("Cannot connect to nRF Cloud host: %s, error: %d", cfg->host, sock);
		return sock;
	}

	LOG_DBG("Connected to nRF Cloud host: %s. Socket ID: %d", cfg->host, sock);
	return sock;
}
