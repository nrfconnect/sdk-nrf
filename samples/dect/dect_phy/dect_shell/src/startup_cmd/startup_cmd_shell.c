/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <unistd.h>
#include <getopt.h>

#include "desh_print.h"
#include "startup_cmd_settings.h"

enum startup_cmd_common_options {
	SETT_CMD_COMMON_NONE = 0,
	SETT_CMD_COMMON_READ,
};

static const char startup_cmd_usage_str[] =
	"Usage: startup_cmd [--read] | [--mem_slot <1-3> --cmd_str <cmd_str>\n"
	"                               [-t <start_time> -d <delay>]\n"
	"Note: only one command can be given at a time.\n"
	"Options:\n"
	"  -r, --read,      Read all stored MoSh startup commands from settings\n"
	"      --mem_slot,  Store a cmd_str to a given memory slot. Max amount of mem slots is 6.\n"
	"      --cmd_str,   Command string to a given mem_slot.\n"
	"                   Clear the given memslot by giving an empty string:\n"
	"                   startup_cmd --mem_slot 1 --cmd_str \"\"\n"
	"  -t, --time,      Starting time of executing of commands (in seconds after a bootup).\n"
	"                   Default: 1 second after a bootup.\n"
	"  -d, --delay,     Pre-delay before executing cmd_str at mem_slot in shell.\n"
	"                   Default: 5 seconds.\n"
	"  -h, --help,      Shows this help information";

/* The following do not have short options */
enum {
	SETT_CMD_SHELL_OPT_MEM_SLOT_NBR = 1001,
	SETT_CMD_SHELL_OPT_CMD_STR,
};

/* Specifying the expected options (both long and short) */
static struct option long_options[] = {
	{ "read", no_argument, 0, 'r' },
	{ "time", required_argument, 0, 't' },
	{ "delay", required_argument, 0, 'd' },
	{ "mem_slot", required_argument, 0, SETT_CMD_SHELL_OPT_MEM_SLOT_NBR },
	{ "cmd_str", required_argument, 0, SETT_CMD_SHELL_OPT_CMD_STR },
	{ 0, 0, 0, 0 }
};

static int startup_cmd_shell(const struct shell *shell, size_t argc, char **argv)
{
	int err = 0;
	int delay = STARTUP_CMD_DELAY_DEFAULT;
	bool delay_set = false;
	int memslot = 0;
	char *startup_cmd_str = NULL;

	if (argc < 2) {
		goto show_usage;
	}
	struct startup_cmd_data current_settings;
	struct startup_cmd_data newsettings;

	optreset = 1;
	optind = 1;
	int opt, tmp_value;

	startup_cmd_settings_data_read(&current_settings);
	newsettings = current_settings;

	while ((opt = getopt_long(argc, argv, "d:t:rh", long_options, NULL)) != -1) {
		switch (opt) {
		case 'r':
			startup_cmd_settings_data_print();
			return 0;
		case 't':
			tmp_value = atoi(optarg);
			if (tmp_value < 0) {
				desh_error("Give decent value (value >= 0)");
				return -EINVAL;
			}
			/* Start time is for all commands*/
			newsettings.starttime = tmp_value;
			break;
		case 'd':
			delay = atoi(optarg);
			if (tmp_value < 0) {
				desh_error("Give decent value (value >= 0)");
				return -EINVAL;
			}
			delay_set = true;
			break;
		case SETT_CMD_SHELL_OPT_MEM_SLOT_NBR:
			tmp_value = atoi(optarg);
			if (tmp_value < 1 || tmp_value > STARTUP_CMD_MAX_COUNT) {
				desh_error("Give decent value for the memory slot (1-3)");
				return -EINVAL;
			}
			memslot = tmp_value;
			break;
		case SETT_CMD_SHELL_OPT_CMD_STR: {
			if (strlen(optarg) > STARTUP_CMD_MAX_LEN) {
				desh_error("Command string too long. Max length is %d.",
					   STARTUP_CMD_MAX_LEN);
				goto show_usage;
			}
			startup_cmd_str = optarg;
			break;
		}

		case 'h':
			goto show_usage;
		case '?':
		default:
			desh_error("Unknown option (%s). See usage:", argv[optind - 1]);
			goto show_usage;
		}
	}

	if (optind < argc) {
		desh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	if (startup_cmd_str && !memslot) {
		desh_error("Memory slot is missing.");
		goto show_usage;
	}


	if (memslot && !startup_cmd_str) {
		desh_error("Command string is missing.");
		goto show_usage;
	}

	if (memslot && startup_cmd_str) {
		strcpy(newsettings.cmd[memslot - 1].cmd_str, startup_cmd_str);
	}

	if (memslot && delay_set) {
		newsettings.cmd[memslot - 1].delay = delay;
	}

	err = startup_cmd_settings_data_save(&newsettings);
	if (!err) {
		desh_print("Command saved successfully.");
	}

	return err;

show_usage:
	desh_print_no_format(startup_cmd_usage_str);
	return err;
}

SHELL_CMD_REGISTER(startup_cmd, NULL,
	"Command to store shell commands to be run sequentially after a bootup.",
	startup_cmd_shell);
