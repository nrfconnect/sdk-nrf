/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF71 idle power optimization helpers.
 */

#ifndef NRF71_IDLE_POWER_H__
#define NRF71_IDLE_POWER_H__

#include <zephyr/kernel.h>

/**
 * @defgroup nrf71_idle_power nRF71 idle power helpers
 * @{
 *
 * @brief Lower nRF71 System ON idle current.
 *
 * The RAM retention level selected through the NRF71_IDLE_POWER_RAM_RETAIN
 * choice is applied automatically at boot from a SYS_INIT hook, so an
 * application only needs to enable the Kconfig options -- no code changes.
 * The snapshot helper below is optional and only for measurement builds.
 */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NRF71_IDLE_DIAGNOSTICS)

/**
 * @brief Print a power domain, regulator and RAM retention snapshot.
 *
 * Dumps the raw POWER, MEMCONF, GRTC and LFXO registers so the state can be
 * correlated with a power analyzer trace. Available only when
 * @kconfig{CONFIG_NRF71_IDLE_DIAGNOSTICS} is enabled.
 *
 * @param phase Free-form label for the current phase ("idle", "active", ...).
 */
void nrf71_idle_power_print_snapshot(const char *phase);

#else

static inline void nrf71_idle_power_print_snapshot(const char *phase)
{
	ARG_UNUSED(phase);
}

#endif /* CONFIG_NRF71_IDLE_DIAGNOSTICS */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NRF71_IDLE_POWER_H__ */
