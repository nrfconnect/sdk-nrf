/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/* This configuration file is included only once from leds module and holds
 * information about LED mapping to PWM channels.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} leds_def_include_once;

/* Mapping the PWM channels to pin numbers */
static const size_t led_pins[CONFIG_DESKTOP_LED_COUNT]
			    [CONFIG_DESKTOP_LED_COLOR_COUNT] = {
	{
		DT_PROP(DT_ALIAS(pwm_0), ch0_pin),
	},
	{
		DT_PROP(DT_ALIAS(pwm_1), ch0_pin),
	}
};
