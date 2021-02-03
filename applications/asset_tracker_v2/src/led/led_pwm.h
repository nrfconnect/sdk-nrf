/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/**@file
 *
 * @brief   LED PWM control.
 */

#ifndef LED_PWM_H__
#define LED_PWM_H__

#include <zephyr.h>
#include "led.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Initializes LEDs. */
int led_pwm_init(void);

/**@brief Starts the PWM and handling of LED effects. */
void led_pwm_start(void);

/**@brief Stops the LED effects. */
void led_pwm_stop(void);

/**@brief Sets LED effect based in UI LED state. */
void led_pwm_set_effect(enum led_pattern state);

/**@brief Sets RGB and light intensity values, in 0 - 255 ranges. */
int led_pwm_set_rgb(uint8_t red, uint8_t green, uint8_t blue);

#ifdef __cplusplus
}
#endif

#endif /* LED_PWM_H__ */
