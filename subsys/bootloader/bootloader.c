/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "debug.h"
#include <zephyr/types.h>
#include <toolchain.h>
#include <misc/util.h>
#include <nrf.h>
#include <errno.h>
#include <generated_dts_board.h>
#include "bootloader.h" /* TODO: Remove multi_image */
#include "bl_crypto.h"
#include "fw_metadata.h"

#ifdef CONFIG_SB_FLASH_PROTECT
#include <fprotect.h>
#endif

#include <provision.h>

static bool verify_firmware(u32_t address)
{
	int retval = -EFAULT;
	const struct fw_firmware_info *fw_info;
	const struct fw_validation_info *fw_ver_info;
	u8_t key_data[CONFIG_SB_PUBLIC_KEY_HASH_LEN];

	fw_info = firmware_info_get(address);

	printk("Attempting to boot from address 0x%x.\n\r", address);

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

	u32_t num_public_keys = num_public_keys_read();

	for (u32_t key_data_idx = 0; key_data_idx < num_public_keys;
			key_data_idx++) {
		if (public_key_data_read(key_data_idx, &key_data[0],
				CONFIG_SB_PUBLIC_KEY_HASH_LEN) < 0) {
			retval = -EFAULT;
			break;
		}
		retval = crypto_root_of_trust(fw_ver_info->public_key,
					      key_data,
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

void uninit_used_peripherals(void)
{
	/* We do not want to uninitialize cryptocell as we want to retain the
	 * root of trust key loaded inside cryptocell.
	 */
#ifdef CONFIG_SB_DEBUG_PORT_UART
	uart_uninit();
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
	if (!verify_firmware((u32_t)address)) {
		return;
	}

	__ASSERT(!(CONTROL_nPRIV_Msk & __get_CONTROL()),
			"Not in Privileged mode");

	/* Allow any pending interrupts to be recognized */
	__ISB();
	__disable_irq();
	NVIC_Type *nvic = NVIC;
	/* Disable NVIC interrupts
	 * TODO: @sigvartmh May be redundant CPSID would maybe clear this
	 */
	for (u8_t i = 0; i < ARRAY_SIZE(nvic->ICER); i++) {
		nvic->ICER[i] = 0xFFFFFFFF;
	}
	/* Clear pending NVIC interrupts */
	for (u8_t i = 0; i < ARRAY_SIZE(nvic->ICPR); i++) {
		nvic->ICPR[i] = 0xFFFFFFFF;
	}

	uninit_used_peripherals();

	SysTick->CTRL = 0;

	/* Disable fault handlers used by the bootloader
	 * TODO: @sigvartmh currently not implemented or used
	 */
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

	/* Set MSP to the new address and clear any information from PSP */
	__set_MSP(address[0]);
	__set_PSP(0);

	/* Call reset handler. */
	((void (*)(void))address[1])();
	CODE_UNREACHABLE;
}

void _Cstart(void) __attribute__((alias("main_bl")));
void main_bl(void)
{
#if defined(CONFIG_SB_DEBUG_PORT_SEGGER_RTT)
	SEGGER_RTT_Init();
#elif defined(CONFIG_SB_DEBUG_PORT_UART)
	uart_init();
#endif /* CONFIG_SB_RTT */
#if CONFIG_SB_FLASH_PROTECT
	int err;
	err = fprotect_area(DT_FLASH_AREA_SECURE_BOOT_OFFSET,
			DT_FLASH_AREA_SECURE_BOOT_SIZE);
	if (err) {
		printk("Protect B0 flash failed, cancel startup.\n\r");
		return;
	}

#ifndef CONFIG_SOC_NRF9160
	err = fprotect_area(DT_FLASH_AREA_PROVISION_OFFSET,
			DT_FLASH_AREA_PROVISION_SIZE);
	if (err) {
		printk("Protect provision data failed, cancel startup.\n\r");
		return;
	}
#endif /* CONFIG_SOC_NRF9160 */

#endif /* CONFIG_SB_FLASH_PROTECT */

	boot_from((u32_t *)(0x00000000 + DT_FLASH_AREA_APP_OFFSET));
	CODE_UNREACHABLE;
}
