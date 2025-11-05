/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RPC_UTILS_GROUP_H_
#define RPC_UTILS_GROUP_H_

#include <nrf_rpc.h>

NRF_RPC_GROUP_DECLARE(rpc_utils_group);

/** @brief Command IDs accepted by RPC utils server.
 */
enum rpc_utils_cmd_server {
	RPC_UTIL_DEV_INFO_GET_VERSION = 0,
	RPC_UTIL_DEV_INFO_INVOKE_SHELL_CMD = 1,
	RPC_UTIL_CRASH_GEN_ASSERT = 2,
	RPC_UTIL_CRASH_GEN_HARD_FAULT = 3,
	RPC_UTIL_CRASH_GEN_STACK_OVERFLOW = 4,
	RPC_UTIL_SYSTEM_HEALTH_GET = 5,
	RPC_UTIL_HEALTH_MONITOR_DATA = 6,
	RPC_UTIL_HEALTH_MONITOR_RAPORT_INTERVAL = 7,
	RPC_UTIL_HEALTH_MONITOR_CALLBACK = 8,
	RPC_UTIL_DIE_TEMP_GET = 9,
};

#endif /* RPC_UTILS_GROUP_H_ */
