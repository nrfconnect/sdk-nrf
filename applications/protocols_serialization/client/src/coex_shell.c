/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mpsl/mpsl_cx_software.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <stdlib.h>

static int coex_set_cmd(const struct shell *sh, size_t argc, char *argv[])
{
	mpsl_cx_op_map_t ops = (mpsl_cx_op_map_t)strtoul(argv[1], NULL, 0);
	int64_t timeout_ms = strtoll(argv[2], NULL, 0);

	return mpsl_cx_software_set_granted_ops(ops,
						timeout_ms >= 0 ? K_MSEC(timeout_ms) : K_FOREVER);
}

SHELL_STATIC_SUBCMD_SET_CREATE(coex_cmds,
			       SHELL_CMD_ARG(set, NULL, "<granted_ops> <timeout_ms>", coex_set_cmd,
					     3, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(coex, &coex_cmds, "Software Coexistence commands", NULL, 1, 0);
