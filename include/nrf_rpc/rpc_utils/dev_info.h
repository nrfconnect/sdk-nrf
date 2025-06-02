/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_DEV_INFO_H_
#define NRF_RPC_DEV_INFO_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup nrf_rpc_utils nRF RPC utility commands
 * @{
 * @defgroup nrf_rpc_dev_info nRF RPC device information
 * @{
 */

/** @brief Get version of remote server the RPC client is connected to.
 *
 *  @retval version of the remote on success.
 *  @retval NULL on failure.
 */
char *nrf_rpc_get_ncs_commit_sha(void);

/**
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NRF_RPC_DEV_INFO_H_ */
