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
#include <net/connect_by_name.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Attempt to connect to an nRF Cloud host.
 *
 * Sets the address family of @p cfg, and its host if a static IPv4 address is configured, then
 * makes one round of connection attempts, see @ref connect_by_name.
 *
 * On success, @p ctx stays initialized so that @p cfg->lost_cb can be called. Call
 * connect_by_name_deinit() when the connection ends.
 *
 * @param ctx Context. Must remain valid until deinitialized.
 * @param cfg Configuration with host, port, socktype, connect_cb and optionally lost_cb set.
 *	      Must remain valid until @p ctx is deinitialized.
 * @return int Either a negative error code on failure, or a nonnegative socket ID on success.
 */
int nrf_cloud_connect_host(struct connect_by_name *ctx, struct connect_by_name_cfg *cfg);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_NET_UTIL_H_ */
