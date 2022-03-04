/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_ADV_HELPER_H_
#define BT_ADV_HELPER_H_

#include <bluetooth/bluetooth.h>

/**
 * @defgroup fast_pair_sample_adv_helper Fast Pair sample advertising helper API
 * @brief Fast Pair sample advertising helper API
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Start Fast Pair sample advertising
 *
 * Include: device name, TX power and Fast Pair advertising data into an advertising packet and
 * start advertising.
 *
 * The function handles also periodic RPA rotation and Fast Pair advertising data update.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_adv_helper_adv_start(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_ADV_HELPER_H_ */
