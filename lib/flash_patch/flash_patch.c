/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/init.h>

#ifdef CONFIG_DISABLE_FLASH_PATCH
static int disable_flash_patch(void)
{
	/* Check if register has been written. */
	if ((NRF_UICR->DEBUGCTRL & UICR_DEBUGCTRL_CPUFPBEN_Msk) != 0x0) {
		/* Enable flash write. */
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
		__DSB();
		__ISB();

		/* Write to register. */
		NRF_UICR->DEBUGCTRL = ~UICR_DEBUGCTRL_CPUFPBEN_Msk;

		/* Wait for flash ready */
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
			;

		/* Disable flash write. */
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
		__DSB();
		__ISB();

		/* Reset for change to take effect. */
		sys_reboot(SYS_REBOOT_WARM);
	}
	return 0;
}

SYS_INIT(disable_flash_patch, PRE_KERNEL_1, 0);
#endif
