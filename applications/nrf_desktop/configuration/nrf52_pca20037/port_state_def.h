/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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
#if !defined(CONFIG_DESKTOP_LED_ENABLE)
	{30, 0}, /* LED red */
	{29, 0}, /* LED green */
	{28, 0}, /* LED blue */
#endif
};

static const struct pin_state port0_off[] = {
#if !defined(CONFIG_DESKTOP_LED_ENABLE)
	{30, 1}, /* LED red */
	{29, 1}, /* LED green */
	{28, 1}, /* LED blue */
#endif
};


static const struct port_state port_state_on[] = {
	{
		.name     = DT_LABEL(DT_NODELABEL(gpio0)),
		.ps       = port0_on,
		.ps_count = ARRAY_SIZE(port0_on),
	}
};

static const struct port_state port_state_off[] = {
	{
		.name     = DT_LABEL(DT_NODELABEL(gpio0)),
		.ps       = port0_off,
		.ps_count = ARRAY_SIZE(port0_off),
	}
};
