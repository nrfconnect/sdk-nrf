/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SHELL_MODEM_TRACE_MEMFAULT_H
#define SHELL_MODEM_TRACE_MEMFAULT_H

#include <stddef.h>
#include <stdint.h>
#include <zephyr/shell/shell.h>

void modem_trace_memfault_send(const struct shell *sh, size_t size, uint32_t trace_duration_ms);

#endif /* SHELL_MODEM_TRACE_MEMFAULT_H */
