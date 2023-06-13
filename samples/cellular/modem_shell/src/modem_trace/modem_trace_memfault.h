/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_MODEM_TRACE_MEMFAULT_H
#define MOSH_MODEM_TRACE_MEMFAULT_H

#include <stddef.h>
#include <stdint.h>

void modem_trace_memfault_send(uint32_t trace_duration_ms);

#endif /* MOSH_MODEM_TRACE_MEMFAULT_H */
