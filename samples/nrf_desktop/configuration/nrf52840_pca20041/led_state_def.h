/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "led_state.h"

/* This configuration file is included only once from led_state module and holds
 * information about LED effect associated with each state.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} led_state_def_include_once;


/* Map function to LED ID */
static const u8_t led_map[LED_ID_COUNT] = {
	[LED_ID_SYSTEM_STATE] = 0,
	[LED_ID_PEER_STATE] = 1
};

static const struct led_effect led_system_state_effect[LED_SYSTEM_STATE_COUNT] = {
	[LED_SYSTEM_STATE_IDLE] = {
		.mode = LED_MODE_ON,
	},
	[LED_SYSTEM_STATE_CHARGING] = {
		.mode = LED_MODE_ON,
	},
	[LED_SYSTEM_STATE_ERROR] = {
		.mode = LED_MODE_BLINKING,
		.period = 200,
	}
};

static const struct led_color led_system_state_color[LED_SYSTEM_STATE_COUNT] = {
	[LED_SYSTEM_STATE_IDLE] = {
		.c = {255, 0, 20},
	},
	[LED_SYSTEM_STATE_CHARGING] = {
		.c = {250, 250, 250},
	},
	[LED_SYSTEM_STATE_ERROR] = {
		.c = {255, 0, 0},
	}
};

static const struct led_effect led_peer_state_effect[LED_PEER_STATE_COUNT] = {
	[LED_PEER_STATE_DISCONNECTED] = {
		.mode = LED_MODE_BLINKING,
		.period = 1000,
	},
	[LED_PEER_STATE_CONNECTED] = {
		.mode = LED_MODE_ON,
	},
	[LED_PEER_STATE_CONFIRM_SELECT] = {
		.mode = LED_MODE_BLINKING,
		.period = 50,
	},
	[LED_PEER_STATE_CONFIRM_ERASE] = {
		.mode = LED_MODE_BLINKING,
		.period = 25,
	}
};

static const struct led_color led_peer_state_color[CONFIG_BT_MAX_PAIRED] = {
	[0] = {
		.c = {150, 0, 0},
	},
	[1] = {
		.c = {0, 150, 0},
	},
	[2] = {
		.c = {0,   0, 150},
	}
};
