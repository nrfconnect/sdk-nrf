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

#if defined(CONFIG_LOG_BACKEND_RPC_HISTORY_STORE_ON_CRASH)
void log_rpc_history_dump_to_flash(void);
bool log_rpc_history_dump_has_stored(void);
size_t log_rpc_history_dump_get_stored_size(void);
int log_rpc_history_dump_copy(size_t offset, uint8_t *buffer, size_t size);
int log_rpc_history_dump_clear(void);

/** Format a log message to a buffer using the same format as RPC log output (client log). */
size_t log_rpc_format_msg_to_buf(struct log_msg *msg, uint8_t *buf, size_t buf_len);
#endif

#endif /* LOG_RPC_HISTORY_H_ */
