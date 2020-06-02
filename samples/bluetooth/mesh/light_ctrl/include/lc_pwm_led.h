/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @brief Lc pwm led module
 */

#ifndef LC_PWM_LED_H__
#define LC_PWM_LED_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void lc_pwm_led_init(void);

void lc_pwm_led_set(u16_t desired_lvl);

#ifdef __cplusplus
}
#endif

#endif /* LC_PWM_LED_H__ */
