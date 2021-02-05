/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_FTP_
#define SLM_AT_FTP_

/**@file slm_at_tcpip.h
 *
 * @brief Vendor-specific AT command for FTP service.
 * @{
 */

#include <zephyr/types.h>
#include <modem/at_cmd.h>

/**
 * @brief FTP AT command parser.
 *
 * @param at_cmd AT command string.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_ftp_parse(const char *at_cmd);

/**
 * @brief List FTP AT commands.
 *
 */
void slm_at_ftp_clac(void);

/**
 * @brief Initialize FTP AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_ftp_init(void);

/**
 * @brief Uninitialize FTP AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_ftp_uninit(void);
/** @} */

#endif /* SLM_AT_FTP_ */
