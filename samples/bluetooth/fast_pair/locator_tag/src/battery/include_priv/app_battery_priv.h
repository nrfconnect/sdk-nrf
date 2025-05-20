/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef APP_BATTERY_PRIV_H_
#define APP_BATTERY_PRIV_H_

#include <stdint.h>

/**
 * @defgroup fmdn_sample_battery_priv Locator Tag sample battery module private API
 * @brief Locator Tag sample battery module private API
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Log the battery level in an uniform way. You cannot use the
 *  @ref BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE value when the
 *  CONFIG_BT_FAST_PAIR_FMDN_BATTERY_DULT is enabled.
 *
 * @param percentage_level Battery level as a percentage [0-100%] or
 *                         @ref BT_FAST_PAIR_FMDN_BATTERY_LEVEL_NONE value.
 */
void app_battery_level_log(uint8_t percentage_level);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* APP_BATTERY_PRIV_H_ */
