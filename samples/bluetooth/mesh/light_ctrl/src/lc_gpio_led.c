/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * On/off replacement for lc_pwm_led.c on boards without a PWM peripheral
 * (nRF54LC10: nordic,nrf-sw-pwm does not build on nRF54L,
 * will potentially be fixed in the future).
 * Any non-zero level turns the LED on, level 0 turns it off.
 */

#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>

#include "lc_pwm_led.h"

#define LC_LED DK_LED2

void lc_pwm_led_init(void)
{
	/* LED is configured by dk_leds_init() in bt_ready() */
}

void lc_pwm_led_set(uint16_t desired_lvl)
{
	dk_set_led(LC_LED, desired_lvl > 0);
}
