/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF71 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Nordic Semiconductor nRF71 family processor.
 */

#ifdef __NRF_TFM__
#include <zephyr/autoconf.h>
#endif

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#ifndef __NRF_TFM__
#include <zephyr/cache.h>
#endif

#if defined(NRF_APPLICATION)
#include <cmsis_core.h>
#include <hal/nrf_glitchdet.h>
#endif
#include <soc/nrfx_coredep.h>

#include <system_nrf7120_enga.h>
#include <nrf7120_enga_wificore.h>
#include <hal/nrf_spu.h>
#include <hal/nrf_mpc.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

static inline void write_reg32(uintptr_t addr, uint32_t value)
{
	*((volatile uint32_t *)addr) = value;
}

/* Copied from TF-M implementation */
struct mpc_region_override {
	nrf_mpc_override_config_t config;
	nrf_owner_t owner_id;
	uintptr_t start_address;
	uintptr_t endaddr;
	uint32_t perm;
	uint32_t permmask;
	size_t index;
};

static void mpc_configure_override(NRF_MPC_Type *mpc, struct mpc_region_override *override)
{
	nrf_mpc_override_startaddr_set(mpc, override->index, override->start_address);
	nrf_mpc_override_endaddr_set(mpc, override->index, override->endaddr);
	nrf_mpc_override_perm_set(mpc, override->index, override->perm);
	nrf_mpc_override_permmask_set(mpc, override->index, override->permmask);
#if defined(NRF_MPC_HAS_OVERRIDE_OWNERID) && NRF_MPC_HAS_OVERRIDE_OWNERID
	nrf_mpc_override_ownerid_set(mpc, override->index, override->owner_id);
#endif
	nrf_mpc_override_config_set(mpc, override->index, &override->config);
}

/*
 * Configure the override struct with reasonable defaults. This includes:
 *
 * Use a slave number of 0 to avoid redirecting bus transactions from
 * one slave to another.
 *
 * Lock the override to prevent the code that follows from tampering
 * with the configuration.
 *
 * Enable the override so it takes effect.
 *
 * Indicate that secdom is not enabled as this driver is not used on
 * platforms with secdom.
 */
static void init_mpc_region_override(struct mpc_region_override *override)
{
	*override = (struct mpc_region_override){
		.config =
			(nrf_mpc_override_config_t){
				.slave_number = 0,
				.lock = true,
				.enable = true,
				.secdom_enable = false,
				.secure_mask = false,
			},
		/* 0-NS R,W,X =1 */
		.perm = 0x7,
		.permmask = 0xF,
		.owner_id = 0,
	};
}

/**
 * Return the SPU instance that can be used to configure the
 * peripheral at the given base address.
 */
static inline NRF_SPU_Type *spu_instance_from_peripheral_addr(uint32_t peripheral_addr)
{
	/* See the SPU chapter in the IPS for how this is calculated */

	uint32_t apb_bus_number = peripheral_addr & 0x00FC0000;

	return (NRF_SPU_Type *)(0x50000000 | apb_bus_number);
}

void spu_peripheral_config_non_secure(const uint32_t periph_base_address, bool periph_lock)
{
	uint8_t periph_id = NRFX_PERIPHERAL_ID_GET(periph_base_address);

#if NRF_SPU_HAS_MEMORY
	/* ASSERT checking that this is not an explicit Secure peripheral */
	NRFX_ASSERT(
		(NRF_SPU->PERIPHID[periph_id].PERM & SPU_PERIPHID_PERM_SECUREMAPPING_Msk) !=
		(SPU_PERIPHID_PERM_SECUREMAPPING_Secure << SPU_PERIPHID_PERM_SECUREMAPPING_Pos));

	nrf_spu_peripheral_set(NRF_SPU, periph_id, 0 /* Non-Secure */, 0 /* Non-Secure DMA */,
			       periph_lock);
#else
	NRF_SPU_Type *nrf_spu = spu_instance_from_peripheral_addr(periph_base_address);

	uint8_t spu_id = NRFX_PERIPHERAL_ID_GET(nrf_spu);

	uint8_t index = periph_id - spu_id;

	nrf_spu_periph_perm_secattr_set(nrf_spu, index, false /* Non-Secure */);
	nrf_spu_periph_perm_dmasec_set(nrf_spu, index, false /* Non-Secure */);
	nrf_spu_periph_perm_lock_enable(nrf_spu, index);
#endif
}

#define MPC00_OVERRIDE_COUNT 7
static uint32_t get_mpc00_override_next_index(void)
{
	static uint32_t index;

	NRFX_ASSERT(index < MPC00_OVERRIDE_COUNT);
	return index++;
}

