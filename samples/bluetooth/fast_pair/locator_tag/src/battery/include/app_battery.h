/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef APP_BATTERY_H_
#define APP_BATTERY_H_

#include <stdint.h>

/**
 * @defgroup fmdn_sample_battery Locator Tag sample battery module
 * @brief Locator Tag sample battery module
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the battery module.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int app_battery_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* APP_BATTERY_H_ */
