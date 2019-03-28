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

#if !defined(CONFIG_DESKTOP_BATTERY_DISCRETE)
	{6,  0}, /* battery monitor enable */
#endif

#if !defined(CONFIG_DESKTOP_LED_ENABLE)
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

#if defined(CONFIG_DESKTOP_LED_ENABLE)
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
