/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF71 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Nordic Semiconductor nRF71 family processor.
 */

#ifdef __NRF_TFM__
#include <zephyr/autoconf.h>
#endif

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#ifndef __NRF_TFM__
#include <zephyr/cache.h>
#endif

#if defined(NRF_APPLICATION)
#include <cmsis_core.h>
#include <hal/nrf_glitchdet.h>
#endif
#include <soc/nrfx_coredep.h>

#include <system_nrf7120_enga.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

void soc_early_init_hook(void)
{
	/* Update the SystemCoreClock global variable with current core clock
	 * retrieved from hardware state.
	 */
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) || defined(__NRF_TFM__)
	/* Currently not supported for non-secure */
	SystemCoreClockUpdate();
#endif

#ifdef __NRF_TFM__
	/* TF-M enables the instruction cache from target_cfg.c, so we
	 * don't need to enable it here.
	 */
#else
	/* Enable ICACHE */
	sys_cache_instr_enable();
#endif

	if (IS_ENABLED(CONFIG_SOC_NRF_FORCE_CONSTLAT)) {
		nrf_power_task_trigger(NRF_POWER, NRF_POWER_TASK_CONSTLAT);
	}

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
