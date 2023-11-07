/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef UI_INPUT_H__
#define UI_INPUT_H__

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the dk_buttons_and_leds library
 *        with a callback function that submits a ui input event
 *        when a input device's state changes.
 *        The DK buttons library interpret switch state changes as button state changes,
 *        and can thus be used for both.
 *
 * @return int 0 if successful, negative error code if not.
 */
int ui_input_init(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_INPUT_H__ */
