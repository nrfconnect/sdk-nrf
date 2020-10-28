/**
 * @file
 * @defgroup ble_utils Bluetooth LE utilities API
 * @{
 */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef __BLE_UTILS_H__
#define __BLE_UTILS_H__

#include <bluetooth/services/nus.h>

/** @brief Type indicates function called when Bluetooth LE connection
 *         is established.
 *
 * @param[in] item pointer to work item.
 */
typedef void (*ble_connection_cb_t)(struct k_work *item);

/** @brief Type indicates function called when Bluetooth LE connection is ended.
 *
 * @param[in] item pointer to work item.
 */
typedef void (*ble_disconnection_cb_t)(struct k_work *item);

/** @brief Initialize CoAP utilities.
 *
 * @param[in] on_nus_received function to call when NUS receives message
 * @param[in] on_nus_send     function to call when NUS sends message
 * @param[in] on_connect      function to call when Bluetooth LE connection
 *                            is established
 * @param[in] on_disconnect   function to call when Bluetooth LE connection
 *                            is ended
 * @retval 0    On success.
 * @retval != 0 On failure.
 */
int ble_utils_init(struct bt_nus_cb *nus_clbs,
		   ble_connection_cb_t on_connect,
		   ble_disconnection_cb_t on_disconnect);

#endif

/**
 * @}
 */
