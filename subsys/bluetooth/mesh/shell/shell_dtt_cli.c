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

static void dtt_print(const struct shell *shell, int err, int32_t rsp)
{
	if (!err) {
		shell_print(shell, "Transition time: %d", rsp);
	}
}

static int cmd_dtt_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dtt_cli *cli = mod->user_data;
	int32_t rsp;

	int err = bt_mesh_dtt_get(cli, NULL, &rsp);

	dtt_print(shell, err, rsp);
	return err;
}

static int dtt_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	int32_t trans_time = shell_strtol(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dtt_cli *cli = mod->user_data;

	if (acked) {
		int32_t rsp;

		err = bt_mesh_dtt_set(cli, NULL, trans_time, &rsp);
		dtt_print(shell, err, rsp);
		return err;
	} else {
		return bt_mesh_dtt_set_unack(cli, NULL, trans_time);
	}
}

static int cmd_dtt_set(const struct shell *shell, size_t argc, char *argv[])
{
	return dtt_set(shell, argc, argv, true);
}

static int cmd_dtt_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return dtt_set(shell, argc, argv, false);
}

static int cmd_instance_get_all(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_all(shell, BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_CLI);
}

static int cmd_instance_set(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t elem_idx = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	return shell_model_instance_set(shell, &mod, BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_CLI,
					elem_idx);
}

SHELL_STATIC_SUBCMD_SET_CREATE(instance_cmds,
			       SHELL_CMD_ARG(set, NULL, "<ElemIdx> ", cmd_instance_set, 2, 0),
			       SHELL_CMD_ARG(get-all, NULL, NULL, cmd_instance_get_all, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	dtt_cmds, SHELL_CMD_ARG(get, NULL, NULL, cmd_dtt_get, 1, 0),
	SHELL_CMD_ARG(set, NULL, "<TransTime(ms)>", cmd_dtt_set, 2, 0),
	SHELL_CMD_ARG(set-unack, NULL, "<TransTime(ms)>", cmd_dtt_set_unack, 2, 0),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", shell_model_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), dtt, &dtt_cmds, "Default transition time Cli commands",
		 shell_model_cmds_help, 1, 1);
