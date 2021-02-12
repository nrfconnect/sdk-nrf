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

#include <zephyr/types.h>
#include "slm_at_host.h"

/**
 * @brief HTTPC AT command parser.
 *
 * @param at_cmd AT command or data string.
 * @param length AT command or data string length.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, negative code means error.
 *           Otherwise, positive code means data is sent in passthrough mode.
 */
int slm_at_httpc_parse(const char *at_cmd, size_t length);

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

/**
 * @brief List HTTPC AT commands.
 *
 */
void slm_at_httpc_clac(void);

/** @} */

#endif /* SLM_AT_HTTPC_ */
