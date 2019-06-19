/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**@file
 *
 * @defgroup battery_charge Battery Charge
 * @brief  Module that governs the battery charge of the system.
 * @{
 */

#ifndef BATTERY_CHARGE_H__
#define BATTERY_CHARGE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init the battery charge monitor module.
 *
 */
void battery_monitor_init(void);

/**
 * @brief Read the current battery charge.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int battery_monitor_read(u8_t *buf);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_CHARGE_H__ */
