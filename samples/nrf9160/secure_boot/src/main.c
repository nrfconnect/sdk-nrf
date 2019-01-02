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

#define SECURE_FLASH_REGION_FIRST    0
#define SECURE_FLASH_REGION_LAST     7
#define NONSECURE_FLASH_REGION_FIRST 8
#define NONSECURE_FLASH_REGION_LAST  31

#define SECURE_SRAM_REGION_FIRST     0
#define SECURE_SRAM_REGION_LAST      7
#define NONSECURE_SRAM_REGION_FIRST  8
#define NONSECURE_SRAM_REGION_LAST   31

/* Local convenience macro to extract the peripheral
 * ID from the base address.
 */
#define NRFX_PERIPHERAL_ID_GET(base_addr) \
	(uint8_t)((uint32_t)(base_addr) >> 12)

static void secure_boot_config_flash(void)
{
	int i;

	/* Lower 256 kB of flash is allocated to MCUboot (if present)
	 * and to the Secure firmware image. The rest of flash is
	 * allocated to Non-Secure firmware image, thus SPU attributes
	 * it as Non-Secure.
	 */
	printk("Secure Boot: configure flash\n");

	/* Configuration for Regions 0 - 7 (0 - 256 kB):
	 * Read/Write/Execute: Yes
	 * Secure: Yes
	 */
	for (i = SECURE_FLASH_REGION_FIRST;
		i <= SECURE_FLASH_REGION_LAST; i++) {
		NRF_SPU->FLASHREGION[i].PERM =
			((SPU_FLASHREGION_PERM_EXECUTE_Enable <<
					SPU_FLASHREGION_PERM_EXECUTE_Pos)
				& SPU_FLASHREGION_PERM_EXECUTE_Msk)
			|
			((SPU_FLASHREGION_PERM_WRITE_Enable <<
					SPU_FLASHREGION_PERM_WRITE_Pos)
				& SPU_FLASHREGION_PERM_WRITE_Msk)
			|
			((SPU_FLASHREGION_PERM_READ_Enable <<
					SPU_FLASHREGION_PERM_READ_Pos)
				& SPU_FLASHREGION_PERM_READ_Msk)
			|
			((SPU_FLASHREGION_PERM_SECATTR_Secure <<
					SPU_FLASHREGION_PERM_SECATTR_Pos)
				& SPU_FLASHREGION_PERM_SECATTR_Msk)
			|
			((SPU_FLASHREGION_PERM_LOCK_Locked <<
					SPU_FLASHREGION_PERM_LOCK_Pos)
				& SPU_FLASHREGION_PERM_LOCK_Msk);
		printk("Secure Boot: SPU: set region %u as Secure\n", i);
	}

	/* Configuration for Regions 8 - 31 (256 kB - 1 MB):
	 * Read/Write/Execute: Yes
	 * Secure: No
	 */
	for (i = NONSECURE_FLASH_REGION_FIRST;
		i <= NONSECURE_FLASH_REGION_LAST; i++) {
		NRF_SPU->FLASHREGION[i].PERM =
			((SPU_FLASHREGION_PERM_EXECUTE_Enable <<
					SPU_FLASHREGION_PERM_EXECUTE_Pos)
				& SPU_FLASHREGION_PERM_EXECUTE_Msk)
			|
			((SPU_FLASHREGION_PERM_WRITE_Enable <<
					SPU_FLASHREGION_PERM_WRITE_Pos)
				& SPU_FLASHREGION_PERM_WRITE_Msk)
			|
			((SPU_FLASHREGION_PERM_READ_Enable <<
					SPU_FLASHREGION_PERM_READ_Pos)
				& SPU_FLASHREGION_PERM_READ_Msk)
			|
			((SPU_FLASHREGION_PERM_SECATTR_Non_Secure <<
					SPU_FLASHREGION_PERM_SECATTR_Pos)
				& SPU_FLASHREGION_PERM_SECATTR_Msk)
			|
			((SPU_FLASHREGION_PERM_LOCK_Locked <<
					SPU_FLASHREGION_PERM_LOCK_Pos)
				& SPU_FLASHREGION_PERM_LOCK_Msk);
		printk("Secure Boot: SPU: set Flash region %u as Non-Secure\n",
			i);
	}
}

