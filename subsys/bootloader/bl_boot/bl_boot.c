/*
 * Copyright (c) 2020-2025 Nordic Semiconductor ASA
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

#include <zephyr/linker/linker-defs.h>
#define CLEANUP_RAM_GAP_START ((int)__ramfunc_region_start)
#define CLEANUP_RAM_GAP_SIZE ((int) (__ramfunc_end - __ramfunc_region_start))

#if defined(CONFIG_SB_DISABLE_NEXT_W)
#include <hal/nrf_rramc.h>
#define RRAMC_REGION_FOR_NEXT_W 4
#define NRF_RRAM_REGION_SIZE_UNIT 0x400
#define NRF_RRAM_REGION_ADDRESS_RESOLUTION 0x400

#if defined(CONFIG_SOC_NRF54L15_CPUAPP) || defined(CONFIG_SOC_NRF54L05_CPUAPP) ||                  \
	defined(CONFIG_SOC_NRF54L10_CPUAPP)
#define MAX_NEXT_W_SIZE (31 * 1024)
#elif defined(CONFIG_SOC_NRF54LV10A_ENGA_CPUAPP) || defined(CONFIG_SOC_NRF54LM20A_ENGA_CPUAPP)
#define MAX_NEXT_W_SIZE (127 * 1024)
#elif defined(CONFIG_SOC_NRF54LS05B_ENGA_CPUAPP)
#define MAX_NEXT_W_SIZE (1023 * 1024)
#endif

BUILD_ASSERT((PM_S0_IMAGE_ADDRESS % NRF_RRAM_REGION_ADDRESS_RESOLUTION) == 0,
	     "Start of S0 image region is not aligned - not possible to protect");

BUILD_ASSERT((PM_S0_IMAGE_SIZE % NRF_RRAM_REGION_SIZE_UNIT) == 0,
	     "Size of S0 image region is not aligned - not possible to protect");

BUILD_ASSERT(PM_S0_IMAGE_SIZE <= MAX_NEXT_W_SIZE, "Size of S0 partition is too big for protection");

#if defined(PM_S1_IMAGE_ADDRESS)
BUILD_ASSERT((PM_S1_IMAGE_ADDRESS % NRF_RRAM_REGION_ADDRESS_RESOLUTION) == 0,
	     "Start of S1 image region is not aligned - not possible to protect");

BUILD_ASSERT((PM_S1_IMAGE_SIZE % NRF_RRAM_REGION_SIZE_UNIT) == 0,
	     "Size of S1 image region is not aligned - not possible to protect");

BUILD_ASSERT(PM_S1_IMAGE_SIZE <= MAX_NEXT_W_SIZE, "Size of S1 partition is too big for protection");
#endif /* defined(PM_S1_IMAGE_ADDRESS) */

static int disable_next_w(const uint32_t address)
{
	uint32_t region_size_kb = 0;

	/* Note: the protection is only applied to the image itself, not the header (pad).
	 * When building with MCUBoot, applying protection to the header is not needed, as the
	 * header is only used during DFU and is only left for compatibility. Without MCUBoot, the
	 * header is not present.
	 */
	if (address == PM_S0_IMAGE_ADDRESS) {
		region_size_kb = PM_S0_IMAGE_SIZE / NRF_RRAM_REGION_SIZE_UNIT;
	} else if (address == PM_S1_IMAGE_ADDRESS) {
		region_size_kb = PM_S1_IMAGE_SIZE / NRF_RRAM_REGION_SIZE_UNIT;
	} else {
		return -EINVAL;
	}

	nrf_rramc_region_config_t config = {
		.address = address,
		.permissions =  NRF_RRAMC_REGION_PERM_READ_MASK |
				NRF_RRAMC_REGION_PERM_EXECUTE_MASK,
		.writeonce = false,
		/* There are no issues with locking the region here,
		 * as the next stage can still impose more strict
		 * protection by writing 0 to R/X disable bits.
		 */
		.lock = true,
		.size_kb = region_size_kb,
	};

	nrf_rramc_region_config_set(NRF_RRAMC, RRAMC_REGION_FOR_NEXT_W, &config);
	nrf_rramc_region_config_get(NRF_RRAMC, RRAMC_REGION_FOR_NEXT_W, &config);
	if (config.permissions & (NRF_RRAMC_REGION_PERM_WRITE_MASK)) {
		return -ENOSPC;
	}
	if (config.size_kb != region_size_kb) {
		return -ENOSPC;
	}

	return 0;
}

#endif

#if defined(CONFIG_SB_DISABLE_SELF_RWX)
/* Disabling R_X has to be done while running from RAM for obvious reasons.
 * Moreover as a last step before jumping to application it must work even after
 * RAM has been cleared, therefore these operations are performed while executing from RAM.
 * RAM cleanup ommits portion of the memory where code lives.
 */
