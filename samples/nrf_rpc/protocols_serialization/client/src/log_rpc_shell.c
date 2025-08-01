/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <logging/log_rpc.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#include <stdlib.h>

#define COREDUMP_LOG_PREFIX   "#CD:"
#define COREDUMP_LOG_BEGIN    "BEGIN#"
#define COREDUMP_LOG_END      "END#"
#define COREDUMP_LOG_LINE_LEN 32

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

	if (argc == 1) {
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
	shell_print(sh, "%u", log_rpc_get_history_usage_threshold());

	return 0;
}

static int cmd_log_rpc_history_usage_current(const struct shell *sh, size_t argc, char *argv[])
{
	size_t usage_size;
	size_t max_size;
	size_t usage;

	usage_size = log_rpc_get_history_usage_current();
	max_size = log_rpc_get_history_usage_max();
	usage = (usage_size * 100 + max_size / 2) / max_size;
	shell_print(sh,
		    "History usage size: %zu bytes, max size: %zu bytes, usage: %zu%%",
		    usage_size, max_size, usage);

	return 0;
}

static int cmd_log_rpc_crash(const struct shell *sh, size_t argc, char *argv[])
{
	int rc;
	char buffer[CONFIG_NRF_PS_CLIENT_CRASH_DUMP_READ_BUFFER_SIZE];
	char line[COREDUMP_LOG_LINE_LEN * 2 + 1];
	size_t offset = 0;

	shell_print(sh, COREDUMP_LOG_PREFIX COREDUMP_LOG_BEGIN);

	while (true) {
		rc = log_rpc_get_crash_dump(offset, buffer, sizeof(buffer));
		if (rc <= 0) {
			break;
		}

		for (int i = 0; i < rc; i += COREDUMP_LOG_LINE_LEN) {
			bin2hex(&buffer[i], MIN(COREDUMP_LOG_LINE_LEN, rc - i), line, sizeof(line));
			shell_print(sh, COREDUMP_LOG_PREFIX "%s", line);
		}

		offset += (size_t)rc;
	}

	shell_print(sh, COREDUMP_LOG_PREFIX COREDUMP_LOG_END);

	if (rc) {
		shell_error(sh, "Error: %d", rc);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_log_rpc_crash_invalidate(const struct shell *sh, size_t argc, char *argv[])
{
	int rc;

	rc = log_rpc_invalidate_crash_dump();

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

static int cmd_log_rpc_time(const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;
	uint64_t time_us;

	if (strcmp(argv[1], "now") == 0) {
		time_us = k_ticks_to_us_near64(k_uptime_ticks());
	} else {
		time_us = shell_strtoull(argv[1], 10, &rc);
	}

	if (rc) {
		shell_error(sh, "Invalid argument: %d", rc);
		return -EINVAL;
	}

	log_rpc_set_time(time_us);

	return 0;
}

static int cmd_log_rpc_get_crash_info(const struct shell *sh, size_t argc, char *argv[])
{
	int rc;
	struct nrf_rpc_crash_info info;

	rc = log_rpc_get_crash_info(&info);
	if (rc) {
		shell_print(sh, "No coredump stored");

		return 0;
	}

	shell_print(sh, "uuid: %u", info.uuid);
	shell_print(sh, "reason: %u", info.reason);
	shell_print(sh, "PC: 0x%08x", info.pc);
	shell_print(sh, "LR: 0x%08x", info.lr);
	shell_print(sh, "SP: 0x%08x", info.sp);
	shell_print(sh, "XPSR: 0x%08x", info.xpsr);

	if (info.assert_line) {
		shell_print(sh, "ASSERT at %s:%u", info.assert_filename, info.assert_line);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(crash_cmds,
			       SHELL_CMD_ARG(invalidate, NULL, "Invalidate crash dump",
					     cmd_log_rpc_crash_invalidate, 1, 0),
			       SHELL_SUBCMD_SET_END);

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
	SHELL_CMD_ARG(history_usage, NULL, "Get current history usage",
		      cmd_log_rpc_history_usage_current, 1, 0),
	SHELL_CMD_ARG(crash, &crash_cmds, "Retrieve crash dump from remote", cmd_log_rpc_crash, 1,
		      0),
	SHELL_CMD_ARG(echo, NULL, "Generate log message on remote <0-4> <msg>", cmd_log_rpc_echo, 3,
		      0),
	SHELL_CMD_ARG(time, NULL, "Set current time <time_us|now>", cmd_log_rpc_time, 2, 0),
	SHELL_CMD_ARG(crash_info, NULL, "Fetch crash dump summary", cmd_log_rpc_get_crash_info, 1,
		      0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(log_rpc, &log_rpc_cmds, "RPC logging commands", NULL, 1, 0);
