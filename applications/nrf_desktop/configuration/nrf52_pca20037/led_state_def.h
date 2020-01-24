/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "led_state.h"
#include "led_effect.h"

/* This configuration file is included only once from led_state module and holds
 * information about LED effect associated with each state.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} led_state_def_include_once;


/* Map function to LED ID */
static const u8_t led_map[LED_ID_COUNT] = {
	[LED_ID_PEER_STATE] = 0,
	[LED_ID_SYSTEM_STATE] = LED_UNAVAILABLE,
};

/* System state LED is unavailable - leave undefined */
static const struct led_effect led_system_state_effect[LED_SYSTEM_STATE_COUNT];

static const struct led_effect led_peer_state_effect[LED_PEER_COUNT][LED_PEER_STATE_COUNT] = {
	{
		[LED_PEER_STATE_DISCONNECTED]   = LED_EFFECT_LED_OFF(),
		[LED_PEER_STATE_CONNECTED]      = LED_EFFECT_LED_ON_GO_OFF(LED_COLOR(255), 5000, 3000),
		[LED_PEER_STATE_PEER_SEARCH]    = LED_EFFECT_LED_BREATH(500, LED_COLOR(255)),
		[LED_PEER_STATE_CONFIRM_SELECT] = LED_EFFECT_LED_CLOCK(1, LED_COLOR(255)),
		[LED_PEER_STATE_CONFIRM_ERASE]  = LED_EFFECT_LED_BLINK(25, LED_COLOR(255)),
		[LED_PEER_STATE_ERASE_ADV]	= LED_EFFECT_LED_BREATH(100, LED_COLOR(255)),
	},
	{
		[LED_PEER_STATE_DISCONNECTED]   = LED_EFFECT_LED_OFF(),
		[LED_PEER_STATE_CONNECTED]      = LED_EFFECT_LED_ON_GO_OFF(LED_COLOR(255), 5000, 3000),
		[LED_PEER_STATE_PEER_SEARCH]    = LED_EFFECT_LED_BREATH(500, LED_COLOR(255)),
		[LED_PEER_STATE_CONFIRM_SELECT] = LED_EFFECT_LED_CLOCK(2, LED_COLOR(255)),
		[LED_PEER_STATE_CONFIRM_ERASE]  = LED_EFFECT_LED_BLINK(25, LED_COLOR(255)),
		[LED_PEER_STATE_ERASE_ADV]	= LED_EFFECT_LED_BREATH(100, LED_COLOR(255)),
	},
	{
		[LED_PEER_STATE_DISCONNECTED]   = LED_EFFECT_LED_OFF(),
		[LED_PEER_STATE_CONNECTED]      = LED_EFFECT_LED_ON_GO_OFF(LED_COLOR(255), 5000, 3000),
		[LED_PEER_STATE_PEER_SEARCH]    = LED_EFFECT_LED_BREATH(500, LED_COLOR(255)),
		[LED_PEER_STATE_CONFIRM_SELECT] = LED_EFFECT_LED_CLOCK(3, LED_COLOR(255)),
		[LED_PEER_STATE_CONFIRM_ERASE]  = LED_EFFECT_LED_BLINK(25, LED_COLOR(255)),
		[LED_PEER_STATE_ERASE_ADV]	= LED_EFFECT_LED_BREATH(100, LED_COLOR(255)),
	},

};
