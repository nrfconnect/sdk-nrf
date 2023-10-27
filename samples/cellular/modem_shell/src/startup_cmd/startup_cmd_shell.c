/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <unistd.h>
#include <getopt.h>

#include "mosh_print.h"
#include "startup_cmd_settings.h"

enum startup_cmd_common_options {
	SETT_CMD_COMMON_NONE = 0,
	SETT_CMD_COMMON_READ,
};

static const char startup_cmd_usage_str[] =
	"Usage: startup_cmd --read | --mem<1-3> <cmd_str> | -t <timeout_in_secs>\n"
	"Options:\n"
	"  -r, --read,      Read all stored MoSh startup commands from settings\n"
	"      --mem[1-3],  Store a MoSh command to a given memory slot.\n"
	"                   Clear the given memslot by giving an empty string:\n"
	"                   startup_cmd --mem2 \"\"\n"
	"  -t, --time,      Starting time of executing of commands (in seconds after a bootup).\n"
	"                   By default execution will be done when LTE is connected\n"
	"                   for the first time after the bootup. Restore default value\n"
	"                   by giving a negative value.\n"
	"  -h, --help,      Shows this help information";

/* The following do not have short options */
enum {
	SETT_CMD_SHELL_OPT_MEM_SLOT_1 = 1001,
	SETT_CMD_SHELL_OPT_MEM_SLOT_2,
	SETT_CMD_SHELL_OPT_MEM_SLOT_3,
};

/* Specifying the expected options (both long and short) */
static struct option long_options[] = {
	{ "read", no_argument, 0, 'r' },
	{ "time", required_argument, 0, 't' },
	{ "mem1", required_argument, 0, SETT_CMD_SHELL_OPT_MEM_SLOT_1 },
	{ "mem2", required_argument, 0, SETT_CMD_SHELL_OPT_MEM_SLOT_2 },
	{ "mem3", required_argument, 0, SETT_CMD_SHELL_OPT_MEM_SLOT_3 },
	{ 0, 0, 0, 0 }
};

static void startup_cmd_shell_print_usage(void)
{
	mosh_print_no_format(startup_cmd_usage_str);
}

static int startup_cmd_shell(const struct shell *shell, size_t argc, char **argv)
{
	enum startup_cmd_common_options common_option = SETT_CMD_COMMON_NONE;
	int ret = 0;
	int starttime;
	uint8_t startup_cmd_mem_slot = 0;
	char *startup_cmd_str = NULL;
	bool starttime_given = false;

	if (argc < 2) {
		goto show_usage;
	}

	optreset = 1;
	optind = 1;
	int opt;

	while ((opt = getopt_long(argc, argv, "t:r", long_options, NULL)) != -1) {
		switch (opt) {
		case 'r':
			common_option = SETT_CMD_COMMON_READ;
			break;
		case 't':
			starttime = atoi(optarg);
			starttime_given = true;
			break;
		case SETT_CMD_SHELL_OPT_MEM_SLOT_1:
			startup_cmd_str = optarg;
			startup_cmd_mem_slot = 1;
			break;
		case SETT_CMD_SHELL_OPT_MEM_SLOT_2:
			startup_cmd_str = optarg;
			startup_cmd_mem_slot = 2;
			break;
		case SETT_CMD_SHELL_OPT_MEM_SLOT_3:
			startup_cmd_str = optarg;
			startup_cmd_mem_slot = 3;
			break;

		case '?':
			mosh_error("Unknown option. See usage:");
			goto show_usage;
		case 'h':
		default:
			goto show_usage;
		}
	}
	if (!starttime_given && common_option == SETT_CMD_COMMON_NONE &&
	    startup_cmd_str == NULL) {
		mosh_warn("No command option given. Please, see the usage.");
		goto show_usage;
	}

	if (starttime_given) {
		startup_cmd_settings_starttime_set(starttime);
	}
	if (common_option == SETT_CMD_COMMON_READ) {
		startup_cmd_settings_conf_shell_print();
	} else if (startup_cmd_str != NULL) {
		ret = startup_cmd_settings_save(startup_cmd_str, startup_cmd_mem_slot);
		if (ret < 0) {
			mosh_error("Cannot set command %s to settings: \"%s\"", startup_cmd_str);
		} else {
			mosh_print("Command \"%s\" set successfully to "
				   "memory slot %d.",
				   ((strlen(startup_cmd_str)) ? startup_cmd_str : "<empty>"),
				   startup_cmd_mem_slot);
		}
	}

	return ret;

show_usage:
	startup_cmd_shell_print_usage();
	return ret;
}

SHELL_CMD_REGISTER(startup_cmd, NULL,
	"Command to store MoSH commands to be run sequentially after a bootup.",
	startup_cmd_shell);
