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
#include <modem/modem_attest_token.h>

#define NRF_PROVISIONING_HELP_CMD "nRF Provisioning commands"
#define NRF_PROVISIONING_HELP_NOW "Do provisioning now"
#define NRF_PROVISIONING_HELP_INIT "Start the client"
#define NRF_PROVISIONING_HELP_TOKEN "Get the attestation token"
#define NRF_PROVISIONING_HELP_UUID "Get device UUID"
#define NRF_PROVISIONING_HELP_INTERVAL "Set provisioning interval"

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

static int cmd_token(const struct shell *sh, size_t argc, char **argv)
{
	struct nrf_attestation_token token = { 0 };
	int rc;

	rc = modem_attest_token_get(&token);
	if (rc) {
		shell_print(sh, "Failed to get token, err %d\n", rc);
		return -ENOEXEC;
	}
	shell_print(sh, "%.*s.%.*s\n", token.attest_sz, token.attest, token.cose_sz, token.cose);
	modem_attest_token_free(&token);
	return 0;
}

static int cmd_uuid(const struct shell *sh, size_t argc, char **argv)
{
	int rc;
	struct nrf_device_uuid dev;

	rc = modem_attest_token_get_uuids(&dev, NULL);
	if (rc) {
		shell_print(sh, "Failed to get UUID, err %d\n", rc);
		return -ENOEXEC;
	}
	shell_print(sh, "%.*s\n", NRF_DEVICE_UUID_STR_LEN, dev.str);
	return 0;
}

static int cmd_interval(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);

	if (argc < 2) {
		shell_error(sh, "No interval provided");
		shell_print(sh, "Usage: nrf_provisioning interval <seconds>");
		return -EINVAL;
	}

	nrf_provisioning_set_interval(atoi(argv[1]));

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_nrf_provisioning,
	SHELL_CMD(init, NULL, NRF_PROVISIONING_HELP_INIT, cmd_init),
	SHELL_CMD(now, NULL, NRF_PROVISIONING_HELP_NOW, cmd_now),
	SHELL_CMD(token, NULL, NRF_PROVISIONING_HELP_TOKEN, cmd_token),
	SHELL_CMD(uuid, NULL, NRF_PROVISIONING_HELP_UUID, cmd_uuid),
	SHELL_CMD(interval, NULL, NRF_PROVISIONING_HELP_INTERVAL, cmd_interval),

	SHELL_SUBCMD_SET_END);

SHELL_COND_CMD_ARG_REGISTER(CONFIG_NRF_PROVISIONING_SHELL, nrf_provisioning, &sub_nrf_provisioning,
			    NRF_PROVISIONING_HELP_CMD, NULL, 1, 0);
