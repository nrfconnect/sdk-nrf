/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Header file for internal nrf_cloud DNS utils. */

#ifndef NRF_CLOUD_NET_UTIL_H_
#define NRF_CLOUD_NET_UTIL_H_

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** @brief Callback called for every address tried by nrf_cloud_connect_host.
 * Should return either a valid socket, or a negative error code.
 */
typedef int (*nrf_cloud_connect_host_cb)(struct sockaddr *const addr);

/**
 * @brief Repeatedly use getaddrinfo to try first IPv6, then IPv4 addresses for a given hostname.
 * Calls the provided callback for each address found.
 * The callback should attempt to connect a socket to the provided address, and return either
 * the successfully-connected socket, or a negative error code.
 */

/**
 * @brief Attempt to connect to a provided nRF Cloud hostname.
 *
 * This function performs DNS lookup of the provided hostname, then tries to connect to each
 * address found, until successful.
 *
 * If a static IPv4 address is configured, this will be attempted first. Then, IPv6 addresses will
 * be searched for and tried. Finally, IPv4 addresses will be searched for and tried.
 *
 * The provided connect callback is called for each IP address tried. This callback should attempt
 * to connect to the provided IP address, and either return a negative error code on failure, or a
 * nonnegative socket ID on success.
 *
 * This function will return either a negative error code if all found IP addresses fail, or the
 * socket ID of the first successful connection.
 *
 * @param host_name The nRF Cloud hostname to attempt to connect to.
 * @param port The port to attempt to connect to.
 * @param hints Additional hints to pass to getaddrinfo.
 *		The ai_family property will be overwritten.
 * @param connect_cb A callback which, when provided with an IP address, should attempt to connect
 *		     to it, and return either a negative error code on failure, or the nonnegative
 *		     socket ID of the successful connection.
 * @return int Either a negative error code on failure, or a nonnegative socket ID on success.
 */
int nrf_cloud_connect_host(const char *host_name, uint16_t port, struct zsock_addrinfo *hints,
			   nrf_cloud_connect_host_cb connect_cb);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_NET_UTIL_H_ */
