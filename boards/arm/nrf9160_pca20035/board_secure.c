/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <init.h>
#include <hal/nrf_clock.h>


static void pca20035_switch_to_xtal(void)
{
	nrf_clock_task_trigger(NRF_CLOCK_TASK_HFCLKSTART);
}

static int pca20035_board_init(struct device *dev)
{
	pca20035_switch_to_xtal();

	return 0;
}

SYS_INIT(pca20035_board_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
