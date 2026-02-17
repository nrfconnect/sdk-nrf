/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <sys/types.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/shell/shell.h>
#include <modem/at_parser.h>
#include <nrf_modem_at.h>

#include "at_cmd_desh_custom.h"

#include "desh_defines.h"
#include "desh_print.h"

int at_cmd_custom_cb_wrapper(char *buf, size_t len, char *at_cmd, desh_at_callback cb)
{
	int err;
	struct at_parser parser;
	size_t valid_count = 0;
	enum at_parser_cmd_type type;

	__ASSERT_NO_MSG(cb);

	err = at_parser_init(&parser, at_cmd);
	if (err) {
		return err;
	}

	err = at_parser_cmd_count_get(&parser, &valid_count);
	if (err) {
		return err;
	}

	err = at_parser_cmd_type_get(&parser, &type);
	if (err) {
		return err;
	}

	err = cb(type, &parser, valid_count);
	if (err == 0) {
		return at_cmd_custom_respond(buf, len, "OK\r\n");
	}

	/* For the modem library, errors must be indicated in the response buffer. */
	return at_cmd_custom_respond(buf, len, "ERROR\r\n");
}
