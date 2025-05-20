/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include "slm_native_tls.h"
#include "slm_at_host.h"
#include "slm_at_cmng.h"

LOG_MODULE_REGISTER(slm_cmng, CONFIG_SLM_LOG_LEVEL);

/**@brief List of supported opcode */
enum slm_cmng_opcode {
	AT_CMNG_OP_WRITE = 0,
	AT_CMNG_OP_LIST = 1,
	AT_CMNG_OP_DELETE = 3
};

SLM_AT_CMD_CUSTOM(xcmng, "AT#XCMNG", handle_at_xcmng);
static int handle_at_xcmng(enum at_parser_cmd_type cmd_type, struct at_parser *parser,
			   uint32_t param_count)
{
	int err = -EINVAL;
	uint16_t op, type;
	nrf_sec_tag_t sec_tag;

	switch (cmd_type) {
	case AT_PARSER_CMD_TYPE_SET:
		if (at_parser_num_get(parser, 1, &op)) {
			return -EINVAL;
		}
		if (param_count > 2 &&
		    at_parser_num_get(parser, 2, &sec_tag)) {
			return -EINVAL;
		}
		if (param_count > 3 && (at_parser_num_get(parser, 3, &type) ||
					type > SLM_AT_CMNG_TYPE_PSK_ID)) {
			return -EINVAL;
		}
		if (op == AT_CMNG_OP_WRITE) {
			const char *at_param;
			size_t len;

			if (at_parser_string_ptr_get(parser, 4, &at_param, &len)) {
				return -EINVAL;
			}
			err = slm_native_tls_store_credential(sec_tag, type, at_param, len);
		} else if (op == AT_CMNG_OP_LIST) {
			err = slm_native_tls_list_credentials();
		} else if (op == AT_CMNG_OP_DELETE) {
			if (param_count < 4) {
				return -EINVAL;
			}
			err = slm_native_tls_delete_credential(sec_tag, type);
		}
		break;

	default:
		break;
	}

	return err;
}
