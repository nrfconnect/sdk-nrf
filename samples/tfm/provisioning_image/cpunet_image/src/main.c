/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/printk.h>
#include <stdio.h>
#include <nrfx_nvmc.h>

void main(void)
{
	bool is_writable = nrfx_nvmc_word_writable_check((uint32_t)&NRF_UICR->APPROTECT,
		UICR_APPROTECT_PALL_Protected);

	if (!is_writable) {
		printk("Cannot write to the UICR->APPROTECT register, exiting...!\n");
		return;
	}

	nrfx_nvmc_word_write((uint32_t)&NRF_UICR->APPROTECT, UICR_APPROTECT_PALL_Protected);
	printk("The UICR->APPROTECT register is now configured to deny debugging access for the "
		   "network core!\n");
	printk("Success!\n");
}
