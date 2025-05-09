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
	DEV_INFO_RPC_GET_VERSION = 0,
	DEV_INFO_RPC_INVOKE_SHELL_CMD = 1,
	CRASH_GEN_RPC_ASSERT = 2,
	CRASH_GEN_RPC_HARD_FAULT = 3,
	CRASH_GEN_RPC_STACK_OVERFLOW = 4,
};

#endif /* DEV_INFO_RPC_IDS_H_ */
