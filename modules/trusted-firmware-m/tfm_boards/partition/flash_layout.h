/*
 * Copyright (c) 2021 - 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __FLASH_LAYOUT_H__
#define __FLASH_LAYOUT_H__

#include <zephyr/autoconf.h>

#if defined(CONFIG_PARTITION_MANAGER_ENABLED)
#include <pm_config.h>
#else
/* This header is included from linker scripts, which can't see the C
 * constructs in <zephyr/devicetree.h>. Pull in the generated header
 * directly and provide a minimal linker-safe DT macro set below.
 * TFM_DT_ prefix avoids clashes with <zephyr/devicetree.h> when this
 * header is reached via C code.
 */
#include <zephyr/devicetree_generated.h>
#include <zephyr/sys/util_macro.h>

#define TFM_DT_CAT(a, b)              a##b
#define TFM_DT_CAT4(a, b, c, d)       a##b##c##d
#define TFM_DT_CAT6(a, b, c, d, e, f) a##b##c##d##e##f

#define TFM_DT_NODELABEL(label)     TFM_DT_CAT(DT_N_NODELABEL_, label)
#define TFM_DT_CHOSEN(name)         TFM_DT_CAT(DT_CHOSEN_, name)
#define TFM_DT_INST(inst, compat)   TFM_DT_CAT4(DT_N_INST_, inst, _, compat)
#define TFM_DT_REG_ADDR(node_id)    TFM_DT_CAT4(node_id, _REG_IDX_, 0, _VAL_ADDRESS)
#define TFM_DT_REG_SIZE(node_id)    TFM_DT_CAT4(node_id, _REG_IDX_, 0, _VAL_SIZE)
#define TFM_DT_NODE_EXISTS(node_id) TFM_DT_CAT(node_id, _EXISTS)

/* Resolve element idx of phandle-list property prop on node_id to a node id. */
#define TFM_DT_PHANDLE_BY_IDX(node_id, prop, idx)                                                  \
	TFM_DT_CAT6(node_id, _P_, prop, _IDX_, idx, _PH)

/* Iterate fn(node_id, prop, idx) over each element of a phandle-list property.
 * Pastes against the DT_N_<node>_P_<prop>_FOREACH_PROP_ELEM tokens that
 * gen_defines emits per phandle-list property.
 */
#define TFM_DT_FOREACH_PROP_ELEM(node_id, prop, fn)                                                \
	TFM_DT_CAT4(node_id, _P_, prop, _FOREACH_PROP_ELEM)(fn)

/* "Does node_id have property prop set?" Pastes to 1 when set, to an
 * undefined identifier (treated as 0 by #if and IF_ENABLED) otherwise.
 */
#define TFM_DT_PROP_EXISTS(node_id, prop) TFM_DT_CAT4(node_id, _P_, prop, _EXISTS)

/* Sum-fold helper: produces "+<size of partition referenced at idx>" for
 * one element of a phandle-list property. Plugged into TFM_DT_FOREACH_PROP_ELEM.
 */
#define TFM_DT_PROP_ELEM_REG_SIZE(node_id, prop, idx)                                              \
	+TFM_DT_REG_SIZE(TFM_DT_PHANDLE_BY_IDX(node_id, prop, idx))

/* Sum over one descriptor node's code-partitions / data-partitions.
 * data-partitions is optional; the IF_ENABLED gate expands to nothing
 * on nodes that omit it.
 */
#define TFM_DT_NODE_SUM_CODE(node_id)                                                              \
	TFM_DT_FOREACH_PROP_ELEM(node_id, code_partitions, TFM_DT_PROP_ELEM_REG_SIZE)
#define TFM_DT_NODE_SUM_DATA(node_id)                                                              \
	IF_ENABLED(TFM_DT_PROP_EXISTS(node_id, data_partitions),                                   \
		   (TFM_DT_FOREACH_PROP_ELEM(node_id, data_partitions,                              \
					     TFM_DT_PROP_ELEM_REG_SIZE)))

