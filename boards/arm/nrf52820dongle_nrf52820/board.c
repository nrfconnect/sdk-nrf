/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <hal/nrf_power.h>
#include <hal/nrf_nvmc.h>

static int board_nrf52820dongle_init(void)
{

	/* If the nrf52820dongle board is powered from USB
	 * (high voltage mode), GPIO output voltage is set to 1.8 volts by
	 * default and that is not enough to turn the green and blue LEDs on.
	 * Increase GPIO voltage to 3.0 volts.
	 */
	if ((nrf_power_mainregstatus_get(NRF_POWER) ==
	     NRF_POWER_MAINREGSTATUS_HIGH) &&
	    ((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) ==
	     (UICR_REGOUT0_VOUT_DEFAULT << UICR_REGOUT0_VOUT_Pos))) {

		nrf_nvmc_mode_set(NRF_NVMC, NRF_NVMC_MODE_WRITE);
		while (!nrf_nvmc_ready_check(NRF_NVMC)) {
			;
		}

		NRF_UICR->REGOUT0 =
		    (NRF_UICR->REGOUT0 & ~((uint32_t)UICR_REGOUT0_VOUT_Msk)) |
		    (UICR_REGOUT0_VOUT_3V0 << UICR_REGOUT0_VOUT_Pos);

		nrf_nvmc_mode_set(NRF_NVMC, NRF_NVMC_MODE_READONLY);
		while (!nrf_nvmc_ready_check(NRF_NVMC)) {
			;
		}

		/* A reset is required for changes to take effect. */
		NVIC_SystemReset();
	}

	return 0;
}

SYS_INIT(board_nrf52820dongle_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
