/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <linker/linker-defs.h>
#include <device.h>
#include <drivers/gpio.h>
#include <hal/nrf_spu.h>
#include <arch/arm/aarch32/irq.h>
#include "spm_internal.h"

#if !defined(CONFIG_ARM_SECURE_FIRMWARE)
#error "Module requires compiling for Secure ARM Firmware"
#endif

/* Include required APIs for TrustZone-M */
#include <arm_cmse.h>
#include <aarch32/cortex_m/tz.h>

#include <nrfx.h>

#if USE_PARTITION_MANAGER
#include <pm_config.h>
#define NON_SECURE_APP_ADDRESS PM_APP_ADDRESS
#ifdef PM_SRAM_SECURE_SIZE
#define NON_SECURE_RAM_OFFSET PM_SRAM_SECURE_SIZE
#else
#define NON_SECURE_RAM_OFFSET 0
#endif
#else
#include <storage/flash_map.h>
#define NON_SECURE_APP_ADDRESS FLASH_AREA_OFFSET(image_0_nonsecure)
/* This reflects the configuration in DTS. */
#define NON_SECURE_RAM_OFFSET 0x10000
#endif /* USE_PARTITION_MANAGER */

#define NON_SECURE_FLASH_REGION_INDEX \
	((NON_SECURE_APP_ADDRESS) / (FLASH_SECURE_ATTRIBUTION_REGION_SIZE))
#define NON_SECURE_RAM_REGION_INDEX \
	((NON_SECURE_RAM_OFFSET) / (RAM_SECURE_ATTRIBUTION_REGION_SIZE))

/*
 *  * The security configuration for depends on where the non secure app
 *  * is placed. All flash regions before the region which contains the
 *  * non secure app is configured as Secure.
 *
 *                FLASH
 *  1 MB  |---------------------|
 *        |                     |
 *        |                     |
 *        |                     |
 *        |                     |
 *        |                     |
 *        |     Non-Secure      |
 *        |       Flash         |
 *        |                     |
 *  X kB  |---------------------|
 *        |                     |
 *        |     Secure          |
 *        |      Flash          |
 *  0 kB  |---------------------|
 *
 *  * The SRAM configuration is given by the partition manager.
 *  * To see the current configuration, run the 'pm_report' target.
 *  * E.g. 'ninja pm_report, in your build folder. All partitions
 *  * within the 'secure_ram' span is configured as secure by the SPM.
 *
 */

extern irq_target_state_t irq_target_state_set(unsigned int irq,
	irq_target_state_t irq_target_state);
extern int irq_target_state_is_secure(unsigned int irq);

/* printk wrapper, to turn off logs when booting silently */
#define PRINT(...)                                                             \
	do {                                                                   \
		if (!IS_ENABLED(CONFIG_SPM_BOOT_SILENTLY)) {                   \
			printk(__VA_ARGS__);                                   \
		}                                                              \
	} while (0)

/* Local convenience macro to extract the peripheral
 * ID from the base address.
 */
#define NRFX_PERIPHERAL_ID_GET(base_addr) \
	(uint8_t)((uint32_t)(base_addr) >> 12)

#ifdef CONFIG_SPM_BOOT_SILENTLY
#define PERIPH(name, reg, config)                                              \
	{                                                                      \
		.id = NRFX_PERIPHERAL_ID_GET(reg), IS_ENABLED(config)          \
	}
#else
#define PERIPH(name, reg, config)                                              \
	{                                                                      \
		name, .id = NRFX_PERIPHERAL_ID_GET(reg), IS_ENABLED(config)    \
	}
#endif

/* Check if configuration exceeds the number of
 * DPPI Channels available on device.
 */
#if (CONFIG_SPM_NRF_DPPIC_PERM_MASK >= (1 << DPPI_CH_NUM))
#error "SPM_NRF_DPPIC_PERM_MASK exceeds number of available DPPI channels"
#endif

#if defined(CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS)

