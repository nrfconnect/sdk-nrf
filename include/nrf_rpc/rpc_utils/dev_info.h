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

 struct nrf_rpc_crash_info
{
	uint32_t uuid;
	uint16_t reason;
	uint32_t pc;
	uint32_t lr;
	uint32_t sp;
	uint32_t xpsr;
	uint32_t assert_line;
	char assert_filename[255];
};

/** @brief Get version of remote server the RPC client is connected to.
 *
 *  @retval version of the remote on success.
 *  @retval NULL on failure.
 */
char *nrf_rpc_get_ncs_commit_sha(void);

int nrf_rpc_get_crash_info(struct nrf_rpc_crash_info *);

/**
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NRF_RPC_DEV_INFO_H_ */
