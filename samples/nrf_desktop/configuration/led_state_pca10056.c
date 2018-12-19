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
	}
};

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
	},
	/* SYSTEM_STATE_DISCONNECTED_CHARGING */
	{
		{
			.mode = LED_MODE_BLINKING,
			.color.c = {200},
			.period = 1000
		}
	},
	/* SYSTEM_STATE_CONNECTED_CHARGING */
	{
		{
			.mode = LED_MODE_ON,
			.color.c = {200},
		}
	},
	/* SYSTEM_STATE_ERROR */
	{
		{
			.mode = LED_MODE_BLINKING,
			.color.c = {200},
			.period = 200
		}
	},
};
