/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOG_RPC_HISTORY_H_
#define LOG_RPC_HISTORY_H_

#include <zephyr/logging/log_msg.h>

/* Callback for reaching buffer usage threshold */
typedef void (*log_rpc_history_buffer_thresh_callback_t)(void);

void log_rpc_history_init(log_rpc_history_buffer_thresh_callback_t cb);

void log_rpc_history_push(const union log_msg_generic *msg);

union log_msg_generic *log_rpc_history_pop(void);
void log_rpc_history_free(const union log_msg_generic *msg);

uint8_t log_rpc_history_threshold_get(void);
void log_rpc_history_threshold_set(uint8_t threshold);

#endif /* LOG_RPC_HISTORY_H_ */
