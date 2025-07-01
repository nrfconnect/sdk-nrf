/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_CRASH_GEN_H_
#define NRF_RPC_CRASH_GEN_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup nrf_rpc_utils nRF RPC utility commands
 * @{
 * @defgroup nrf_rpc_crash_gen nRF RPC crash generator
 * @{
 * @brief nRF RPC crash generator functions.
 *
 */

/** @brief Generate assertion failure on RPC server.
 *
 * @param[in] delay_ms Assertion timeout in milliseconds.
 */
void nrf_rpc_crash_gen_assert(uint32_t delay_ms);


/** @brief Generate hard fault on RPC server.
 *
 * @param[in] delay_ms Hard fault timeout in milliseconds.
 */
void nrf_rpc_crash_gen_hard_fault(uint32_t delay_ms);


/** @brief Generate stack overflow on RPC server.
 *
 * @param[in] delay_ms Stack overflow timeout in milliseconds.
 */
void nrf_rpc_crash_gen_stack_overflow(uint32_t delay_ms);

/**
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NRF_RPC_CRASH_GEN_H_ */
