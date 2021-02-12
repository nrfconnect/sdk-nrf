/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include "zigbee_cli.h"
#include "zigbee_cli_cmd_zcl.h"

#define PING_HELP \
	("Send ping command over ZCL.\n" \
	"Usage: ping [--no-echo] [--aps-ack] <h:addr> <d:payload size>")


SHELL_STATIC_SUBCMD_SET_CREATE(sub_zcl,
	SHELL_CMD_ARG(ping, NULL, PING_HELP, cmd_zb_ping, 3, 2),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(zcl, &sub_zcl, "ZCL subsystem commands.", NULL);
