/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_BATTERY_H_
#define _FP_BATTERY_H_

#include <bluetooth/services/fast_pair/fast_pair.h>

/**
 * @defgroup fp_battery Fast Pair battery
 * @brief Internal API for Fast Pair battery module implementation
 *
 * Fast Pair battery module provides API for application to set battery data and for Fast Pair
 * subsystem to get battery data and use it when generating advertising data.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Fast Pair battery data related to all three batteries (left bud, right bud, case). */
struct bt_fast_pair_battery_data {
	/** Fast Pair battery values. */
	struct bt_fast_pair_battery batteries[BT_FAST_PAIR_BATTERY_COMP_COUNT];
};

/** Get battery data.
 *
 * @return Battery data.
 */
struct bt_fast_pair_battery_data fp_battery_get_battery_data(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_BATTERY_H_ */
