/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_TCP_PROXY_
#define SLM_AT_TCP_PROXY_

/**@file slm_at_tcp_proxy.h
 *
 * @brief Vendor-specific AT command for TCP proxy service.
 * @{
 */

#include <zephyr/types.h>
#include <modem/at_cmd.h>

/**
 * @brief TCP proxy AT command parser.
 *
 * @param at_cmd AT command or data string.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, negative code means error.
 */
int slm_at_tcp_proxy_parse(const char *at_cmd);

/**
 * @brief List TCP proxy AT commands.
 *
 */
void slm_at_tcp_proxy_clac(void);

/**
 * @brief Initialize TCP proxy AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_tcp_proxy_init(void);

/**
 * @brief Uninitialize TCP proxy AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_tcp_proxy_uninit(void);
/** @} */

/**@brief API to set datamode off from external
 */
void slm_tcp_set_datamode_off(void);
#endif /* SLM_AT_TCP_PROXY_ */
