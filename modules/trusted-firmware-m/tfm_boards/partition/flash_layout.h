/*
 * Copyright (c) 2021 - 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __FLASH_LAYOUT_H__
#define __FLASH_LAYOUT_H__

#include <zephyr/autoconf.h>

/*
 * When not using Partition Manager, get partition info from devicetree.
 * We can't use <zephyr/devicetree.h> directly because this header is included
 * from linker scripts, and devicetree.h includes C headers that are not valid
 * in linker script context. Instead, we include only the generated devicetree
 * defines and provide minimal DT macros for extracting partition information.
 */
#include <zephyr/devicetree_generated.h>

/*
 * Minimal devicetree macros for linker script compatibility.
 * These use TFM_DT_ prefix to avoid conflicts with <zephyr/devicetree.h>
 * which may be included via other headers in C code context.
 */
#define TFM_DT_CAT(a, b) a ## b
#define TFM_DT_CAT4(a, b, c, d) a ## b ## c ## d
#define TFM_DT_NODELABEL(label) TFM_DT_CAT(DT_N_NODELABEL_, label)
#define TFM_DT_REG_ADDR(node_id) TFM_DT_CAT4(node_id, _REG_IDX_, 0, _VAL_ADDRESS)
#define TFM_DT_REG_SIZE(node_id) TFM_DT_CAT4(node_id, _REG_IDX_, 0, _VAL_SIZE)
#define TFM_DT_NODE_EXISTS(node_id) TFM_DT_CAT(node_id, _EXISTS)

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

/* Size of a Secure and of a Non-secure image */
#if TFM_DT_NODE_EXISTS(TFM_DT_NODELABEL(slot0_s_partition))
/* When slot0_partition is the combined MCUboot slot, use slot0_s_partition
 * for the size of the secure-only sub-partition.
 */
#define FLASH_S_PARTITION_SIZE	TFM_DT_REG_SIZE(TFM_DT_NODELABEL(slot0_s_partition))
#else
#define FLASH_S_PARTITION_SIZE	TFM_DT_REG_SIZE(TFM_DT_NODELABEL(slot0_partition))
#endif
#define FLASH_NS_PARTITION_SIZE TFM_DT_REG_SIZE(TFM_DT_NODELABEL(slot0_ns_partition))
#define FLASH_MAX_PARTITION_SIZE                                                                   \
	((FLASH_S_PARTITION_SIZE > FLASH_NS_PARTITION_SIZE) ? FLASH_S_PARTITION_SIZE               \
							    : FLASH_NS_PARTITION_SIZE)

/* Sector size of the embedded flash hardware (erase/program).
 * Flash memory program/erase operations have a page granularity.
 */
#define FLASH_AREA_IMAGE_SECTOR_SIZE (0x1000) /* 4 kB */

/* Note that CONFIG_FLASH_SIZE has a unit of kB and
 * FLASH_TOTAL_SIZE has a unit of bytes
 */
#define FLASH_TOTAL_SIZE (CONFIG_FLASH_SIZE * 1024)

/* Not used, only the Non-swapping firmware upgrade operation
 * is supported on nRF5340. The maximum number of status entries
 * supported by the bootloader.
 */
#define MCUBOOT_STATUS_MAX_ENTRIES (0)

/* Protected Storage (PS) partition */
#if TFM_DT_NODE_EXISTS(TFM_DT_NODELABEL(tfm_ps_partition))
#define FLASH_PS_AREA_OFFSET TFM_DT_REG_ADDR(TFM_DT_NODELABEL(tfm_ps_partition))
#define FLASH_PS_AREA_SIZE   TFM_DT_REG_SIZE(TFM_DT_NODELABEL(tfm_ps_partition))
#endif

/* Internal Trusted Storage (ITS) Service definitions */
#if TFM_DT_NODE_EXISTS(TFM_DT_NODELABEL(tfm_its_partition))
#define FLASH_ITS_AREA_OFFSET TFM_DT_REG_ADDR(TFM_DT_NODELABEL(tfm_its_partition))
#define FLASH_ITS_AREA_SIZE   TFM_DT_REG_SIZE(TFM_DT_NODELABEL(tfm_its_partition))
#endif

/* OTP / NV counters partition */
#define FLASH_OTP_NV_COUNTERS_SECTOR_SIZE (FLASH_AREA_IMAGE_SECTOR_SIZE)
#if TFM_DT_NODE_EXISTS(TFM_DT_NODELABEL(tfm_otp_partition))
#define FLASH_OTP_NV_COUNTERS_AREA_OFFSET TFM_DT_REG_ADDR(TFM_DT_NODELABEL(tfm_otp_partition))
#define FLASH_OTP_NV_COUNTERS_AREA_SIZE	  TFM_DT_REG_SIZE(TFM_DT_NODELABEL(tfm_otp_partition))
#endif

/* Non-secure storage region */
#if TFM_DT_NODE_EXISTS(TFM_DT_NODELABEL(storage_partition))
#define NRF_FLASH_NS_STORAGE_AREA_OFFSET TFM_DT_REG_ADDR(TFM_DT_NODELABEL(storage_partition))
#define NRF_FLASH_NS_STORAGE_AREA_SIZE	 TFM_DT_REG_SIZE(TFM_DT_NODELABEL(storage_partition))
#endif

/* Offset and size definition in flash area used by assemble.py */
#define SECURE_IMAGE_OFFSET   (0x0)
#define SECURE_IMAGE_MAX_SIZE FLASH_S_PARTITION_SIZE

#define NON_SECURE_IMAGE_OFFSET	  (SECURE_IMAGE_OFFSET + SECURE_IMAGE_MAX_SIZE)
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

/* When not using partition manager, get SRAM size from DTS */
#define TOTAL_RAM_SIZE                                                                             \
	(TFM_DT_REG_SIZE(TFM_DT_NODELABEL(sram0_s)) + TFM_DT_REG_SIZE(TFM_DT_NODELABEL(sram0_ns)))

#endif /* __FLASH_LAYOUT_H__ */
