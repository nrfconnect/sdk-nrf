/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <bluetooth/mesh/models.h>
#include <shell/shell.h>

#include "mesh/net.h"
#include "mesh/access.h"
#include "shell_utils.h"

static struct bt_mesh_model *mod;

static int shell_battery_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod) {
		if (!shell_model_first_get(BT_MESH_MODEL_ID_GEN_BATTERY_CLI, &mod)) {
			return -ENODEV;
		}
	}

	struct bt_mesh_battery_cli *cli = mod->user_data;
	struct bt_mesh_battery_status rsp;

	int err = bt_mesh_battery_cli_get(cli, NULL, &rsp);

	if (!err) {
		shell_print(shell,
			    "Battery lvl: %d, discharge time: %d, charge time: %d\n"
			    "Presence state: %d, indicator state: %d, "
			    "charging state: %d, service state: %d",
			    rsp.battery_lvl, rsp.discharge_minutes, rsp.charge_minutes,
			    rsp.presence, rsp.indicator, rsp.charging, rsp.service);
	}

	return err;
}

static int shell_instance_get_all(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_all(shell, BT_MESH_MODEL_ID_GEN_BATTERY_CLI);
}

static int shell_instance_get_curr(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_curr(shell, mod);
}

static int shell_instance_set(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t elem_idx = (uint8_t)strtol(argv[1], NULL, 0);

	return shell_model_instance_set(shell, &mod, BT_MESH_MODEL_ID_GEN_BATTERY_CLI, elem_idx);
}

SHELL_STATIC_SUBCMD_SET_CREATE(instance_cmds,
			       SHELL_CMD_ARG(set, NULL, "<elem_idx> ", shell_instance_set, 2, 0),
			       SHELL_CMD_ARG(get-curr, NULL, NULL, shell_instance_get_curr, 1, 0),
			       SHELL_CMD_ARG(get-all, NULL, NULL, shell_instance_get_all, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(bat_cmds, SHELL_CMD_ARG(get, NULL, NULL, shell_battery_get, 1, 0),
			       SHELL_CMD(instance, &instance_cmds, "Instance commands",
					 shell_model_cmds_help),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(mesh_battery, &bat_cmds, "Battery Cli commands", shell_model_cmds_help, 1,
		       1);
