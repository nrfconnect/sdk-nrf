/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF71 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Nordic Semiconductor nRF71 family processor.
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#if defined(NRF_APPLICATION)
#include <cmsis_core.h>
#include <hal/nrf_glitchdet.h>
#endif
#include <soc/nrfx_coredep.h>

#include <system_nrf7120_enga.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

void soc_early_init_hook(void)
{
	/* Enable ICACHE */
	sys_cache_instr_enable();
}

void arch_busy_wait(uint32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}

#ifdef CONFIG_SOC_RESET_HOOK
void soc_reset_hook(void)
{
	SystemInit();
}
#endif
