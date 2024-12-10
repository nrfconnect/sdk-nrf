/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <soc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <pm_config.h>
#include <fw_info.h>
#include <fprotect.h>
#include <hal/nrf_clock.h>
#ifdef CONFIG_UART_NRFX_UART
#include <hal/nrf_uart.h>
#endif
#ifdef CONFIG_UART_NRFX_UARTE
#include <hal/nrf_uarte.h>
#include <hal/nrf_gpio.h>
#endif

#ifdef CONFIG_UART_NRFX_UARTE
static void uninit_used_uarte(NRF_UARTE_Type *p_reg)
{
	uint32_t pin[4];

	nrf_uarte_disable(p_reg);

	pin[0] = nrf_uarte_tx_pin_get(p_reg);
	pin[1] = nrf_uarte_rx_pin_get(p_reg);
	pin[2] = nrf_uarte_rts_pin_get(p_reg);
	pin[3] = nrf_uarte_cts_pin_get(p_reg);

	for (int i = 0; i < 4; i++) {
		if (pin[i] != NRF_UARTE_PSEL_DISCONNECTED) {
			nrf_gpio_cfg_default(pin[i]);
		}
	}
}
#endif

static void uninit_used_peripherals(void)
{
#if defined(CONFIG_UART_NRFX)
#if defined(CONFIG_HAS_HW_NRF_UART0)
	nrf_uart_disable(NRF_UART0);
#elif defined(CONFIG_HAS_HW_NRF_UARTE0)
	uninit_used_uarte(NRF_UARTE0);
#endif
#if defined(CONFIG_HAS_HW_NRF_UARTE1)
	uninit_used_uarte(NRF_UARTE1);
#endif
#if defined(CONFIG_HAS_HW_NRF_UARTE2)
	uninit_used_uarte(NRF_UARTE2);
#endif
#if defined(CONFIG_HAS_HW_NRF_UARTE20)
	uninit_used_uarte(NRF_UARTE20);
#endif
#endif /* CONFIG_UART_NRFX */

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
	|| defined(CONFIG_SOC_SERIES_NRF54LX) \
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
#if defined(CONFIG_FPROTECT)
	int err = fprotect_area(PM_PROVISION_ADDRESS, PM_PROVISION_SIZE);

	if (err) {
		printk("Failed to protect bootloader storage.\n\r");
		return;
	}
#else
		printk("Fprotect disabled. No protection applied.\n\r");
#endif

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

#if CONFIG_SB_CLEANUP_RAM
	__asm__ volatile (
		/* vector_table[1] -> r0 */
		"   mov r0, %0\n"
		/* Base to write -> r1 */
		"   mov r1, %1\n"
		/* Size to write -> r2 */
		"   mov r2, %2\n"
		/* Value to write -> r3 */
		"   mov r3, %3\n"
		"clear:\n"
		"   str r3, [r1]\n"
		"   add r1, r1, #4\n"
		"   sub r2, r2, #4\n"
		"   cbz r2, out\n"
		"   b   clear\n"
		"out:\n"
		"   dsb\n"
		/* Jump to reset vector of an app */
		"   bx r0\n"
		:
		: "r" (vector_table[1]), "i" (CONFIG_SRAM_BASE_ADDRESS),
		  "i" (CONFIG_SRAM_SIZE * 1024), "i" (0)
		: "r0", "r1", "r2", "r3", "memory"
	);
#else
	/* Call reset handler. */
	((void (*)(void))vector_table[1])();
#endif
	CODE_UNREACHABLE;
}
