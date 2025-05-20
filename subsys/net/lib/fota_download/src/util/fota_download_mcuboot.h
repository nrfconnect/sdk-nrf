/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FOTA_DOWNLOAD_MCUBOOT_H__
#define FOTA_DOWNLOAD_MCUBOOT_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize MCUboot DFU target.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_mcuboot_target_init(void);

#ifdef __cplusplus
}
#endif

#endif /*FOTA_DOWNLOAD_MCUBOOT_H__ */
