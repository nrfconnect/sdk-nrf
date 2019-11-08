/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef SLM_AT_TCPIP_
#define SLM_AT_TCPIP_

/**@file slm_at_tcpip.h
 *
 * @brief Vendor-specific AT command for TCPIP service.
 * @{
 */

#include <zephyr/types.h>
#include "slm_at_host.h"

/**
 * @brief TCP/IP AT command parser.
 *
 * @param at_cmd  AT command string.
 * @param param   Length of string.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_tcpip_parse(const u8_t *at_cmd, u8_t length);

/**
 * @brief Initialize TCP/IP AT command parser.
 *
 * @param callback Callback function to send AT response
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_tcpip_init(at_cmd_handler_t callback);
/** @} */

#endif /* SLM_AT_TCPIP_ */
