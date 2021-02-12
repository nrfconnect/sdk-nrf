/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_TCPIP_
#define SLM_AT_TCPIP_

/**@file slm_at_tcpip.h
 *
 * @brief Vendor-specific AT command for TCPIP service.
 * @{
 */

#include <zephyr/types.h>
#include <modem/at_cmd.h>

/**
 * @brief TCP/IP AT command parser.
 *
 * @param at_cmd AT command string.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_tcpip_parse(const char *at_cmd);

/**
 * @brief List TCP/IP AT commands.
 *
 */
void slm_at_tcpip_clac(void);

/**
 * @brief Initialize TCP/IP AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_tcpip_init(void);

/**
 * @brief Uninitialize TCP/IP AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_tcpip_uninit(void);
/** @} */

#endif /* SLM_AT_TCPIP_ */
