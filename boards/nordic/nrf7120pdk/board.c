/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/devicetree.h>
#include <zephyr/platform/hooks.h>
#include <soc.h>

#include "nrf7120_enga_types.h"
#include "nrf7120_enga_global.h"
#include <zephyr/drivers/timer/nrf_grtc_timer.h>

#if CONFIG_BOARD_LATE_INIT_HOOK
/* Temporary workaround while VPR does not handle starting LFRC */
void board_late_init_hook(void)
{
	NRF_CLOCK_S->LFCLK.SRC = CLOCK_LFCLK_SRC_SRC_LFRC;
	NRF_CLOCK_S->TASKS_LFCLKSTART = 1;

	/* Wait for event */
	while (NRF_CLOCK_S->EVENTS_LFCLKSTARTED !=
	CLOCK_EVENTS_LFCLKSTARTED_EVENTS_LFCLKSTARTED_Generated) {

	}
	nrf_grtc_timer_clock_driver_init();
}
#endif
