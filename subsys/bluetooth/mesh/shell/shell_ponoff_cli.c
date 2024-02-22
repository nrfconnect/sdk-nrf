/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <bluetooth/mesh/models.h>
#include <zephyr/shell/shell.h>

#include "mesh/net.h"
#include "mesh/access.h"
#include "shell_utils.h"

static const struct bt_mesh_model *mod;

static void pwr_up_print(const struct shell *shell, int err, enum bt_mesh_on_power_up rsp)
{
	if (!err) {
		shell_print(shell, "Power Up state: %d", rsp);
	}
}

static int cmd_on_power_up_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_ponoff_cli *cli = mod->rt->user_data;
	enum bt_mesh_on_power_up rsp;

	int err = bt_mesh_ponoff_cli_on_power_up_get(cli, NULL, &rsp);

	pwr_up_print(shell, err, rsp);
	return err;
}

static int on_power_up_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	enum bt_mesh_on_power_up pow_up = (enum bt_mesh_on_power_up)shell_strtol(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_ponoff_cli *cli = mod->rt->user_data;

	if (acked) {
		enum bt_mesh_on_power_up rsp;

		err = bt_mesh_ponoff_cli_on_power_up_set(cli, NULL, pow_up, &rsp);
		pwr_up_print(shell, err, rsp);
		return err;
	} else {
		return bt_mesh_ponoff_cli_on_power_up_set_unack(cli, NULL, pow_up);
	}
}

static int cmd_on_power_up_set(const struct shell *shell, size_t argc, char *argv[])
{
	return on_power_up_set(shell, argc, argv, true);
}

static int cmd_on_power_up_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return on_power_up_set(shell, argc, argv, false);
}

static int cmd_instance_get_all(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_all(shell, BT_MESH_MODEL_ID_GEN_POWER_ONOFF_CLI);
}

static int cmd_instance_set(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t elem_idx = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	return shell_model_instance_set(shell, &mod, BT_MESH_MODEL_ID_GEN_POWER_ONOFF_CLI,
					elem_idx);
}

SHELL_STATIC_SUBCMD_SET_CREATE(instance_cmds,
			       SHELL_CMD_ARG(set, NULL, "<ElemIdx> ", cmd_instance_set, 2, 0),
			       SHELL_CMD_ARG(get-all, NULL, NULL, cmd_instance_get_all, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	ponoff_cmds, SHELL_CMD_ARG(get, NULL, NULL, cmd_on_power_up_get, 1, 0),
	SHELL_CMD_ARG(set, NULL, "<PowUpState>", cmd_on_power_up_set, 2, 0),
	SHELL_CMD_ARG(set-unack, NULL, "<PowUpState>", cmd_on_power_up_set_unack, 2, 0),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", shell_model_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), ponoff, &ponoff_cmds, "Power OnOff Cli commands",
		 shell_model_cmds_help, 1, 1);
