/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 *
 * @brief   Watchdog module for asset tracker
 */

#ifndef WATCHDOG_H__
#define WATCHDOG_H__

#include <zephyr.h>

#ifdef __cplusplus
extern "C" {
#endif

int watchdog_init_and_start(void);

#ifdef __cplusplus
}
#endif

#endif /* WATCHDOG_H__ */
