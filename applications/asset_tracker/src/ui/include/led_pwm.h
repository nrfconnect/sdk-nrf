/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**@file
 *
 * @brief   LED control for the User Interface module. The module uses PWM to
 *	    control RGB colors and light intensity.
 */

#ifndef UI_LEDS_H__
#define UI_LEDS_H__

#include <zephyr.h>
#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Initializes LEDs in the user interface module. */
int ui_leds_init(void);

/**@brief Starts the PWM and handling of LED effects. */
void ui_leds_start(void);

/**@brief Stops the LED effects. */
void ui_leds_stop(void);

/**@brief Sets LED effect based in UI LED state. */
void ui_led_set_effect(enum ui_led_pattern state);

/**@brief Sets RGB and light intensity values, in 0 - 255 ranges. */
int ui_led_set_rgb(u8_t red, u8_t green, u8_t blue, size_t on, size_t off);

#ifdef __cplusplus
}
#endif

#endif /* UI_LEDS_H__ */
