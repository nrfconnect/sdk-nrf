/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_FTP_
#define SLM_AT_FTP_

/**@file
 *
 * @brief Vendor-specific AT command for FTP and TFTP services.
 * @{
 */

/**
 * @brief Initialize FTP AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_ftp_init(void);

/**
 * @brief Initialize TFTP AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_tftp_init(void);

/**
 * @brief Uninitialize FTP AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_ftp_uninit(void);
/** @} */

#endif /* SLM_AT_FTP_ */
