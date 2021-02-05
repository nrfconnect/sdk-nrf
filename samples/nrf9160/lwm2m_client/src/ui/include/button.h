/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**@file
 *
 * @brief   Button controls for the User Interface module.
 */

#ifndef UI_BUTTON_H__
#define UI_BUTTON_H__

#include <zephyr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Initialize buttons in the user interface module. */
int ui_button_init(ui_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif /* UI_BUTTON_H__ */
