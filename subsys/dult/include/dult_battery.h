/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _DULT_BATTERY_H_
#define _DULT_BATTERY_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup dult_battery Detecting Unwanted Location Trackers battery
 * @brief Private API for DULT specification implementation of the battery module
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Encode the battery type configuration.
 *  The configuration is encoded as required by the DULT specification.
 *
 * @return Byte with an encoded information about the battery type.
 */
uint8_t dult_battery_type_encode(void);

/** Encode the battery level configuration.
 *  The configuration is encoded as required by the DULT specification.
 *
 * @return Byte with an encoded information about the battery level.
 */
uint8_t dult_battery_level_encode(void);

/** @brief Enable DULT battery.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_battery_enable(void);

/** @brief Reset DULT battery.
 *
 *  @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int dult_battery_reset(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _DULT_BATTERY_H_ */
