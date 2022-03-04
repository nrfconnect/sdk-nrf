/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_TX_POWER_ADV_H_
#define BT_TX_POWER_ADV_H_

#include <bluetooth/bluetooth.h>

/**
 * @defgroup fast_pair_sample_bt_tx_power_adv Fast Pair sample TX power advertising API
 * @brief Fast Pair sample TX power advertising API
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Get TX power advertising data buffer size.
 *
 * @return TX power advertising data buffer size in bytes.
 */
size_t bt_tx_power_adv_data_size(void);

/** Fill Bluetooth advertising packet with TX power advertising data.
 *
 * Provided buffer will be used in bt_data structure. The data must be valid while the structure is
 * in use.
 *
 * The buffer size must be at least @ref bt_tx_power_adv_data_size.
 *
 * @param[out] adv_data         Pointer to the Bluetooth advertising data structure to be filled.
 * @param[out] buf              Pointer to the buffer used to store TX power advertising data.
 * @param[in]  buf_size         Size of the buffer used to store TX power advertising data.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_tx_power_adv_data_fill(struct bt_data *adv_data, uint8_t *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_TX_POWER_ADV_H_ */
