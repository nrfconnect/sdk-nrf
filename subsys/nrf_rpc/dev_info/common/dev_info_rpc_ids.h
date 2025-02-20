/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DEV_INFO_RPC_IDS_H_
#define DEV_INFO_RPC_IDS_H_

/** @brief Command IDs accepted by the device information over RPC server.
 */
enum def_info_rpc_cmd_server {
	DEV_INFO_RPC_GET_VERSION = 0,
	DEV_INFO_RPC_INVOKE_SHELL_CMD = 1,
};

#endif /* DEV_INFO_RPC_IDS_H_ */
