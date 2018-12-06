/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BOOTLOADER_UART_H__
#define BOOTLOADER_UART_H__

#include <stddef.h>

/**
 * @brief Initialize UART module.
 */
void uart_init(void);

/**
 * @brief Uninitialize UART module.
 */
void uart_uninit(void);

/**
 * @brief Print formatted string to UART.
 *
 * @param[in]  __fmt Format string.
 * @param[in]  argptr Argument pointer.
 */
void uart_vprintf(const char *__fmt, va_list argptr);

/**
 * @brief Print formatted string to UART.
 *
 * @param[in]  __fmt Format string.
 */
void uart_printf(const char *__fmt, ...);

/**
 * @brief Print string to UART.
 *
 * @param[in]  str String to print.
 * @param[in]  len Length of string
 */
void uart_print(const char *str, size_t len);

#endif
