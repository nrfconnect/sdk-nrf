/*
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/*
 * Example code for a Secure Boot application.
 * The application uses the SPU to set the security attributions of
 * the MCU resources (Flash, SRAM and Peripherals). It uses the core
 * TrustZone-M API to prepare the MCU to jump into Non-Secure firmware
 * execution.
 *
 * The following security configuration for Flash and SRAM is applied:
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
 * 256 kB |---------------------|
 *        |                     |
 *        |     Secure          |
 *        |      Flash          |
 *  0 kB  |---------------------|
 *
 *
 *                SRAM
 * 256 kB |---------------------|
 *        |                     |
 *        |                     |
 *        |                     |
 *        |     Non-Secure      |
 *        |    SRAM (image)     |
 *        |                     |
 * 128 kB |.................... |
 *        |     Non-Secure      |
 *        |  SRAM (BSD Library) |
 *  64 kB |---------------------|
 *        |      Secure         |
 *        |       SRAM          |
 *  0 kB  |---------------------|
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <gpio.h>

#if !defined(CONFIG_ARM_SECURE_FIRMWARE)
#error "Sample requires compiling for Secure ARM Firmware"
#endif

/* Include required APIs for TrustZone-M */
#include <arm_cmse.h>
#include <cortex_m/tz.h>

#include <nrfx.h>

extern void irq_target_state_set(unsigned int irq, int secure_state);
extern int irq_target_state_is_secure(unsigned int irq);

/* printk wrapper, to turn off logs when booting silently */
#define PRINT(...)                                                             \
	do {                                                                   \
		if (!IS_ENABLED(CONFIG_SB_BOOT_SILENTLY)) {                    \
			printk(__VA_ARGS__);                                   \
		}                                                              \
	} while (0)

/* Local convenience macro to extract the peripheral
 * ID from the base address.
 */
#define NRFX_PERIPHERAL_ID_GET(base_addr) \
	(uint8_t)((uint32_t)(base_addr) >> 12)

#ifdef CONFIG_SB_BOOT_SILENTLY
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


#define FLASH_EXEC                                                             \
	((SPU_FLASHREGION_PERM_EXECUTE_Enable                                  \
	  << SPU_FLASHREGION_PERM_EXECUTE_Pos) &                               \
	 SPU_FLASHREGION_PERM_EXECUTE_Msk)

#define FLASH_WRITE                                                            \
	((SPU_FLASHREGION_PERM_WRITE_Enable                                    \
	  << SPU_FLASHREGION_PERM_WRITE_Pos) &                                 \
	 SPU_FLASHREGION_PERM_WRITE_Msk)

#define FLASH_READ                                                             \
	((SPU_FLASHREGION_PERM_READ_Enable << SPU_FLASHREGION_PERM_READ_Pos) & \
	 SPU_FLASHREGION_PERM_READ_Msk)

#define FLASH_LOCK                                                             \
	((SPU_FLASHREGION_PERM_LOCK_Locked << SPU_FLASHREGION_PERM_LOCK_Pos) & \
	 SPU_FLASHREGION_PERM_LOCK_Msk)

#define FLASH_SECURE                                                           \
	((SPU_FLASHREGION_PERM_SECATTR_Secure                                  \
	  << SPU_FLASHREGION_PERM_SECATTR_Pos) &                               \
	 SPU_FLASHREGION_PERM_SECATTR_Msk)

#define FLASH_NONSEC                                                           \
	((SPU_FLASHREGION_PERM_SECATTR_Non_Secure                              \
	  << SPU_FLASHREGION_PERM_SECATTR_Pos) &                               \
	 SPU_FLASHREGION_PERM_SECATTR_Msk)

#define SRAM_EXEC                                                              \
	((SPU_RAMREGION_PERM_EXECUTE_Enable                                    \
	  << SPU_RAMREGION_PERM_EXECUTE_Pos) &                                 \
	 SPU_RAMREGION_PERM_EXECUTE_Msk)

#define SRAM_WRITE                                                             \
	((SPU_RAMREGION_PERM_WRITE_Enable << SPU_RAMREGION_PERM_WRITE_Pos) &   \
	 SPU_RAMREGION_PERM_WRITE_Msk)

