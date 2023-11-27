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

static void status_print(const struct shell *shell, int err, struct bt_mesh_plvl_status *rsp)
{
	if (!err) {
		shell_print(shell, "Current val: %d, target val: %d, rem time: %d", rsp->current,
			    rsp->target, rsp->remaining_time);
	}
}

static int cmd_power_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_plvl_cli *cli = mod->user_data;
	struct bt_mesh_plvl_status rsp;

	int err = bt_mesh_plvl_cli_power_get(cli, NULL, &rsp);

	status_print(shell, err, &rsp);
	return err;
}

static int power_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t lvl = shell_strtoul(argv[1], 0, &err);
	uint32_t time = (argc >= 3) ? shell_strtoul(argv[2], 0, &err) : 0;
	uint32_t delay = (argc == 4) ? shell_strtoul(argv[3], 0, &err) : 0;

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_plvl_cli *cli = mod->user_data;
	struct bt_mesh_model_transition trans = { .time = time, .delay = delay };
	struct bt_mesh_plvl_set set = { .power_lvl = lvl,
					.transition = (argc > 2) ? &trans : NULL };

	if (acked) {
		struct bt_mesh_plvl_status rsp;

		err = bt_mesh_plvl_cli_power_set(cli, NULL, &set, &rsp);
		status_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_plvl_cli_power_set_unack(cli, NULL, &set);
	}
}

static int cmd_power_set(const struct shell *shell, size_t argc, char *argv[])
{
	return power_set(shell, argc, argv, true);
}

static int cmd_power_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return power_set(shell, argc, argv, false);
}

static void range_print(const struct shell *shell, int err, struct bt_mesh_plvl_range_status *rsp)
{
	if (!err) {
		shell_print(shell, "Status: %d, min: %d, max: %d", rsp->status, rsp->range.min,
			    rsp->range.max);
	}
}

static int cmd_range_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_plvl_cli *cli = mod->user_data;
	struct bt_mesh_plvl_range_status rsp;

	int err = bt_mesh_plvl_cli_range_get(cli, NULL, &rsp);

	range_print(shell, err, &rsp);
	return err;
}

static int range_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t min = shell_strtoul(argv[1], 0, &err);
	uint16_t max = shell_strtoul(argv[2], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_plvl_cli *cli = mod->user_data;
	struct bt_mesh_plvl_range set = { .min = min, .max = max };

	if (acked) {
		struct bt_mesh_plvl_range_status rsp;

		err = bt_mesh_plvl_cli_range_set(cli, NULL, &set, &rsp);
		range_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_plvl_cli_range_set_unack(cli, NULL, &set);
	}
}

static int cmd_range_set(const struct shell *shell, size_t argc, char *argv[])
{
	return range_set(shell, argc, argv, true);
}

static int cmd_range_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return range_set(shell, argc, argv, false);
}

static void default_print(const struct shell *shell, int err, uint16_t rsp)
{
	if (!err) {
		shell_print(shell, "Default power: %d", rsp);
	}
}

static int cmd_default_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_plvl_cli *cli = mod->user_data;
	uint16_t rsp;

	int err = bt_mesh_plvl_cli_default_get(cli, NULL, &rsp);

	default_print(shell, err, rsp);
	return err;
}

static int default_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t lvl = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_plvl_cli *cli = mod->user_data;

	if (acked) {
		uint16_t rsp;

		err = bt_mesh_plvl_cli_default_set(cli, NULL, lvl, &rsp);
		default_print(shell, err, rsp);
		return err;
	} else {
		return bt_mesh_plvl_cli_default_set_unack(cli, NULL, lvl);
	}
}

static int cmd_default_set(const struct shell *shell, size_t argc, char *argv[])
{
	return default_set(shell, argc, argv, true);
}

static int cmd_default_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return default_set(shell, argc, argv, false);
}

static int cmd_last_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_plvl_cli *cli = mod->user_data;
	uint16_t rsp;

	int err = bt_mesh_plvl_cli_last_get(cli, NULL, &rsp);

	if (!err) {
		shell_print(shell, "Last power Level: %d", rsp);
	}

	return err;
}

static int cmd_instance_get_all(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_all(shell, BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI);
}

static int cmd_instance_set(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t elem_idx = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	return shell_model_instance_set(shell, &mod, BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI,
					elem_idx);
}

SHELL_STATIC_SUBCMD_SET_CREATE(instance_cmds,
			       SHELL_CMD_ARG(set, NULL, "<ElemIdx> ", cmd_instance_set, 2, 0),
			       SHELL_CMD_ARG(get-all, NULL, NULL, cmd_instance_get_all, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	plvl_cmds, SHELL_CMD_ARG(get, NULL, NULL, cmd_power_get, 1, 0),
	SHELL_CMD_ARG(set, NULL, "<Lvl> [TransTime(ms) [Delay(ms)]]", cmd_power_set,
		      2, 2),
	SHELL_CMD_ARG(set-unack, NULL, "<Lvl> [TransTime(ms) [Delay(ms)]]",
		      cmd_power_set_unack, 2, 2),
	SHELL_CMD_ARG(range-get, NULL, NULL, cmd_range_get, 1, 0),
	SHELL_CMD_ARG(range-set, NULL, "<Min> <Max>", cmd_range_set, 3, 0),
	SHELL_CMD_ARG(range-set-unack, NULL, "<Min> <Max>", cmd_range_set_unack, 3, 0),
	SHELL_CMD_ARG(default-get, NULL, NULL, cmd_default_get, 1, 0),
	SHELL_CMD_ARG(default-set, NULL, "<Lvl>", cmd_default_set, 2, 0),
	SHELL_CMD_ARG(default-set-unack, NULL, "<Lvl>", cmd_default_set_unack, 2, 0),
	SHELL_CMD_ARG(last-get, NULL, NULL, cmd_last_get, 1, 0),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", shell_model_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), plvl, &plvl_cmds, "Power level Cli commands",
		 shell_model_cmds_help, 1, 1);
