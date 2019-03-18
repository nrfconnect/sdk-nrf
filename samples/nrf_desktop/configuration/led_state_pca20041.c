/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "led_state.h"

/* Mapping the PWM channels to pin numbers */
const size_t led_pins[CONFIG_DESKTOP_LED_COUNT]
		     [CONFIG_DESKTOP_LED_COLOR_COUNT] = {
	{
		DT_NORDIC_NRF_PWM_PWM_0_CH0_PIN,
		DT_NORDIC_NRF_PWM_PWM_0_CH1_PIN,
		DT_NORDIC_NRF_PWM_PWM_0_CH2_PIN
	},
	{
		DT_NORDIC_NRF_PWM_PWM_1_CH0_PIN,
		DT_NORDIC_NRF_PWM_PWM_1_CH1_PIN,
		DT_NORDIC_NRF_PWM_PWM_1_CH2_PIN
	}
};

const u8_t led_map[LED_ID_COUNT] = {0, 1};

const struct led_effect led_system_state_effect[LED_SYSTEM_STATE_COUNT] = {
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

const struct led_color led_system_state_color[LED_SYSTEM_STATE_COUNT] = {
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

const struct led_effect led_peer_state_effect[LED_PEER_STATE_COUNT] = {
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
		.period = 25
	}
};

const struct led_color led_peer_state_color[CONFIG_BT_MAX_PAIRED] = {
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
