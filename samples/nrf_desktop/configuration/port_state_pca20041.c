/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <misc/util.h>

#include <board.h>

#include "port_state.h"


static const struct pin_state port0_on[] = {
	{4,  0}, /* accelerometer power switch */

	{14, 0}, /* motion power switch */

	{5,  0}, /* battery monitor sense */
	{6,  0}, /* battery monitor enable */

	{9,  0}, /* NFC1 */
	{10, 0}, /* NFC2 */
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


const struct port_state port_state_on[] = {
	{
		.name     = CONFIG_GPIO_P0_DEV_NAME,
		.ps       = port0_on,
		.ps_count = ARRAY_SIZE(port0_on),
	},
	{
		.name     = CONFIG_GPIO_P1_DEV_NAME,
		.ps       = port1_on,
		.ps_count = ARRAY_SIZE(port1_on),
	}
};

const size_t port_state_on_size = ARRAY_SIZE(port_state_on);

const struct port_state port_state_off[] = {
	{
		.name     = CONFIG_GPIO_P0_DEV_NAME,
		.ps       = port0_off,
		.ps_count = ARRAY_SIZE(port0_off),
	},
	{
		.name     = CONFIG_GPIO_P1_DEV_NAME,
		.ps       = port1_off,
		.ps_count = ARRAY_SIZE(port1_off),
	}
};

const size_t port_state_off_size = ARRAY_SIZE(port_state_off);
