/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/* This file must be included only once */
const struct {} leds_def_include_once;

/* Mapping the PWM channels to pin numbers */
static const size_t led_pins[CONFIG_DESKTOP_LED_COUNT]
			    [CONFIG_DESKTOP_LED_COLOR_COUNT] = {
	{
		DT_NORDIC_NRF_PWM_PWM_0_CH0_PIN,
		DT_NORDIC_NRF_PWM_PWM_0_CH1_PIN,
		DT_NORDIC_NRF_PWM_PWM_0_CH2_PIN
	}
};
