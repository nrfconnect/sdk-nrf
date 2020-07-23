/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <shell/shell.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include "zigbee_cli.h"
#include "zigbee_cli_cmd_zcl.h"

#define ATTR_HELP "Read/write an attribute."

#define ATTR_READ_HELP \
	("Send Read Attribute Zigbee command.\n" \
	"Usage: read <h:dst_addr> <d:ep> <h:cluster>" \
		" [-c] <h:profile> <h:attr_id>\n" \
	"-c switches the server-to-client direction.\n" \
	"h: is for hex, d: is for decimal.")

#define ATTR_WRITE_HELP \
	("Send Write Attribute Zigbee command.\n" \
	"Usage: write <h:dst_addr> <d:ep> <h:cluster> [-c] <h:profile>" \
		" <h:attr_id> <h:attr_type> <h:attr_value>\n" \
	"-c switches the server-to-client direction.\n" \
	"h: is for hex, d: is for decimal.")

#define SUBSCRIBE_HELP "(un)subscribes to an attribute."

#define SUBSCRIBE_ON_HELP \
	("Subscribe to an attribute.\n" \
	"Usage: on <h:addr> <d:ep> <h:cluster>" \
	" <h:profile> <h:attr_id> <d:attr_type>" \
	" [<d:min_interval (s)>] [<d:max_interval (s)>]")

#define SUBSCRIBE_OFF_HELP \
	("Unubscribe from an attribute.\n" \
	"Usage: off <h:addr> <d:ep> <h:cluster>" \
	" <h:profile> <h:attr_id> <d:attr_type>")

#define PING_HELP \
	("Send ping command over ZCL.\n" \
	"Usage: ping [--no-echo] [--aps-ack] <h:addr> <d:payload size>")

#define CMD_HELP \
	("Send generic ZCL command.\n" \
	"Usage: cmd [-d] <h:dst_addr> <d:ep> <h:cluster> [-p h:profile]" \
		" <h:cmd_ID> [-l h:payload]\n" \
	"-d - Require default response.\n" \
	"-p - Set profile ID, HA profile by default.\n" \
	"-l - Send payload in command, Little Endian bytes order.")

#define RAW_HELP \
	("Send raw ZCL frame.\n" \
	"Usage: raw <h:dst_addr> <d:ep> <h:cluster> <h:profile> <h:raw_data>")

#define ZCL_HELP "ZCL subsystem commands."

SHELL_STATIC_SUBCMD_SET_CREATE(sub_attr,
	SHELL_CMD_ARG(read, NULL, ATTR_READ_HELP, cmd_zb_readattr, 6, 1),
	SHELL_CMD_ARG(write, NULL, ATTR_WRITE_HELP, cmd_zb_writeattr, 8, 1),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_subsbcribe,
	SHELL_CMD_ARG(on, NULL, SUBSCRIBE_ON_HELP, cmd_zb_subscribe_on,
		      7, 2),
	SHELL_CMD_ARG(off, NULL, SUBSCRIBE_OFF_HELP, cmd_zb_subscribe_off,
		      7, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_zcl,
	SHELL_CMD(attr, &sub_attr, ATTR_HELP, NULL),
	SHELL_CMD_ARG(cmd, NULL, CMD_HELP, cmd_zb_generic_cmd, 5, 10),
	SHELL_CMD_ARG(ping, NULL, PING_HELP, cmd_zb_ping, 3, 2),
	SHELL_COND_CMD_ARG(CONFIG_ZIGBEE_SHELL_DEBUG_CMD, raw, NULL,
			   RAW_HELP, cmd_zb_zcl_raw, 6, 0),
	SHELL_CMD(subscribe, &sub_subsbcribe, SUBSCRIBE_HELP, NULL),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(zcl, &sub_zcl, ZCL_HELP, NULL);