static void spm_config_nsc_flash(void)
{
	/* Configure a single region in Secure Flash as Non-Secure Callable
	 * (NSC) area.
	 *
	 * Area to configure is dynamically decided with help from linker code.
	 *
	 * Note: Any Secure Entry functions, exposing secure services to the
	 * Non-Secure firmware, shall be located inside this NSC area.
	 *
	 * If the start address of the NSC area is hard-coded, it must follow
	 * the HW restrictions: The size must be a power of 2 between 32 and
	 * 4096, and the end address must fall on a SPU region boundary.
	 */
	uint32_t nsc_size = FLASH_NSC_SIZE_FROM_ADDR(__sg_start);

	__ASSERT((uint32_t)__sg_size <= nsc_size,
		"The Non-Secure Callable region is overflowed by %d byte(s).\n",
		(uint32_t)__sg_size - nsc_size);

	nrf_spu_flashnsc_set(NRF_SPU, 0, FLASH_NSC_SIZE_REG(nsc_size),
			FLASH_NSC_REGION_FROM_ADDR(__sg_start), false);

	PRINT("Non-secure callable region 0 placed in flash region %d with size %d.\n",
		NRF_SPU->FLASHNSC[0].REGION, NRF_SPU->FLASHNSC[0].SIZE << 5);
}
#endif /* CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS */


static void config_regions(bool ram, size_t start, size_t end, uint32_t perm)
{
	const size_t region_size = ram ? RAM_SECURE_ATTRIBUTION_REGION_SIZE
					: FLASH_SECURE_ATTRIBUTION_REGION_SIZE;

	__ASSERT_NO_MSG(end >= start);
	if (end <= start) {
		return;
	}

	for (size_t i = start; i < end; i++) {
		if (ram) {
			NRF_SPU->RAMREGION[i].PERM = perm;
		} else {
			NRF_SPU->FLASHREGION[i].PERM = perm;
		}
	}

	PRINT("%02u %02u 0x%05x 0x%05x \t", start, end - 1,
				region_size * start, region_size * end);
	PRINT("%s", perm & (ram ? SRAM_SECURE : FLASH_SECURE) ? "Secure\t\t" :
								"Non-Secure\t");
	PRINT("%c", perm & (ram ? SRAM_READ : FLASH_READ)  ? 'r' : '-');
	PRINT("%c", perm & (ram ? SRAM_WRITE : FLASH_WRITE) ? 'w' : '-');
	PRINT("%c", perm & (ram ? SRAM_EXEC : FLASH_EXEC)  ? 'x' : '-');
	PRINT("%c", perm & (ram ? SRAM_LOCK : FLASH_LOCK)  ? 'l' : '-');
	PRINT("\n");
}


static void spm_config_flash(void)
{
	/* Regions of flash up to and including SPM are configured as Secure.
	 * The rest of flash is configured as Non-Secure.
	 */
	const uint32_t secure_flash_perm = FLASH_READ | FLASH_WRITE | FLASH_EXEC
			| FLASH_LOCK | FLASH_SECURE;
	const uint32_t nonsecure_flash_perm = FLASH_READ | FLASH_WRITE | FLASH_EXEC
			| FLASH_LOCK | FLASH_NONSEC;

	PRINT("Flash regions\t\tDomain\t\tPermissions\n");

	config_regions(false, 0, NON_SECURE_FLASH_REGION_INDEX,
			secure_flash_perm);
	config_regions(false, NON_SECURE_FLASH_REGION_INDEX,
			NUM_FLASH_SECURE_ATTRIBUTION_REGIONS,
			nonsecure_flash_perm);
	PRINT("\n");

#if defined(CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS)
	spm_config_nsc_flash();
	PRINT("\n");

#if defined(CONFIG_SPM_SECURE_SERVICES)
	int err = spm_secure_services_init();

	if (err != 0) {
		PRINT("Could not initialize secure services (err %d).\n", err);
	}
#endif
#endif /* CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS */
}

