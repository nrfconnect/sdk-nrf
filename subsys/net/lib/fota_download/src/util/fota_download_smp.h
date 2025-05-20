/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FOTA_DOWNLOAD_SMP_H__
#define FOTA_DOWNLOAD_SMP_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Activate SMP update image.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_smp_update_apply(void);

#ifdef __cplusplus
}
#endif

#endif /*FOTA_DOWNLOAD_SMP_H__ */
