/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_ADV_PROV_FAST_PAIR_H_
#define BT_ADV_PROV_FAST_PAIR_H_

#include <bluetooth/services/fast_pair/fast_pair.h>

/**
 * @defgroup bt_le_adv_prov_fast_pair Fast Pair advertising data provider API
 * @brief Fast Pair advertising data provider API
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Enable/disable Fast Pair advertising provider.
 *
 * User shall make sure that this function is not called while Fast Pair advertising data provider
 * is providing advertising data.
 *
 * @param[in] enable		Enable/disable Fast Pair provider.
 */
void bt_le_adv_prov_fast_pair_enable(bool enable);

/** Show/hide UI indication in Fast Pair not discoverable advertising.
 *
 * This configuration does not affect Fast Pair discoverable advertising.
 *
 * User shall make sure that this function is not called while Fast Pair advertising data provider
 * is providing advertising data.
 *
 * @param[in] enable		Show/hide the UI indication.
 */
void bt_le_adv_prov_fast_pair_show_ui_pairing(bool enable);

/** Set advertising battery mode in Fast Pair not dicoverable advertising.
 *
 * To prevent tracking, the Fast Pair Provider should not include battery data in the advertising
 * packet all the time.
 *
 * User shall make sure that this function is not called while Fast Pair advertising data provider
 * is providing advertising data.
 *
 * @param[in] mode		Advertising battery mode.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_le_adv_prov_fast_pair_set_battery_mode(enum bt_fast_pair_adv_battery_mode mode);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_ADV_PROV_FAST_PAIR_H_ */
