/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Internal net lib utils for nrf_cloud. */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "nrf_cloud_dns.h"

LOG_MODULE_REGISTER(nrf_cloud_dns, CONFIG_NRF_CLOUD_LOG_LEVEL);

static int nrf_cloud_try_addresses(const char *const host_name, uint16_t port,
				   struct zsock_addrinfo *hints,
				   nrf_cloud_connect_host_cb connect_cb)
{
	int err = 0;
	struct zsock_addrinfo *info;
	struct zsock_addrinfo *result;

	err = zsock_getaddrinfo(host_name, NULL, hints, &info);
	if (err) {
		LOG_DBG("getaddrinfo for %s, port: %d failed: %d, errno: %d", host_name, port, err,
			errno);
		return -EADDRNOTAVAIL;
	}

	/* Hold copy of original result so that we can free it later. */
	result = info;

	/* Try to connect with whatever IP addresses we get.
	 * Stop and return the socket if one succeeds.
	 */
	for (; info != NULL; info = info->ai_next) {
		char ip[INET6_ADDRSTRLEN] = {0};
		struct sockaddr *const sa = info->ai_addr;

		switch (sa->sa_family) {
		case AF_INET6:
			((struct sockaddr_in6 *)sa)->sin6_port = port;
			break;
		case AF_INET:
			((struct sockaddr_in *)sa)->sin_port = port;
			break;
		}

		zsock_inet_ntop(sa->sa_family, (void *)&((struct sockaddr_in *)sa)->sin_addr, ip,
				sizeof(ip));

		LOG_DBG("Trying IP address and port for server %s: %s, port: %d", host_name, ip,
			port);

		/* The try_socket callback will attempt to create a socket, and either return that,
		 * or a negative error code.
		 */
		int sock = connect_cb(sa);

		if (sock < 0) {
			LOG_DBG("Failed to connect to IP address %s. Error: %d", ip, sock);
			continue;
		}

		/* Pass the socket back to the initial caller, if creating/connecting it was
		 * successful.
		 */
		zsock_freeaddrinfo(result);
		return sock;
	}

	zsock_freeaddrinfo(result);
	return -ECONNREFUSED;
}

int nrf_cloud_connect_host(const char *hostname, uint16_t port, struct zsock_addrinfo *hints,
			   nrf_cloud_connect_host_cb connect_cb)
{
	int sock;

	if (hints == NULL) {
		return -EINVAL;
	}

	LOG_DBG("Connecting to nRF Cloud");

#if defined(CONFIG_NRF_CLOUD_STATIC_IPV4)
	static struct sockaddr_in static_addr;
	uint16_t static_port = htons(CONFIG_NRF_CLOUD_PORT);

	LOG_DBG("Trying static IPv4 address: %s, port: %d", CONFIG_NRF_CLOUD_STATIC_IPV4_ADDR,
		port);

	zsock_inet_pton(AF_INET, CONFIG_NRF_CLOUD_STATIC_IPV4_ADDR, &(static_addr.sin_addr));
	static_addr.sin_family = AF_INET;
	static_addr.sin_port = static_port;

	sock = connect_cb((struct sockaddr *)&static_addr);

	if (sock >= 0) {
		goto out;
	}

	LOG_DBG("Failed to connect to static IP address %s", CONFIG_NRF_CLOUD_STATIC_IPV4_ADDR);

	/* Do not fall back to DNS if static IP address is set. */
	goto out;
#endif

	/* Query IPv6 first, then IPv4. This is necessary because some socket backends do not
	 * return all available IP addresses, so if we just use AF_UNSPEC, we might get only an
	 * IPv6 address, even if both IPv4 and IPv6 are available.
	 */

#if defined(CONFIG_NET_IPV6)
	LOG_DBG("Trying IPv6 addresses for %s", hostname);

	hints->ai_family = AF_INET6;
	sock = nrf_cloud_try_addresses(hostname, port, hints, connect_cb);

	if (sock >= 0) {
		goto out;
	}

#endif /* defined (CONFIG_NET_IPV6) */

#if defined(CONFIG_NET_IPV4) && !defined(CONFIG_NRF_CLOUD_IPV6)
	LOG_DBG("Trying IPv4 addresses for %s", hostname);

	hints->ai_family = AF_INET;
	sock = nrf_cloud_try_addresses(hostname, port, hints, connect_cb);

	if (sock >= 0) {
		goto out;
	}

#endif /* defined (CONFIG_NET_IPV4) */

out:
	if (sock < 0) {
		LOG_DBG("Cannot connect to nRF Cloud host: %s, error: %d", hostname, sock);
		return -ECONNREFUSED;
	}

	LOG_DBG("Connected to nRF Cloud host: %s. Socket ID: %d", hostname, sock);
	return sock;
}
