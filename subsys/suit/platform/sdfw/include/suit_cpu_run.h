/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_RUN_CPU_H__
#define SUIT_PLAT_RUN_CPU_H__

#include <suit_platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run specified CPU.
 * @note Implementation depends on SoC on which it's built.
 *
 * @param cpu_id ID of CPU to be started
 * @param run_address Start address for given CPU
 * @return int 0 in case of success, otherwise error code
 */
int suit_plat_cpu_run(uint8_t cpu_id, intptr_t run_address);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_RUN_CPU_H__ */
