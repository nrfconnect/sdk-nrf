/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <debug/etb_trace.h>
#include <memfault/panics/platform/coredump.h>

#define ETB_BUFFER_VALID_MAGIC 0xDEADBEEF

static volatile uint32_t etb_buf[ETB_BUFFER_SIZE / 4];
static volatile uint32_t etb_buf_valid;

/**
 * @brief When a fault occurs, save ETB trace data to RAM
 *
 * @param regs unused
 * @param reason unused
 */
void memfault_platform_fault_handler(const sMfltRegState *regs, eMemfaultRebootReason reason)
{
	ARG_UNUSED(regs);
	ARG_UNUSED(reason);

	etb_trace_stop();
	etb_data_get((uint32_t *)etb_buf, ARRAY_SIZE(etb_buf));

	etb_buf_valid = ETB_BUFFER_VALID_MAGIC;
}
