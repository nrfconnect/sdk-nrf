/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_FAST_PAIR_H_
#define BT_FAST_PAIR_H_

#include <zephyr/bluetooth/bluetooth.h>

/**
 * @defgroup bt_fast_pair Fast Pair API
 * @brief Fast Pair API
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Get Fast Pair advertising data buffer size.
 *
 * @param[in]  fp_discoverable	Boolean indicating if device is Fast Pair discoverable.
 *
 * @return Fast Pair advertising data buffer size in bytes if the operation was successful.
 *         Otherwise zero is returned.
 */
size_t bt_fast_pair_adv_data_size(bool fp_discoverable);

/** Fill Bluetooth advertising packet with Fast Pair advertising data.
 *
 * Provided buffer will be used in bt_data structure. The data must be valid while the structure is
 * in use.
 *
 * The buffer size must be at least @ref bt_fast_pair_adv_data_size. Caller shall also make sure
 * that Account Key write from a connected Fast Pair Seeker would not preempt generating Fast Pair
 * not discoverable advertising data. To achieve this, this function and
 * @ref bt_fast_pair_adv_data_size should be called from context with cooperative priority.
 *
 * @param[out] adv_data		Pointer to the Bluetooth advertising data structure to be filled.
 * @param[out] buf		Pointer to the buffer used to store Fast Pair advertising data.
 * @param[in]  buf_size		Size of the buffer used to store Fast Pair advertising data.
 * @param[in]  fp_discoverable	Boolean indicating if device is Fast Pair discoverable.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int bt_fast_pair_adv_data_fill(struct bt_data *adv_data, uint8_t *buf, size_t buf_size,
			       bool fp_discoverable);

/** Enable or disable Fast Pair pairing mode
 *
 * Pairing mode must be enabled if discoverable Fast Pair advertising is used.
 * Pairing mode must be disabled if not discoverable Fast Pair advertising is used.
 *
 * It's user responsibility to make sure that proper pairing mode is used before advertising is
 * started. Fast Pair pairing mode is enabled by default.
 *
 * @param[in] pairing_mode Boolean indicating if device is in pairing mode.
 */
void bt_fast_pair_set_pairing_mode(bool pairing_mode);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_FAST_PAIR_H_ */