/* TF-M derives the TrustZone-M flash layout from two devicetree
 * descriptor nodes ("nordic,tz-secure" / "nordic,tz-nonsecure") that
 * point at the board's existing "zephyr,mapped-partition" leaves via
 * code-partitions / data-partitions phandle lists.
 */
#if !defined(DT_COMPAT_HAS_OKAY_nordic_tz_secure)
#error "TF-M cannot derive the secure flash layout: the board's NS partition dtsi is missing a 'nordic,tz-secure' descriptor."
#endif
#if !defined(DT_COMPAT_HAS_OKAY_nordic_tz_nonsecure)
#error "TF-M cannot derive the non-secure flash layout: the board's NS partition dtsi is missing a 'nordic,tz-nonsecure' descriptor."
#endif
#endif

/* This header file is included from linker scatter file as well, where only a
 * limited C constructs are allowed. Therefore it is not possible to include
 * here the platform_base_address.h to access flash related defines. To resolve
 * this some of the values are redefined here with different names, these are
 * marked with comment.
 */

/* The Kconfig symbol CONFIG_FLASH_SIZE is used to define the TF-M
 * symbol FLASH_TOTAL_SIZE.
 */
#ifndef CONFIG_FLASH_SIZE
#error "CONFIG_FLASH_SIZE not defined"
#endif

/* Size of a single Secure / Non-Secure image slot (drives SECURE_IMAGE_MAX_SIZE).
 *
 * Picks code-partitions[0] of the first okay "nordic,tz-{secure,nonsecure}"
 * descriptor. partition_check.c enforces (BUILD_ASSERT) that all
 * code-partitions entries across all descriptor instances are equal-sized,
 * so picking the first is sufficient.
 */
#if defined(CONFIG_PARTITION_MANAGER_ENABLED)
#define FLASH_S_PARTITION_SIZE	(PM_TFM_SECURE_SIZE)
#define FLASH_NS_PARTITION_SIZE (PM_TFM_NONSECURE_SIZE)
#else
#define FLASH_S_PARTITION_SIZE                                                                     \
	TFM_DT_REG_SIZE(TFM_DT_PHANDLE_BY_IDX(TFM_DT_INST(0, nordic_tz_secure), code_partitions, 0))
#define FLASH_NS_PARTITION_SIZE                                                                    \
	TFM_DT_REG_SIZE(                                                                           \
		TFM_DT_PHANDLE_BY_IDX(TFM_DT_INST(0, nordic_tz_nonsecure), code_partitions, 0))
#endif
#define FLASH_MAX_PARTITION_SIZE                                                                   \
	((FLASH_S_PARTITION_SIZE > FLASH_NS_PARTITION_SIZE) ? FLASH_S_PARTITION_SIZE               \
							    : FLASH_NS_PARTITION_SIZE)

/* Total flash footprint per security state: sum of all code-partitions and
 * data-partitions reg sizes across every okay "nordic,tz-{secure,nonsecure}"
 * descriptor. DTS-only; in PM builds the partition layout comes from
 * pm_static.yml and these macros are not emitted.
 */
#if !defined(CONFIG_PARTITION_MANAGER_ENABLED)
#if defined(DT_COMPAT_HAS_OKAY_nordic_tz_secure)
#define FLASH_SECURE_TOTAL_SIZE                                                                    \
	(0UL DT_FOREACH_OKAY_nordic_tz_secure(TFM_DT_NODE_SUM_CODE)                                \
		 DT_FOREACH_OKAY_nordic_tz_secure(TFM_DT_NODE_SUM_DATA))
#endif

#if defined(DT_COMPAT_HAS_OKAY_nordic_tz_nonsecure)
#define FLASH_NONSECURE_TOTAL_SIZE                                                                 \
	(0UL DT_FOREACH_OKAY_nordic_tz_nonsecure(TFM_DT_NODE_SUM_CODE)                             \
		 DT_FOREACH_OKAY_nordic_tz_nonsecure(TFM_DT_NODE_SUM_DATA))
