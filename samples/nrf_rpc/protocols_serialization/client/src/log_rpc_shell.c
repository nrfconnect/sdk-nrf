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
			       SHELL_CMD_ARG(stream_level, NULL, "Set stream logging level",
					     cmd_log_rpc_stream_level, 2, 0),
			       SHELL_CMD_ARG(crash, NULL, "Retrieve remote device crash log",
					     cmd_log_rpc_crash, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(log_rpc, &log_rpc_cmds, "RPC logging commands", NULL, 1, 0);
