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

#if !defined(CONFIG_DESKTOP_BATTERY_DISCRETE)
	{6,  0}, /* battery monitor enable */
#endif

#if !defined(CONFIG_CAF_LEDS)
	{23, 0}, /* Front LED red */
	{25, 0}, /* Front LED green */
	{7,  0}, /* Front LED blue */
	{11, 0}, /* Back LED red */
	{26, 0}, /* Back LED green */
	{27, 0}, /* Back LED blue */
#endif

	{9,  0}, /* NFC1 */
	{10, 0}, /* NFC2 */
};

static const struct pin_state port1_on[] = {
	{3,  0}, /* LED3 */
	{5,  0}, /* LED3 */
	{7,  0}, /* LED3 */

#if defined(CONFIG_CAF_LEDS)
	{13, 1}, /* LED power enable */
#else
	{13, 0}, /* LED power enable */
#endif
};

static const struct pin_state port0_off[] = {
};

static const struct pin_state port1_off[] = {
	{13, 0}, /* LED power enable */
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
