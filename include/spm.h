/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 * @brief Secure Partition Manager header.
 */

#ifndef SPM_H_
#define SPM_H_

/**
 * @brief Secure Partition Manager (SPM)
 * @defgroup secure_partition_manager Secure Partition Manager
 *
 * The Secure Partition Manager (SPM) provides functions for configuring
 * the security attributes of flash, ram and periherals.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Jump to non-secure partition.
 *
 * This function extracts the VTOR_NS from
 * DT_FLASH_AREA_IMAGE_0_NONSECURE_OFFSET_0, and configures the MSP
 * accordingly before jumping to VTOR_NS[1]
 */
void spm_jump(void);


/* @brief Configure security attributes of flash, ram and peripherals.
 *
 * This function reads the security attribute options set for periherals in
 * Kconfig. The RAM and flash partitioning is configured statically.
 */
void spm_config(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* SPM_H_ */
