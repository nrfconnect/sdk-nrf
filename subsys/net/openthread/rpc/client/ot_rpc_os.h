/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_RPC_OS_H
#define OT_RPC_OS_H

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/time_units.h>

/** Returns the current time in microseconds. */
static inline uint64_t ot_rpc_os_time_us(void)
{
	return k_ticks_to_us_floor64(k_uptime_ticks());
}

/** Converts a 16-bit integer between host and network byte order. */
static inline uint16_t ot_rpc_os_htons(uint16_t value)
{
	return sys_be16_to_cpu(value);
}

/** Asserts that the condition is satisfied. */
#define OT_RPC_ASSERT(condition) __ASSERT_NO_MSG(condition)

#endif /* OT_RPC_OS_H */
