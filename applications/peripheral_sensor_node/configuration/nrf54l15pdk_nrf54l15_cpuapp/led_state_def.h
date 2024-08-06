/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/led_effect.h>

/* This configuration file is included only once from led_state module and holds
 * information about LED effect associated with each state.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} led_state_def_include_once;

enum led_id {
	LED_ID_1,

	LED_ID_COUNT
};

static const struct led_effect led_effect_on = LED_EFFECT_LED_ON(LED_COLOR(255, 255, 255));
