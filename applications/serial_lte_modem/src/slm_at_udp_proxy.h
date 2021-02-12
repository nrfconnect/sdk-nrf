/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
 * @param at_cmd AT command or data string.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, negative error code means error.
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

/**@brief API to get datamode from external
 */
bool slm_udp_get_datamode(void);

/**@brief API to set datamode off from external
 */
void slm_udp_set_datamode_off(void);

/**@brief API to send UDP data in datamode
 *
 * @param data Raw data string to send.
 * @param len  Length of the raw data.
 *
 * @retval positive code means data is sent in data mode.
 *           Otherwise, negative code means error.
 */
int slm_udp_send_datamode(const uint8_t *data, int len);

#endif /* SLM_AT_UDP_PROXY_ */
