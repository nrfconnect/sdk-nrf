/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef SLM_AT_UDP_PROXY_
#define SLM_AT_UDP_PROXY_

/**@file slm_at_udp_proxy.h
 *
 * @brief Vendor-specific AT command for UDP proxy service.
 * @{
 */

#include <zephyr/types.h>
#include <modem/at_cmd.h>

/**
 * @brief UDP proxy AT command parser.
 *
 * @param at_cmd AT command string.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_udp_proxy_parse(const char *at_cmd);

/**
 * @brief List UDP/IP AT commands.
 *
 */
void slm_at_udp_proxy_clac(void);

/**
 * @brief Initialize UDP proxy AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_udp_proxy_init(void);

/**
 * @brief Uninitialize UDP proxy AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_udp_proxy_uninit(void);
/** @} */

#endif /* SLM_AT_UDP_PROXY_ */
