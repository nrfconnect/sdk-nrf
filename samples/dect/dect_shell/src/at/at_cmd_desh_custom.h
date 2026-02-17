/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DESH_AT_CMD_CUSTOM_H
#define DESH_AT_CMD_CUSTOM_H

#include <zephyr/types.h>
#include <ctype.h>
#include <nrf_modem_at.h>
#include <modem/at_monitor.h>
#include <modem/at_cmd_custom.h>
#include <modem/at_parser.h>

/** @brief DESH AT command callback type. */
typedef int desh_at_callback(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			     uint32_t param_count);

/**
 * @brief Generic wrapper for a custom DESH AT command callback.
 *
 * This will call the AT command handler callback, which is declared with DESH_AT_CMD_CUSTOM.
 *
 * @param buf Response buffer.
 * @param len Response buffer size.
 * @param at_cmd AT command.
 * @param cb AT command callback.
 * @retval 0 on success.
 */
int at_cmd_custom_cb_wrapper(char *buf, size_t len, char *at_cmd, desh_at_callback cb);

/**
 * @brief Define a wrapper for a DESH custom AT command callback.
 *
 * Wrapper will call the generic wrapper, which will call the actual AT command handler.
 *
 * @param entry The entry name.
 * @param _filter The (partial) AT command on which the callback should trigger.
 * @param _callback The AT command handler callback.
 *
 */
#define DESH_AT_CMD_CUSTOM(entry, _filter, _callback)                                              \
	static int _callback(enum at_parser_cmd_type cmd_type, struct at_parser *parser,           \
			     uint32_t);                                                            \
	static int _callback##_wrapper_##entry(char *buf, size_t len, char *at_cmd)                \
	{                                                                                          \
		return at_cmd_custom_cb_wrapper(buf, len, at_cmd, _callback);                      \
	}                                                                                          \
	AT_CMD_CUSTOM(entry, _filter, _callback##_wrapper_##entry);

#endif /* DESH_AT_CMD_CUSTOM_H */