static void spm_config_sram(void)
{
	/* Lower 64 kB of SRAM is allocated to the Secure firmware image.
	 * The rest of SRAM is allocated to Non-Secure firmware image.
	 */

	const uint32_t secure_ram_perm = SRAM_READ | SRAM_WRITE | SRAM_EXEC
		| SRAM_LOCK | SRAM_SECURE;
	const uint32_t nonsecure_ram_perm = SRAM_READ | SRAM_WRITE | SRAM_EXEC
		| SRAM_LOCK | SRAM_NONSEC;

	PRINT("SRAM region\t\tDomain\t\tPermissions\n");

	/* Configuration for Secure RAM Regions (0 - 64 kB) */
	config_regions(true, 0, NON_SECURE_RAM_REGION_INDEX,
			secure_ram_perm);
	/* Configuration for Non-Secure RAM Regions (64 kb - end) */
	config_regions(true, NON_SECURE_RAM_REGION_INDEX,
			NUM_RAM_SECURE_ATTRIBUTION_REGIONS,
			nonsecure_ram_perm);
	PRINT("\n");
}

static bool usel_or_split(uint8_t id)
{
	const uint32_t perm = NRF_SPU->PERIPHID[id].PERM;

	/* NRF_GPIOTE1_NS needs special handling as its
	 * peripheral ID for non-secure han incorrect properties
	 * in the NRF_SPM->PERIPHID[id].perm register.
	 */
	if (id == NRFX_PERIPHERAL_ID_GET(NRF_GPIOTE1_NS)) {
		return true;
	}

	bool present = (perm & SPU_PERIPHID_PERM_PRESENT_Msk) ==
		       SPU_PERIPHID_PERM_PRESENT_Msk;

	/* User-selectable attribution */
	bool usel = (perm & SPU_PERIPHID_PERM_SECUREMAPPING_Msk) ==
		    SPU_PERIPHID_PERM_SECUREMAPPING_UserSelectable;

	/* Split attribution */
	bool split = (perm & SPU_PERIPHID_PERM_SECUREMAPPING_Msk) ==
		     SPU_PERIPHID_PERM_SECUREMAPPING_Split;

	return present && (usel || split);
}

static int config_peripheral(uint8_t id, bool dma_present, bool lock)
{
	/* Set a peripheral to Non-Secure state, if
	 * - it is present
	 * - has UserSelectable/Split attribution.
	 *
	 * Assign DMA capabilities and lock down the attribution.
	 *
	 * Note: the function assumes that the peripheral ID matches
	 * the IRQ line.
	 */
	NVIC_DisableIRQ(id);

	if (usel_or_split(id)) {
		NRF_SPU->PERIPHID[id].PERM = PERIPH_PRESENT | PERIPH_NONSEC |
			(dma_present ? PERIPH_DMA_NOSEP : 0) |
			(lock ? PERIPH_LOCK : 0);
	}

	/* Even for non-present peripherals we force IRQs to be routed
	 * to Non-Secure state.
	 */
	irq_target_state_set(id, IRQ_TARGET_STATE_NON_SECURE);
	return 0;
}

static int spm_config_peripheral(uint8_t id, bool dma_present)
{
	return config_peripheral(id, dma_present, true);
}

static int spm_config_unlocked_peripheral(uint8_t id, bool dma_present)
{
	return config_peripheral(id, dma_present, false);
}

static void spm_dppi_configure(uint32_t mask)
{
	NRF_SPU->DPPI[0].PERM = mask;
}

