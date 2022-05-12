/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief RAM section power down header.
 */

#ifndef RAM_PWRDN_H__
#define RAM_PWRDN_H__

/**
 * @defgroup ram_pwrdn RAM Power Down
 * @{
 * @brief Module for powering on and off RAM sections.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Request powering down RAM sections for the given address range.
 *
 * Power down RAM sections which fully fall within the given address range.
 */
void power_down_ram(uintptr_t start_address, uintptr_t end_address);

/**
 * @brief Request powering up RAM sections for the given address range.
 *
 * Power up RAM sections which overlap with the given address range.
 */
void power_up_ram(uintptr_t start_address, uintptr_t end_address);

/**
 * @brief Request powering down unused RAM sections.
 *
 * Power down RAM sections which are not used by the application image.
 *
 * @note
 * Some libc implementations use the memory area following the application
 * image for heap. If this is the case and the application relies on dynamic
 * memory allocations, this function should not be used.
 */
void power_down_unused_ram(void);

/**
 * @brief Request powering up unused RAM sections.
 *
 * Power up RAM sections which were disabled by @ref power_down_unused_ram function.
 */
void power_up_unused_ram(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* RAM_PWRDN_H__ */
