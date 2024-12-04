/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <logging/log_rpc.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <stdlib.h>

static int cmd_log_rpc_stream_level(const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;
	enum log_rpc_level level;

	level = (enum log_rpc_level)shell_strtol(argv[1], 10, &rc);

	if (rc) {
		shell_error(sh, "Invalid argument: %d", rc);
		return -EINVAL;
	}

	rc = log_rpc_set_stream_level(level);

	if (rc) {
		shell_error(sh, "Error: %d", rc);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_log_rpc_history_level(const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;
	enum log_rpc_level level;

	level = (enum log_rpc_level)shell_strtol(argv[1], 10, &rc);

	if (rc) {
		shell_error(sh, "Invalid argument: %d", rc);
		return -EINVAL;
	}

	rc = log_rpc_set_history_level(level);

	if (rc) {
		shell_error(sh, "Error: %d", rc);
		return -ENOEXEC;
	}

	return 0;
}

static const struct shell *shell;

static void history_handler(enum log_rpc_level level, const char *msg, size_t msg_len)
{
	enum shell_vt100_color color;

	if (!msg) {
		shell_print(shell, "Done");
		return;
	}

	switch (level) {
	case LOG_RPC_LEVEL_INF:
		color = SHELL_INFO;
		break;
	case LOG_RPC_LEVEL_WRN:
		color = SHELL_WARNING;
		break;
	case LOG_RPC_LEVEL_ERR:
		color = SHELL_ERROR;
		break;
	default:
		color = SHELL_NORMAL;
		break;
	}

	shell_fprintf(shell, color, "%.*s\n", msg_len, msg);
}

static int cmd_log_rpc_history_fetch(const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;

	shell = sh;
	rc = log_rpc_fetch_history(history_handler);

	if (rc) {
		shell_error(sh, "Error: %d", rc);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_log_rpc_crash(const struct shell *sh, size_t argc, char *argv[])
{
	int rc;
	char buffer[CONFIG_RPC_CRASH_LOG_READ_BUFFER_SIZE];
	size_t offset = 0;

	while (true) {
		rc = log_rpc_get_crash_log(offset, buffer, sizeof(buffer));
		if (rc <= 0) {
			break;
		}

		shell_fprintf(sh, SHELL_NORMAL, "%.*s", (size_t)rc, buffer);
		offset += (size_t)rc;
	}

	shell_fprintf(sh, SHELL_NORMAL, "\n");

	if (rc) {
		shell_error(sh, "Error: %d", rc);
		return -ENOEXEC;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(log_rpc_cmds,
			       SHELL_CMD_ARG(stream_level, NULL, "Set log streaming level",
					     cmd_log_rpc_stream_level, 2, 0),
			       SHELL_CMD_ARG(history_level, NULL, "Set log history level",
					     cmd_log_rpc_history_level, 2, 0),
			       SHELL_CMD_ARG(history_fetch, NULL, "Fetch log history",
					     cmd_log_rpc_history_fetch, 1, 0),
			       SHELL_CMD_ARG(crash, NULL, "Retrieve remote device crash log",
					     cmd_log_rpc_crash, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(log_rpc, &log_rpc_cmds, "RPC logging commands", NULL, 1, 0);
