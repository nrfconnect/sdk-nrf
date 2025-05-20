/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_AT_HTTPC_
#define SLM_AT_HTTPC_

/**@file slm_at_httpc.h
 *
 * @brief Vendor-specific AT command for HTTP service.
 * @{
 */

/**
 * @brief HTTPC raw data send.
 *
 * @param data Raw data string.
 * @param length Raw data string length.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, negative code means error.
 *           Otherwise, positive code means data is sent in passthrough mode.
 */
int handle_at_httpc_send(const char *data, size_t length);

/**
 * @brief Initialize HTTPC AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_httpc_init(void);

/**
 * @brief Uninitialize HTTPC AT command parser.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_at_httpc_uninit(void);

/** @} */

#endif /* SLM_AT_HTTPC_ */
