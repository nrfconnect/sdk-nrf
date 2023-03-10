/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_ADV_PROV_SWIFT_PAIR_H_
#define BT_ADV_PROV_SWIFT_PAIR_H_

#include <stdbool.h>

/**
 * @defgroup bt_le_adv_prov_swift_pair Swift Pair advertising data provider API
 * @brief Swift Pair advertising data provider API
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Enable/disable Swift Pair advertising provider.
 *
 * User shall make sure that this function is not called while Swift Pair advertising data provider
 * is providing advertising data.
 *
 * @param[in] enable	Enable/disable Swift Pair provider.
 */
void bt_le_adv_prov_swift_pair_enable(bool enable);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_ADV_PROV_SWIFT_PAIR_H_ */
