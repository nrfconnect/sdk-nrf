/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
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

static const struct led_effect led_system_state_effect[LED_SYSTEM_STATE_COUNT] = {
	[LED_SYSTEM_STATE_IDLE]     = LED_EFFECT_LED_ON(LED_COLOR(200, 200, 200)),
	[LED_SYSTEM_STATE_CHARGING] = LED_EFFECT_LED_ON(LED_COLOR(200, 200, 200)),
	[LED_SYSTEM_STATE_ERROR]    = LED_EFFECT_LED_BLINK(200, LED_COLOR(200, 200, 200)),
};

static const struct led_effect led_peer_state_effect[LED_PEER_COUNT][LED_PEER_STATE_COUNT] = {
	{[LED_PEER_STATE_DISCONNECTED]   = LED_EFFECT_LED_OFF(),
	 [LED_PEER_STATE_CONNECTED]      = LED_EFFECT_LED_ON(LED_COLOR(100, 100, 100)),
	 [LED_PEER_STATE_PEER_SEARCH]    = LED_EFFECT_LED_BREATH(500, LED_COLOR(100, 100, 100)),
	 [LED_PEER_STATE_CONFIRM_SELECT] = LED_EFFECT_LED_BLINK(50, LED_COLOR(100, 100, 100)),
	 [LED_PEER_STATE_CONFIRM_ERASE]  = LED_EFFECT_LED_BLINK(25, LED_COLOR(100, 100, 100)),
	 [LED_PEER_STATE_ERASE_ADV]	= LED_EFFECT_LED_BREATH(100, LED_COLOR(100, 100, 100)),
	},
};
