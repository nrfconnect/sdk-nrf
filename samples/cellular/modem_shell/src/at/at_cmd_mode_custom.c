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
#include <modem/at_cmd_custom.h>
#include <nrf_modem_at.h>

#include "mosh_defines.h"
#include "mosh_print.h"

#if defined(CONFIG_MOSH_PING)
#include "icmp_ping.h"
#endif

#define MOSH_AT_CMD_MAX_PARAM 8

extern bool at_cmd_mode_dont_print;

#if defined(CONFIG_MOSH_PING)

AT_CMD_CUSTOM(nping_custom, "AT+NPING", nping_callback);

/**@brief handle AT+NPING commands
 *  AT+NPING=<addr>,[<payload_length>,<timeout_msecs>,<count>[,<interval_msecs>[,<cid>]]]
 *  AT+NPING? READ command not supported
 *  AT+NPING=? TEST command not supported
 */
static int at_cmd_ping_handle(struct at_parser *parser, enum at_parser_cmd_type cmd_type)
{
	int err;
	int size = ICMP_MAX_ADDR;
	bool dont_print_prev;
	struct icmp_ping_shell_cmd_argv ping_args;

	if (cmd_type != AT_PARSER_CMD_TYPE_SET) {
		return -EINVAL;
	}

	/* ping is an exception where we want to use mosh_print during AT command mode */
	dont_print_prev = at_cmd_mode_dont_print;
	at_cmd_mode_dont_print = false;

	icmp_ping_cmd_defaults_set(&ping_args);

	err = at_parser_string_get(parser, 1, ping_args.target_name, &size);
	if (err < 0 || size == 0) {
		err = -EINVAL;
		return err;
	}
	ping_args.target_name[size] = '\0';

	err = at_parser_num_get(parser, 2, &ping_args.len);
	if (err < 0) {
		return err;
	}

	err = at_parser_num_get(parser, 3, &ping_args.timeout);
	if (err < 0) {
		return err;
	}

	err = at_parser_num_get(parser, 4, &ping_args.count);
	if (err < 0) {
		return err;
	};

	err = at_parser_num_get(parser, 5, &ping_args.interval);
	if (err < 0) {
		return err;
	};

	err = at_parser_num_get(parser, 6, &ping_args.cid);
	if (err < 0) {
		return err;
	};

	err = icmp_ping_start(&ping_args);

	at_cmd_mode_dont_print = dont_print_prev;

	return err;
}

static int nping_callback(char *buf, size_t len, char *at_cmd)
{
	int err;
	struct at_parser parser;
	enum at_parser_cmd_type type;

	err = at_parser_init(&parser, at_cmd);
	if (err) {
		printk("Could not init AT parser, error: %d\n", err);
		goto exit;
	}

	err = at_parser_cmd_type_get(&parser, &type);
	if (err) {
		printk("Could not get AT command type, error: %d\n", err);
		goto exit;
	}

	err = at_cmd_ping_handle(&parser, type);

exit:
	return at_cmd_custom_respond(buf, len, "%s", err ? "ERROR\r\n" : "OK\r\n");
}

#endif /* CONFIG_MOSH_PING*/