#define SRAM_READ                                                              \
	((SPU_RAMREGION_PERM_READ_Enable << SPU_RAMREGION_PERM_READ_Pos) &     \
	 SPU_RAMREGION_PERM_READ_Msk)

#define SRAM_LOCK                                                              \
	((SPU_RAMREGION_PERM_LOCK_Locked << SPU_RAMREGION_PERM_LOCK_Pos) &     \
	 SPU_RAMREGION_PERM_LOCK_Msk)

#define SRAM_SECURE                                                            \
	((SPU_RAMREGION_PERM_SECATTR_Secure                                    \
	  << SPU_RAMREGION_PERM_SECATTR_Pos) &                                 \
	 SPU_RAMREGION_PERM_SECATTR_Msk)

#define SRAM_NONSEC                                                            \
	((SPU_RAMREGION_PERM_SECATTR_Non_Secure                                \
	  << SPU_RAMREGION_PERM_SECATTR_Pos) &                                 \
	 SPU_RAMREGION_PERM_SECATTR_Msk)

#define PERIPH_PRESENT                                                         \
	((SPU_PERIPHID_PERM_PRESENT_IsPresent                                  \
	  << SPU_PERIPHID_PERM_PRESENT_Pos) &                                  \
	 SPU_PERIPHID_PERM_PRESENT_Msk)

#define PERIPH_NONSEC                                                          \
	((SPU_PERIPHID_PERM_SECATTR_NonSecure                                  \
	  << SPU_PERIPHID_PERM_SECATTR_Pos) &                                  \
	 SPU_PERIPHID_PERM_SECATTR_Msk)

#define PERIPH_DMA_NOSEP                                                       \
	((SPU_PERIPHID_PERM_DMA_NoSeparateAttribute                            \
	  << SPU_PERIPHID_PERM_DMA_Pos) &                                      \
	 SPU_PERIPHID_PERM_DMA_Msk)

#define PERIPH_LOCK                                                            \
	((SPU_PERIPHID_PERM_LOCK_Locked << SPU_PERIPHID_PERM_LOCK_Pos) &       \
	 SPU_PERIPHID_PERM_LOCK_Msk)

static void secure_boot_config_flash(void)
{
	/* Lower 256 kB of flash is allocated to MCUboot (if present)
	 * and to the Secure firmware image. The rest of flash is
	 * allocated to Non-Secure firmware image.
	 */
	static const u32_t flash_perm[] = {
		/* Configuration for Regions 0 - 7 (0 - 256 kB) */
		[0 ... 7] = FLASH_READ | FLASH_WRITE | FLASH_EXEC |
			    FLASH_LOCK | FLASH_SECURE,
		/* Configuration for Regions 8 - 31 (256 kB - 1 MB) */
		[8 ... 31] = FLASH_READ | FLASH_WRITE | FLASH_EXEC |
			     FLASH_LOCK | FLASH_NONSEC,
	};

	PRINT("Flash region\t\tDomain\t\tPermissions\n");

	/* Assign permissions */
	for (size_t i = 0; i < ARRAY_SIZE(flash_perm); i++) {

		NRF_SPU->FLASHREGION[i].PERM = flash_perm[i];

		PRINT("%02u 0x%05x 0x%05x \t", i, 8 * KB(i), 8 * KB(i + 1));
		PRINT("%s", flash_perm[i] & FLASH_SECURE ? "Secure\t\t" :
							   "Non-Secure\t");

		PRINT("%c", flash_perm[i] & FLASH_READ  ? 'r' : '-');
		PRINT("%c", flash_perm[i] & FLASH_WRITE ? 'w' : '-');
		PRINT("%c", flash_perm[i] & FLASH_EXEC  ? 'x' : '-');
		PRINT("%c", flash_perm[i] & FLASH_LOCK  ? 'l' : '-');
		PRINT("\n");
	}
	PRINT("\n");
}

