/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DEV_INFO_RPC_IDS_H_
#define DEV_INFO_RPC_IDS_H_

/** @brief Command IDs accepted by RPC utils server.
 */
enum rpc_utils_cmd_server {
	RPC_UTIL_DEV_INFO_GET_VERSION = 0,
	RPC_UTIL_DEV_INFO_INVOKE_SHELL_CMD = 1,
	RPC_UTIL_CRASH_GEN_ASSERT = 2,
	RPC_UTIL_CRASH_GEN_HARD_FAULT = 3,
	RPC_UTIL_CRASH_GEN_STACK_OVERFLOW = 4,
};

#endif /* DEV_INFO_RPC_IDS_H_ */
