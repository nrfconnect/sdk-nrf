/*
 * Copyright (c) 2020-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <soc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#if USE_PARTITION_MANAGER
#include <pm_config.h>
#include <flash_map_pm.h>
#else
#include <zephyr/storage/flash_map.h>
#endif
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
#if USE_PARTITION_MANAGER
#include <pm_config.h>
/* Address of storage as seen in processor address space */
#define BL_STORAGE_ADDRESS	PM_PROVISION_ADDRESS
#define BL_STORAGE_SIZE		PM_PROVISION_SIZE
#else
/* Address of storage as seen in processor address space */
#if DT_NODE_HAS_COMPAT(DT_PARENT(DT_NODELABEL(bl_storage)), nordic_nrf_uicr)
#define BL_STORAGE_ADDRESS	DT_REG_ADDR(DT_NODELABEL(bl_storage))
#define BL_STORAGE_SIZE		DT_REG_SIZE(DT_NODELABEL(bl_storage))
#else
#define BL_STORAGE_ADDRESS	PARTITION_ADDRESS(bl_storage)
#define BL_STORAGE_SIZE		PARTITION_SIZE(bl_storage)
#endif
#endif

#if USE_PARTITION_MANAGER
#define RWX_PROTECTION_REGION	PM_B0_SIZE
/* There is no skip with PM as we operate within image that does not
 * include MCUboot header.
 */
#define RWX_SKIP_SIZE		0
#else
#define RWX_PROTECTION_REGION	PARTITION_SIZE(b0_partition)
#define RWX_SKIP_SIZE		CONFIG_SB_DISABLE_SELF_RWX_SKIP_SIZE
#endif

#include <zephyr/linker/linker-defs.h>
#define CLEANUP_RAM_GAP_START ((int)__ramfunc_region_start)
#define CLEANUP_RAM_GAP_SIZE ((int) (__ramfunc_end - __ramfunc_region_start))

#if defined(CONFIG_SB_DISABLE_NEXT_W)
#if defined(CONFIG_SOC_SERIES_NRF71)
#include <hal/nrf_mramc.h>
#define NVM_REGION_FOR_NEXT_W 4
#define NVM_REGION_FOR_NEXT_W_SECOND 5
#define NVM_REGION_SIZE_UNIT 0x400
#define NVM_REGION_ADDRESS_RESOLUTION 0x400
/* SIZE field is 5 bits: 31 KiB per region, chained REGION[4]+[5] for up to 62 KiB. */
#define NVM_REGION_MAX_SIZE_KB 31
#define MAX_NEXT_W_SIZE (2 * NVM_REGION_MAX_SIZE_KB * 1024)
#else
#include <hal/nrf_rramc.h>
#define NVM_REGION_FOR_NEXT_W 4
#define NVM_REGION_SIZE_UNIT 0x400
#define NVM_REGION_ADDRESS_RESOLUTION 0x400

#if defined(CONFIG_SOC_NRF54L15_CPUAPP) || defined(CONFIG_SOC_NRF54L05_CPUAPP) ||                  \
	defined(CONFIG_SOC_NRF54L10_CPUAPP)
#define MAX_NEXT_W_SIZE (31 * 1024)
#elif defined(CONFIG_SOC_NRF54LV10A_CPUAPP) || defined(CONFIG_SOC_NRF54LC10A_CPUAPP) || \
	defined(CONFIG_SOC_NRF54LM20A_CPUAPP) || defined(CONFIG_SOC_NRF54LM20B_CPUAPP)
#define MAX_NEXT_W_SIZE (127 * 1024)
#elif defined(CONFIG_SOC_NRF54LS05B_CPUAPP)
#define MAX_NEXT_W_SIZE (1023 * 1024)
#endif
#endif

/* Note: the write protection is only applied to the image itself, not the header (pad).
 * When building with MCUBoot, applying protection to the header is not needed, as the
 * header is only used during DFU and is only left for compatibility. Without MCUBoot, the
 * header is not present.
 */
