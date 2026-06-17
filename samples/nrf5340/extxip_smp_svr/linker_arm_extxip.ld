/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2024-2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief External QSPI flash Linker command/script file
 *
 * Linker script for moving desired .text and .data to the external
 * memory space.
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

/* Let SystemInit() be called in place of z_arm_platform_init() by default. */
PROVIDE(z_arm_platform_init = SystemInit);

/*
 * Select the MCUboot slot for the QSPI XIP image based on the assigned image
 * number and whether this is a variant (secondary slot) build.
 */
#if CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER == 1
#if CONFIG_NCS_IS_VARIANT_IMAGE
#define QSPI_XIP_SLOT_NODE DT_NODELABEL(slot3_partition)
#else
#define QSPI_XIP_SLOT_NODE DT_NODELABEL(slot2_partition)
#endif
#elif CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER == 2
#if CONFIG_NCS_IS_VARIANT_IMAGE
#define QSPI_XIP_SLOT_NODE DT_NODELABEL(slot5_partition)
#else
#define QSPI_XIP_SLOT_NODE DT_NODELABEL(slot4_partition)
#endif
#else
#error "Unsupported CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER for linker_arm_extxip.ld"
#endif

#define QSPI_XIP_MM_BASE DT_REG_ADDR_BY_NAME(DT_NODELABEL(qspi), qspi_mm)

/*
 * ROM_START_OFFSET is 0 for TF-M NS builds (header lives at slot0 start only).
 * The QSPI XIP part is still signed with --pad-header, so reserve header space
 * at the start of the external slot.
 */
#if CONFIG_BUILD_WITH_TFM
#define EXTFLASH_IMAGE_OFFSET CONFIG_TFM_MCUBOOT_HEADER_SIZE
#else
#define EXTFLASH_IMAGE_OFFSET CONFIG_ROM_START_OFFSET
#endif

#define EXTFLASH_ADDR (QSPI_XIP_MM_BASE + DT_REG_ADDR(QSPI_XIP_SLOT_NODE) + EXTFLASH_IMAGE_OFFSET)
#define EXTFLASH_SIZE (DT_REG_SIZE(QSPI_XIP_SLOT_NODE) - EXTFLASH_IMAGE_OFFSET)

MEMORY
{
	EXTFLASH (wx) : ORIGIN = EXTFLASH_ADDR, LENGTH = EXTFLASH_SIZE
}

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