static void secure_boot_config_sram(void)
{
	/* Lower 64 kB of SRAM is allocated to the Secure firmware image.
	 * The rest of SRAM is allocated to Non-Secure firmware image.
	 */
	static const u32_t sram_perm[] = {
		/* Configuration for Regions 0 - 7 (0 - 64 kB) */
		[0 ... 7] = SRAM_READ | SRAM_WRITE | SRAM_EXEC |
			    SRAM_LOCK | SRAM_SECURE,
		/* Configuration for Regions 8 - 31 (64 - 256 kB) */
		[8 ... 31] = SRAM_READ | SRAM_WRITE | SRAM_EXEC |
			     SRAM_LOCK | SRAM_NONSEC,
	};

	PRINT("SRAM region\t\tDomain\t\tPermissions\n");

	/* Assign permissions */
	for (size_t i = 0; i < ARRAY_SIZE(sram_perm); i++) {

		NRF_SPU->RAMREGION[i].PERM = sram_perm[i];

		PRINT("%02u 0x%05x 0x%05x\t", i, 4 * KB(i), 4 * KB(i + 1));
		PRINT("%s", sram_perm[i] & SRAM_SECURE ? "Secure\t\t" :
							 "Non-Secure\t");

		PRINT("%c", sram_perm[i] & SRAM_READ  ? 'r' : '-');
		PRINT("%c", sram_perm[i] & SRAM_WRITE ? 'w' : '-');
		PRINT("%c", sram_perm[i] & SRAM_EXEC  ? 'x' : '-');
		PRINT("%c", sram_perm[i] & SRAM_LOCK  ? 'l' : '-');
		PRINT("\n");
	}
	PRINT("\n");
}

static bool usel_or_split(u8_t id)
{
	const u32_t perm = NRF_SPU->PERIPHID[id].PERM;

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

static int secure_boot_config_peripheral(u8_t id, bool dma_present)
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

	if (!usel_or_split(id)) {
		return -1;
	}

	NVIC_DisableIRQ(id);
	NRF_SPU->PERIPHID[id].PERM = PERIPH_PRESENT | PERIPH_NONSEC |
				     (dma_present ? PERIPH_DMA_NOSEP : 0) |
				     PERIPH_LOCK;

	/* Even for non-present peripherals we force IRQs to be rooted
	 * to Non-Secure state.
	 */
	irq_target_state_set(id, 0);
	return 0;
}

static void secure_boot_config_peripherals(void)
{
	struct periph_cfg {
#ifndef CONFIG_SB_BOOT_SILENTLY
		char *name;
#endif
		u8_t id;
		u8_t nonsecure;
	};

	/* - All user peripherals are allocated to the Non-Secure domain.
	 * - All GPIOs are allocated to the Non-Secure domain.
	 */
	static const struct periph_cfg periph[] = {
		PERIPH("NRF_P0", NRF_P0, CONFIG_SB_NRF_P0_NS),
		PERIPH("NRF_CLOCK", NRF_CLOCK, CONFIG_SB_NRF_CLOCK_NS),
		PERIPH("NRF_RTC1", NRF_RTC1, CONFIG_SB_NRF_RTC1_NS),
		PERIPH("NRF_NVMC", NRF_NVMC, CONFIG_SB_NRF_NVMC_NS),
		PERIPH("NRF_UARTE1", NRF_UARTE1, CONFIG_SB_NRF_UARTE1_NS),
		PERIPH("NRF_UARTE2", NRF_UARTE2, CONFIG_SB_NRF_UARTE2_NS),
		/* There is no DTS node for the peripherals below,
		 * so address them using nrfx macros directly.
		 */
		PERIPH("NRF_IPC", NRF_IPC_S, CONFIG_SB_NRF_IPC_NS),
		PERIPH("NRF_VMC", NRF_VMC_S, CONFIG_SB_NRF_VMC_NS),
		PERIPH("NRF_FPU", NRF_FPU_S, CONFIG_SB_NRF_FPU_NS),
		PERIPH("NRF_EGU1", NRF_EGU1_S, CONFIG_SB_NRF_EGU1_NS),
		PERIPH("NRF_EGU2", NRF_EGU2_S, CONFIG_SB_NRF_EGU2_NS),
		PERIPH("NRF_TWIM2", NRF_TWIM2_S, CONFIG_SB_NRF_TWIM2_NS),
		PERIPH("NRF_SPIM3", NRF_SPIM3_S, CONFIG_SB_NRF_SPIM3_NS),
		PERIPH("NRF_TIMER0", NRF_TIMER0_S, CONFIG_SB_NRF_TIMER0_NS),
		PERIPH("NRF_TIMER1", NRF_TIMER1_S, CONFIG_SB_NRF_TIMER1_NS),
		PERIPH("NRF_TIMER2", NRF_TIMER2_S, CONFIG_SB_NRF_TIMER2_NS),

		PERIPH("NRF_GPIOTE1", NRF_GPIOTE1_NS,
				      CONFIG_SB_NRF_GPIOTE1_NS),
	};

	PRINT("Peripheral\t\tDomain\t\tStatus\n");

	if (IS_ENABLED(CONFIG_SB_NRF_P0_NS)) {
		/* Configure GPIO pins to be Non-Secure */
		NRF_SPU->GPIOPORT[0].PERM = 0;
	}

	for (size_t i = 0; i < ARRAY_SIZE(periph); i++) {
		int err;

#ifndef CONFIG_SB_BOOT_SILENTLY
		PRINT("%02u %s\t\t%s", i, periph[i].name,
		      periph[i].nonsecure ? "Non-Secure" : "Secure\t");
#endif

		if (!periph[i].nonsecure) {
			PRINT("\tSKIP\n");
			continue;
		}

		err = secure_boot_config_peripheral(periph[i].id, false);
		if (err) {
			PRINT("\tERROR\n");
		} else {
			PRINT("\tOK\n");
		}
	}
	PRINT("\n");
}