#if USE_PARTITION_MANAGER
#define S0_IMAGE_ADDRESS	(PM_S0_IMAGE_ADDRESS + RWX_SKIP_SIZE)
#define S0_IMAGE_SIZE		(PM_S0_IMAGE_SIZE - RWX_SKIP_SIZE)
#define S1_IMAGE_ADDRESS	(PM_S1_IMAGE_ADDRESS + RWX_SKIP_SIZE)
#define S1_IMAGE_SIZE		(PM_S1_IMAGE_SIZE - RWX_SKIP_SIZE)
#else
#define S0_IMAGE_ADDRESS	(FIXED_PARTITION_OFFSET(s0_partition) + RWX_SKIP_SIZE)
#define S0_IMAGE_SIZE		(FIXED_PARTITION_SIZE(s0_partition) - RWX_SKIP_SIZE)
#define S1_IMAGE_ADDRESS	(FIXED_PARTITION_OFFSET(s1_partition) + RWX_SKIP_SIZE)
#define S1_IMAGE_SIZE		(FIXED_PARTITION_SIZE(s1_partition) - RWX_SKIP_SIZE)
#endif

BUILD_ASSERT((S0_IMAGE_ADDRESS % NVM_REGION_ADDRESS_RESOLUTION) == 0,
	     "Start of S0 image region is not aligned - not possible to protect");

BUILD_ASSERT((S0_IMAGE_SIZE % NVM_REGION_SIZE_UNIT) == 0,
	     "Size of S0 image region is not aligned - not possible to protect");

BUILD_ASSERT(S0_IMAGE_SIZE <= MAX_NEXT_W_SIZE, "Size of S0 partition is too big for protection");

BUILD_ASSERT((S1_IMAGE_ADDRESS % NVM_REGION_ADDRESS_RESOLUTION) == 0,
	     "Start of S1 image region is not aligned - not possible to protect");

BUILD_ASSERT((S1_IMAGE_SIZE % NVM_REGION_SIZE_UNIT) == 0,
	     "Size of S1 image region is not aligned - not possible to protect");

BUILD_ASSERT(S1_IMAGE_SIZE <= MAX_NEXT_W_SIZE, "Size of S1 partition is too big for protection");

#if defined(CONFIG_SOC_SERIES_NRF71)
static int lock_mramc_region(uint8_t region, uint32_t address, uint32_t size_kb)
{
	nrf_mramc_region_config_t config = {
		.read = true,
		.write = false,
		.execute = true,
		.secure = true,
		.write_once = false,
		/* The next stage can still impose stricter protection by clearing R/X. */
		.lock = true,
		.size = size_kb,
	};

	nrf_mramc_region_address_set(NRF_MRAMC, region, address);
	nrf_mramc_region_config_set(NRF_MRAMC, region, &config);
	nrf_mramc_region_config_get(NRF_MRAMC, region, &config);

	if (config.write) {
		return -ENOSPC;
	}
	if (config.size != size_kb) {
		return -ENOSPC;
	}

	return 0;
}
#endif

static int disable_next_w(const uint32_t address)
{
	uint32_t region_size_kb = 0;

	if (address == S0_IMAGE_ADDRESS) {
		region_size_kb = S0_IMAGE_SIZE / NVM_REGION_SIZE_UNIT;
	} else if (address == S1_IMAGE_ADDRESS) {
		region_size_kb = S1_IMAGE_SIZE / NVM_REGION_SIZE_UNIT;
	} else {
		return -EINVAL;
	}

#if defined(CONFIG_SOC_SERIES_NRF71)
	/* REGION[4] protects the first (up to) 31 KiB of the active slot. */
	uint32_t region4_size_kb = MIN(region_size_kb, NVM_REGION_MAX_SIZE_KB);
	int err = lock_mramc_region(NVM_REGION_FOR_NEXT_W, address, region4_size_kb);

	if (err) {
		return err;
	}

	/* When the slot is larger than one region, chain REGION[5] for the rest.
	 * BUILD_ASSERT guarantees the remainder never exceeds a single region.
	 */
	if (region_size_kb > NVM_REGION_MAX_SIZE_KB) {
		uint32_t region5_address = address + NVM_REGION_MAX_SIZE_KB * 1024;
		uint32_t region5_size_kb = region_size_kb - NVM_REGION_MAX_SIZE_KB;

		err = lock_mramc_region(NVM_REGION_FOR_NEXT_W_SECOND,
					region5_address, region5_size_kb);
		if (err) {
			return err;
		}
	}
#else
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

	nrf_rramc_region_config_set(NRF_RRAMC, NVM_REGION_FOR_NEXT_W, &config);
	nrf_rramc_region_config_get(NRF_RRAMC, NVM_REGION_FOR_NEXT_W, &config);
	if (config.permissions & (NRF_RRAMC_REGION_PERM_WRITE_MASK)) {
		return -ENOSPC;
	}
	if (config.size_kb != region_size_kb) {
		return -ENOSPC;
	}
#endif

	return 0;
}

