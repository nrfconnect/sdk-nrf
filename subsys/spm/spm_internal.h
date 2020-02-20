/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef SPM_INTERNAL_H__
#define SPM_INTERNAL_H__

#include <zephyr.h>
#include <nrfx.h>
#include <sys/util.h>
#include <sys/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Total RAM Size */
#ifdef CONFIG_SOC_NRF9160
#define TOTAL_RAM_SIZE (256*1024)
#elif defined(CONFIG_SOC_NRF5340_CPUAPP)
#define TOTAL_RAM_SIZE (512*1024)
#endif

/* SPU RAM regions */
#define RAM_SECURE_ATTRIBUTION_REGION_SIZE CONFIG_NRF_SPU_RAM_REGION_SIZE
#define NUM_RAM_SECURE_ATTRIBUTION_REGIONS (TOTAL_RAM_SIZE \
				/ RAM_SECURE_ATTRIBUTION_REGION_SIZE)

/* SPU FLASH regions */
#if (defined(CONFIG_SOC_NRF5340_CPUAPP) \
	&& defined(CONFIG_NRF5340_CPUAPP_ERRATUM19))
static inline u32_t spu_flash_region_size(void)
{
	if (NRF_FICR->INFO.PART == 0x5340) {
		if (NRF_FICR->INFO.VARIANT == 0x41414142) {
			return 32*1024;
		}
		return 16*1024;
	}
	__ASSERT(false, "Function should only be called on an nRF53 device.");
	return 0;
}

static inline u32_t num_spu_flash_regions(void)
{
	if (NRF_FICR->INFO.PART == 0x5340) {
		if (NRF_FICR->INFO.VARIANT == 0x41414142) {
			return 32;
		}
		return 64;
	}
	__ASSERT(false, "Function should only be called on an nRF53 device.");
	return 0;
}
#define FLASH_SECURE_ATTRIBUTION_REGION_SIZE spu_flash_region_size()
#define NUM_FLASH_SECURE_ATTRIBUTION_REGIONS num_spu_flash_regions()
#else

#define FLASH_SECURE_ATTRIBUTION_REGION_SIZE CONFIG_NRF_SPU_FLASH_REGION_SIZE
#define NUM_FLASH_SECURE_ATTRIBUTION_REGIONS (DT_SOC_NV_FLASH_0_SIZE \
				/ FLASH_SECURE_ATTRIBUTION_REGION_SIZE)
#endif

/* Minimum size of Non-Secure Callable regions. */
#define FLASH_NSC_MIN_SIZE 32

/* Maximum size of Non-Secure Callable regions. */
#define FLASH_NSC_MAX_SIZE 4096

/* Local convenience macros for interfacing with the SPU. */

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

#define FLASH_NSC_REGION(reg_number)                                           \
	(((reg_number) << SPU_FLASHNSC_REGION_REGION_Pos) &                    \
	SPU_FLASHNSC_REGION_REGION_Msk)

#define FLASH_NSC_REGION_FROM_ADDR(addr)                                       \
	FLASH_NSC_REGION(((u32_t)addr / FLASH_SECURE_ATTRIBUTION_REGION_SIZE))

#define FLASH_NSC_REGION_LOCK                                                  \
	((SPU_FLASHNSC_REGION_LOCK_Locked << SPU_FLASHNSC_REGION_LOCK_Pos) &   \
	SPU_FLASHNSC_REGION_LOCK_Msk)

#define FLASH_NSC_SIZE(reg_size)                                               \
	(((reg_size) << SPU_FLASHNSC_SIZE_SIZE_Pos) &                          \
	SPU_FLASHNSC_SIZE_SIZE_Msk)

#define FLASH_NSC_SIZE_LOCK                                                    \
	((SPU_FLASHNSC_SIZE_LOCK_Locked << SPU_FLASHNSC_SIZE_LOCK_Pos) &       \
	SPU_FLASHNSC_SIZE_LOCK_Msk)

#define FLASH_NSC_SIZE_FROM_ADDR(addr) FLASH_SECURE_ATTRIBUTION_REGION_SIZE    \
	- (((u32_t)(addr)) % FLASH_SECURE_ATTRIBUTION_REGION_SIZE)

#define FLASH_NSC_SIZE_REG(size) FLASH_NSC_SIZE((size) / FLASH_NSC_MIN_SIZE)


/** Initialze secure services.
 *
 * @return 0 if successful, negative error otherwise.
 */
int spm_secure_services_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SPM_INTERNAL_H__ */
