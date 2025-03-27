/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DEVICE_MANAGEMANT_H__
#define DEVICE_MANAGEMANT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Definitions below reflect a way of usage of manifest-controlled variables
 * in this specific application.
 */
#define BOOT_STATUS_A		1
#define BOOT_STATUS_B		2
#define BOOT_STATUS_A_DEGRADED	3
#define BOOT_STATUS_B_DEGRADED	4
#define BOOT_STATUS_A_NO_RADIO	5
#define BOOT_STATUS_B_NO_RADIO	6
#define BOOT_STATUS_CANNOT_BOOT 7

/** @brief Performs a self-tests and confirms
 * successful firmware update, if needed
 */
void device_healthcheck(void);

/** @brief Toggles preferred boot set
 */
void toggle_preferred_boot_set(void);

/** @brief Displays information about state of image sets,
 * boot preference and a boot status
 */
void device_boot_state_report(void);

/** @brief Checks if the device is in recovery mode
 */
bool device_is_in_recovery_mode(void);

/** @brief Gets the boot status of the device
 */
uint32_t device_boot_status_get(void);

#endif /* DEVICE_MANAGEMANT_H__ */
