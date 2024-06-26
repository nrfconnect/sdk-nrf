/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @brief Command IDs accepted by the OpenThread over RPC client.
 */
enum ot_rpc_cmd_client {
	OT_RPC_CMD_CLI_OUTPUT = 0,
	OT_RPC_CMD_STATE_CHANGED,
	OT_RPC_CMD_IF_RECEIVE,
};

/** @brief Command IDs accepted by the OpenThread over RPC server.
 */
enum ot_rpc_cmd_server {
	OT_RPC_CMD_CLI_INIT = 0,
	OT_RPC_CMD_CLI_INPUT_LINE,
	OT_RPC_CMD_IP6_GET_UNICAST_ADDRESSES,
	OT_RPC_CMD_SET_STATE_CHANGED_CALLBACK,
	OT_RPC_CMD_REMOVE_STATE_CHANGED_CALLBACK,
	OT_RPC_CMD_IF_ENABLE,
	OT_RPC_CMD_IF_SEND,
	OT_RPC_CMD_IF_EXTADDR,
};
