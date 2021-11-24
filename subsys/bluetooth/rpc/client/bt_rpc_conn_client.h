/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_RPC_CONN_CLIENT_H_
#define BT_RPC_CONN_CLIENT_H_

/**
 * @file
 * @defgroup bt_rpc_conn_client RPC connection management client API
 * @{
 * @brief API for the RPC connection management client.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize connection management functionality over RPC.
 *
 * This function registers connection callbacks on host side if this side has
 * statically initialized callbacks in sections.
 */
void bt_rpc_conn_init(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_RPC_CONN_CLIENT_H_ */
