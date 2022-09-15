/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BATTERY_MODULE_H_
#define BATTERY_MODULE_H_

/**
 * @defgroup fast_pair_sample_battery_module Fast Pair sample battery module
 * @brief Fast Pair sample battery module
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize battery module.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int battery_module_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BATTERY_MODULE_H_ */