static void secure_boot_config_sram(void)
{
	int i;

	/* Lower 64 kB of SRAM is allocated to the Secure firmware image.
	 * The rest of SRAM is allocated to Non-Secure firmware image, thus
	 * SPU attributes it as Non-Secure.
	 */
	printk("Secure Boot: configure SRAM\n");

	/* Configuration for Regions 0 - 7 (0 - 64 kB):
	 * Read/Write/Execute: Yes
	 * Secure: Yes
	 */
	for (i = SECURE_SRAM_REGION_FIRST;
		i <= SECURE_SRAM_REGION_LAST; i++) {
		NRF_SPU->RAMREGION[i].PERM =
			((SPU_RAMREGION_PERM_EXECUTE_Enable <<
					SPU_RAMREGION_PERM_EXECUTE_Pos)
				& SPU_RAMREGION_PERM_EXECUTE_Msk)
			|
			((SPU_RAMREGION_PERM_WRITE_Enable <<
					SPU_RAMREGION_PERM_WRITE_Pos)
				& SPU_RAMREGION_PERM_WRITE_Msk)
			|
			((SPU_RAMREGION_PERM_READ_Enable <<
					SPU_RAMREGION_PERM_READ_Pos)
				& SPU_RAMREGION_PERM_READ_Msk)
			|
			((SPU_RAMREGION_PERM_SECATTR_Secure <<
					SPU_RAMREGION_PERM_SECATTR_Pos)
				& SPU_RAMREGION_PERM_SECATTR_Msk)
			|
			((SPU_RAMREGION_PERM_LOCK_Locked <<
					SPU_RAMREGION_PERM_LOCK_Pos)
				& SPU_RAMREGION_PERM_LOCK_Msk);

		printk("Secure Boot: SPU: set SRAM region %u as Secure\n", i);
	}

	/* Configuration for Regions 8 - 31 (64 kB - 256 kB):
	 * Read/Write/Execute: Yes
	 * Secure: No
	 */
	for (i = NONSECURE_SRAM_REGION_FIRST;
		i <= NONSECURE_SRAM_REGION_LAST; i++) {
		NRF_SPU->RAMREGION[i].PERM =
			((SPU_RAMREGION_PERM_EXECUTE_Enable <<
					SPU_RAMREGION_PERM_EXECUTE_Pos)
				& SPU_RAMREGION_PERM_EXECUTE_Msk)
			|
			((SPU_RAMREGION_PERM_WRITE_Enable <<
					SPU_RAMREGION_PERM_WRITE_Pos)
				& SPU_RAMREGION_PERM_WRITE_Msk)
			|
			((SPU_RAMREGION_PERM_READ_Enable <<
					SPU_RAMREGION_PERM_READ_Pos)
				& SPU_RAMREGION_PERM_READ_Msk)
			|
			((SPU_RAMREGION_PERM_SECATTR_Non_Secure <<
					SPU_RAMREGION_PERM_SECATTR_Pos)
				& SPU_RAMREGION_PERM_SECATTR_Msk)
			|
			((SPU_RAMREGION_PERM_LOCK_Locked <<
					SPU_RAMREGION_PERM_LOCK_Pos)
				& SPU_RAMREGION_PERM_LOCK_Msk);

		printk("Secure Boot: SPU: set SRAM region %u as Non-Secure\n",
			i);
	}
}

