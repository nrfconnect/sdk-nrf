/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_PPP_SETTINGS_H
#define MOSH_PPP_SETTINGS_H

int ppp_settings_init(void);

#include <zephyr/drivers/uart.h>

int ppp_uart_settings_read(struct uart_config *uart_conf);
int ppp_uart_settings_write(struct uart_config *uart_conf);

#endif /* MOSH_PPP_SETTINGS_H */
