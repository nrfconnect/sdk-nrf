/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <toolchain.h>
#include <misc/util.h>
#include <nrf.h>
#include <errno.h>
#include <pm_config.h>
#include "bootloader.h"
#include "bl_crypto.h"
#include "fw_metadata.h"
#include <misc/printk.h>

#include <fprotect.h>

#include <provision.h>
#ifdef CONFIG_UART_NRFX
#ifdef CONFIG_UART_0_NRF_UART
#include <hal/nrf_uart.h>
#else
#include <hal/nrf_uarte.h>
#endif
#endif

static bool verify_firmware(u32_t address)
{
	/* Some key data storage backends require word sized reads, hence
	 * we need to ensure word alignment for 'key_data'
	 */
	u32_t key_data[CONFIG_SB_PUBLIC_KEY_HASH_LEN/4];
	int retval = -EFAULT;
	int err;
	const struct fw_firmware_info *fw_info;
	const struct fw_validation_info *fw_ver_info;

	fw_info = fw_firmware_info_get(address);

	if (!fw_info) {
		printk("Could not find valid firmware info inside "
				    "firmware. Aborting boot!\n\r");
		return false;
	}

	fw_ver_info = validation_info_find(fw_info, 4);

	if (!fw_ver_info) {
		printk("Could not find valid firmware validation "
			  "info trailing firmware. Aborting boot!\n\r");
		return false;
	}

	err = bl_crypto_init();
	if (err) {
		printk("bl_crypto_init() returned %d. Aborting boot!\n\r", err);
		return false;
	}

	u32_t num_public_keys = num_public_keys_read();

	for (u32_t key_data_idx = 0; key_data_idx < num_public_keys;
			key_data_idx++) {
		if (public_key_data_read(key_data_idx, &key_data[0],
				CONFIG_SB_PUBLIC_KEY_HASH_LEN) < 0) {
			retval = -EFAULT;
			break;
		}
		retval = bl_root_of_trust_verify(fw_ver_info->public_key,
					      (u8_t *)key_data,
					      fw_ver_info->signature,
					      (u8_t *)address,
					      fw_info->firmware_size);
		if (retval != -ESIGINV) {
			break;
		}
	}

	if (retval != 0) {
		printk("Firmware validation failed with error %d. "
			    "Aborting boot!\n\r",
			    retval);
		return false;
	}

	return true;
}

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

static void boot_from(u32_t *address)
{
	printk("Attempting to boot from address 0x%x.\n\r", (u32_t)address);

	if (!verify_firmware((u32_t)address)) {
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

	printk("Booting (0x%x).\r\n", (u32_t)address);

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
	__ISB(); /* Flush and refill pipeline with updated premissions */

	VTOR = (u32_t)address;

	fw_abi_provide((u32_t)address);

	/* Set MSP to the new address and clear any information from PSP */
	__set_MSP(address[0]);
	__set_PSP(0);

	/* Call reset handler. */
	((void (*)(void))address[1])();
	CODE_UNREACHABLE;
}

void main(void)
{
	int err;
	err = fprotect_area(PM_B0_ADDRESS, PM_B0_SIZE);
	if (err) {
		printk("Protect B0 flash failed, cancel startup.\n\r");
		return;
	}

#ifndef CONFIG_SOC_NRF9160
	err = fprotect_area(PM_PROVISION_ADDRESS, PM_PROVISION_SIZE);
	if (err) {
		printk("Protect provision data failed, cancel startup.\n\r");
		return;
	}
#endif /* CONFIG_SOC_NRF9160 */

	u32_t s0_addr = s0_address_read();
	u32_t s1_addr = s1_address_read();
	const struct fw_firmware_info *s0_info = fw_firmware_info_get(s0_addr);
	const struct fw_firmware_info *s1_info = fw_firmware_info_get(s1_addr);

	if (s0_info != NULL && s1_info != NULL) {
		if (s0_info->firmware_version >= s1_info->firmware_version) {
			boot_from((u32_t *)s0_addr);
		} else {
			boot_from((u32_t *)s1_addr);
		}
	} else if (s0_info != NULL) {
		boot_from((u32_t *)s0_addr);
	} else if (s1_info != NULL) {
		boot_from((u32_t *)s1_addr);
	} else {
		printk("No bootable image found. aborting...\n\r");
		return;
	}

	CODE_UNREACHABLE;
}
