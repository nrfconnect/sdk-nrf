/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOG_RPC_HISTORY_H_
#define LOG_RPC_HISTORY_H_

#include <zephyr/logging/log_msg.h>

void log_rpc_history_init(void);

void log_rpc_history_push(const union log_msg_generic *msg);
void log_rpc_history_set_overwriting(bool overwriting);

union log_msg_generic *log_rpc_history_pop(void);
void log_rpc_history_free(const union log_msg_generic *msg);

uint8_t log_rpc_history_get_usage(void);

size_t log_rpc_history_get_usage_size(void);

size_t log_rpc_history_get_max_size(void);

#endif /* LOG_RPC_HISTORY_H_ */
