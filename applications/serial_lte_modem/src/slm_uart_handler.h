/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SLM_UART_HANDLER_
#define SLM_UART_HANDLER_

/**@file slm_uart_handler.h
 *
 * @brief UART handler for serial LTE modem
 * @{
 */
#define UART_RX_MARGIN_MS	10

#define HEXDUMP_LIMIT		16

extern uint32_t slm_uart_baudrate;

/**@brief UART RX data callback type.
 *
 * @param data Received data
 * @param len Length of data
 */
typedef void (*slm_uart_rx_callback_t)(const uint8_t *data, size_t len);

/**
 * @brief Switch UART power on.
 *
 * @retval 0 If the UART was successfully powered on.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_uart_power_on(void);

/**
 * @brief Switch UART power off.
 *
 * @retval 0 If the UART was successfully powered off.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_uart_power_off(void);

/** @return Whether a UART message can be sent from the current context. */
bool slm_uart_can_context_send(const uint8_t *data, size_t len);

/**
 * @brief Write the data to TX buffer and trigger sending.
 *
 * @param data Data to write.
 * @param len Length of data.
 * @param print_full_debug Print full debug traces.
 * @param indicate Can trigger the indication pin.
 *
 * @retval 0 If the data was successfully written to buffer.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_uart_tx_write(const uint8_t *data, size_t len, bool print_full_debug, bool indicate);

/**
 * @brief Initialize SLM UART handler for serial LTE modem
 *
 * @param callback_t Callback function to receive RX data.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int slm_uart_handler_init(slm_uart_rx_callback_t callback_t);

/**
 * @brief Uninitialize SLM UART handler for serial LTE modem
 *
 */
void slm_uart_handler_uninit(void);
/** @} */

#endif /* SLM_UART_HANDLER_ */