#endif
#endif /* !CONFIG_PARTITION_MANAGER_ENABLED */

/* Sector size of the embedded flash hardware (erase/program).
 * Flash memory program/erase operations have a page granularity.
 */
#define FLASH_AREA_IMAGE_SECTOR_SIZE (0x1000) /* 4 kB */

/* Note that CONFIG_FLASH_SIZE has a unit of kB and
 * FLASH_TOTAL_SIZE has a unit of bytes
 */
#define FLASH_TOTAL_SIZE (CONFIG_FLASH_SIZE * 1024)

/* MCUboot partition definitions - only available when using partition manager */
#if defined(CONFIG_PARTITION_MANAGER_ENABLED)
#if !defined(MCUBOOT_IMAGE_NUMBER) || (MCUBOOT_IMAGE_NUMBER == 1)
/* Secure image primary slot */
#define FLASH_AREA_0_ID	    (1)
#define FLASH_AREA_0_OFFSET (PM_MCUBOOT_PRIMARY_ADDRESS)
#define FLASH_AREA_0_SIZE   (PM_MCUBOOT_PRIMARY_SIZE)
/* Secure image secondary slot */
#define FLASH_AREA_2_ID	    (FLASH_AREA_0_ID + 1)
#define FLASH_AREA_2_OFFSET (PM_MCUBOOT_SECONDARY_ADDRESS)
#define FLASH_AREA_2_SIZE   (PM_MCUBOOT_SECONDARY_SIZE)

/* Not used, only the Non-swapping firmware upgrade operation
 * is supported on NRF5340 Application MCU.
 */
#define FLASH_AREA_SCRATCH_ID	  (FLASH_AREA_2_ID + 1)
#define FLASH_AREA_SCRATCH_OFFSET (FLASH_AREA_2_OFFSET + FLASH_AREA_2_SIZE)
#define FLASH_AREA_SCRATCH_SIZE	  (0)

/* Maximum number of image sectors supported by the bootloader. */
#define MCUBOOT_MAX_IMG_SECTORS                                                                    \
	((FLASH_S_PARTITION_SIZE + FLASH_NS_PARTITION_SIZE) / FLASH_AREA_IMAGE_SECTOR_SIZE)
#else
#error "Only MCUBOOT_IMAGE_NUMBER 1 is supported!"
#endif
#endif /* CONFIG_PARTITION_MANAGER_ENABLED */

/* Not used, only the Non-swapping firmware upgrade operation
 * is supported on nRF5340. The maximum number of status entries
 * supported by the bootloader.
 */
#define MCUBOOT_STATUS_MAX_ENTRIES (0)

/* Protected Storage / ITS / OTP / Non-Secure storage.
 *
 * Non-PM builds resolve these by node label; the labels below are part
 * of the public contract and any dtsi wiring up these TF-M services
 * must use them. The data-partitions arrays only feed the aggregate
 * FLASH_*_TOTAL_SIZE numbers and partition_check.c's contiguity check;
 * they don't identify which entry is which service.
 */
#if defined(CONFIG_PARTITION_MANAGER_ENABLED)
#ifdef PM_TFM_PS_ADDRESS
#define FLASH_PS_AREA_OFFSET (PM_TFM_PS_ADDRESS)
#define FLASH_PS_AREA_SIZE   (PM_TFM_PS_SIZE)
#endif
#elif TFM_DT_NODE_EXISTS(TFM_DT_NODELABEL(tfm_ps_partition))
#define FLASH_PS_AREA_OFFSET TFM_DT_REG_ADDR(TFM_DT_NODELABEL(tfm_ps_partition))
#define FLASH_PS_AREA_SIZE   TFM_DT_REG_SIZE(TFM_DT_NODELABEL(tfm_ps_partition))
#endif