static void secure_boot_config_peripheral(u8_t id, u32_t dma_present)
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
	if (((NRF_SPU->PERIPHID[id].PERM & SPU_PERIPHID_PERM_PRESENT_Msk)
			== SPU_PERIPHID_PERM_PRESENT_Msk)
		&&
		(((NRF_SPU->PERIPHID[id].PERM &
				SPU_PERIPHID_PERM_SECUREMAPPING_Msk)
			== SPU_PERIPHID_PERM_SECUREMAPPING_UserSelectable)
		||
		((NRF_SPU->PERIPHID[id].PERM &
				SPU_PERIPHID_PERM_SECUREMAPPING_Msk)
			== SPU_PERIPHID_PERM_SECUREMAPPING_Split)))	{
		NVIC_DisableIRQ(id);

		NRF_SPU->PERIPHID[id].PERM =
			((SPU_PERIPHID_PERM_PRESENT_IsPresent <<
					SPU_PERIPHID_PERM_PRESENT_Pos) &
				SPU_PERIPHID_PERM_PRESENT_Msk)
			|
			((SPU_PERIPHID_PERM_SECATTR_NonSecure <<
					SPU_PERIPHID_PERM_SECATTR_Pos) &
				SPU_PERIPHID_PERM_SECATTR_Msk)
			|
			(dma_present ?
				((SPU_PERIPHID_PERM_DMA_NoSeparateAttribute <<
						SPU_PERIPHID_PERM_DMA_Pos) &
					SPU_PERIPHID_PERM_DMA_Msk) : 0)
			|
			((SPU_PERIPHID_PERM_LOCK_Locked <<
					SPU_PERIPHID_PERM_LOCK_Pos) &
				SPU_PERIPHID_PERM_LOCK_Msk);

	}

	/* Even for non-present peripherals we force IRQs to be rooted
	 * to Non-Secure state.
	 */
	irq_target_state_set(id, 0);
}

static void secure_boot_config_peripherals(void)
{
	/* - All user peripherals are allocated to
	 *   the Non-Secure domain.
	 * - All GPIOs are allocated to the Non-Secure domain.
	 */
	printk("Secure Boot: configure peripherals\n");

	/* Configure GPIO pins to be non-Secure */
	NRF_SPU->GPIOPORT[0].PERM = 0;

	/* Configure Clock Control as Non-Secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_CLOCK), 0);
	/* Configure RTC1 as Non-Secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_RTC1), 0);
	/* Configure IPC as Non-Secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_IPC_S), 0);
	/* Configure NVMC as Non-Secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_NVMC), 0);
	/* Configure VMC as Non-Secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_VMC_S), 0);
	/* Configure GPIO as Non-Secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_GPIO), 0);
	/* Make GPIOTE1 interrupt available in Non-Secure domain */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_GPIOTE1_NS), 0);
	/* Configure UARTE1 as Non-Secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_UARTE1), 0);
	/* Configure UARTE2 as Non-Secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_UARTE2), 0);
	/* Configure EGU1 as Non-Secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_EGU1_S), 0);
	/* Configure EGU2 as Non-Secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_EGU2_S), 0);
	/* Configure FPU as non-secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_FPU_S), 0);
	/* Configure TWIM2 as non-secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_TWIM2_S), 0);
	/* Configure SPIM3 as non-secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_SPIM3_S), 0);
	/* Configure TIMER0 as non-secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_TIMER0_S), 0);
	/* Configure TIMER1 as non-secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_TIMER1_S), 0);
	/* Configure TIMER2 as non-secure */
	secure_boot_config_peripheral(
		NRFX_PERIPHERAL_ID_GET(NRF_TIMER2_S), 0);
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
	u32_t *vtor_ns = (u32_t *)FLASH_AREA_IMAGE_0_NONSECURE_OFFSET_0;

	printk("Secure Boot: MSP_NS %x\n", vtor_ns[0]);

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
		printk("Secure Boot: prepare to jump to Non-Secure image\n");

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
		printk("Secure Boot: wrong pointer type: 0x%x\n",
			(u32_t)reset_ns);
	}
}

void main(void)
{
	secure_boot_config();
	secure_boot_jump();
}
