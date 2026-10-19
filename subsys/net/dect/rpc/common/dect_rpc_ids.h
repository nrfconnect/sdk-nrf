/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_RPC_IDS_H_
#define DECT_RPC_IDS_H_

/**
 * @defgroup dect_rpc_ids DECT NR+ RPC command and event IDs
 * @ingroup dect_rpc
 * @{
 */

/** Command and event IDs decoded on the DECT NR+ RPC client (sent by the server). */
enum dect_rpc_cmd_client {
	/** One received IPv6 packet (server → client event). */
	DECT_RPC_CMD_IF_RECEIVE = 0,
	/** Carrier and dormant flags. */
	DECT_RPC_CMD_IF_LINK_STATE,
	/** Notify client to re-sync addresses and prefixes. */
	DECT_RPC_CMD_IF_ADDRS_CHANGED,
	/** One line of DECT NR+ shell output. */
	DECT_RPC_CMD_SHELL_LINE,
};

/** Command IDs decoded on the DECT NR+ RPC server (sent by the client). */
enum dect_rpc_cmd_server {
	/** Enable server DECT interface (server decoder only). */
	DECT_RPC_CMD_IF_ENABLE = 0,
	/** Inject one IPv6 packet into server L2 send path. */
	DECT_RPC_CMD_IF_SEND,
	/** Return addresses, prefixes, and MTU. */
	DECT_RPC_CMD_IF_GET_ADDRS,
	/** Return @ref dect_status_info. */
	DECT_RPC_CMD_IF_STATUS,
	/** Run a DECT NR+ L2 shell command on the server. */
	DECT_RPC_CMD_SHELL,
	/** Link test; server responds with 0. */
	DECT_RPC_CMD_PING,
	/** Run conn_mgr_if_connect() on the server DECT iface. */
	DECT_RPC_CMD_CONNECT,
	/** Run conn_mgr_if_disconnect() on the server DECT iface. */
	DECT_RPC_CMD_DISCONNECT,
};

/** @} */

#endif /* DECT_RPC_IDS_H_ */
