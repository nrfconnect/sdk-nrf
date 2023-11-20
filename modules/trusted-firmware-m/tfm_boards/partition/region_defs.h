/*
 * Copyright (c) 2021 - 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __REGION_DEFS_H__
#define __REGION_DEFS_H__

#include "flash_layout.h"

#define BL2_HEAP_SIZE           (0x00001000)
#define BL2_MSP_STACK_SIZE      (0x00001800)

#ifdef ENABLE_HEAP
#define S_HEAP_SIZE             (0x00001000)
#endif

#define S_MSP_STACK_SIZE        (0x00000800)
#define S_PSP_STACK_SIZE        (0x00000800)

#define NS_HEAP_SIZE            (0x00001000)
#define NS_STACK_SIZE           (0x000001E0)

/* Size of nRF SPU (Nordic IDAU) regions */
#define SPU_FLASH_REGION_SIZE   (CONFIG_NRF_SPU_FLASH_REGION_SIZE)
#define SPU_SRAM_REGION_SIZE    (0x00002000)

/* This size of buffer is big enough to store an attestation
 * token produced by initial attestation service
 */
#define PSA_INITIAL_ATTEST_TOKEN_MAX_SIZE   (0x250)


#if !defined(LINK_TO_SECONDARY_PARTITION)
#ifdef NRF_NS_SECONDARY
#define S_IMAGE_PRIMARY_PARTITION_OFFSET   (PM_MCUBOOT_PRIMARY_ADDRESS)
#define S_IMAGE_SECONDARY_PARTITION_OFFSET (PM_MCUBOOT_SECONDARY_ADDRESS)
#else
#define S_IMAGE_PRIMARY_PARTITION_OFFSET   (PM_TFM_SECURE_ADDRESS)
#endif /* NRF_NS_SECONDARY */
#define NS_IMAGE_PRIMARY_PARTITION_OFFSET  (PM_TFM_NONSECURE_ADDRESS)
#else
#error "Execute from secondary partition is not supported!"
#endif /* !defined(LINK_TO_SECONDARY_PARTITION) */

/* Secure regions */
#define S_CODE_START    (PM_TFM_OFFSET)
#define S_CODE_SIZE     (PM_TFM_SIZE)
#define S_CODE_LIMIT    (S_CODE_START + S_CODE_SIZE - 1)

#define S_DATA_START    (PM_TFM_SRAM_ADDRESS)
#define S_DATA_SIZE     (PM_TFM_SRAM_SIZE)
#define S_DATA_LIMIT    (S_DATA_START + S_DATA_SIZE - 1)

#define S_CODE_VECTOR_TABLE_SIZE (CONFIG_TFM_S_CODE_VECTOR_TABLE_SIZE)

#if defined(NULL_POINTER_EXCEPTION_DETECTION) && S_CODE_START == 0
/* If this image is placed at the beginning of flash make sure we
 * don't put any code in the first 256 bytes of flash as that area
 * is used for null-pointer dereference detection.
 */
#define TFM_LINKER_CODE_START_RESERVED (256)
#if S_CODE_VECTOR_TABLE_SIZE < TFM_LINKER_CODE_START_RESERVED
#error "The interrupt table is too short for null pointer detection"
#endif
#endif

/* The veneers needs to be placed at the end of the secure image.
 * This is because the NCS sub-region is defined as starting at the highest
 * address of an SPU region and going downwards.
 */
#define TFM_LINKER_VENEERS_LOCATION_END

/* The CMSE veneers shall be placed in an NSC region
 * which will be placed in a secure SPU region with the given alignment.
 */
#define TFM_LINKER_VENEERS_SIZE     (0x400)
/* The Nordic IDAU has different alignment requirements than the ARM SAU, so
 * these override the default start and end alignments.
 */
#define TFM_LINKER_VENEERS_START \
	(ALIGN(SPU_FLASH_REGION_SIZE) - TFM_LINKER_VENEERS_SIZE + \
		(. > (ALIGN(SPU_FLASH_REGION_SIZE) - TFM_LINKER_VENEERS_SIZE) \
			? SPU_FLASH_REGION_SIZE : 0))

#define TFM_LINKER_VENEERS_END ALIGN(SPU_FLASH_REGION_SIZE)

/* Non-secure regions */
#define NS_CODE_START   (PM_APP_OFFSET)
#define NS_CODE_SIZE    (PM_APP_SIZE)
#define NS_CODE_LIMIT   (NS_CODE_START + NS_CODE_SIZE - 1)

#define NS_DATA_START   (PM_SRAM_NONSECURE_ADDRESS)
#define NS_DATA_SIZE    (PM_SRAM_NONSECURE_SIZE)
#define NS_DATA_LIMIT   (NS_DATA_START + NS_DATA_SIZE - 1)

/* NS partition information is used for SPU configuration */
#define NS_PARTITION_START (NS_IMAGE_PRIMARY_PARTITION_OFFSET)
#define NS_PARTITION_SIZE (FLASH_NS_PARTITION_SIZE)