#ifdef CONFIG_SOC_NRF7120_HAS_AMBIX03
#define MPC03_OVERRIDE_COUNT 4
static uint32_t get_mpc03_override_next_index(void)
{
	static uint32_t index;

	NRFX_ASSERT(index < MPC03_OVERRIDE_COUNT);
	return index++;
}
#endif /* CONFIG_SOC_NRF7120_HAS_AMBIX03 */

static void wifi_setup(void)
{
	struct mpc_region_override override;

	/* Make RAM_00/01/02 (AMBIX00 + AMBIX03) accessible to the Wi-Fi domain*/
	init_mpc_region_override(&override);
	override.start_address = 0x20000000;
	override.endaddr = 0x200E0000;
	override.index = get_mpc00_override_next_index();
	mpc_configure_override(NRF_MPC00, &override);

	/* MRAM MPC overrides for wifi */
	init_mpc_region_override(&override);
	override.start_address = 0x00000000;
	override.endaddr = 0x01000000;
	override.index = get_mpc00_override_next_index();
	mpc_configure_override(NRF_MPC00, &override);

#ifdef CONFIG_SOC_NRF7120_HAS_AMBIX03
	/* Make RAM_02 (AMBIX03) accessible to the Wi-Fi domain for IPC */
	init_mpc_region_override(&override);
	override.start_address = 0x200C0000;
	override.endaddr = 0x200E0000;
	override.index = get_mpc03_override_next_index();
	mpc_configure_override(NRF_MPC03, &override);
#endif /* CONFIG_SOC_NRF7120_HAS_AMBIX03 */

	/* Make GRTC accessible from the WIFI-Core */
	spu_peripheral_config_non_secure(NRF_GRTC_S_BASE, true);

	/* EMU platform uses UART 20 for the Wi-Fi console */
#ifdef CONFIG_BOARD_NRF7120PDK_NRF7120_CPUAPP_EMU
	/* Wi-Fi VPR uses UART 20 (PORT 2 Pin 2 is for the TX) */
	spu_peripheral_config_non_secure(NRF_UARTE20_S_BASE, true);
	nrf_spu_feature_secattr_set(NRF_SPU00, NRF_SPU_FEATURE_GPIO_PIN, 2, 2,
				    SPU_FEATURE_GPIO_PIN_SECATTR_NonSecure);

	/* Set permission for TXD */
	nrf_spu_feature_secattr_set(NRF_SPU20, NRF_SPU_FEATURE_GPIO_PIN, 1, 4,
				    SPU_FEATURE_GPIO_PIN_SECATTR_NonSecure);
#endif /* CONFIG_BOARD_NRF7120PDK_NRF7120_CPUAPP_EMU */

	/* Split security configuration to let Wi-Fi access GRTC */
	nrf_spu_feature_secattr_set(NRF_SPU20, NRF_SPU_FEATURE_GRTC_CC, 15, 0, 0);
	nrf_spu_feature_secattr_set(NRF_SPU20, NRF_SPU_FEATURE_GRTC_CC, 14, 0, 0);
	nrf_spu_feature_secattr_set(NRF_SPU20, NRF_SPU_FEATURE_GRTC_INTERRUPT, 4, 0, 0);
	nrf_spu_feature_secattr_set(NRF_SPU20, NRF_SPU_FEATURE_GRTC_INTERRUPT, 5, 0, 0);
	nrf_spu_feature_secattr_set(NRF_SPU20, NRF_SPU_FEATURE_GRTC_SYSCOUNTER, 0, 0, 0);

	/* Kickstart the LMAC processor */
	NRF_WIFICORE_LRCCONF_LRC0->POWERON =
		(LRCCONF_POWERON_MAIN_AlwaysOn << LRCCONF_POWERON_MAIN_Pos);
	NRF_WIFICORE_LMAC_VPR->INITPC = NRF_WICR->RESERVED[0];
	NRF_WIFICORE_LMAC_VPR->CPURUN = (VPR_CPURUN_EN_Running << VPR_CPURUN_EN_Pos);
}

void soc_early_init_hook(void)
{
	/* Update the SystemCoreClock global variable with current core clock
	 * retrieved from hardware state.
	 */
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) || defined(__NRF_TFM__)
	/* Currently not supported for non-secure */
	SystemCoreClockUpdate();
#endif

#ifdef __NRF_TFM__
	/* TF-M enables the instruction cache from target_cfg.c, so we
	 * don't need to enable it here.
	 */
#else
	/* Enable ICACHE */
	sys_cache_instr_enable();
#endif

	wifi_setup();
}

void arch_busy_wait(uint32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}

#ifdef CONFIG_SOC_RESET_HOOK
void soc_reset_hook(void)
{
	SystemInit();
}
#endif
