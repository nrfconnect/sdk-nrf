/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/shell/shell.h>
#include <nrf_rpc/nrf_rpc_dev_info.h>

static int remote_version_cmd(const struct shell *sh, size_t argc, char *argv[])
{
	const char *version = nrf_rpc_get_ncs_commit_sha();

	shell_print(sh, "Remote version: %s", version);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(util_cmds,
			       SHELL_CMD_ARG(remote_version, NULL, "Get server version",
					     remote_version_cmd, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(rpc, &util_cmds, "nRF RPC utility commands", NULL, 1, 0);