/* Secondary partition for new images in case of firmware upgrade */
#define SECONDARY_PARTITION_START (S_IMAGE_SECONDARY_PARTITION_OFFSET)
#define SECONDARY_PARTITION_SIZE (FLASH_S_PARTITION_SIZE + \
				  FLASH_NS_PARTITION_SIZE)

/* Non-secure storage region */
#if defined(PM_NONSECURE_STORAGE_ADDRESS)
#define NRF_NS_STORAGE_PARTITION_START  (PM_NONSECURE_STORAGE_ADDRESS)
#define NRF_NS_STORAGE_PARTITION_SIZE   (PM_NONSECURE_STORAGE_SIZE)
#endif

#ifdef BL2
/* Bootloader regions */
#define BL2_CODE_START    (FLASH_AREA_BL2_OFFSET)
#define BL2_CODE_SIZE     (FLASH_AREA_BL2_SIZE)
#define BL2_CODE_LIMIT    (BL2_CODE_START + BL2_CODE_SIZE - 1)

#define BL2_DATA_START    (PM_TFM_SRAM_ADDRESS)
#define BL2_DATA_SIZE     (PM_TFM_SRAM_SIZE)
#define BL2_DATA_LIMIT    (BL2_DATA_START + BL2_DATA_SIZE - 1)
#endif /* BL2 */

/* Shared data area between bootloader and runtime firmware.
 * Shared data area is allocated at the beginning of the RAM, it is overlapping
 * with TF-M Secure code's MSP stack
 */
#define BOOT_TFM_SHARED_DATA_BASE PM_TFM_SRAM_ADDRESS
#define BOOT_TFM_SHARED_DATA_SIZE (0x400)
#define BOOT_TFM_SHARED_DATA_LIMIT (BOOT_TFM_SHARED_DATA_BASE + \
				    BOOT_TFM_SHARED_DATA_SIZE - 1)

/* Regions used by psa-arch-tests to keep state */
#define PSA_TEST_SCRATCH_AREA_SIZE (0x400)

#ifdef PSA_API_TEST_IPC

/* Firmware Framework test suites */
#define FF_TEST_PARTITION_SIZE 0x100
#define PSA_TEST_SCRATCH_AREA_BASE (NS_DATA_LIMIT + 1 - \
				    PSA_TEST_SCRATCH_AREA_SIZE - \
				    FF_TEST_PARTITION_SIZE)

/* The psa-arch-tests implementation requires that the test partitions are
 * placed in this specific order:
 * TEST_NSPE_MMIO < TEST_SERVER < TEST_DRIVER
 *
 * TEST_NSPE_MMIO region must be in the NSPE, while TEST_SERVER and TEST_DRIVER
 * must be in SPE.
 *
 * The TEST_NSPE_MMIO region is defined in the psa-arch-tests implementation,
 * and it should be placed at the end of the NSPE area, after
 * PSA_TEST_SCRATCH_AREA.
 */
#define FF_TEST_SERVER_PARTITION_MMIO_START (NS_DATA_LIMIT + 1)
#define FF_TEST_SERVER_PARTITION_MMIO_END (FF_TEST_SERVER_PARTITION_MMIO_START + \
					   FF_TEST_PARTITION_SIZE - 1)
#define FF_TEST_DRIVER_PARTITION_MMIO_START (FF_TEST_SERVER_PARTITION_MMIO_END + 1)
#define FF_TEST_DRIVER_PARTITION_MMIO_END (FF_TEST_DRIVER_PARTITION_MMIO_START + \
					   FF_TEST_PARTITION_SIZE - 1)
#else
/* Development APIs test suites */
#define PSA_TEST_SCRATCH_AREA_BASE (NS_DATA_LIMIT + 1 - \
				    PSA_TEST_SCRATCH_AREA_SIZE)

#endif /* PSA_API_TEST_IPC */

#ifdef PM_MCUBOOT_ADDRESS
#define REGION_MCUBOOT_ADDRESS PM_MCUBOOT_ADDRESS
#define REGION_MCUBOOT_END_ADDRESS PM_MCUBOOT_END_ADDRESS
#endif
#ifdef PM_B0_ADDRESS
#define REGION_B0_ADDRESS PM_B0_ADDRESS
#define REGION_B0_END_ADDRESS PM_B0_END_ADDRESS
#endif
#ifdef PM_S0_ADDRESS
#define REGION_S0_ADDRESS PM_S0_ADDRESS
#define REGION_S0_END_ADDRESS PM_S0_END_ADDRESS
#endif
#ifdef PM_S1_ADDRESS
#define REGION_S1_ADDRESS PM_S1_ADDRESS
#define REGION_S1_END_ADDRESS PM_S1_END_ADDRESS
#endif
#ifdef PM_PCD_SRAM_ADDRESS
#define REGION_PCD_SRAM_ADDRESS PM_PCD_SRAM_ADDRESS
#define REGION_PCD_SRAM_END_ADDRESS PM_PCD_SRAM_END_ADDRESS
#endif

#endif /* __REGION_DEFS_H__ */
