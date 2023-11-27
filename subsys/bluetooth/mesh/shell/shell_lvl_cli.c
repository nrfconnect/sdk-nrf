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

static void status_print(const struct shell *shell, int err, struct bt_mesh_lvl_status *rsp)
{
	if (!err) {
		shell_print(shell, "Current val: %d, target val: %d, rem time: %d", rsp->current,
			    rsp->target, rsp->remaining_time);
	}
}

static int cmd_lvl_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_LEVEL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_lvl_cli *cli = mod->user_data;
	struct bt_mesh_lvl_status rsp;

	int err = bt_mesh_lvl_cli_get(cli, NULL, &rsp);

	status_print(shell, err, &rsp);
	return err;
}

static int lvl_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	int16_t lvl = shell_strtol(argv[1], 0, &err);
	uint32_t time = (argc >= 3) ? shell_strtoul(argv[2], 0, &err) : 0;
	uint32_t delay = (argc == 4) ? shell_strtoul(argv[3], 0, &err) : 0;

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_LEVEL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_lvl_cli *cli = mod->user_data;
	struct bt_mesh_model_transition trans = { .time = time, .delay = delay };
	struct bt_mesh_lvl_set set = {
		.lvl = lvl,
		.transition = (argc > 2) ? &trans : NULL,
	};

	if (acked) {
		struct bt_mesh_lvl_status rsp;

		err = bt_mesh_lvl_cli_set(cli, NULL, &set, &rsp);
		status_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_lvl_cli_set_unack(cli, NULL, &set);
	}
}

static int cmd_lvl_set(const struct shell *shell, size_t argc, char *argv[])
{
	return lvl_set(shell, argc, argv, true);
}

static int cmd_lvl_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return lvl_set(shell, argc, argv, false);
}

static int delta_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	int32_t delta = shell_strtol(argv[1], 0, &err);
	uint32_t time = (argc >= 3) ? shell_strtoul(argv[2], 0, &err) : 0;
	uint32_t delay = (argc == 4) ? shell_strtoul(argv[3], 0, &err) : 0;

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_LEVEL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_lvl_cli *cli = mod->user_data;
	struct bt_mesh_model_transition trans = { .time = time, .delay = delay };
	struct bt_mesh_lvl_delta_set set = {
		.delta = delta,
		.transition = (argc > 2) ? &trans : NULL,
	};

	if (acked) {
		struct bt_mesh_lvl_status rsp;

		err = bt_mesh_lvl_cli_delta_set(cli, NULL, &set, &rsp);
		status_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_lvl_cli_delta_set_unack(cli, NULL, &set);
	}
}

static int cmd_delta_set(const struct shell *shell, size_t argc, char *argv[])
{
	return delta_set(shell, argc, argv, true);
}

static int cmd_delta_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return delta_set(shell, argc, argv, false);
}

static int move_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	int16_t delta = shell_strtol(argv[1], 0, &err);
	uint32_t time = (argc >= 3) ? shell_strtoul(argv[2], 0, &err) : 0;
	uint32_t delay = (argc == 4) ? shell_strtoul(argv[3], 0, &err) : 0;

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_LEVEL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_lvl_cli *cli = mod->user_data;
	struct bt_mesh_model_transition trans = { .time = time, .delay = delay };
	struct bt_mesh_lvl_move_set set = {
		.delta = delta,
		.transition = (argc > 2) ? &trans : NULL,
	};

	if (acked) {
		struct bt_mesh_lvl_status rsp;

		err = bt_mesh_lvl_cli_move_set(cli, NULL, &set, &rsp);
		status_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_lvl_cli_move_set_unack(cli, NULL, &set);
	}
}

static int cmd_move_set(const struct shell *shell, size_t argc, char *argv[])
{
	return move_set(shell, argc, argv, true);
}

static int cmd_move_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return move_set(shell, argc, argv, false);
}

static int cmd_instance_get_all(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_all(shell, BT_MESH_MODEL_ID_GEN_LEVEL_CLI);
}

static int cmd_instance_set(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t elem_idx = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	return shell_model_instance_set(shell, &mod, BT_MESH_MODEL_ID_GEN_LEVEL_CLI, elem_idx);
}

SHELL_STATIC_SUBCMD_SET_CREATE(instance_cmds,
			       SHELL_CMD_ARG(set, NULL, "<ElemIdx> ", cmd_instance_set, 2, 0),
			       SHELL_CMD_ARG(get-all, NULL, NULL, cmd_instance_get_all, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	lvl_cmds, SHELL_CMD_ARG(get, NULL, NULL, cmd_lvl_get, 1, 0),
	SHELL_CMD_ARG(set, NULL, "<Lvl> [TransTime(ms) [Delay(ms)]]",
		      cmd_lvl_set, 2, 2),
	SHELL_CMD_ARG(set-unack, NULL,
		      "<Lvl> [TransTime(ms) [Delay(ms)]]",
		      cmd_lvl_set_unack, 2, 2),
	SHELL_CMD_ARG(delta-set, NULL,
		      "<Dlt> [TransTime(ms) [Delay(ms)]]]", cmd_delta_set,
		      2, 2),
	SHELL_CMD_ARG(delta-set-unack, NULL,
		      "<Dlt> [TransTime(ms) [Delay(ms)]]",
		      cmd_delta_set_unack, 2, 2),
	SHELL_CMD_ARG(move-set, NULL, "<Dlt> [TransTime(ms) [Delay(ms)]]",
		      cmd_move_set, 2, 2),
	SHELL_CMD_ARG(move-set-unack, NULL,
		      "<Dlt> [TransTime(ms) [Delay(ms)]]",
		      cmd_move_set_unack, 2, 2),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", shell_model_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), lvl, &lvl_cmds, "Level Cli commands", shell_model_cmds_help, 1, 1);
