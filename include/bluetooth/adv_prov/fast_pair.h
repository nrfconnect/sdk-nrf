/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_ADV_PROV_FAST_PAIR_H_
#define BT_ADV_PROV_FAST_PAIR_H_

#include <bluetooth/services/fast_pair.h>

/**
 * @defgroup bt_le_adv_prov_fast_pair Fast Pair advertising data provider API
 * @brief Fast Pair advertising data provider API
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Set Fast Pair advertising mode.
 *
 * User shall make sure that advertising mode is not modified while Fast Pair advertising data
 * provider is providing advertising data.
 *
 * @param[in] fp_adv_mode	Fast Pair advertising mode.
 */
void bt_le_adv_prov_fast_pair_mode_set(enum bt_fast_pair_adv_mode fp_adv_mode);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_ADV_PROV_FAST_PAIR_H_ */
