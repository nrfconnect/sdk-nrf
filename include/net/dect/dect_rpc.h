/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief DECT NR+ RPC client — public bring-up hooks.
 */

#ifndef DECT_RPC_H_
#define DECT_RPC_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dect_rpc DECT NR+ RPC
 * @brief DECT NR+ RPC client bring-up API.
 *
 * @{
 */

#if defined(CONFIG_DECT_NR_RPC_CLIENT)

#include <nrf_rpc.h>

/**
 * @brief Get the `dect_rpc` nRF RPC group.
 *
 * @return Pointer to the DECT NR+ RPC group (for @c NRF_RPC_GROUP_STATUS()).
 */
const struct nrf_rpc_group *dect_rpc_client_get_group(void);

/**
 * @brief Tell the client subsystem that RPC to the server is ready.
 *
 * Call after @c nrf_rpc_init() and when the group is bound.
 */
void dect_rpc_client_notify_rpc_init_done(void);


#endif /* CONFIG_DECT_NR_RPC_CLIENT */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* DECT_RPC_H_ */
