/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef SER_COMMON_H_
#define SER_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SERIALIZATION_BUFFER_SIZE 64

#define ENTROPY_GET_CMD_PARAM_CNT 1
#define ENTROPY_INIT_RSP_PARAM_CNT 1
#define ENTROPY_GET_RSP_PARAM_CNT 2

enum ser_command {
	SER_COMMAND_ENTROPY_INIT = 0x01,
	SER_COMMAND_ENTROPY_GET = 0x02
};

enum ser_evt {
	SER_EVT_ENTROPY_RECEIVED = 0x01
};

#ifdef __cplusplus
}
#endif

#endif /* SER_COMMON_H_ */
