/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <soc.h>
#include <zephyr/sys/printk.h>
#include <pm_config.h>
#include <fw_info.h>
#include <fprotect.h>
#include <hal/nrf_clock.h>
#ifdef CONFIG_UART_NRFX
#ifdef CONFIG_UART_0_NRF_UART
#include <hal/nrf_uart.h>
#else
#include <hal/nrf_uarte.h>
#endif
#endif

static void uninit_used_peripherals(void)
{
#ifdef CONFIG_UART_0_NRF_UART
	nrf_uart_disable(NRF_UART0);
#elif defined(CONFIG_UART_0_NRF_UARTE)
	nrf_uarte_disable(NRF_UARTE0);
#elif defined(CONFIG_UART_1_NRF_UARTE)
	nrf_uarte_disable(NRF_UARTE1);
#elif defined(CONFIG_UART_2_NRF_UARTE)
	nrf_uarte_disable(NRF_UARTE2);
#endif
	nrf_clock_int_disable(NRF_CLOCK, 0xFFFFFFFF);
}

#ifdef CONFIG_SW_VECTOR_RELAY
extern uint32_t _vector_table_pointer;
#define VTOR _vector_table_pointer
#else
#define VTOR SCB->VTOR
#endif

void bl_boot(const struct fw_info *fw_info)
{
#if !(defined(CONFIG_SOC_SERIES_NRF91X) \
      || defined(CONFIG_SOC_NRF5340_CPUNET) \
      || defined(CONFIG_SOC_NRF5340_CPUAPP))
	/* Protect bootloader storage data after firmware is validated so
	 * invalidation of public keys can be written into the page if needed.
	 * Note that for some devices (for example, nRF9160 and the nRF5340
	 * application core), the bootloader storage data is kept in OTP which
	 * does not need or support protection. For nRF5340 network core the
	 * bootloader storage data is locked together with the network core
	 * application.
	 */
	int err = fprotect_area(PM_PROVISION_ADDRESS, PM_PROVISION_SIZE);

	if (err) {
		printk("Failed to protect bootloader storage.\n\r");
		return;
	}
#endif

#if CONFIG_ARCH_HAS_USERSPACE
	__ASSERT(!(CONTROL_nPRIV_Msk & __get_CONTROL()),
			"Not in Privileged mode");
#endif

	printk("Booting (0x%x).\r\n", fw_info->address);

	uninit_used_peripherals();

	/* Allow any pending interrupts to be recognized */
	__ISB();
	__disable_irq();
	NVIC_Type *nvic = NVIC;
	/* Disable NVIC interrupts */
	for (uint8_t i = 0; i < ARRAY_SIZE(nvic->ICER); i++) {
		nvic->ICER[i] = 0xFFFFFFFF;
	}
	/* Clear pending NVIC interrupts */
	for (uint8_t i = 0; i < ARRAY_SIZE(nvic->ICPR); i++) {
		nvic->ICPR[i] = 0xFFFFFFFF;
	}


	SysTick->CTRL = 0;

	/* Disable fault handlers used by the bootloader */
	SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;

#ifndef CONFIG_CPU_CORTEX_M0
	SCB->SHCSR &= ~(SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk |
			SCB_SHCSR_MEMFAULTENA_Msk);
#endif

	/* Activate the MSP if the core is currently running with the PSP */
	if (CONTROL_SPSEL_Msk & __get_CONTROL()) {
		__set_CONTROL(__get_CONTROL() & ~CONTROL_SPSEL_Msk);
	}

	__DSB(); /* Force Memory Write before continuing */
	__ISB(); /* Flush and refill pipeline with updated permissions */

	VTOR = fw_info->address;
	uint32_t *vector_table = (uint32_t *)fw_info->address;

	if (!fw_info_ext_api_provide(fw_info, true)) {
		return;
	}

#if defined(CONFIG_BUILTIN_STACK_GUARD) && \
    defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	/* Reset limit registers to avoid inflicting stack overflow on image
	 * being booted.
	 */
	__set_PSPLIM(0);
	__set_MSPLIM(0);
#endif

	/* Set MSP to the new address and clear any information from PSP */
	__set_MSP(vector_table[0]);
	__set_PSP(0);

	/* Call reset handler. */
	((void (*)(void))vector_table[1])();
	CODE_UNREACHABLE;
}
