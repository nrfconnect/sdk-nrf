/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FOTA_DOWNLOAD_DELTA_MODEM_H__
#define FOTA_DOWNLOAD_DELTA_MODEM_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Activate delta modem update image.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_apply_delta_modem_update(void);

#ifdef __cplusplus
}
#endif

#endif /*FOTA_DOWNLOAD_DELTA_MODEM_H__ */
