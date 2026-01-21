/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/shell/shell.h>
#include <nrf_rpc/rpc_utils/crash_gen.h>
#include <nrf_rpc/rpc_utils/dev_info.h>
#include <nrf_rpc/rpc_utils/remote_shell.h>
#include <nrf_rpc/rpc_utils/system_health.h>
#include <nrf_rpc.h>

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

#if defined(CONFIG_NRF_RPC_UTILS_SYSTEM_HEALTH)
static int cmd_system_health(const struct shell *sh, size_t argc, char *argv[])
{
	struct nrf_rpc_system_health health;

	nrf_rpc_system_health_get(&health);

	shell_print(sh, "Hung threads: %u", health.hung_threads);

	return 0;
}
#endif /* CONFIG_NRF_RPC_UTILS_SYSTEM_HEALTH */

static int cmd_rpc(const struct shell *sh, size_t argc, char *argv[])
{
	static bool enabled = true;
	int err = 0;

	if (argc > 1) {
		bool enable = shell_strtobool(argv[1], 10, &err);

		if (err == 0) {
			if (enable) {
				nrf_rpc_resume();
			} else {
				nrf_rpc_stop(true);
			}

			enabled = enable;
		}
	}

	shell_print(sh, "RPC is: %s", enabled ? "Enabled" : "Disabled");

	return err;
}

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
#if defined(CONFIG_NRF_RPC_UTILS_SYSTEM_HEALTH)
	SHELL_CMD_ARG(system_health, NULL, "Get system health", cmd_system_health, 0, 0),
#endif
	SHELL_CMD_ARG(control, NULL, "Control RPC subsystem ", cmd_rpc, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(rpc, &util_cmds, "nRF RPC utility commands", NULL, 1, 0);
