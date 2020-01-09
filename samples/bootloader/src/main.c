/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <errno.h>
#include <toolchain.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <nrf.h>
#include <pm_config.h>
#include <bl_validation.h>
#include <bl_crypto.h>
#include <fw_info.h>
#include <fprotect.h>
#include <provision.h>
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
}

#ifdef CONFIG_SW_VECTOR_RELAY
extern u32_t _vector_table_pointer;
#define VTOR _vector_table_pointer
#else
#define VTOR SCB->VTOR
#endif

static void boot_from(const struct fw_info *fw_info)
{
	u32_t *vector_table = (u32_t *)fw_info->address;

	printk("Attempting to boot from address 0x%x.\n\r",
		fw_info->address);

	if (!bl_validate_firmware_local(fw_info->address,
					fw_info)) {
		printk("Failed to validate!\n\r");
		return;
	}

#if CONFIG_ARCH_HAS_USERSPACE
	__ASSERT(!(CONTROL_nPRIV_Msk & __get_CONTROL()),
			"Not in Privileged mode");
#endif

	/* Allow any pending interrupts to be recognized */
	__ISB();
	__disable_irq();
	NVIC_Type *nvic = NVIC;
	/* Disable NVIC interrupts */
	for (u8_t i = 0; i < ARRAY_SIZE(nvic->ICER); i++) {
		nvic->ICER[i] = 0xFFFFFFFF;
	}
	/* Clear pending NVIC interrupts */
	for (u8_t i = 0; i < ARRAY_SIZE(nvic->ICPR); i++) {
		nvic->ICPR[i] = 0xFFFFFFFF;
	}

	printk("Booting (0x%x).\r\n", fw_info->address);

	uninit_used_peripherals();

	SysTick->CTRL = 0;

	/* Disable fault handlers used by the bootloader */
	SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;

#ifndef CONFIG_CPU_CORTEX_M0
	SCB->SHCSR &= ~(SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk |
			SCB_SHCSR_MEMFAULTENA_Msk);
#endif

	/* Activate the MSP if the core is found to currently run with the PSP */
	if (CONTROL_SPSEL_Msk & __get_CONTROL()) {
		__set_CONTROL(__get_CONTROL() & ~CONTROL_SPSEL_Msk);
	}

	__DSB(); /* Force Memory Write before continuing */
	__ISB(); /* Flush and refill pipeline with updated permissions */

	VTOR = fw_info->address;

	fw_info_ext_api_provide(fw_info);

	/* Set MSP to the new address and clear any information from PSP */
	__set_MSP(vector_table[0]);
	__set_PSP(0);

	/* Call reset handler. */
	((void (*)(void))vector_table[1])();
	CODE_UNREACHABLE;
}

void main(void)
{
	int err = fprotect_area(PM_B0_ADDRESS, PM_B0_SIZE);

	if (err) {
		printk("Protect B0 flash failed, cancel startup.\n\r");
		return;
	}

	u32_t s0_addr = s0_address_read();
	u32_t s1_addr = s1_address_read();
	const struct fw_info *s0_info = fw_info_find(s0_addr);
	const struct fw_info *s1_info = fw_info_find(s1_addr);

	if (!s1_info || (s0_info->version >= s1_info->version)) {
		boot_from(s0_info);
		boot_from(s1_info);
	} else {
		boot_from(s1_info);
		boot_from(s0_info);
	}

	printk("No bootable image found. Aborting boot.\n\r");
	return;
}
