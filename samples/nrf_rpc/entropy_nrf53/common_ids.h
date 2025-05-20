/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef COMMON_IDS_H_
#define COMMON_IDS_H_

#ifdef __cplusplus
extern "C" {
#endif

enum rpc_command {
	RPC_COMMAND_ENTROPY_INIT = 0x01,
	RPC_COMMAND_ENTROPY_GET = 0x02,
	RPC_COMMAND_ENTROPY_GET_CBK = 0x03,
	RPC_COMMAND_ENTROPY_GET_CBK_RESULT = 0x04,
};

enum rpc_event {
	RPC_EVENT_ENTROPY_GET_ASYNC = 0x01,
	RPC_EVENT_ENTROPY_GET_ASYNC_RESULT = 0x02,
};

#ifdef __cplusplus
}
#endif

#endif /* COMMON_IDS_H_ */
