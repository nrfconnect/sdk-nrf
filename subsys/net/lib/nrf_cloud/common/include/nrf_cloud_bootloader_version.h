/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_BOOTLOADER_VERSION_H__
#define NRF_CLOUD_BOOTLOADER_VERSION_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Read the active MCUboot (B1) fw_info version as a decimal string.
 *
 * TF-M non-secure builds only. Uses s0_partition and s1_partition from devicetree
 * and reads the active slot through tfm_platform_s0_active() and
 * tfm_platform_firmware_info().
 *
 * @param buf Buffer for the version string.
 * @param len Size of @p buf.
 *
 * @retval 0 Success. @p buf contains the fw_info version as a decimal string.
 * @retval -EINVAL @p buf is NULL or @p len is zero.
 * @retval -ENOENT Firmware info was read but is not valid.
 * @retval -ENOMEM @p buf is too small for the formatted version string.
 */
int nrf_cloud_bootloader_version_string_get(char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_BOOTLOADER_VERSION_H__ */
