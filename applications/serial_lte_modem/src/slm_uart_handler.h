/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_UART_HANDLER_
#define SLM_UART_HANDLER_

/** @file slm_uart_handler.h
 *
 * @brief pure UART handler for serial LTE modem
 * @{
 */
#include "slm_trap_macros.h"
#include <zephyr/device.h>

#define UART_RX_MARGIN_MS	10

extern const struct device *const slm_uart_dev;
extern uint32_t slm_uart_baudrate;

/** @retval 0 on success. Otherwise, the error code is returned. */
int slm_uart_handler_enable(void);

/** @} */

#endif /* SLM_UART_HANDLER_ */
