/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/platform/hooks.h>
#include "nrf7120_enga_types.h"
#include "nrf7120_enga_global.h"

#if CONFIG_BOARD_EARLY_INIT_HOOK
/* Temporary workaround while VPR does not handle starting clocks */
void board_early_init_hook(void)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC)
	NRF_CLOCK_S->LFCLK.SRC = CLOCK_LFCLK_SRC_SRC_LFRC;
#elif DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lfxo))
	NRF_CLOCK_S->LFCLK.SRC = CLOCK_LFCLK_SRC_SRC_LFXO;
#else
	NRF_CLOCK_S->LFCLK.SRC = CLOCK_LFCLK_SRC_SRC_LFSYNT;
#endif
	NRF_CLOCK_S->TASKS_LFCLKSTART = 1;

	/* Wait for event */
	while (NRF_CLOCK_S->EVENTS_LFCLKSTARTED !=
	CLOCK_EVENTS_LFCLKSTARTED_EVENTS_LFCLKSTARTED_Generated) {

	}
}
#endif
