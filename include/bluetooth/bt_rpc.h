/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_RPC_H_
#define BT_RPC_H_

#include <zephyr/bluetooth/gatt.h>

/**
 * @file
 * @defgroup bt_rpc RPC bluetooth API additions
 * @{
 * @brief API additions for the bluetooth over RPC.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Set flag in the @a flags field of the bt_gatt_subscribe_params structure.
 *
 * This function must be used instead of atomic_set_bit() if you are using
 * BLE API over RPC.
 *
 * @param params    Subscribe parameters.
 * @param flags_bit Index of bit to set.
 *
 * @return Previous flag value (retrieved non-atomically) in case of success or negative value
 * in case of error.
 */
int bt_rpc_gatt_subscribe_flag_set(struct bt_gatt_subscribe_params *params, uint32_t flags_bit);

/** @brief Clear flag in the @a flags field of the bt_gatt_subscribe_params structure.
 *
 * This function must be used instead of atomic_clear_bit() if you are using
 * BLE API over RPC.
 *
 * @param params    Subscribe parameters.
 * @param flags_bit Index of bit to clear.
 *
 * @return Previous flag value (retrieved non-atomically) in case of success or negative value
 * in case of error.
 */
int bt_rpc_gatt_subscribe_flag_clear(struct bt_gatt_subscribe_params *params, uint32_t flags_bit);

/** @brief Get flag value from the @a flags field of the bt_gatt_subscribe_params structure.
 *
 * This function must be used instead of atomic_test_bit() if you are using
 * BLE API over RPC.
 *
 * @param params    Subscribe parameters.
 * @param flags_bit Index of bit to test.
 *
 * @return Flag value in case of success or negative value in case of error.
 */
int bt_rpc_gatt_subscribe_flag_get(struct bt_gatt_subscribe_params *params, uint32_t flags_bit);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_RPC_H_ */
