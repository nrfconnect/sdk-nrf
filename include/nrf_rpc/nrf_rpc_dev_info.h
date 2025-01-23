/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef UTILS_NRF_RPC_UTILS_H_
#define UTILS_NRF_RPC_UTILS_H_

/**
 * @defgroup nrf_rpc_dev_info nRF RPC device information.
 * @{
 * @brief nRF RPC device information functions.
 *
 */

/** @brief Get version of remote server the RPC client is connected to.
 *
 *  @retval version of the remote on success.
 *  @retval NULL on failure.
 */
char *nrf_rpc_get_ncs_commit_sha(void);

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
 */

#endif /* UTILS_NRF_RPC_UTILS_H_ */
