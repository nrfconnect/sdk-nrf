/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/led_effect.h>

/* This configuration file is included only once from the main module and holds
 * information about LED on and off effects.
 */

/* This structure enforces the header file to be included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} leds_def_include_once;

enum led_id {
	LED_ID_0,
	LED_ID_1,
	LED_ID_2,
	LED_ID_3,

	LED_ID_COUNT
};

static const struct led_effect led_effect_on = LED_EFFECT_LED_ON(LED_COLOR(255, 255, 255));
static const struct led_effect led_effect_off = LED_EFFECT_LED_OFF();
