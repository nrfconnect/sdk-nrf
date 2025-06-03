/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_SYSTEM_HEALTH_H_
#define NRF_RPC_SYSTEM_HEALTH_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup nrf_rpc_utils nRF RPC utility commands
 * @{
 * @defgroup nrf_rpc_system_health nRF RPC system health commands
 * @{
 */

/** @brief System health information. */
struct nrf_rpc_system_health {
	/** @brief Bitmask of hung threads.
	 *
	 * Each bit represents a thread.
	 * If a thread is hung, the corresponding bit is set.
	 */
	uint32_t hung_threads;
};

/** @brief Get system health information from the remote server.
 *
 * @param[out] out Pointer to the system health information.
 */
void nrf_rpc_system_health_get(struct nrf_rpc_system_health *out);

/**
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NRF_RPC_REMOTE_SHELL_H_ */
