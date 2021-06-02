/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <shell/shell.h>

#include "th/th_ctrl.h"

static int print_help(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 1;

	shell_help(shell);

	return ret;
}

static int cmd_th_startbg(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(shell, "please, give also the command to be run");
		return -EINVAL;
	}
	th_ctrl_start(shell, argc, argv, true);
	return 0;
}

static int cmd_th_startfg(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(shell, "please, give also the command to be run");
		return -EINVAL;
	}
	th_ctrl_start(shell, argc, argv, false);
	return 0;
}

static int cmd_th_get_results(const struct shell *shell, size_t argc,
			      char **argv)
{
	int process_nbr;

	process_nbr = atoi(argv[1]);
	if (process_nbr <= 0 || process_nbr > TH_BG_THREADS_MAX_AMOUNT) {
		shell_error(shell, "invalid thread number %d", process_nbr);
		return -EINVAL;
	}
	th_ctrl_result_print(shell, process_nbr);

	return 0;
}

static int cmd_th_ctrl_kill(const struct shell *shell, size_t argc, char **argv)
{
	int process_nbr;

	process_nbr = atoi(argv[1]);
	if (process_nbr <= 0 || process_nbr > TH_BG_THREADS_MAX_AMOUNT) {
		shell_error(shell, "invalid thread number %d", process_nbr);
		return -EINVAL;
	}
	th_ctrl_kill(shell, process_nbr);

	return 0;
}

static int cmd_th_status(const struct shell *shell, size_t argc, char **argv)
{
	th_ctrl_status_print(shell);
	return 0;
}

static int cmd_th(const struct shell *shell, size_t argc, char **argv)
{
	return print_help(shell, argc, argv);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_bg,
	SHELL_CMD(startbg, NULL, "iperf3 <params>\nStart a background thread.",
		  cmd_th_startbg),
	SHELL_CMD(startfg, NULL, "iperf3 <params>\nStart a foreground thread.",
		  cmd_th_startfg),
	SHELL_CMD_ARG(results, NULL,
		      "th results <thread nbr>\nGet results for the background thread.",
		      cmd_th_get_results, 2, 0),
	SHELL_CMD_ARG(kill, NULL, "th kill <thread nbr>\nKill a thread.",
		      cmd_th_ctrl_kill, 2, 0),
	SHELL_CMD_ARG(status, NULL, "Get status of the threads.", cmd_th_status,
		      1, 0),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(th, &sub_bg,
		   "Commands for running iperf3 in separate threads.", cmd_th);
