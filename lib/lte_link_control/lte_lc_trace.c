/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <modem/lte_lc_trace.h>

static lte_lc_trace_handler_t cb;

void lte_lc_trace_handler_set(lte_lc_trace_handler_t handler)
{
	if (handler) {
		cb = handler;
	}
}

void lte_lc_trace_capture(enum lte_lc_trace_type type)
{
	if (cb) {
		cb(type);
	}
}
