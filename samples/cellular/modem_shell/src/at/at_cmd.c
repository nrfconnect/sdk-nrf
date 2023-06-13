/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <ctype.h>

#include <zephyr/shell/shell.h>

#include <modem/at_cmd_parser.h>

#include "mosh_defines.h"
#include "mosh_print.h"

#include "at_cmd_ping.h"
#include "at_cmd.h"

/* Globals */
struct at_param_list at_param_list;      /* For AT parser */

/**@brief AT command handler type. */
typedef int (*mosh_at_handler_t) (enum at_cmd_type);

/* MoSh specific AT commands */
static struct mosh_at_cmd {
	char *string;
	mosh_at_handler_t handler;
} mosh_at_cmd_list[] = {
	{"AT+NPING", at_cmd_ping_handle},
};

#define MOSH_AT_CMD_MAX_PARAM 8

/**
 * @brief Case insensitive string comparison between given at_cmd and supported mosh_cmd.
 */

static bool at_cmd_mode_util_mosh_cmd(const char *at_cmd, const char *mosh_cmd)
{
	int i;
	int mosh_cmd_len = strlen(mosh_cmd);

	if (strlen(at_cmd) < mosh_cmd_len) {
		return false;
	}

	for (i = 0; i < mosh_cmd_len; i++) {
		if (toupper((int)*(at_cmd + i)) != toupper((int)*(mosh_cmd + i))) {
			return false;
		}
	}
#if defined(CONFIG_MOSH_AT_CMD_MODE_CR_LF_TERMINATION)
	if (strlen(at_cmd) > (mosh_cmd_len + 2)) {
#else
	if (strlen(at_cmd) > (mosh_cmd_len + 1)) {
#endif
		char ch = *(at_cmd + i);
		/* With parameter, SET TEST, "="; READ, "?" */
		return ((ch == '=') || (ch == '?'));
	}

	return true;
}

int at_cmd_mosh_execute(const char *at_cmd)
{
	int ret;
	int total = ARRAY_SIZE(mosh_at_cmd_list);

	ret = at_params_list_init(&at_param_list, MOSH_AT_CMD_MAX_PARAM);
	if (ret) {
		printk("Could not init AT params list, error: %d\n", ret);
		return ret;
	}
	ret = -ENOENT;
	for (int i = 0; i < total; i++) {
		/* Check if given command is MoSh specific AT command */
		if (at_cmd_mode_util_mosh_cmd(at_cmd, mosh_at_cmd_list[i].string)) {
			enum at_cmd_type type = at_parser_cmd_type_get(at_cmd);

			at_params_list_clear(&at_param_list);
			ret = at_parser_params_from_str(at_cmd, NULL, &at_param_list);
			if (ret) {
				printk("Failed to parse AT command %d\n", ret);
				ret = -EINVAL;
				break;
			}
			ret = mosh_at_cmd_list[i].handler(type);
			break;
		}
	}
	at_params_list_free(&at_param_list);
	return ret;
}
