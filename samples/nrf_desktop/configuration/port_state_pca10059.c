/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <misc/util.h>
#include <soc.h>

#include "port_state.h"


static const struct pin_state port0_on[] = {
	{DT_GPIO_LEDS_LED0_GREEN_GPIO_PIN, 0},

	{DT_GPIO_LEDS_LED1_RED_GPIO_PIN, 1},
	{DT_GPIO_LEDS_LED1_BLUE_GPIO_PIN, 0},
};

static const struct pin_state port1_on[] = {
	{DT_GPIO_LEDS_LED1_GREEN_GPIO_PIN, 1},
};

static const struct pin_state port0_off[] = {
};

static const struct pin_state port1_off[] = {
};


const struct port_state port_state_on[] = {
	{
		.name     = DT_GPIO_P0_DEV_NAME,
		.ps       = port0_on,
		.ps_count = ARRAY_SIZE(port0_on),
	},
	{
		.name     = DT_GPIO_P1_DEV_NAME,
		.ps       = port1_on,
		.ps_count = ARRAY_SIZE(port1_on),
	}
};

const size_t port_state_on_size = ARRAY_SIZE(port_state_on);

const struct port_state port_state_off[] = {
	{
		.name     = DT_GPIO_P0_DEV_NAME,
		.ps       = port0_off,
		.ps_count = ARRAY_SIZE(port0_off),
	},
	{
		.name     = DT_GPIO_P1_DEV_NAME,
		.ps       = port1_off,
		.ps_count = ARRAY_SIZE(port1_off),
	}
};

const size_t port_state_off_size = ARRAY_SIZE(port_state_off);
