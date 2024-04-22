/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "led_state.h"
#include <caf/led_effect.h>

/* This configuration file is included only once from led_state module and holds
 * information about LED effect associated with each state.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} led_state_def_include_once;


/* Map function to LED ID */
static const uint8_t led_map[LED_ID_COUNT] = {
	[LED_ID_SYSTEM_STATE] = 0,
	[LED_ID_PEER_STATE] = 1
};

/* On the nRF54L15 SoC, you can only use the **GPIO1** port for PWM hardware peripheral output.
 * Because of that, the PDK PCA10156 has the following limitations:
 *
 * - On the PDK revision v0.2.1, **LED 1** cannot be used for PWM output.
 * - On the PDK revision v0.3.0, **LED 0** and **LED 2** cannot be used for PWM output.
 *
 * You can still use these LEDs with the PWM LED driver, but you must set the LED color to
 * ``LED_COLOR(255, 255, 255)`` or ``LED_COLOR(0, 0, 0)``. This ensures the PWM peripheral is not
 * used for the mentioned LEDs.
 */

static const struct led_effect led_system_state_effect[LED_SYSTEM_STATE_COUNT] = {
	[LED_SYSTEM_STATE_IDLE]     = LED_EFFECT_LED_ON(LED_COLOR(255, 255, 255)),
	[LED_SYSTEM_STATE_CHARGING] = LED_EFFECT_LED_ON(LED_COLOR(255, 255, 255)),
	[LED_SYSTEM_STATE_ERROR]    = LED_EFFECT_LED_BLINK(200, LED_COLOR(255, 255, 255)),
};

static const struct led_effect led_peer_state_effect[LED_PEER_COUNT][LED_PEER_STATE_COUNT] = {
	{[LED_PEER_STATE_DISCONNECTED]   = LED_EFFECT_LED_OFF(),
	 [LED_PEER_STATE_CONNECTED]      = LED_EFFECT_LED_ON(LED_COLOR(100, 100, 100)),
	 [LED_PEER_STATE_PEER_SEARCH]    = LED_EFFECT_LED_BREATH(1000, LED_COLOR(100, 100, 100)),
	 [LED_PEER_STATE_CONFIRM_SELECT] = LED_EFFECT_LED_BLINK(50, LED_COLOR(100, 100, 100)),
	 [LED_PEER_STATE_CONFIRM_ERASE]  = LED_EFFECT_LED_BLINK(25, LED_COLOR(100, 100, 100)),
	 [LED_PEER_STATE_ERASE_ADV]      = LED_EFFECT_LED_BREATH(100, LED_COLOR(100, 100, 100)),
	},
};
