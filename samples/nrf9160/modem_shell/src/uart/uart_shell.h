/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_UART_SHELL_H
#define MOSH_UART_SHELL_H

void uart_toggle_power_state_at_event(const struct shell *shell,
				       const struct lte_lc_evt *const evt);

#endif /* MOSH_UART_SHELL_H */
