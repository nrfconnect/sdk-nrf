/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 *
 * @brief   NMOS control for the User Interface module.
 */

#ifndef UI_NMOS_H__
#define UI_NMOS_H__

#include <zephyr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Initializes NMOS transistor control. */
int ui_nmos_init(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_NMOS_H__ */
