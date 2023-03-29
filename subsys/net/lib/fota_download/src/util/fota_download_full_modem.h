/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FOTA_DOWNLOAD_FULL_MODEM_H__
#define FOTA_DOWNLOAD_FULL_MODEM_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize and check full modem DFU target driver.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_full_modem_pre_init(void);

/** @brief Activate new full modem image.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_full_modem_apply_update(void);

/** @brief Initialize the target parameters of DFU full modem stream.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_full_modem_stream_params_init(void);

#ifdef __cplusplus
}
#endif
#endif /*FOTA_DOWNLOAD_FULL_MODEM_H__ */
