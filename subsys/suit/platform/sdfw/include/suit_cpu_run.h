/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_RUN_CPU_H__
#define SUIT_PLAT_RUN_CPU_H__

#include <stdint.h>
#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Halt specified CPU.
 *
 * @note Implementation depends on SoC on which it's built.
 *
 * @param[in] cpu_id  ID of CPU to be halted
 *
 * @retval SUIT_PLAT_SUCCESS          in case of successful halt
 * @retval SUIT_PLAT_ERR_CRASH        if CPU halt failed
 * @retval SUIT_PLAT_ERR_UNSUPPORTED  if handling CPU halt is not supported
 * @retval SUIT_PLAT_ERR_INVAL        if CPU ID is unknown
 */
suit_plat_err_t suit_plat_cpu_halt(uint8_t cpu_id);

/**
 * @brief Run specified CPU.
 *
 * @note Implementation depends on SoC on which it's built.
 *
 * @param[in] cpu_id       ID of CPU to be started
 * @param[in] run_address  Start address for given CPU
 *
 * @retval SUIT_PLAT_SUCCESS              in case of successful start
 * @retval SUIT_PLAT_ERR_CRASH            if CPU start failed
 * @retval SUIT_PLAT_ERR_UNSUPPORTED      if handling CPU start is not supported
 * @retval SUIT_PLAT_ERR_INVAL            if CPU ID is unknown
 * @retval SUIT_PLAT_ERR_INCORRECT_STATE  if CPU is already started
 */
suit_plat_err_t suit_plat_cpu_run(uint8_t cpu_id, uintptr_t run_address);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_RUN_CPU_H__ */