static void secure_boot_config(void)
{
	secure_boot_config_flash();
	secure_boot_config_sram();
	secure_boot_config_peripherals();
}

static void secure_boot_configure_ns(const tz_nonsecure_setup_conf_t
	*secure_boot_ns_conf)
{
	/* Configure core register block for Non-Secure state. */
	tz_nonsecure_state_setup(secure_boot_ns_conf);
	/* Prioritize Secure exceptions over Non-Secure */
	tz_nonsecure_exception_prio_config(1);
	/* Set non-banked exceptions to target Non-Secure */
	tz_nbanked_exception_target_state_set(0);
	/* Allow Non-Secure firmware to issue System resets. */
	tz_nonsecure_system_reset_req_block(0);
	/* Allow SPU to have precedence over (non-existing) ARMv8-M SAU. */
	tz_sau_configure(0, 1);

#if defined(CONFIG_ARMV7_M_ARMV8_M_FP)
	/* Allow Non-Secure firmware to use the FPU */
	SCB->NSACR |=
		(1UL << SCB_NSACR_CP10_Pos) | (1UL << SCB_NSACR_CP11_Pos);
#endif /* CONFIG_ARMV7_M_ARMV8_M_FP */
}

static void secure_boot_jump(void)
{
	/* Extract initial MSP of the Non-Secure firmware image.
	 * The assumption is that the MSP is located at VTOR_NS[0].
	 */
	u32_t *vtor_ns = (u32_t *)DT_FLASH_AREA_IMAGE_0_NONSECURE_OFFSET_0;

	PRINT("Secure Boot: MSP_NS %x\n", vtor_ns[0]);

	/* Configure Non-Secure stack */
	tz_nonsecure_setup_conf_t secure_boot_ns_conf = {
		.vtor_ns = (u32_t)vtor_ns,
		.msp_ns = vtor_ns[0],
		.psp_ns = 0,
		.control_ns.npriv = 0, /* Privileged mode*/
		.control_ns.spsel = 0 /* Use MSP in Thread mode */
	};

	secure_boot_configure_ns(&secure_boot_ns_conf);

	/* Generate function pointer for Non-Secure function call. */
	TZ_NONSECURE_FUNC_PTR_DECLARE(reset_ns);
	reset_ns = TZ_NONSECURE_FUNC_PTR_CREATE(vtor_ns[1]);

	if (TZ_NONSECURE_FUNC_PTR_IS_NS(reset_ns)) {
		PRINT("Secure Boot: prepare to jump to Non-Secure image\n");

		/* Note: Move UARTE0 before jumping, if it is
		 * to be used on the Non-Secure domain.
		 */

		/* Configure UARTE0 as non-secure */
		secure_boot_config_peripheral(
			NRFX_PERIPHERAL_ID_GET(NRF_UARTE0), 0);

		__DSB();
		__ISB();

		/* Jump to Non-Secure firmware */
		reset_ns();

		CODE_UNREACHABLE;

	} else {
		PRINT("Secure Boot: wrong pointer type: 0x%x\n",
		      (u32_t)reset_ns);
	}
}

void main(void)
{
	secure_boot_config();
	secure_boot_jump();
}
