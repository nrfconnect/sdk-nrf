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
#include <zephyr/modem/pipe.h>

#define UART_RX_MARGIN_MS	10

extern const struct device *const slm_uart_dev;
extern uint32_t slm_uart_baudrate;

/** UART pipe transmit callback type. */
typedef int (*slm_pipe_tx_t)(const uint8_t *data, size_t len);

/** @retval 0 on success. Otherwise, the error code is returned. */
int slm_uart_handler_enable(void);

/** @retval 0 on success. Otherwise, the error code is returned. */
int slm_uart_handler_disable(void);

/**
 * @brief Write data to UART or to a modem pipe.
 *
 * @retval 0 on success. Otherwise, the error code is returned.
 */
int slm_tx_write(const uint8_t *data, size_t len);

/* Initialize UART pipe for SLM. */
struct modem_pipe *slm_uart_pipe_init(slm_pipe_tx_t pipe_tx);

/** @} */

#endif /* SLM_UART_HANDLER_ */
