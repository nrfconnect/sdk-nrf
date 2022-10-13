/*
 * Copyright (c) 2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32L5 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/arch/arm/aarch32/nmi.h>
#include <zephyr/irq.h>
#include <stm32l5xx_ll_icache.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32l5_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	/* Enable ICACHE */
	while (LL_ICACHE_IsActiveFlag_BUSY()) {
	}
	LL_ICACHE_Enable();

	key = irq_lock();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 4 MHz from MSI */
	SystemCoreClock = 4000000;

	/* Enable Scale 0 to achieve 110MHz */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);

	/* Disable USB Type-C dead battery pull-down behavior */
	LL_PWR_DisableUCPDDeadBattery();

	return 0;
}

SYS_INIT(stm32l5_init, PRE_KERNEL_1, 0);
