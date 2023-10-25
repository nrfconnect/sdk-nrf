/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr/shell/shell.h>

#include "th/th_ctrl.h"
#include "mosh_print.h"

static int cmd_th_startbg(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		mosh_error("Missing the command to be run");
		return -EINVAL;
	}
	th_ctrl_start(shell, argc, argv, true, false);
	return 0;
}

static int cmd_th_startfg(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		mosh_error("Missing the command to be run");
		return -EINVAL;
	}
	th_ctrl_start(shell, argc, argv, false, false);
	return 0;
}

static int cmd_th_start_pipeline(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		mosh_error("Missing the command to be run");
		return -EINVAL;
	}
	th_ctrl_start(shell, argc, argv, false, true);
	return 0;
}

static int cmd_th_get_results(const struct shell *shell, size_t argc, char **argv)
{
	int process_nbr;

	process_nbr = atoi(argv[1]);
	if (process_nbr <= 0 || process_nbr > TH_BG_THREADS_MAX_AMOUNT) {
		mosh_error("invalid thread number %d", process_nbr);
		return -EINVAL;
	}
	th_ctrl_result_print(process_nbr);

	return 0;
}

static int cmd_th_ctrl_kill(const struct shell *shell, size_t argc, char **argv)
{
	int process_nbr;

	process_nbr = atoi(argv[1]);
	if (process_nbr <= 0 || process_nbr > TH_BG_THREADS_MAX_AMOUNT) {
		mosh_error("invalid thread number %d", process_nbr);
		return -EINVAL;
	}
	th_ctrl_kill(process_nbr);

	return 0;
}

static int cmd_th_status(const struct shell *shell, size_t argc, char **argv)
{
	th_ctrl_status_print();
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_th,
	SHELL_CMD(startbg, NULL, "iperf3 | ping <params>\nStart a background thread.",
		  cmd_th_startbg),
	SHELL_CMD(startfg, NULL, "iperf3 | ping <params>\nStart a foreground thread.",
		  cmd_th_startfg),
	SHELL_CMD(pipeline, NULL, "\"<cmd> <params>\" \"<cmd> <params>\" \"<cmd> <params>\"...\n"
				  "Execute any MoSh commands sequentially in one thread.",
		  cmd_th_start_pipeline),
	SHELL_CMD_ARG(
		results, NULL,
		"th results <thread nbr>\nGet results for the background thread.",
		cmd_th_get_results, 2, 0),
	SHELL_CMD_ARG(kill, NULL, "th kill <thread nbr>\nKill a thread.", cmd_th_ctrl_kill, 2, 0),
	SHELL_CMD_ARG(status, NULL, "Get status of the threads.", cmd_th_status, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(th, &sub_th,
		   "Commands for running iperf3 and ping in separate threads.\n"
		   "Note: ping cannot be run with other commands within the same PDN ID.\n"
		   "      Use \"link connect\" to create PDP context.",
		   mosh_print_help_shell);