static void spm_config_peripherals(void)
{
	struct periph_cfg {
#ifndef CONFIG_SPM_BOOT_SILENTLY
		char *name;
#endif
		uint8_t id;
		uint8_t nonsecure;
	};

	/* - All user peripherals are allocated to the Non-Secure domain.
	 * - All GPIOs are allocated to the Non-Secure domain.
	 */
	static const struct periph_cfg periph[] = {
#ifdef NRF_P0
		PERIPH("NRF_P0", NRF_P0, CONFIG_SPM_NRF_P0_NS),
#endif
#ifdef NRF_CLOCK
		PERIPH("NRF_CLOCK", NRF_CLOCK, CONFIG_SPM_NRF_CLOCK_NS),
#endif
#ifdef NRF_RTC0
		PERIPH("NRF_RTC0", NRF_RTC0, CONFIG_SPM_NRF_RTC0_NS),
#endif
#ifdef NRF_RTC1
		PERIPH("NRF_RTC1", NRF_RTC1, CONFIG_SPM_NRF_RTC1_NS),
#endif
#ifdef NRF_NFCT
		PERIPH("NRF_NFCT", NRF_NFCT, CONFIG_SPM_NRF_NFCT_NS),
#endif
#ifdef NRF_NVMC
		PERIPH("NRF_NVMC", NRF_NVMC, CONFIG_SPM_NRF_NVMC_NS),
#endif
#ifdef NRF_UARTE1
		PERIPH("NRF_UARTE1", NRF_UARTE1, CONFIG_SPM_NRF_UARTE1_NS),
#endif
#ifdef NRF_UARTE2
		PERIPH("NRF_UARTE2", NRF_UARTE2, CONFIG_SPM_NRF_UARTE2_NS),
#endif
#ifdef NRF_TWIM2
		PERIPH("NRF_TWIM2", NRF_TWIM2, CONFIG_SPM_NRF_TWIM2_NS),
#endif
#ifdef NRF_SPIM3
		PERIPH("NRF_SPIM3", NRF_SPIM3, CONFIG_SPM_NRF_SPIM3_NS),
#endif
#ifdef NRF_TIMER0
		PERIPH("NRF_TIMER0", NRF_TIMER0, CONFIG_SPM_NRF_TIMER0_NS),
#endif
#ifdef NRF_TIMER1
		PERIPH("NRF_TIMER1", NRF_TIMER1, CONFIG_SPM_NRF_TIMER1_NS),
#endif
#ifdef NRF_TIMER2
		PERIPH("NRF_TIMER2", NRF_TIMER2, CONFIG_SPM_NRF_TIMER2_NS),
#endif
#ifdef NRF_SAADC
		PERIPH("NRF_SAADC", NRF_SAADC, CONFIG_SPM_NRF_SAADC_NS),
#endif
#ifdef NRF_PWM0
		PERIPH("NRF_PWM0", NRF_PWM0, CONFIG_SPM_NRF_PWM0_NS),
#endif
#ifdef NRF_PWM1
		PERIPH("NRF_PWM1", NRF_PWM1, CONFIG_SPM_NRF_PWM1_NS),
#endif
#ifdef NRF_PWM2
		PERIPH("NRF_PWM2", NRF_PWM2, CONFIG_SPM_NRF_PWM2_NS),
#endif
#ifdef NRF_PWM3
		PERIPH("NRF_PWM3", NRF_PWM3, CONFIG_SPM_NRF_PWM3_NS),
#endif
#ifdef NRF_WDT
		PERIPH("NRF_WDT", NRF_WDT, CONFIG_SPM_NRF_WDT_NS),
#endif
#ifdef NRF_IPC
		PERIPH("NRF_IPC", NRF_IPC, CONFIG_SPM_NRF_IPC_NS),
#endif
#ifdef NRF_VMC
		PERIPH("NRF_VMC", NRF_VMC, CONFIG_SPM_NRF_VMC_NS),
#endif
#ifdef NRF_FPU
		PERIPH("NRF_FPU", NRF_FPU, CONFIG_SPM_NRF_FPU_NS),
#endif
#ifdef NRF_EGU1
		PERIPH("NRF_EGU1", NRF_EGU1, CONFIG_SPM_NRF_EGU1_NS),
#endif
#ifdef NRF_EGU2
		PERIPH("NRF_EGU2", NRF_EGU2, CONFIG_SPM_NRF_EGU2_NS),
#endif
#ifdef NRF_DPPIC
		PERIPH("NRF_DPPIC", NRF_DPPIC, CONFIG_SPM_NRF_DPPIC_NS),
#endif
#ifdef NRF_REGULATORS
		PERIPH("NRF_REGULATORS", NRF_REGULATORS,
				      CONFIG_SPM_NRF_REGULATORS_NS),
#endif
#ifdef NRF_DCNF
		PERIPH("NRF_DCNF", NRF_DCNF, CONFIG_SPM_NRF_DCNF_NS),
#endif
#ifdef NRF_CTRLAP
		PERIPH("NRF_CTRLAP", NRF_CTRLAP, CONFIG_SPM_NRF_CTRLAP_NS),
#endif
#ifdef NRF_SPIM4
		PERIPH("NRF_SPIM4", NRF_SPIM4, CONFIG_SPM_NRF_SPIM4_NS),
#endif
#ifdef NRF_WDT0
		PERIPH("NRF_WDT0", NRF_WDT0, CONFIG_SPM_NRF_WDT0_NS),
#endif
#ifdef NRF_WDT1
		PERIPH("NRF_WDT1", NRF_WDT1, CONFIG_SPM_NRF_WDT1_NS),
#endif
#ifdef NRF_COMP
		PERIPH("NRF_COMP", NRF_COMP, CONFIG_SPM_NRF_COMP_NS),
#endif
#ifdef NRF_LPCOMP
		PERIPH("NRF_LPCOMP", NRF_LPCOMP, CONFIG_SPM_NRF_LPCOMP_NS),
#endif
#ifdef NRF_PDM
		PERIPH("NRF_PDM", NRF_PDM, CONFIG_SPM_NRF_PDM_NS),
#endif
#ifdef NRF_PDM0
		PERIPH("NRF_PDM0", NRF_PDM0, CONFIG_SPM_NRF_PDM0_NS),
#endif
#ifdef NRF_I2S
		PERIPH("NRF_I2S", NRF_I2S, CONFIG_SPM_NRF_I2S_NS),
#endif
#ifdef NRF_I2S0
		PERIPH("NRF_I2S0", NRF_I2S0, CONFIG_SPM_NRF_I2S0_NS),
#endif
#ifdef NRF_QSPI
		PERIPH("NRF_QSPI", NRF_QSPI, CONFIG_SPM_NRF_QSPI_NS),
#endif
#ifdef NRF_NFCT
		PERIPH("NRF_NFCT", NRF_NFCT, CONFIG_SPM_NRF_NFCT_NS),
#endif
#ifdef NRF_MUTEX
		PERIPH("NRF_MUTEX", NRF_MUTEX, CONFIG_SPM_NRF_MUTEX_NS),
#endif
#ifdef NRF_QDEC0
		PERIPH("NRF_QDEC0", NRF_QDEC0, CONFIG_SPM_NRF_QDEC0_NS),
#endif
#ifdef NRF_QDEC1
		PERIPH("NRF_QDEC1", NRF_QDEC1, CONFIG_SPM_NRF_QDEC1_NS),
#endif
#ifdef NRF_USBD
		PERIPH("NRF_USBD", NRF_USBD, CONFIG_SPM_NRF_USBD_NS),
#endif
#ifdef NRF_USBREGULATOR
		PERIPH("NRF_USBREGULATOR", NRF_USBREGULATOR,
		       CONFIG_SPM_NRF_USBREGULATOR_NS),
#endif
#ifdef NRF_P1
		PERIPH("NRF_P1", NRF_P1, CONFIG_SPM_NRF_P1_NS),
#endif
#ifdef NRF_OSCILLATORS
		PERIPH("NRF_OSCILLATORS", NRF_OSCILLATORS,
		       CONFIG_SPM_NRF_OSCILLATORS_NS),
#endif
#ifdef NRF_RESET
		PERIPH("NRF_RESET", NRF_RESET, CONFIG_SPM_NRF_RESET_NS),
#endif

		/* There is no DTS node for the peripherals below,
		 * so address them using nrfx macros directly.
		 */
		PERIPH("NRF_GPIOTE1", NRF_GPIOTE1_NS,
				      CONFIG_SPM_NRF_GPIOTE1_NS),

	};

	if (IS_ENABLED(CONFIG_SPM_NRF_DPPIC_NS)) {
		spm_dppi_configure(CONFIG_SPM_NRF_DPPIC_PERM_MASK);
	}

	PRINT("Peripheral\t\tDomain\t\tStatus\n");

	if (IS_ENABLED(CONFIG_SPM_NRF_P0_NS)) {
		/* Configure GPIO pins to be Non-Secure */
		NRF_SPU->GPIOPORT[0].PERM = 0;
	}

#ifdef CONFIG_SOC_NRF5340_CPUAPP
	if (IS_ENABLED(CONFIG_SPM_NRF_P1_NS)) {
		/* Configure GPIO pins to be Non-Secure */
		NRF_SPU->GPIOPORT[1].PERM = 0;
	}
#endif

	for (size_t i = 0; i < ARRAY_SIZE(periph); i++) {
		int err;

#ifndef CONFIG_SPM_BOOT_SILENTLY
		PRINT("%02u %-21s%s", i, periph[i].name,
		      periph[i].nonsecure ? "Non-Secure" : "Secure\t");
#endif

		if (!periph[i].nonsecure) {
			PRINT("\tSKIP\n");
			continue;
		}

		err = spm_config_peripheral(periph[i].id, false);
		if (err) {
			PRINT("\tERROR\n");
		} else {
			PRINT("\tOK\n");
		}
	}
	PRINT("\n");
}


