/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* This configuration file is included only once from leds module and holds
 * information about LED mapping to PWM channels.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} leds_def_include_once;

/* Mapping the PWM channels to pin numbers */
static const size_t led_pins[CONFIG_CAF_LEDS_COUNT]
			    [CONFIG_CAF_LEDS_COLOR_COUNT] = {
	{
		DT_PROP(DT_NODELABEL(pwm0), ch0_pin),
	},
};
