/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <mgmt/fmfu_mgmt.h>
#include <mgmt/fmfu_mgmt_stat.h>
#include <modem/modem_info.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/dfu/mcuboot.h>

void main(void)
{
	int err = nrf_modem_lib_init(NORMAL_MODE);

	if (err == 0) {
		char modem_version[MODEM_INFO_MAX_RESPONSE_SIZE];

		err = modem_info_init();
		if (err != 0) {
			printk("Modem info Init error: %d\n\r", err);
		} else {
			modem_info_string_get(MODEM_INFO_FW_VERSION,
					modem_version,
					MODEM_INFO_MAX_RESPONSE_SIZE);
			printk("Starting modem mgmt sample\n\r");
			printk("Modem version: %s\n\r", modem_version);
		}
	}
	/* Shutdown modem to prepare for DFU */
	nrf_modem_lib_shutdown();

	nrf_modem_lib_init(FULL_DFU_MODE);
	/* Register SMP Communication stats */
	fmfu_mgmt_stat_init();
	/* Initialize MCUMgr handlers for full modem update */
	err = fmfu_mgmt_init();
	if (err) {
		printk("Error in fmfu init: %d\n\r", err);
	}
	printk("Initialized and ready to receive firmware\n\r");
	if (boot_is_img_confirmed()) {
		printk("Current image confirmed\n\r");
	} else {
		printk("Current image not confirmed yet\n\r");
	}
}
