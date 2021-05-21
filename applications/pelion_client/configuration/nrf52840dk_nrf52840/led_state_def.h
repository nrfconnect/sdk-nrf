/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/led_effect.h>

#include "led_state.h"
#include "net_event.h"
#include "pelion_event.h"

/* This configuration file is included only once from led_state module and holds
 * information about LED effect associated with each state.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} led_state_def_include_once;


/* Map function to LED ID */
static const uint8_t led_map[LED_ID_COUNT] = {
	[LED_ID_NET_STATE] = 0,
	[LED_ID_PELION_STATE] = 1
};

static const struct led_effect led_net_state_effect[NET_STATE_COUNT] = {
	[NET_STATE_DISABLED]     = LED_EFFECT_LED_OFF(),
	[NET_STATE_DISCONNECTED] = LED_EFFECT_LED_BREATH(500, LED_COLOR(100, 100, 100)),
	[NET_STATE_CONNECTED]    = LED_EFFECT_LED_ON(LED_COLOR(150, 150, 150)),
};

static const struct led_effect led_pelion_state_effect[PELION_STATE_COUNT] = {
	[PELION_STATE_DISABLED]     = LED_EFFECT_LED_OFF(),
	[PELION_STATE_INITIALIZED]  = LED_EFFECT_LED_BREATH(500, LED_COLOR(100, 100, 100)),
	[PELION_STATE_REGISTERED]   = LED_EFFECT_LED_ON(LED_COLOR(150, 150, 150)),
	[PELION_STATE_UNREGISTERED] = LED_EFFECT_LED_BREATH(500, LED_COLOR(100, 100, 100)),
	[PELION_STATE_SUSPENDED]    = LED_EFFECT_LED_BREATH(200, LED_COLOR(100, 100, 100)),
};

static const struct led_effect led_system_error_effect =
	LED_EFFECT_LED_BLINK(200, LED_COLOR(255, 255, 255));
