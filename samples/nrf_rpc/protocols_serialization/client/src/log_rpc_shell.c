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

	if (rc || level < LOG_RPC_LEVEL_NONE || level > LOG_RPC_LEVEL_DBG) {
		shell_error(sh, "Invalid argument: %d", rc);
		return -EINVAL;
	}

	log_rpc_set_stream_level(level);

	return 0;
}

static int cmd_log_rpc_history_level(const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;
	enum log_rpc_level level;

	level = (enum log_rpc_level)shell_strtol(argv[1], 10, &rc);

	if (rc || level < LOG_RPC_LEVEL_NONE || level > LOG_RPC_LEVEL_DBG) {
		shell_error(sh, "Invalid argument: %d", rc);
		return -EINVAL;
	}

	log_rpc_set_history_level(level);

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

static int cmd_log_rpc_history_stop_fetch(const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;
	bool pause;

	pause = shell_strtobool(argv[1], 0, &rc);

	if (rc) {
		shell_error(sh, "Invalid argument: %d", rc);
		return -EINVAL;
	}

	log_rpc_stop_fetch_history(pause);

	return 0;
}

static void history_threshold_reached_handler(void)
{
	shell_print(shell, "History usage threshold reached");
}

static int cmd_log_rpc_history_threshold(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t threshold;
	int rc = 0;

	if (argc == 0) {
		shell_print(sh, "%u", log_rpc_get_history_usage_threshold());
		return 0;
	}

	threshold = shell_strtoul(argv[1], 0, &rc);

	if (rc) {
		shell_error(sh, "Invalid argument: %d", rc);
		return -EINVAL;
	}

	if (threshold > 100) {
		shell_error(sh, "%u exceeds max value (100)", threshold);
		return -EINVAL;
	}

	shell = sh;
	log_rpc_set_history_usage_threshold(history_threshold_reached_handler, (uint8_t)threshold);

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

static int cmd_log_rpc_echo(const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;
	enum log_rpc_level level;

	level = (enum log_rpc_level)shell_strtol(argv[1], 10, &rc);

	if (rc) {
		shell_error(sh, "Invalid argument: %d", rc);
		return -EINVAL;
	}

	log_rpc_echo(level, argv[2]);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	log_rpc_cmds,
	SHELL_CMD_ARG(stream_level, NULL, "Set log streaming level <0-4>", cmd_log_rpc_stream_level,
		      2, 0),
	SHELL_CMD_ARG(history_level, NULL, "Set log history level <0-4>", cmd_log_rpc_history_level,
		      2, 0),
	SHELL_CMD_ARG(history_fetch, NULL, "Fetch log history", cmd_log_rpc_history_fetch, 1, 0),
	SHELL_CMD_ARG(history_stop_fetch, NULL, "Stop log history transfer <pause?>",
		      cmd_log_rpc_history_stop_fetch, 2, 0),
	SHELL_CMD_ARG(history_threshold, NULL, "Get or set history usage threshold [0-100]",
		      cmd_log_rpc_history_threshold, 1, 1),
	SHELL_CMD_ARG(crash, NULL, "Retrieve remote device crash log", cmd_log_rpc_crash, 1, 0),
	SHELL_CMD_ARG(echo, NULL, "Generate log message on remote <0-4> <msg>", cmd_log_rpc_echo, 3,
		      0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(log_rpc, &log_rpc_cmds, "RPC logging commands", NULL, 1, 0);
