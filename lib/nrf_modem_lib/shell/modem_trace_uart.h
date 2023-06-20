/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SHELL_MODEM_TRACE_UART_H
#define SHELL_MODEM_TRACE_UART_H

#include <stddef.h>
#include <stdint.h>
#include <zephyr/shell/shell.h>

void modem_trace_uart_send(const struct shell *sh, size_t size);

#endif /* SHELL_MODEM_TRACE_UART_H */
