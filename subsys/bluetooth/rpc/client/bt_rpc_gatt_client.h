/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_RPC_GATT_CLIENT_H_
#define BT_RPC_GATT_CLIENT_H_

/**
 * @file
 * @defgroup bt_rpc_gatt_client RPC GATT client API
 * @{
 * @brief API for the RPC GATT client.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize GATT functionality over RPC.
 *
 * This function registers all GATT static services and its attributes on the host.
 *
 * @note On the host, the client's static services are represented as dynamic services.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_rpc_gatt_init(void);

/** @brief Uninitialize GATT functionality over RPC.
 *
 * This function reverses the operations performed by :c:func:`bt_rpc_gatt_init`.
 * Specifically, it unregisters all GATT static services on the host.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_rpc_gatt_uninit(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_RPC_GATT_CLIENT_H_ */
