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
typedef int (*nrf_cloud_connect_host_cb)(const struct net_sockaddr *addr, net_socklen_t addrlen);

/**
 * @brief Attempt to connect to an nRF Cloud host.
 *
 * Resolves the host, or uses the static IPv4 address if configured, for the address families
 * that are ready, and calls @p connect_cb for each address until one succeeds. Makes a single
 * round of attempts.
 *
 * @param host_name The nRF Cloud host name.
 * @param port The port to connect to.
 * @param socktype Socket type.
 * @param connect_cb Called for each address. Returns a nonnegative socket ID on success, or a
 *		     negative error code.
 * @retval -ENETDOWN The network is not ready.
 * @retval -EAGAIN Resolving the host failed temporarily.
 * @retval -EHOSTUNREACH The host could not be resolved.
 * @return Otherwise, the nonnegative socket ID, or the last error from @p connect_cb.
 */
int nrf_cloud_connect_host(const char *host_name, uint16_t port, int socktype,
			   nrf_cloud_connect_host_cb connect_cb);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_NET_UTIL_H_ */
