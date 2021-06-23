/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_RPC_GATT_SVC_CLIENT_H_
#define BT_RPC_GATT_SVC_CLIENT_H_

/**
 * @file
 * @defgroup bt_rpc_gatt_svc_client RPC GATT service client API
 * @{
 * @brief API for the RPC GATT service client.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize GATT functionality over RPC.
 *
 * This function registers all GATT static services and its attribute on a host.
 * Serives are registered as dynamic services on a host.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_rpc_gatt_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_RPC_GATT_SVC_CLIENT_H_ */
