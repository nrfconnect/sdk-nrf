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

const struct led_config led_config[SYSTEM_STATE_COUNT][CONFIG_DESKTOP_LED_COUNT] = {
	/* SYSTEM_STATE_DISCONNECTED */
	{
		/* Back LED */
		{
			.mode = LED_MODE_ON,
			.color.c = {255, 0, 20},
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
			.color.c = {255, 0, 20},
		},
		/* Front LED */
		{
			.mode = LED_MODE_ON,
			.color.c = {200, 0, 15},
		}
	},
	/* SYSTEM_STATE_DISCONNECTED_CHARGING */
	{
		/* Back LED */
		{
			.mode = LED_MODE_ON,
			.color.c = {250, 250, 250},
		},
		/* Front LED */
		{
			.mode = LED_MODE_BLINKING,
			.color.c = {150, 0, 10},
			.period = 1000,
		}
	},
	/* SYSTEM_STATE_CONNECTED_CHARGING */
	{
		/* Back LED */
		{
			.mode = LED_MODE_ON,
			.color.c = {250, 250, 250},
		},
		/* Front LED */
		{
			.mode = LED_MODE_ON,
			.color.c = {200, 0, 15},
		}
	},
	/* SYSTEM_STATE_ERROR */
	{
		/* Back LED */
		{
			.mode = LED_MODE_BLINKING,
			.color.c = {255, 0, 0},
			.period = 200,
		},
		/* Front LED */
		{
			.mode = LED_MODE_OFF,
		}
	},
};
