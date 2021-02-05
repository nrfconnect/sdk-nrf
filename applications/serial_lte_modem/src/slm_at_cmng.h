/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_CMNG_
#define SLM_AT_CMNG_

/**@file slm_at_cmng.h
 *
 * @brief Vendor-specific AT command for CMNG service.
 * @{
 */

#include <zephyr/types.h>
#include <modem/at_cmd.h>

/**
 * @brief CMNG AT command parser.
 *
 * @param at_cmd AT command string.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_cmng_parse(const char *at_cmd);

/**
 * @brief List CMNG AT commands.
 *
 */
void slm_at_cmng_clac(void);

/**
 * @brief Initialize CMNG AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_cmng_init(void);

/**
 * @brief Uninitialize CMNG AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_cmng_uninit(void);
/** @} */

#endif /* SLM_AT_CMNG_ */
