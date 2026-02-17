/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <nrf_modem_at.h>

#include "desh_native_tls.h"
#include "at_cmd_desh_custom.h"
#include "desh_at_cmng.h"

/**@brief List of supported opcode */
enum desh_cmng_opcode {
	AT_CMNG_OP_WRITE = 0,
	AT_CMNG_OP_LIST = 1,
	AT_CMNG_OP_DELETE = 3
};

DESH_AT_CMD_CUSTOM(cmng, "AT%CMNG", handle_at_cmng);
static int handle_at_cmng(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			  uint32_t param_count)
{
	int err = -EINVAL;
	uint16_t op, type;
	sec_tag_t sec_tag;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		if (at_parser_num_get(parser, 1, &op)) {
			return -EINVAL;
		}
		if (param_count > 2 && at_parser_num_get(parser, 2, &sec_tag)) {
			return -EINVAL;
		}
		if (param_count > 3 &&
		    (at_parser_num_get(parser, 3, &type) || type > DESH_AT_CMNG_TYPE_PSK_ID)) {
			return -EINVAL;
		}
		if (op == AT_CMNG_OP_WRITE) {
			const char *at_param;
			size_t len;

			if (at_parser_string_ptr_get(parser, 4, &at_param, &len)) {
				return -EINVAL;
			}
			err = desh_native_tls_store_credential(sec_tag, type, at_param, len);
		} else if (op == AT_CMNG_OP_LIST) {
			err = desh_native_tls_list_credentials();
		} else if (op == AT_CMNG_OP_DELETE) {
			if (param_count < 4) {
				return -EINVAL;
			}
			err = desh_native_tls_delete_credential(sec_tag, type);
		}
		break;

	default:
		break;
	}

	return err;
}
