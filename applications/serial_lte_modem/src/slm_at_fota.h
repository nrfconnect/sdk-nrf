/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_FOTA_
#define SLM_AT_FOTA_

/**@file slm_at_fota.h
 *
 * @brief Vendor-specific AT command for FOTA service.
 * @{
 */

#include <zephyr/types.h>
#include <modem/at_cmd.h>

/**
 * @brief FOTA AT command parser.
 *
 * @param at_cmd  AT command string.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_fota_parse(const char *at_cmd);

/**
 * @brief List FOTA AT commands.
 *
 */
void slm_at_fota_clac(void);

/**
 * @brief Initialize FOTA AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_fota_init(void);

/**
 * @brief Uninitialize FOTA AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_fota_uninit(void);

/** @} */

#endif /* SLM_AT_FOTA_ */