#if defined(CONFIG_PARTITION_MANAGER_ENABLED)
#ifdef PM_TFM_ITS_ADDRESS
#define FLASH_ITS_AREA_OFFSET (PM_TFM_ITS_ADDRESS)
#define FLASH_ITS_AREA_SIZE   (PM_TFM_ITS_SIZE)
#endif
#elif TFM_DT_NODE_EXISTS(TFM_DT_NODELABEL(tfm_its_partition))
#define FLASH_ITS_AREA_OFFSET TFM_DT_REG_ADDR(TFM_DT_NODELABEL(tfm_its_partition))
#define FLASH_ITS_AREA_SIZE   TFM_DT_REG_SIZE(TFM_DT_NODELABEL(tfm_its_partition))
#endif

#define FLASH_OTP_NV_COUNTERS_SECTOR_SIZE (FLASH_AREA_IMAGE_SECTOR_SIZE)
#if defined(CONFIG_PARTITION_MANAGER_ENABLED)
#ifdef PM_TFM_OTP_NV_COUNTERS_ADDRESS
#define FLASH_OTP_NV_COUNTERS_AREA_OFFSET (PM_TFM_OTP_NV_COUNTERS_ADDRESS)
#define FLASH_OTP_NV_COUNTERS_AREA_SIZE	  (PM_TFM_OTP_NV_COUNTERS_SIZE)
#endif
#elif TFM_DT_NODE_EXISTS(TFM_DT_NODELABEL(tfm_otp_partition))
#define FLASH_OTP_NV_COUNTERS_AREA_OFFSET TFM_DT_REG_ADDR(TFM_DT_NODELABEL(tfm_otp_partition))
#define FLASH_OTP_NV_COUNTERS_AREA_SIZE	  TFM_DT_REG_SIZE(TFM_DT_NODELABEL(tfm_otp_partition))
#endif

#if defined(CONFIG_PARTITION_MANAGER_ENABLED)
#if defined(PM_NONSECURE_STORAGE_ADDRESS)
#define NRF_FLASH_NS_STORAGE_AREA_OFFSET (PM_NONSECURE_STORAGE_ADDRESS)
#define NRF_FLASH_NS_STORAGE_AREA_SIZE	 (PM_NONSECURE_STORAGE_SIZE)
#endif
#elif TFM_DT_NODE_EXISTS(TFM_DT_NODELABEL(storage_partition))
#define NRF_FLASH_NS_STORAGE_AREA_OFFSET TFM_DT_REG_ADDR(TFM_DT_NODELABEL(storage_partition))
#define NRF_FLASH_NS_STORAGE_AREA_SIZE	 TFM_DT_REG_SIZE(TFM_DT_NODELABEL(storage_partition))
#endif

/* Non-Secure starts at the end of the entire Secure footprint (code +
 * data), not just past the code slot. When FLASH_SECURE_TOTAL_SIZE is
 * available use it directly; otherwise fall back to "just past the code
 * slot", which is only correct when there are no secure data partitions.
 */
#define SECURE_IMAGE_OFFSET   (0x0)
#define SECURE_IMAGE_MAX_SIZE FLASH_S_PARTITION_SIZE

#if defined(FLASH_SECURE_TOTAL_SIZE)
#define NON_SECURE_IMAGE_OFFSET (SECURE_IMAGE_OFFSET + FLASH_SECURE_TOTAL_SIZE)
#else
#define NON_SECURE_IMAGE_OFFSET (SECURE_IMAGE_OFFSET + SECURE_IMAGE_MAX_SIZE)
#endif
#define NON_SECURE_IMAGE_MAX_SIZE FLASH_NS_PARTITION_SIZE

/*
 * Name is defined in flash driver file: Driver_Flash.c
 */
#define FLASH_DEV_NAME Driver_FLASH0

