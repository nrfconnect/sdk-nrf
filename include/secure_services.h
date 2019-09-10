/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 * @brief Secure Services header.
 */

#ifndef SECURE_SERVICES_H__
#define SECURE_SERVICES_H__

/**
 * @defgroup secure_services Secure Services
 * @{
 * @brief Secure services available to the Non-Secure Firmware.
 *
 * The Secure Services provide access to functionality controlled by the
 * Secure Firmware.
 */

#include <stddef.h>
#include <zephyr/types.h>
#include <fw_info.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Request a system reboot from the Secure Firmware.
 *
 * Rebooting is not available from the Non-Secure Firmware.
 */
void spm_request_system_reboot(void);


/** Request a random number from the Secure Firmware.
 *
 * This provides a True Random Number from the on-board random number generator.
 *
 * @note Currently, the RNG hardware is run each time this is called. This
 *       spends significant time and power.
 *
 * @param[out] output  The random number. Must be at least @c len long.
 * @param[in]  len     The length of the output array. Currently, @c len must be
 *                     144.
 * @param[out] olen    The length of the random number provided.
 *
 * @retval 0        If successful.
 * @retval -EINVAL  If @c len is invalid. Currently, @c len must be 144.
 */
int spm_request_random_number(u8_t *output, size_t len, size_t *olen);


/** Request a read operation to be executed from Secure Firmware.
 *
 * @param[out] destination Pointer to destination array where the content is
 *                         to be copied.
 * @param[in]  addr        Address to be copied from.
 * @param[in]  len         Number of bytes to copy.
 *
 * @retval 0        If successful.
 * @retval -EINVAL  If destination is NULL, or if len is <= 0.
 * @retval -EPERM   If source is outside of allowed ranges.
 */
int spm_request_read(void *destination, u32_t addr, size_t len);

/** Search for the fw_info structure in firmware image located at address.
 *
 * @param[in]   fw_address  Address where firmware image is stored.
 * @param[out]  info        Pointer to where found info is stored.
 *
 * @retval 0        If successful.
 * @retval -EINVAL  If info is NULL.
 * @retval -EFAULT  If no info is found.
 */
int spm_firmware_info(u32_t fw_address, struct fw_info *info);

/** Prevalidate a B1 update
 *
 * This is performed by the B0 bootloader.
 *
 * @param[in]  dst_addr  Target location for the upgrade. This will typically
 *                       be the start address of either S0 or S1.
 * @param[in]  src_addr  Current location of the upgrade.
 *
 * @retval 1         If the upgrade is valid.
 * @retval 0         If the upgrade is invalid.
 * @retval -ENOTSUP  If the functionality is unavailable.
 */
int spm_prevalidate_b1_upgrade(u32_t dst_addr, u32_t src_addr);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* SECURE_SERVICES_H__ */
