/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#define LOG_MODULE_NAME net_nrf_provisioning_shell
#define LOG_LEVEL	CONFIG_NRF_PROVISIONING_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/shell/shell.h>
#include <net/nrf_provisioning.h>

#define NRF_PROVISIONING_HELP_CMD "nRF Provisioning commands"
#define NRF_PROVISIONING_HELP_NOW "Do provisioning now"
#define NRF_PROVISIONING_HELP_INIT "Start the client"

static int cmd_now(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	nrf_provisioning_trigger_manually();
	return 0;
}

static int cmd_init(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	nrf_provisioning_init(NULL, NULL);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_nrf_provisioning,
	SHELL_CMD_ARG(init, NULL, NRF_PROVISIONING_HELP_INIT, cmd_init, 1, 0),
	SHELL_CMD_ARG(now, NULL, NRF_PROVISIONING_HELP_NOW, cmd_now, 1, 0),

	SHELL_SUBCMD_SET_END);

SHELL_COND_CMD_ARG_REGISTER(CONFIG_NRF_PROVISIONING_SHELL, nrf_provisioning, &sub_nrf_provisioning,
			    NRF_PROVISIONING_HELP_CMD, NULL, 1, 0);
