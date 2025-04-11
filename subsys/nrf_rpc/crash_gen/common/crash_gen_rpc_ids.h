/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRASH_GEN_RPC_IDS_H_
#define CRASH_GEN_RPC_IDS_H_

/** @brief Command IDs accepted by the crash generator over RPC server.
 */
enum crash_gen_rpc_cmd_server {
	CRASH_GEN_RPC_ASSERT = 0,
	CRASH_GEN_RPC_HARD_FAULT = 1,
	CRASH_GEN_RPC_STACK_OVERFLOW = 2,
};

#endif /* CRASH_GEN_RPC_IDS_H_ */
