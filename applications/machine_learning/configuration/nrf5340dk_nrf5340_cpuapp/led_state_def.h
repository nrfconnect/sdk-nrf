/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/led_effect.h>
#include "led_state.h"
#include "ei_data_forwarder_event.h"

/* This configuration file is included only once from led_state module and holds
 * information about LED effects associated with data forwarder states and machine
 * learning results.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} led_state_def_include_once;


/* Map function to LED ID */
static const uint8_t led_map[] = {
	[LED_ID_ML_STATE] = 0,
	[LED_ID_SENSOR_SIM] = 1
};

static const struct led_effect ei_data_forwarder_led_effects[] = {
	[EI_DATA_FORWARDER_STATE_DISCONNECTED]	=
		LED_EFFECT_LED_BLINK(2000, LED_COLOR(100, 100, 100)),
	[EI_DATA_FORWARDER_STATE_CONNECTED]	=
		LED_EFFECT_LED_BLINK(500, LED_COLOR(100, 100, 100)),
	[EI_DATA_FORWARDER_STATE_TRANSMITTING]	=
		LED_EFFECT_LED_BLINK(50, LED_COLOR(100, 100, 100)),
};

static const struct ml_result_led_effect ml_result_led_effects[] = {
	{
		.label = NULL,
		.effect = LED_EFFECT_LED_BREATH(500, LED_COLOR(100, 100, 100)),
	},
	{
		.label = ANOMALY_LABEL,
		.effect = LED_EFFECT_LED_BREATH(250, LED_COLOR(100, 100, 100)),
	},
	{
		.label = "sine",
		.effect = LED_EFFECT_LED_CLOCK(1, LED_COLOR(100, 100, 100)),
	},
	{
		.label = "triangle",
		.effect = LED_EFFECT_LED_CLOCK(2, LED_COLOR(100, 100, 100)),
	},
	{
		.label = "square",
		.effect = LED_EFFECT_LED_CLOCK(3, LED_COLOR(100, 100, 100)),
	},
	{
		.label = "idle",
		.effect = LED_EFFECT_LED_CLOCK(4, LED_COLOR(100, 100, 100)),
	},
};
