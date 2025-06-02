/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/shell/shell.h>
#include <nrf_rpc/rpc_utils/crash_gen.h>
#include <nrf_rpc/rpc_utils/dev_info.h>
#include <nrf_rpc/rpc_utils/remote_shell.h>

#if defined(CONFIG_NRF_RPC_UTILS_DEV_INFO)
static int remote_version_cmd(const struct shell *sh, size_t argc, char *argv[])
{
	char *version = nrf_rpc_get_ncs_commit_sha();

	if (!version) {
		shell_error(sh, "Failed to get remote version");
		return -ENOEXEC;
	}

	shell_print(sh, "Remote version: %s", version);

	k_free(version);

	return 0;
}
#endif

#if defined(CONFIG_NRF_RPC_UTILS_REMOTE_SHELL)
static int remote_shell_cmd(const struct shell *sh, size_t argc, char *argv[])
{
	char *output = nrf_rpc_invoke_remote_shell_cmd(argc - 1, argv + 1);

	if (!output) {
		shell_error(sh, "Failed to invoke remote shell command");
		return -ENOEXEC;
	}

	shell_print(sh, "%s", output);

	k_free(output);

	return 0;
}
#endif /* CONFIG_NRF_RPC_UTILS_REMOTE_SHELL */

#if defined(CONFIG_NRF_RPC_UTILS_CRASH_GEN)
static int cmd_assert(const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;
	uint32_t delay = 0;

	if (argc > 1) {
		delay = shell_strtol(argv[1], 10, &rc);
	}

	if (rc) {
		shell_print(sh, "Invalid argument: %s", argv[1]);
		return -EINVAL;
	}

	nrf_rpc_crash_gen_assert(delay);

	return 0;
}

static int cmd_hard_fault(const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;
	uint32_t delay = 0;

	if (argc > 1) {
		delay = shell_strtol(argv[1], 10, &rc);
	}

	if (rc) {
		shell_print(sh, "Invalid argument: %s", argv[1]);
		return -EINVAL;
	}

	nrf_rpc_crash_gen_hard_fault(delay);

	return 0;
}

static int cmd_stack_overflow(const struct shell *sh, size_t argc, char *argv[])
{
	int rc = 0;
	uint32_t delay = 0;

	if (argc > 1) {
		delay = shell_strtol(argv[1], 10, &rc);
	}

	if (rc) {
		shell_print(sh, "Invalid argument: %s", argv[1]);
		return -EINVAL;
	}

	nrf_rpc_crash_gen_stack_overflow(delay);

	return 0;
}
#endif /* CONFIG_NRF_RPC_UTILS_CRASH_GEN */

SHELL_STATIC_SUBCMD_SET_CREATE(
	util_cmds,
#if defined(CONFIG_NRF_RPC_UTILS_DEV_INFO)
	SHELL_CMD_ARG(remote_version, NULL, "Get server version", remote_version_cmd, 1, 0),
#endif
#if defined(CONFIG_NRF_RPC_UTILS_REMOTE_SHELL)
	SHELL_CMD_ARG(remote_shell, NULL, "Call remote shell command", remote_shell_cmd, 2, 255),
#endif
#if defined(CONFIG_NRF_RPC_UTILS_CRASH_GEN)
	SHELL_CMD_ARG(assert, NULL, "Invoke assert", cmd_assert, 1, 1),
	SHELL_CMD_ARG(hard_fault, NULL, "Invoke hard fault", cmd_hard_fault, 1, 1),
	SHELL_CMD_ARG(stack_overflow, NULL, "Invoke stack overflow", cmd_stack_overflow, 1, 1),
#endif
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(rpc, &util_cmds, "nRF RPC utility commands", NULL, 1, 0);
