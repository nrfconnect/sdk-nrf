/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include "mosh_print.h"
#include "cloud_lwm2m.h"

BUILD_ASSERT(!IS_ENABLED(CONFIG_MOSH_CLOUD_MQTT));

static int lwm2m_shell_print_usage(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 1;

	if (argc > 1) {
		mosh_error("%s: subcommand not found", argv[1]);
		ret = -EINVAL;
	}

	shell_help(shell);

	return ret;
}

static int cmd_lwm2m(const struct shell *shell, size_t argc, char **argv)
{
	return lwm2m_shell_print_usage(shell, argc, argv);
}

static int cmd_lwm2m_connect(const struct shell *shell, size_t argc, char **argv)
{
	if (cloud_lwm2m_connect() != 0) {
		mosh_error("LwM2M: Establishing LwM2M connection failed");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_lwm2m_disconnect(const struct shell *shell, size_t argc, char **argv)
{
	if (cloud_lwm2m_disconnect() != 0) {
		mosh_error("LwM2M: Disconnecting LwM2M connection failed");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_lwm2m_suspend(const struct shell *shell, size_t argc, char **argv)
{
	if (cloud_lwm2m_suspend() != 0) {
		mosh_error("LwM2M: Suspending LwM2M connection failed");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_lwm2m_resume(const struct shell *shell, size_t argc, char **argv)
{
	if (cloud_lwm2m_resume() != 0) {
		mosh_error("LwM2M: Resuming LwM2M connection failed");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_lwm2m_update(const struct shell *shell, size_t argc, char **argv)
{
	cloud_lwm2m_update();

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_lwm2m,
	SHELL_CMD_ARG(connect, NULL, "Establish LwM2M connection.", cmd_lwm2m_connect,
		      1, 0),
	SHELL_CMD_ARG(disconnect, NULL, "Disconnect LwM2M connection.", cmd_lwm2m_disconnect, 1, 0),
	SHELL_CMD_ARG(suspend, NULL, "Suspend LwM2M engine.", cmd_lwm2m_suspend, 1, 0),
	SHELL_CMD_ARG(resume, NULL, "Resume LwM2M engine.", cmd_lwm2m_resume, 1, 0),
	SHELL_CMD_ARG(update, NULL, "Trigger a registration update.", cmd_lwm2m_update, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(cloud_lwm2m, &sub_lwm2m, "Cloud connection with LwM2M", cmd_lwm2m);
