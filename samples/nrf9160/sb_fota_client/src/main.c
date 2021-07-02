/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <power/reboot.h>
#include <nrf_modem.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <sb_fota.h>
#include <nrf9160.h>
#include <hal/nrf_gpio.h>

/* For Nordic devices (for example the nRF9160 DK) the device ID is format
 * "nrf-<IMEI>".
 */
#define DEV_ID_PREFIX "nrf-"
#define IMEI_LEN (15)
#define DEV_ID_BUFF_SIZE (sizeof(DEV_ID_PREFIX) + IMEI_LEN + 2)

void ping_init(void);

static void modem_fota_callback(enum sb_fota_event e)
{
	switch(e) {
	case FOTA_EVENT_DOWNLOADING:
		printk("Modem FOTA library: Checking for update\n");
		break;
	case FOTA_EVENT_IDLE:
		printk("Modem FOTA library: Idle\n");
		break;
	case FOTA_EVENT_MODEM_SHUTDOWN:
		printk("Modem FOTA library: Update downloaded. Modem will be updated.\n");
		break;
	case FOTA_EVENT_REBOOT_PENDING:
		printk("Modem FOTA library: Modem updated, need to reboot\n");
		sys_reboot(SYS_REBOOT_COLD);
		break;
	}
}

static void modem_trace_enable(void)
{
	/* GPIO configurations for trace and debug */
	#define CS_PIN_CFG_TRACE_CLK	21 //GPIO_OUT_PIN21_Pos
	#define CS_PIN_CFG_TRACE_DATA0	22 //GPIO_OUT_PIN22_Pos
	#define CS_PIN_CFG_TRACE_DATA1	23 //GPIO_OUT_PIN23_Pos
	#define CS_PIN_CFG_TRACE_DATA2	24 //GPIO_OUT_PIN24_Pos
	#define CS_PIN_CFG_TRACE_DATA3	25 //GPIO_OUT_PIN25_Pos

	// Configure outputs.
	// CS_PIN_CFG_TRACE_CLK
	NRF_P0_NS->PIN_CNF[CS_PIN_CFG_TRACE_CLK] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) |
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);

	// CS_PIN_CFG_TRACE_DATA0
	NRF_P0_NS->PIN_CNF[CS_PIN_CFG_TRACE_DATA0] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) |
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);

	// CS_PIN_CFG_TRACE_DATA1
	NRF_P0_NS->PIN_CNF[CS_PIN_CFG_TRACE_DATA1] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) |
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);

	// CS_PIN_CFG_TRACE_DATA2
	NRF_P0_NS->PIN_CNF[CS_PIN_CFG_TRACE_DATA2] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) |
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);

	// CS_PIN_CFG_TRACE_DATA3
	NRF_P0_NS->PIN_CNF[CS_PIN_CFG_TRACE_DATA3] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) |
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);

	NRF_P0_NS->DIR = 0xFFFFFFFF;
}

void main(void)
{
	int err;

	modem_trace_enable();

	printk("Modem FOTA client started\n");

	err = nrf_modem_lib_get_init_ret();

	switch (err) {
	case MODEM_DFU_RESULT_OK:
		printk("Modem firmware update successful!\n");
		printk("Modem will run the new firmware after reboot\n");
		sys_reboot(SYS_REBOOT_WARM);
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		printk("Modem firmware update failed!\n");
		printk("Modem will run non-updated firmware on reboot.\n");
		sys_reboot(SYS_REBOOT_WARM);
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		printk("Modem firmware update failed!\n");
		printk("Fatal error.\n");
		sys_reboot(SYS_REBOOT_WARM);
		break;
	case -1:
		printk("Could not initialize bsdlib.\n");
		printk("Fatal error.\n");
		return;
	default:
		break;
	}

#if defined(CONFIG_SHELL)
	ping_init();
#endif

	if (sb_fota_init(&modem_fota_callback) != 0) {
		printk("Failed to initialize modem FOTA\n");
		return;
	}

	k_sleep(K_FOREVER);
}
