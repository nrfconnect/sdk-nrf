/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_LLS_H_
#define BT_LLS_H_

/**@file
 * @defgroup bt_lls Link Loss Service API
 * @{
 * @brief API for the Link Loss Service (LLS).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>


int bt_lls_init();

uint8_t bt_lls_alert_level_get();

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_LLS_H_ */