#endif

#if defined(CONFIG_SB_DISABLE_SELF_RWX)
/* Disabling R_X has to be done while running from RAM for obvious reasons.
 * Moreover as a last step before jumping to application it must work even after
 * RAM has been cleared, therefore these operations are performed while executing from RAM.
 * RAM cleanup ommits portion of the memory where code lives.
 */
#if defined(CONFIG_SOC_SERIES_NRF71)
#include <hal/nrf_mramc.h>
#define NVM_REGION_TO_LOCK_ADDR NRF_MRAMC->REGION[3].CONFIG
#define NVM_REGION_CONFIG_LOCK_Msk MRAMC_REGION_CONFIG_LOCK_Msk
#define NVM_REGION_CONFIG_SIZE_Msk MRAMC_REGION_CONFIG_SIZE_Msk
#else
#include <hal/nrf_rramc.h>
#define NVM_REGION_TO_LOCK_ADDR NRF_RRAMC->REGION[3].CONFIG
#define NVM_REGION_CONFIG_LOCK_Msk RRAMC_REGION_CONFIG_LOCK_Msk
#define NVM_REGION_CONFIG_SIZE_Msk RRAMC_REGION_CONFIG_SIZE_Msk
#endif

#define NVM_REGION_RWX_LSB 0
#define NVM_REGION_RWX_WIDTH 3
#define NVM_REGION_TO_LOCK_ADDR_H (((uint32_t)(&(NVM_REGION_TO_LOCK_ADDR))) >> 16)
#define NVM_REGION_TO_LOCK_ADDR_L (((uint32_t)(&(NVM_REGION_TO_LOCK_ADDR))) & 0x0000fffful)
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
		/* Reduce loop by gap size */
		"   sub  r2, r2, r5\n"
		"clear:\n"
		"   subs r6, r4, r1\n"
		"   cbnz r6, skip_gap\n"
		"   add  r1, r5\n"
		"skip_gap:\n"
		"   str  r3, [r1]\n"
		"   add  r1, r1, #4\n"
		"   sub  r2, r2, #4\n"
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
		  , "i" (NVM_REGION_TO_LOCK_ADDR_L),
		  "i" (NVM_REGION_TO_LOCK_ADDR_H),
		  "i" (RWX_PROTECTION_REGION / 1024),
		  "i" (NVM_REGION_RWX_LSB),
		  "i" (NVM_REGION_RWX_WIDTH),
		  "i" (NVM_REGION_CONFIG_LOCK_Msk),
		  "i" (NVM_REGION_CONFIG_SIZE_Msk)
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
	|| defined(CONFIG_SOC_SERIES_NRF71) \
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
	int err = fprotect_area(BL_STORAGE_ADDRESS, BL_STORAGE_SIZE);

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

	printk("Booting (0x%x).\r\n", fw_info->boot_address);

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

	VTOR = fw_info->boot_address;
	uint32_t *vector_table = (uint32_t *)fw_info->boot_address;

#if defined(CONFIG_SB_DISABLE_NEXT_W)
	/* Disable write to the next stage. Note that fw_info->address
	 * holds value of where image starts, as a binary.
	 * This matches Sx_IMAGE_ADDRESS for Partition Manager
	 * the header is separated to PAD partition; the Sx_IMAGE_ADDRESS
	 * is the same for PM and DTS partitions, assuming same layout,
	 * but fw_info->address is not.
	 * non-PM configuration the Sx_IMAGE_ADDRESS is address of
	 * an MCUboot header (or other header if added), and as we only protect
	 * binary we have to skip this over; in case of PM the address is
	 * past the PAD, which serves as MCUboot header, and points directly
	 * to executable section we want to protect.
	 */
	if (disable_next_w(fw_info->address + RWX_SKIP_SIZE)) {
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