/* Protected Storage (PS) Service definitions
 * Note: Further documentation of these definitions can be found in the
 * TF-M PS Integration Guide.
 */
#define TFM_HAL_PS_FLASH_DRIVER Driver_FLASH0

/* In this target the CMSIS driver requires only the offset from the base
 * address instead of the full memory address.
 */
#ifdef FLASH_PS_AREA_OFFSET
#define TFM_HAL_PS_FLASH_AREA_ADDR FLASH_PS_AREA_OFFSET
/* Dedicated flash area for PS */
#define TFM_HAL_PS_FLASH_AREA_SIZE FLASH_PS_AREA_SIZE
#define PS_RAM_FS_SIZE		   FLASH_AREA_IMAGE_SECTOR_SIZE
#endif /* FLASH_PS_AREA_OFFSET */

/* Number of PS_SECTOR_SIZE per block */
#define TFM_HAL_PS_SECTORS_PER_BLOCK (0x1)
/* Specifies the smallest flash programmable unit in bytes */
#define TFM_HAL_PS_PROGRAM_UNIT	     (0x4)

/* Internal Trusted Storage (ITS) Service definitions
 * Note: Further documentation of these definitions can be found in the
 * TF-M ITS Integration Guide. The ITS should be in the internal flash, but is
 * allocated in the external flash just for development platforms that don't
 * have internal flash available.
 */
#define TFM_HAL_ITS_FLASH_DRIVER Driver_FLASH0

/* In this target the CMSIS driver requires only the offset from the base
 * address instead of the full memory address.
 */
#define TFM_HAL_ITS_FLASH_AREA_ADDR   FLASH_ITS_AREA_OFFSET
/* Dedicated flash area for ITS */
#define TFM_HAL_ITS_FLASH_AREA_SIZE   FLASH_ITS_AREA_SIZE
#define ITS_RAM_FS_SIZE		      FLASH_AREA_IMAGE_SECTOR_SIZE
/* Number of ITS_SECTOR_SIZE per block */
#define TFM_HAL_ITS_SECTORS_PER_BLOCK (0x1)
/* Specifies the smallest flash programmable unit in bytes */
#define TFM_HAL_ITS_PROGRAM_UNIT      (0x4)

/* OTP / NV counter definitions */
#ifdef FLASH_OTP_NV_COUNTERS_AREA_OFFSET
#define TFM_OTP_NV_COUNTERS_AREA_SIZE	(FLASH_OTP_NV_COUNTERS_AREA_SIZE / 2)
#define TFM_OTP_NV_COUNTERS_AREA_ADDR	FLASH_OTP_NV_COUNTERS_AREA_OFFSET
#define TFM_OTP_NV_COUNTERS_SECTOR_SIZE FLASH_OTP_NV_COUNTERS_SECTOR_SIZE
#define TFM_OTP_NV_COUNTERS_BACKUP_AREA_ADDR                                                       \
	(TFM_OTP_NV_COUNTERS_AREA_ADDR + TFM_OTP_NV_COUNTERS_AREA_SIZE)
#endif

/* Use Flash memory to store Code data */
#define FLASH_BASE_ADDRESS (0x00000000)

/* Use SRAM memory to store RW data */
#define SRAM_BASE_ADDRESS (0x20000000)

#define TOTAL_ROM_SIZE FLASH_TOTAL_SIZE

#if defined(CONFIG_PARTITION_MANAGER_ENABLED)
#define TOTAL_RAM_SIZE (CONFIG_PM_SRAM_SIZE)
#else
/* When not using partition manager, get SRAM size from DTS */
#define TOTAL_RAM_SIZE                                                                             \
	(TFM_DT_REG_SIZE(TFM_DT_NODELABEL(sram0_s)) + TFM_DT_REG_SIZE(TFM_DT_NODELABEL(sram0_ns)))
#endif

#endif /* __FLASH_LAYOUT_H__ */
