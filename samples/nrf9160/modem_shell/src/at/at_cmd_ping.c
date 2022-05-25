/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <sys/types.h>
#include <zephyr/net/net_ip.h>

#include <zephyr/shell/shell.h>
#include <modem/at_cmd_parser.h>
#include <nrf_modem_at.h>

#include "mosh_defines.h"
#include "mosh_print.h"

#if defined(CONFIG_MOSH_PING)
#include "../ping/icmp_ping.h"
#endif
#include "at_cmd_ping.h"

extern struct at_param_list at_param_list;
extern bool at_cmd_mode_dont_print;

#if defined(CONFIG_MOSH_PING)
static struct icmp_ping_shell_cmd_argv ping_args;
#endif

/**@brief handle AT+NPING commands
 *  AT+NPING=<addr>,[<payload_length>,<timeout_msecs>,<count>[,<interval_msecs>[,<cid>]]]
 *  AT+NPING? READ command not supported
 *  AT+NPING=? TEST command not supported
 */
int at_cmd_ping_handle(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
#if defined(CONFIG_MOSH_PING)
	int size = ICMP_MAX_URL;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		/* ping is an exception where we want to use mosh_print during AT command mode */
		at_cmd_mode_dont_print = false;

		icmp_ping_cmd_defaults_set(&ping_args);

		err = at_params_string_get(&at_param_list, 1, ping_args.target_name, &size);
		if (err < 0 || size == 0) {
			err = -EINVAL;
			return err;
		}
		ping_args.target_name[size] = '\0';

		if (at_params_valid_count_get(&at_param_list) > 2) {
			err = at_params_unsigned_int_get(&at_param_list, 2, &ping_args.len);
			if (err < 0) {
				return err;
			}
		}
		if (at_params_valid_count_get(&at_param_list) > 3) {
			err = at_params_unsigned_int_get(&at_param_list, 3, &ping_args.timeout);
			if (err < 0) {
				return err;
			}
		}
		if (at_params_valid_count_get(&at_param_list) > 4) {
			err = at_params_unsigned_int_get(&at_param_list, 4, &ping_args.count);
			if (err < 0) {
				return err;
			};
		}
		if (at_params_valid_count_get(&at_param_list) > 5) {
			err = at_params_unsigned_int_get(&at_param_list, 5, &ping_args.interval);
			if (err < 0) {
				return err;
			};
		}
		if (at_params_valid_count_get(&at_param_list) > 6) {
			err = at_params_int_get(&at_param_list, 6, &ping_args.cid);
			if (err < 0) {
				return err;
			};
		}
		err = icmp_ping_start(&ping_args);
		at_cmd_mode_dont_print = true;
		break;

	default:
		break;
	}
#endif
	return err;
}
