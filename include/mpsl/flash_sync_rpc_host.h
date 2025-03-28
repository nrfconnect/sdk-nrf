/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file flash_sync_rpc_host.h
 *
 * @defgroup flash_sync_rpc_host Host side of flash synchronization via RPC
 *
 * @brief Host side of flash synchronization via RPC
 * @{
 */

#ifndef FLASH_SYNC_RPC_HOST_H__
#define FLASH_SYNC_RPC_HOST_H__

#ifdef __cplusplus
/**
 * @brief Enables or disables flash synchronization on the RPC host (application core).
 *
 * This function is used to enable or disable the flash synchronization
 * mechanism on the RPC host. When enabled, the
 * synchronization ensures proper coordination of flash operations with
 * radio operation performed on the remote (application) core.
 *
 * @param enable A boolean value indicating whether to enable or disable
 *               flash synchronization. Pass `true` to enable and `false`
 *               to disable.
 */
extern "C" {
#endif


void flash_sync_rpc_host_sync_enable(bool enable);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_SYNC_RPC_HOST_H__ */

/**@} */
