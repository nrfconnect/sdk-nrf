/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Internal net lib utils for nrf_cloud. */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>

#include "nrf_cloud_dns.h"

LOG_MODULE_REGISTER(nrf_cloud_dns, CONFIG_NRF_CLOUD_LOG_LEVEL);

#if defined(CONFIG_NRF_MODEM_LIB)

#include <nrf_modem_at.h>

/* On the nRF91 modem-offloaded stack, ask the modem itself as the Zephyr
 * net interfaces only get populated at a later stage.
 */
static bool modem_has_addr(bool ipv6)
{
	char addr1[NET_IPV6_ADDR_LEN] = {0};
	char addr2[NET_IPV6_ADDR_LEN] = {0};
	char tmp[sizeof(struct net_in6_addr)];
	int ret;

	ret = nrf_modem_at_scanf("AT+CGPADDR=0",
				  "+CGPADDR: %*d,\"%46[.:0-9A-F]\",\"%46[:0-9A-F]\"",
				  addr1, addr2);
	if (ret <= 0) {
		return false;
	}

	if (!ipv6) {
		/* IPv4 address, if present, is always at slot 1 */
		return zsock_inet_pton(AF_INET, addr1, tmp) == 1;
	}

	/* IPv6 address, can be at slot 1 or 2 */
	return zsock_inet_pton(AF_INET6, addr1, tmp) == 1 ||
	       zsock_inet_pton(AF_INET6, addr2, tmp) == 1;
}

static bool ipv6_ready(void)
{
	return modem_has_addr(true);
}

static bool ipv4_ready(void)
{
	return modem_has_addr(false);
}

#else

static bool ipv6_ready(void)
{
	struct net_if *iface = NULL;

	return net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &iface) != NULL;
}

static bool ipv4_ready(void)
{
	struct net_if *iface = net_if_get_default();

	return (iface != NULL) &&
	       (net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED) != NULL);
}

#endif /* defined(CONFIG_NRF_MODEM_LIB) */

static int nrf_cloud_try_addresses(const char *const host_name, uint16_t port,
				   struct zsock_addrinfo *hints,
				   nrf_cloud_connect_host_cb connect_cb)
{
	int err = 0;
	struct zsock_addrinfo *info;
	struct zsock_addrinfo *result;

	err = zsock_getaddrinfo(host_name, NULL, hints, &info);
	if (err) {
		LOG_ERR("getaddrinfo for %s, port: %d failed: %d, errno: %d", host_name, port, err,
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
			((struct sockaddr_in6 *)sa)->sin6_port = htons(port);
			break;
		case AF_INET:
			((struct sockaddr_in *)sa)->sin_port = htons(port);
			break;
		}

		const void *const sin_addr = (sa->sa_family == AF_INET6) ?
			(void *)&((struct sockaddr_in6 *)sa)->sin6_addr :
			(void *)&((struct sockaddr_in *)sa)->sin_addr;

		zsock_inet_ntop(sa->sa_family, sin_addr, ip, sizeof(ip));

		LOG_DBG("Trying IP address and port for server %s: %s, port: %d", host_name, ip,
			port);

		/* The try_socket callback will attempt to create a socket, and either return that,
		 * or a negative error code.
		 */
		int sock = connect_cb(sa);

		if (sock < 0) {
			LOG_WRN("Failed to connect to server %s via IP address %s, port %d."
				"Error: %d", host_name, ip, port, sock);
			continue;
		}

		LOG_INF("Connected to server %s via %s address %s, port %d", host_name,
				(sa->sa_family == AF_INET6) ? "IPv6" : "IPv4", ip, port);

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
	int sock = -ENETUNREACH;

	if (hints == NULL) {
		return -EINVAL;
	}

	LOG_DBG("Connecting to nRF Cloud");

#if defined(CONFIG_NRF_CLOUD_STATIC_IPV4)
	static struct sockaddr_in static_addr;
	uint16_t static_port = CONFIG_NRF_CLOUD_PORT;

	LOG_DBG("Trying static IPv4 address: %s, port: %d", CONFIG_NRF_CLOUD_STATIC_IPV4_ADDR,
		port);

	zsock_inet_pton(AF_INET, CONFIG_NRF_CLOUD_STATIC_IPV4_ADDR, &(static_addr.sin_addr));
	static_addr.sin_family = AF_INET;
	static_addr.sin_port = htons(static_port);

	sock = connect_cb((struct sockaddr *)&static_addr);

	if (sock >= 0) {
		goto out;
	}

	LOG_WRN("Failed to connect to static IP address %s", CONFIG_NRF_CLOUD_STATIC_IPV4_ADDR);

	/* Do not fall back to DNS if static IP address is set. */
	goto out;
#endif

	/* Query IPv6 first, then IPv4. This is necessary because some socket backends do not
	 * return all available IP addresses, so if we just use AF_UNSPEC, we might get only an
	 * IPv6 address, even if both IPv4 and IPv6 are available.
	 */

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		if (!ipv6_ready()) {
			LOG_DBG("Skipping IPv6 for %s; no local IPv6 address", hostname);
		} else {
			LOG_DBG("Trying IPv6 addresses for %s", hostname);

			hints->ai_family = AF_INET6;
			sock = nrf_cloud_try_addresses(hostname, port, hints, connect_cb);

			if (sock >= 0) {
				goto out;
			} else {
				LOG_DBG("Could not connect over IPv6 to %s, falling back to IPv4",
					hostname);
			}
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && !IS_ENABLED(CONFIG_NRF_CLOUD_IPV6)) {
		if (!ipv4_ready()) {
			LOG_DBG("Skipping IPv4 for %s; no local IPv4 address", hostname);
		} else {
			LOG_DBG("Trying IPv4 addresses for %s", hostname);

			hints->ai_family = AF_INET;
			sock = nrf_cloud_try_addresses(hostname, port, hints, connect_cb);

			if (sock >= 0) {
				goto out;
			} else {
				LOG_DBG("Could not connect over IPv4 to %s", hostname);
			}
		}
	}

out:
	if (sock < 0) {
		LOG_WRN("Cannot connect to nRF Cloud host: %s, error: %d", hostname, sock);
		return sock;
	}

	LOG_DBG("Connected to nRF Cloud host: %s. Socket ID: %d", hostname, sock);
	return sock;
}
