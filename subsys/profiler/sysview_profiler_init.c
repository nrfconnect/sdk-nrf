/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */


#include <sysview_profiler_init.h>

static U64 get_time_cb(void)
{
	return (U64)k_cycle_get_32();
}

/* Services provided to SYSVIEW by Zephyr */
const SEGGER_SYSVIEW_OS_API SYSVIEW_X_OS_TraceAPI = {
	get_time_cb
};


u32_t sysview_get_timestamp(void)
{
	return k_cycle_get_32();
}


static void _cbSendSystemDesc(void)
{
	SEGGER_SYSVIEW_SendSysDesc("N=ZephyrSysView");
	SEGGER_SYSVIEW_SendSysDesc("D=" CONFIG_BOARD " "
				   CONFIG_SOC_SERIES " " CONFIG_ARCH);
	SEGGER_SYSVIEW_SendSysDesc("O=Zephyr");
}

void SEGGER_SYSVIEW_Conf(void)
{
	SEGGER_SYSVIEW_Init(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
			    CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
			    &SYSVIEW_X_OS_TraceAPI, _cbSendSystemDesc);

#if defined(CONFIG_PHYS_RAM_ADDR)       /* x86 */
	SEGGER_SYSVIEW_SetRAMBase(CONFIG_PHYS_RAM_ADDR);
#elif defined(CONFIG_SRAM_BASE_ADDRESS) /* arm, default */
	SEGGER_SYSVIEW_SetRAMBase(CONFIG_SRAM_BASE_ADDRESS);
#else
	/* No action */
#endif
}
