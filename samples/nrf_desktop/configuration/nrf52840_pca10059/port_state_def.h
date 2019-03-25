/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <soc.h>

#include "port_state.h"

/* This file must be included only once */
const struct {} port_state_def_include_once;

static const struct pin_state port0_on[] = {
	{DT_GPIO_LEDS_LED1_RED_GPIO_PIN, 1},
};

static const struct pin_state port1_on[] = {
	{DT_GPIO_LEDS_LED1_GREEN_GPIO_PIN, 1},
};

static const struct pin_state port0_off[] = {
};

static const struct pin_state port1_off[] = {
};


static const struct port_state port_state_on[] = {
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

static const struct port_state port_state_off[] = {
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
