/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Internal net lib utils for nrf_cloud. */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/conn_mgr_monitor.h>

#include "nrf_cloud_dns.h"

LOG_MODULE_REGISTER(nrf_cloud_dns, CONFIG_NRF_CLOUD_LOG_LEVEL);

int nrf_cloud_connect_host(const char *host_name, uint16_t port, int socktype,
			   nrf_cloud_connect_host_cb connect_cb)
{
	struct zsock_addrinfo hints = {
		.ai_family = IS_ENABLED(CONFIG_NRF_CLOUD_IPV6) ? NET_AF_INET6 : NET_AF_UNSPEC,
		.ai_socktype = socktype,
	};
	struct net_sockaddr numeric;
	struct zsock_addrinfo *res;
	uint32_t mask;
	int sock;

#if defined(CONFIG_NRF_CLOUD_STATIC_IPV4)
	/* Do not fall back to DNS if a static IP address is set */
	host_name = CONFIG_NRF_CLOUD_STATIC_IPV4_ADDR;
	hints.ai_family = NET_AF_INET;
#endif

	/* The wanted families, and DNS for host names */
	mask = hints.ai_family == NET_AF_INET  ? CONN_MGR_READY_IPV4 :
	       hints.ai_family == NET_AF_INET6 ? CONN_MGR_READY_IPV6 :
						 CONN_MGR_READY_IPV4 | CONN_MGR_READY_IPV6;
	if (!net_ipaddr_parse(host_name, strlen(host_name), &numeric)) {
		mask |= CONN_MGR_READY_DNS;
	}

	if (conn_mgr_wait_ready(mask, K_NO_WAIT)) {
		LOG_WRN("Network not ready");
		return -ENETDOWN;
	}

	/* Only query the ready family, if only one is */
	if (hints.ai_family == NET_AF_UNSPEC) {
		hints.ai_family = conn_mgr_get_ready_ai_family();
	}

	sock = zsock_getaddrinfo(host_name, NULL, &hints, &res);
	if (sock) {
		LOG_WRN("Failed to resolve %s, error: %d", host_name, sock);
		return sock == DNS_EAI_AGAIN ? -EAGAIN : -EHOSTUNREACH;
	}

	sock = -EHOSTUNREACH;
	for (struct zsock_addrinfo *ai = res; ai != NULL && sock < 0; ai = ai->ai_next) {
		char ip[NET_IPV6_ADDR_LEN] = "";

		/* Skips addresses of disabled families */
		if (net_port_set(ai->ai_addr, port)) {
			continue;
		}

		(void)net_addr_ntop(ai->ai_family,
				    ai->ai_family == NET_AF_INET6 ?
					    (void *)&net_sin6(ai->ai_addr)->sin6_addr :
					    (void *)&net_sin(ai->ai_addr)->sin_addr,
				    ip, sizeof(ip));
		LOG_INF("Connecting to %s, port %u", ip, port);

		sock = connect_cb(ai->ai_addr, ai->ai_addrlen);
	}

	zsock_freeaddrinfo(res);

	if (sock < 0) {
		LOG_WRN("Cannot connect to nRF Cloud host: %s, error: %d", host_name, sock);
		return sock;
	}

	LOG_DBG("Connected to nRF Cloud host: %s. Socket ID: %d", host_name, sock);
	return sock;
}
