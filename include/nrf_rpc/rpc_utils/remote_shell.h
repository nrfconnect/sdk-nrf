/*
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_REMOTE_SHELL_H_
#define NRF_RPC_REMOTE_SHELL_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup nrf_rpc_utils nRF RPC utility commands
 * @{
 * @defgroup nrf_rpc_remote_shell nRF RPC remote shell
 * @{
 */

/** @brief Invoke the remote server shell command.
 *
 *  @param[in] argc Number of arguments.
 *  @param[in] argv Array of arguments.
 *
 *  @retval Command output on success.
 *  @retval NULL on failure.
 */
char *nrf_rpc_invoke_remote_shell_cmd(size_t argc, char *argv[]);

/**
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NRF_RPC_REMOTE_SHELL_H_ */
