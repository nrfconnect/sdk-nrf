/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "led_state.h"

const struct led_config led_config[SYSTEM_STATE_COUNT][CONFIG_DESKTOP_LED_COUNT] = {
	/* SYSTEM_STATE_DISCONNECTED */
	{
		{
			.mode = LED_MODE_BLINKING,
			.color.c = {200},
			.period = 1000
		}
	},
	/* SYSTEM_STATE_CONNECTED */
	{
		{
			.mode = LED_MODE_ON,
			.color.c = {200},
		}
	}
};
