/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FLASH_RPC_H_
#define FLASH_RPC_H_

#ifdef __cplusplus
extern "C" {
#endif

enum flash_rpc_command {
	RPC_COMMAND_FLASH_INIT = 0x01,
	RPC_COMMAND_FLASH_READ = 0x02,
	RPC_COMMAND_FLASH_WRITE = 0x03,
	RPC_COMMAND_FLASH_ERASE = 0x04,
};

#ifdef __cplusplus
}
#endif

#endif /* FLASH_RPC_H_*/
