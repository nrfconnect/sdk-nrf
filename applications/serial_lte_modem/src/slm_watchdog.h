/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 *
 * @brief   Watchdog module for serial LTE modem
 */

#ifndef SLM_WATCHDOG_H__
#define SLM_WATCHDOG_H__

#include <zephyr.h>

#ifdef __cplusplus
extern "C" {
#endif

int slm_watchdog_init_and_start(void);


#ifdef __cplusplus
}
#endif

#endif /* SLM_WATCHDOG_H__ */