static void spm_configure_ns(const tz_nonsecure_setup_conf_t
	*spm_ns_conf)
{
	/* Configure core register block for Non-Secure state. */
	tz_nonsecure_state_setup(spm_ns_conf);
	/* Prioritize Secure exceptions over Non-Secure */
	tz_nonsecure_exception_prio_config(1);
	/* Set non-banked exceptions to target Non-Secure */
	tz_nbanked_exception_target_state_set(0);
	/* Configure if Non-Secure firmware should be allowed to issue System
	 * reset. If not it could be enabled through a secure service.
	 */
	tz_nonsecure_system_reset_req_block(
		IS_ENABLED(CONFIG_SPM_BLOCK_NON_SECURE_RESET)
	);
	/* Allow SPU to have precedence over (non-existing) ARMv8-M SAU. */
	tz_sau_configure(0, 1);

#if defined(CONFIG_ARMV7_M_ARMV8_M_FP) && defined(CONFIG_SPM_NRF_FPU_NS)
	/* Allow Non-Secure firmware to use the FPU */
	tz_nonsecure_fpu_access_enable();
#endif /* CONFIG_ARMV7_M_ARMV8_M_FP */
}

void spm_jump(void)
{
	/* Extract initial MSP of the Non-Secure firmware image.
	 * The assumption is that the MSP is located at VTOR_NS[0].
	 */
	uint32_t *vtor_ns = (uint32_t *)NON_SECURE_APP_ADDRESS;

	PRINT("SPM: NS image at 0x%x\n", (uint32_t)vtor_ns);
	PRINT("SPM: NS MSP at 0x%x\n", vtor_ns[0]);
	PRINT("SPM: NS reset vector at 0x%x\n", vtor_ns[1]);

	/* Configure Non-Secure stack */
	tz_nonsecure_setup_conf_t spm_ns_conf = {
		.vtor_ns = (uint32_t)vtor_ns,
		.msp_ns = vtor_ns[0],
		.psp_ns = 0,
		.control_ns.npriv = 0, /* Privileged mode*/
		.control_ns.spsel = 0 /* Use MSP in Thread mode */
	};

	spm_configure_ns(&spm_ns_conf);

	/* Generate function pointer for Non-Secure function call. */
	TZ_NONSECURE_FUNC_PTR_DECLARE(reset_ns);
	reset_ns = TZ_NONSECURE_FUNC_PTR_CREATE(vtor_ns[1]);

	if (TZ_NONSECURE_FUNC_PTR_IS_NS(reset_ns)) {
		PRINT("SPM: prepare to jump to Non-Secure image.\n");

		/* Note: Move UARTE0 before jumping, if it is
		 * to be used on the Non-Secure domain.
		 */

		/* Configure UARTE0 as non-secure */
		uint8_t uart_id = NRFX_PERIPHERAL_ID_GET(NRF_UARTE0);

		IS_ENABLED(CONFIG_SPM_SHARE_CONSOLE_UART) ?
			spm_config_unlocked_peripheral(uart_id, 0) :
			spm_config_peripheral(uart_id, 0);

		__DSB();
		__ISB();

		/* Jump to Non-Secure firmware */
		reset_ns();

		CODE_UNREACHABLE;

	} else {
		PRINT("SPM: wrong pointer type: 0x%x\n",
		      (uint32_t)reset_ns);
	}
}

void spm_config(void)
{
	spm_config_flash();
	spm_config_sram();
	spm_config_peripherals();
}
