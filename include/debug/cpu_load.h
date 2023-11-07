/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __CPU_LOAD_H
#define __CPU_LOAD_H

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup cpu_load CPU load
 * @brief Module for measuring CPU load.
 *
 * @{
 */

/** @brief Initialize the CPU load measurement module.
 *
 * The TIMER driver and PPI channels are allocated during the initialization of
 * this module.
 *
 * @retval 0 The initialization is successful.
 * @retval -ENODEV PPI channels could not be allocated.
 * @retval -EBUSY TIMER instance is busy.
 */
int cpu_load_init(void);

/** @brief Reset measurement.
 *
 * Measurement must be reset at least every 4294 seconds. If not, results are
 * invalid.
 */
void cpu_load_reset(void);

/** @brief Get the CPU load measurement value.
 *
 * The CPU load is represented in 0,001% units where a 100000 value represents
 * 100% load (e.g. 12345 represents 12,345% load).
 *
 * @return The current CPU load value.
 */
uint32_t cpu_load_get(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __CPU_LOAD_H */
