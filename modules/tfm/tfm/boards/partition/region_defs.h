/*
 * Copyright (c) 2017-2020 Arm Limited. All rights reserved.
 * Copyright (c) 2021 Nordic Semiconductor ASA. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __REGION_DEFS_H__
#define __REGION_DEFS_H__

#include "flash_layout.h"

#define BL2_HEAP_SIZE           (0x00001000)
#define BL2_MSP_STACK_SIZE      (0x00001800)

#define S_HEAP_SIZE             (0x00001000)
#define S_MSP_STACK_SIZE_INIT   (0x00000400)
#define S_MSP_STACK_SIZE        (0x00000800)
#define S_PSP_STACK_SIZE        (0x00000800)

#define NS_HEAP_SIZE            (0x00001000)
#define NS_MSP_STACK_SIZE       (0x000000A0)
#define NS_PSP_STACK_SIZE       (0x00000140)

/* Size of nRF SPU (Nordic IDAU) regions */
#define SPU_FLASH_REGION_SIZE   (CONFIG_FPROTECT_BLOCK_SIZE)
#define SPU_SRAM_REGION_SIZE    (0x00002000)

/* This size of buffer is big enough to store an attestation
 * token produced by initial attestation service
 */
#define PSA_INITIAL_ATTEST_TOKEN_MAX_SIZE   (0x250)

#ifndef LINK_TO_SECONDARY_PARTITION
#define S_IMAGE_PRIMARY_PARTITION_OFFSET   (PM_TFM_PRIMARY_ADDRESS)
#define S_IMAGE_SECONDARY_PARTITION_OFFSET (PM_TFM_SECONDARY_ADDRESS)
#define NS_IMAGE_PRIMARY_PARTITION_OFFSET  (PM_APP_PRIMARY_ADDRESS)
#else
#define S_IMAGE_PRIMARY_PARTITION_OFFSET   (PM_TFM_SECONDARY_ADDRESS)
#define S_IMAGE_SECONDARY_PARTITION_OFFSET (PM_TFM_PRIMARY_ADDRESS)
#define NS_IMAGE_PRIMARY_PARTITION_OFFSET  (PM_APP_SECONDARY_ADDRESS)
#endif /* !LINK_TO_SECONDARY_PARTITION */

/* Boot partition structure if MCUBoot is used:
 * 0x0_0000 Bootloader header
 * 0x0_0400 Image area
 * 0x0_FC00 Trailer
 */
/* IMAGE_CODE_SIZE is the space available for the software binary image.
 * It is less than the FLASH_S_PARTITION_SIZE + FLASH_NS_PARTITION_SIZE
 * because we reserve space for the image header and trailer introduced
 * by the bootloader.
 */
#ifdef BL2
#define BL2_HEADER_SIZE      (0x400)       /* 1 KB */
#define BL2_TRAILER_SIZE     (0x400)       /* 1 KB */
#else
/* No header if no bootloader, but keep IMAGE_CODE_SIZE the same */
#define BL2_HEADER_SIZE      (0x0)
#define BL2_TRAILER_SIZE     (0x800)
#endif /* BL2 */

#define IMAGE_S_CODE_SIZE \
		(FLASH_S_PARTITION_SIZE - BL2_HEADER_SIZE - BL2_TRAILER_SIZE)
#define IMAGE_NS_CODE_SIZE \
		(FLASH_NS_PARTITION_SIZE - BL2_HEADER_SIZE - BL2_TRAILER_SIZE)

/* Secure regions */
#define S_IMAGE_PRIMARY_AREA_OFFSET \
			(S_IMAGE_PRIMARY_PARTITION_OFFSET + BL2_HEADER_SIZE)
#define S_CODE_START    (S_IMAGE_PRIMARY_AREA_OFFSET)
#define S_CODE_SIZE     (IMAGE_S_CODE_SIZE)
#define S_CODE_LIMIT    (S_CODE_START + S_CODE_SIZE - 1)

#define S_DATA_START    (PM_TFM_SRAM_ADDRESS)
#define S_DATA_SIZE     (PM_TFM_SRAM_SIZE)
#define S_DATA_LIMIT    (S_DATA_START + S_DATA_SIZE - 1)

/* The CMSE veneers shall be placed in an NSC region
 * which will be placed in a secure SPU region with the given alignment.
 */
#define CMSE_VENEER_REGION_SIZE     (0x400)
/* The Nordic IDAU has different alignment requirements than the ARM SAU, so
 * these override the default start and end alignments.
 */
#define CMSE_VENEER_REGION_START_ALIGN \
	(ALIGN(SPU_FLASH_REGION_SIZE) - CMSE_VENEER_REGION_SIZE + \
		(. > (ALIGN(SPU_FLASH_REGION_SIZE) - CMSE_VENEER_REGION_SIZE) \
			? SPU_FLASH_REGION_SIZE : 0))
#define CMSE_VENEER_REGION_END_ALIGN (ALIGN(SPU_FLASH_REGION_SIZE))
/* We want the veneers placed in the secure code so it isn't placed at the very
 * end. When placed in code, we don't need an absolute start address.
 */
#define CMSE_VENEER_REGION_IN_CODE

/* Non-secure regions */
#define NS_IMAGE_PRIMARY_AREA_OFFSET \
			(NS_IMAGE_PRIMARY_PARTITION_OFFSET + BL2_HEADER_SIZE)
#define NS_CODE_START   (NS_IMAGE_PRIMARY_AREA_OFFSET)
#define NS_CODE_SIZE    (IMAGE_NS_CODE_SIZE)
#define NS_CODE_LIMIT   (NS_CODE_START + NS_CODE_SIZE - 1)

#define NS_DATA_START   (PM_SRAM_NONSECURE_ADDRESS)
#define NS_DATA_SIZE    (PM_SRAM_NONSECURE_SIZE)
#define NS_DATA_LIMIT   (NS_DATA_START + NS_DATA_SIZE - 1)

/* NS partition information is used for SPU configuration */
#define NS_PARTITION_START \
		(NS_IMAGE_PRIMARY_PARTITION_OFFSET)
#define NS_PARTITION_SIZE (FLASH_NS_PARTITION_SIZE)

/* Secondary partition for new images in case of firmware upgrade */
#define SECONDARY_PARTITION_START \
		(S_IMAGE_SECONDARY_PARTITION_OFFSET)
#define SECONDARY_PARTITION_SIZE (FLASH_S_PARTITION_SIZE + \
				  FLASH_NS_PARTITION_SIZE)

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

#endif /* __REGION_DEFS_H__ */
