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
	{4,  0}, /* accelerometer power switch */

#if !defined(CONFIG_DESKTOP_MOTION_OPTICAL_ENABLE)
	{14, 0}, /* motion power switch */
#endif

	{5,  0}, /* battery monitor sense */
	{6,  0}, /* battery monitor enable */

	{9,  0}, /* NFC1 */
	{10, 0}, /* NFC2 */

#if !defined(CONFIG_DESKTOP_LED_ENABLE)
	{23, 0}, /* Front LED red */
	{25, 0}, /* Front LED green */
	{7,  0}, /* Front LED blue */
	{11, 0}, /* Back LED red */
	{26, 0}, /* Back LED green */
	{27, 0}, /* Back LED blue */
#endif
};

static const struct pin_state port1_on[] = {
	{3,  0}, /* LED3 */
	{5,  0}, /* LED3 */
	{7,  0}, /* LED3 */

	{0,  0}, /* DBG1 */
	{1,  0}, /* DBG2 */
	{2,  0}, /* N/C */
	{4,  0}, /* N/C */
	{6,  0}, /* N/C */
	{8,  0}, /* N/C */
	{9,  0}, /* N/C */
	{10, 0}, /* N/C */
	{11, 0}, /* N/C */
	{12, 0}, /* N/C */
	{13, 0}, /* N/C */
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
