/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_UART_SHELL_H
#define MOSH_UART_SHELL_H

void uart_toggle_power_state_at_event(const struct lte_lc_evt *const evt);

/**
 * @brief Toggles UART0 and UART1 power states.
 *
 * @details Checks the current power state of UART0. If UART0 is currently in
 * PM_DEVICE_STATE_ACTIVE, both UART0 and UART1 are suspended. Otherwise, both
 * UARTs are resumed.
 *
 * @param[in] shell Pointer to shell instance.
 */
void uart_toggle_power_state(void);

#endif /* MOSH_UART_SHELL_H */
