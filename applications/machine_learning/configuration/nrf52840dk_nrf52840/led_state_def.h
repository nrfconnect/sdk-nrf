/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/led_effect.h>
#include "ml_state_event.h"

/* This configuration file is included only once from led_state module and holds
 * information about LED effect associated with each state.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} led_state_def_include_once;

/* Initialized with zero. */
static const uint8_t ml_state_led_id;

static const struct led_effect ml_state_led_effect[] = {
	[ML_STATE_MODEL_RUNNING]	= LED_EFFECT_LED_BREATH(1000, LED_COLOR(100)),
	[ML_STATE_DATA_FORWARDING]	= LED_EFFECT_LED_BLINK(50, LED_COLOR(100)),
};

BUILD_ASSERT(ARRAY_SIZE(ml_state_led_effect) == ML_STATE_COUNT);
