/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "led_state.h"

const struct led_config led_config[SYSTEM_STATE_COUNT][CONFIG_DESKTOP_LED_COUNT] = {
	/* SYSTEM_STATE_DISCONNECTED */
	{
		/* Back LED */
		{
			.mode = LED_MODE_ON,
			.color.c = {255, 0, 20}
		},
		/* Front LED */
		{
			.mode = LED_MODE_BLINKING,
			.color.c = {150, 0, 10},
			.period = 1000,
		}
	},
	/* SYSTEM_STATE_CONNECTED */
	{
		/* Back LED */
		{
			.mode = LED_MODE_ON,
			.color.c = {255, 0, 20}
		},
		/* Front LED */
		{
			.mode = LED_MODE_ON,
			.color.c = {200, 0, 15},
		}
	}
};
