/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file doxygen_bot_uart.h
 * @brief UART module for Doxygen PR reviewer bot verification.
 *
 * @defgroup doxygen_bot_uart Doxygen bot UART test module
 * @{
 */

#ifndef DOXYGEN_BOT_UART_H
#define DOXYGEN_BOT_UART_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialise the UART interface.
 *
 * Function what setups the uart hardware and prepares it for transfer.
 *
 * @retval 0 on success.
 * @retval negative errno code on failure.
 */
int doxy_uart_init(void);

/**
 * @brief Deinitialise the UART interface.
 *
 * Releases uart hardware resources allocated during init
 *
 * @retval 0 on success.
 * @retval negative errno code on failure.
 */
int doxy_uart_deinit(void);

/**
 * @brief Transmit data over UART.
 *
 * @param [out] data Pointer to data buffer to transmit.
 * @param len Number of bytes to transmit.
 *
 * @retval 0 on success.
 * @retval negative errno code on failure.
 */
int doxy_uart_transmit(const uint8_t *data, size_t len);

/**
 * @brief Receive data from UART.
 *
 * @param buf Buffer to store received data.
 * @param len Maximum number of bytes to receive.
 *
 * @returs Number of bytes received, or negative errno on failure.
 */
int doxy_uart_receive(uint8_t *buf, size_t len);

/** @} */

#endif /* DOXYGEN_BOT_UART_H */
