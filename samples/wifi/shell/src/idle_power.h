/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef WIFI_SHELL_IDLE_POWER_H
#define WIFI_SHELL_IDLE_POWER_H

#include <zephyr/kernel.h>

#ifdef CONFIG_NRF7120_IDLE_DIAGNOSTICS

/**
 * @brief Initialize idle power monitoring
 *
 * Sets up GPIO markers and diagnostic output for power measurements.
 * This function should be called early in main() after basic initialization.
 *
 * @return 0 on success, negative error code on failure
 */
int idle_power_init(void);

/**
 * @brief Print power domain and regulator state
 *
 * Outputs diagnostic information about current power domains and regulators.
 * Useful for before/after snapshots when entering/exiting idle.
 *
 * @param phase String label for this power state ("idle", "active", etc.)
 */
void idle_power_print_snapshot(const char *phase);

#else

static inline int idle_power_init(void)
{
	return 0;
}

static inline void idle_power_print_snapshot(const char *phase)
{
	ARG_UNUSED(phase);
}

#endif

#endif /* WIFI_SHELL_IDLE_POWER_H */