#include <hal/nrf_rramc.h>

#define RRAMC_REGION_RWX_LSB 0
#define RRAMC_REGION_RWX_WIDTH 3
#define RRAMC_REGION_TO_LOCK_ADDR NRF_RRAMC->REGION[3].CONFIG
#define RRAMC_REGION_TO_LOCK_ADDR_H (((uint32_t)(&(RRAMC_REGION_TO_LOCK_ADDR))) >> 16)
#define RRAMC_REGION_TO_LOCK_ADDR_L (((uint32_t)(&(RRAMC_REGION_TO_LOCK_ADDR))) & 0x0000fffful)
#endif /* CONFIG_SB_DISABLE_SELF_RWX */

static void __ramfunc jump_in(uint32_t reset)
{
	__asm__ volatile (
		/* reset -> r0 */
		"   mov  r0, %0\n"
#ifdef CONFIG_SB_CLEANUP_RAM
		/* Base to write -> r1 */
		"   mov  r1, %1\n"
		/* Size to write -> r2 */
		"   mov  r2, %2\n"
		/* Value to write -> r3 */
		"   movw r3, %5\n"
		/* gap start */
		"   mov  r4, %3\n"
		/* gap size */
		"   mov  r5, %4\n"
		"clear:\n"
		"   subs r6, r4, r1\n"
		"   cbnz r6, skip_gap\n"
		"   add  r1, r5\n"
		"skip_gap:\n"
		"   str  r3, [r1]\n"
		"   add  r1, r1, #1\n"
		"   sub  r2, r2, #1\n"
		"   cbz  r2, clear_end\n"
		"   b    clear\n"
		"clear_end:\n"
		"   dsb\n"
#ifdef CONFIG_SB_INFINITE_LOOP_AFTER_RAM_CLEANUP
		"   b    clear_end\n"
#endif /* CONFIG_SB_INFINITE_LOOP_AFTER_RAM_CLEANUP */
#endif /* CONFIG_SB_CLEANUP_RAM */

#ifdef CONFIG_SB_DISABLE_SELF_RWX
		".thumb_func\n"
		"bootconf_disable_rwx:\n"
		"   movw r1, %6\n"
		"   movt r1, %7\n"
		"   ldr  r2, [r1]\n"
		/* Size of the region should be set at this point
		 * by provisioning through BOOTCONF.
		 * If not, set it according partition size.
		 */
		"   ands r4, r2, %12\n"
		"   cbnz r4, clear_rwx\n"
		"   movt r2, %8\n"
		"clear_rwx:\n"
		"   bfc  r2, %9, %10\n"
		/* Disallow further modifications */
		"   orr  r2, %11\n"
		"   str  r2, [r1]\n"
		"   dsb\n"
		/* Next assembly line is important for current function */

 #endif /* CONFIG_SB_DISABLE_SELF_RWX */

		/* Jump to reset vector of an app */
		"   bx   r0\n"
		:
		: "r" (reset),
		  "r" (CONFIG_SRAM_BASE_ADDRESS),
		  "r" (CONFIG_SRAM_SIZE * 1024),
		  "r" (CLEANUP_RAM_GAP_START),
		  "r" (CLEANUP_RAM_GAP_SIZE),
		  "i" (0)
#ifdef CONFIG_SB_DISABLE_SELF_RWX
		  , "i" (RRAMC_REGION_TO_LOCK_ADDR_L),
		  "i" (RRAMC_REGION_TO_LOCK_ADDR_H),
		  "i" (CONFIG_PM_PARTITION_SIZE_B0_IMAGE / 1024),
		  "i" (RRAMC_REGION_RWX_LSB),
		  "i" (RRAMC_REGION_RWX_WIDTH),
		  "i" (RRAMC_REGION_CONFIG_LOCK_Msk),
		  "i" (RRAMC_REGION_CONFIG_SIZE_Msk)
#endif /* CONFIG_SB_DISABLE_SELF_RWX */
		: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "memory"
	);
}

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
#if defined(CONFIG_HAS_HW_NRF_UARTE30)
	uninit_used_uarte(NRF_UARTE30);
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
#if !(defined(CONFIG_SOC_SERIES_NRF91) \
	|| defined(CONFIG_SOC_SERIES_NRF54L) \
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

	if (!fw_info_ext_api_provide(fw_info, true)) {
		printk("Failed to provide ext APIs to image, boot aborted.\r\n");
		return;
	}

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

#if defined(CONFIG_SB_DISABLE_NEXT_W)
	if (disable_next_w(fw_info->address)) {
		printk("Unable to disable writes on next stage");
		return;
	}
#endif

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

	jump_in((vector_table[1]));

	CODE_UNREACHABLE;
}
