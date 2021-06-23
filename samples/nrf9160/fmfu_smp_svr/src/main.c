/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <mgmt/fmfu_mgmt.h>
#include <mgmt/fmfu_mgmt_stat.h>
#include <nrf_modem_full_dfu.h>
#include <modem/modem_info.h>
#include <modem/nrf_modem_lib.h>
#include <modem/at_cmd.h>
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
#include "os_mgmt/os_mgmt.h"
#endif

static void fmfu_callback_handler(const enum fmfu_evt_id id)
{
	switch (id) {
	case FMFU_EVT_PRE_DFU_MODE:
		printk("Going to put the modem into DFU mode\n\r");
		break;
	case FMFU_EVT_FINISHED:
		printk("Modem update finished the device needs to be reset "
		       "for the update to be applied.\n\r");
		break;
	default:
		break;
	}

}

void main(void)
{
	int err = nrf_modem_lib_init(NORMAL_MODE);

	if (err == 0) {
		char modem_version[MODEM_INFO_MAX_RESPONSE_SIZE];

		err = at_cmd_init();
		if (err != 0) {
			printk("AT CMD Init error: %d\n\r", err);
		} else {
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

	}
	/* Register SMP Communication stats */
	fmfu_mgmt_stat_init();
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
	os_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
	img_mgmt_register_group();
#endif
	/* Initialize MCUMgr handlers for full modem update */
	err = fmfu_mgmt_register_group(fmfu_callback_handler);
	if (err) {
		printk("Error in fmfu init: %d\n\r", err);
	}
	printk("FMFU initialized and ready to receive firmware\n\r");
}
