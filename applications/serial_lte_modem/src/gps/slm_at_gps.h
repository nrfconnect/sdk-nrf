/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_GPS_
#define SLM_AT_GPS_

/**@file slm_at_gps.h
 *
 * @brief Vendor-specific AT command for GPS service.
 * @{
 */

#include <zephyr/types.h>
#include <modem/at_cmd.h>

/**
 * @brief GPS AT command parser.
 *
 * @param at_cmd  AT command string.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_gps_parse(const char *at_cmd);

/**
 * @brief List GPS AT commands.
 *
 */
void slm_at_gps_clac(void);

/**
 * @brief Initialize GPS AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_gps_init(void);

/**
 * @brief Uninitialize GPS AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_gps_uninit(void);

/** @} */

#endif /* SLM_AT_GPS_ */
