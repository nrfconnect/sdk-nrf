/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <nrf.h>
#include <bsd.h>
#include <lte_lc.h>
#include <at_cmd.h>
#include <at_notif.h>
#include <net/bsdlib.h>
#include <bsd_platform.h>
#include <devicetree.h>

static u8_t app_mem_sram2_bss[20];
static u32_t app_mem_sram_data = 10U;

void app_func(void)
{
	printk("SRAM addr range: 0x%x-%x\n", CONFIG_SRAM_BASE_ADDRESS,
			CONFIG_SRAM_BASE_ADDRESS + (CONFIG_SRAM_SIZE*1024));
	u32_t sram2_addr = CONFIG_SRAM_BASE_ADDRESS -
			   (BSD_RESERVED_MEMORY_SIZE + (64*1024));
	printk("SRAM2 addr range: 0x%x-%x\n", sram2_addr, sram2_addr+(64*1024));
	printk("Called function %s\n", __func__);
	printk("App_func from sram2_text: %p\n", &app_func);
	printk("App_mem_sram2_bss: %p\n", &app_mem_sram2_bss[0]);
	printk("App mem_sram_data: %p\n", &app_mem_sram_data);
	printk("App mem_sram_data value: %d\n", app_mem_sram_data);
}

#if defined(CONFIG_BSD_LIBRARY)
/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	printk("bsdlib recoverable error: %u\n", err);
}
#endif /* defined(CONFIG_BSD_LIBRARY) */

static void modem_configure(void)
{
#if defined(CONFIG_LTE_LINK_CONTROL)
	BUILD_ASSERT_MSG(!IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT),
			"This sample does not support auto init and connect");
	int err;

	err = at_notif_init();
	__ASSERT(err == 0, "AT Notify could not be initialized.");
	err = at_cmd_init();
	__ASSERT(err == 0, "AT CMD could not be established.");
	printk("LTE Link Connecting ...\n");
	err = lte_lc_init_and_connect();
	__ASSERT(err == 0, "LTE link could not be established.");
	printk("LTE Link Connected!\n");
#endif
}



void main(void)
{
	printk("Secure world BSD example! %s\n", CONFIG_BOARD);
	printk("Initializing bsdlib\n");
	int err = bsdlib_init();

	switch (err) {
	case MODEM_DFU_RESULT_OK:
		printk("Modem firmware update successful!\n");
		printk("Modem will run the new firmware after reboot\n");
		k_thread_suspend(k_current_get());
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		printk("Modem firmware update failed\n");
		printk("Modem will run non-updated firmware on reboot.\n");
		k_thread_suspend(k_current_get());
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		printk("Modem firmware update failed\n");
		printk("Fatal error.\n");
		k_thread_suspend(k_current_get());
		break;
	case -1:
		printk("Could not initialize bsdlib.\n");
		printk("Fatal error.\n");
		k_thread_suspend(k_current_get());
		return;
	case 0:
		break;
	default:
		printk("Unexpected error.\n");
		k_thread_suspend(k_current_get());
		return;
	}
	printk("Initialized bsdlib\n");
	modem_configure();
	app_func();

	while (1) {
	}

	printk("End of sample...\n");
}
