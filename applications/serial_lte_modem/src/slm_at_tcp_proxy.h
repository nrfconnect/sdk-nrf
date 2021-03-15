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
#endif /* SLM_AT_TCP_PROXY_ */
