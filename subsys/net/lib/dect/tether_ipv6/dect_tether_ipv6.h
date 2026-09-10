/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file dect_tether_ipv6.h
 * @brief Host-leg IPv6 for DECT tether: minimal RA (default router + RDNSS + M) and optional
 *        minimal DHCPv6 server (ULA + GUA /128 derived from @c dect0).
 */

#ifndef DECT_TETHER_IPV6_H__
#define DECT_TETHER_IPV6_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dect_tether_ipv6 DECT IPv6 tethering library
 * @brief IPv6 gateway for a tethered host on Ethernet with a DECT NR+ uplink.
 *
 * @{
 */

/**
 * @brief Start RA and/or DHCPv6 server (per Kconfig).
 *
 * Call after nrf_modem_lib_init() and with the network stack initialized.
 *
 * @return 0 on success, negative errno on failure.
 */
int dect_tether_ipv6_init(void);

/**
 * @brief Stop RA and DHCPv6 server threads/handlers.
 */
void dect_tether_ipv6_deinit(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* DECT_TETHER_IPV6_H__ */
