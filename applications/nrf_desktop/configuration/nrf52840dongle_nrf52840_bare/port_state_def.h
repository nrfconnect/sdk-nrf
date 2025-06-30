/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <soc.h>

#include "port_state.h"

/* This configuration file is included only once from board module and holds
 * information about default pin states set while board is on and off.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} port_state_def_include_once;


static const struct pin_state port0_on[] = {
};

static const struct pin_state port1_on[] = {
};

static const struct pin_state port0_off[] = {
};

static const struct pin_state port1_off[] = {
};


static const struct port_state port_state_on[] = {
	{
		.port     = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
		.ps       = port0_on,
		.ps_count = ARRAY_SIZE(port0_on),
	},
	{
		.port     = DEVICE_DT_GET(DT_NODELABEL(gpio1)),
		.ps       = port1_on,
		.ps_count = ARRAY_SIZE(port1_on),
	}
};

static const struct port_state port_state_off[] = {
	{
		.port     = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
		.ps       = port0_off,
		.ps_count = ARRAY_SIZE(port0_off),
	},
	{
		.port     = DEVICE_DT_GET(DT_NODELABEL(gpio1)),
		.ps       = port1_off,
		.ps_count = ARRAY_SIZE(port1_off),
	}
};
