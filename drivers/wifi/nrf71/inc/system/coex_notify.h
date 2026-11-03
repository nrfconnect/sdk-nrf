/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Wi-Fi driver hooks feeding the coexistence state notifier.
 *
 * The hooks are no-ops unless the nRF71 coexistence driver is enabled, so the
 * call sites in the Wi-Fi driver need no conditional compilation.
 */

#ifndef NRF71_WIFI_COEX_NOTIFY_H__
#define NRF71_WIFI_COEX_NOTIFY_H__

#include <stddef.h>
#include <stdint.h>

#ifdef CONFIG_NRF71_SR_COEX_DRIVER

/**
 * @brief A Wi-Fi interface reached the operational UP state.
 *
 * Sends ADMIN_UP on the first interface.
 */
void nrf_wifi_coex_vif_up(void);

/**
 * @brief A Wi-Fi interface that was UP is about to be torn down.
 *
 * Sends ADMIN_DOWN on the last interface. Must be called before the interface
 * is removed from the RPU.
 */
void nrf_wifi_coex_vif_down(void);

/**
 * @brief The FMAC device is being removed.
 *
 * Sends ADMIN_DOWN if any interface is still accounted as UP.
 */
void nrf_wifi_coex_dev_rem(void);

/**
 * @brief The station associated.
 *
 * @param primary_freq_mhz Primary channel frequency in MHz.
 * @param ies              Association response IEs, used to find the HT
 *                         Operation element for the channel width.
 * @param ies_len          Length of @p ies in bytes.
 */
void nrf_wifi_coex_sta_connected(unsigned int primary_freq_mhz, const uint8_t *ies,
				 size_t ies_len);

/** @brief The station disassociated. No-op if it was not connected. */
void nrf_wifi_coex_sta_disconnected(void);

#else

static inline void nrf_wifi_coex_vif_up(void)
{
}

static inline void nrf_wifi_coex_vif_down(void)
{
}

static inline void nrf_wifi_coex_dev_rem(void)
{
}

static inline void nrf_wifi_coex_sta_connected(unsigned int primary_freq_mhz,
					       const uint8_t *ies, size_t ies_len)
{
}

static inline void nrf_wifi_coex_sta_disconnected(void)
{
}

#endif /* CONFIG_NRF71_SR_COEX_DRIVER */

#endif /* NRF71_WIFI_COEX_NOTIFY_H__ */
