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

static void ctl_print(const struct shell *shell, int err, struct bt_mesh_light_ctl_status *rsp)
{
	if (!err) {
		shell_print(shell,
			    "Current light: %d, current temp: %d "
			    "target light: %d, target temp: %d, rem time: %d",
			    rsp->current_light, rsp->current_temp, rsp->target_light,
			    rsp->target_temp, rsp->remaining_time);
	}
}

static int cmd_ctl_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_CTL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctl_cli *cli = mod->rt->user_data;
	struct bt_mesh_light_ctl_status rsp;

	int err = bt_mesh_light_ctl_get(cli, NULL, &rsp);

	ctl_print(shell, err, &rsp);
	return err;
}

static int ctl_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t light = shell_strtoul(argv[1], 0, &err);
	uint16_t temp = shell_strtoul(argv[2], 0, &err);
	int16_t delta = shell_strtol(argv[3], 0, &err);
	uint32_t time = (argc >= 5) ? shell_strtoul(argv[4], 0, &err) : 0;
	uint32_t delay = (argc == 6) ? shell_strtoul(argv[5], 0, &err) : 0;

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_CTL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctl_cli *cli = mod->rt->user_data;
	struct bt_mesh_model_transition trans = { .time = time, .delay = delay };
	struct bt_mesh_light_ctl_set set = {
		.params = {
			.light = light,
			.temp = temp,
			.delta_uv = delta,
		},
		.transition = (argc > 4) ? &trans : NULL,
	};

	if (acked) {
		struct bt_mesh_light_ctl_status rsp;

		err = bt_mesh_light_ctl_set(cli, NULL, &set, &rsp);
		ctl_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_light_ctl_set_unack(cli, NULL, &set);
	}
}

static int cmd_ctl_set(const struct shell *shell, size_t argc, char *argv[])
{
	return ctl_set(shell, argc, argv, true);
}

static int cmd_ctl_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return ctl_set(shell, argc, argv, false);
}

static void temp_print(const struct shell *shell, int err, struct bt_mesh_light_temp_status *rsp)
{
	if (!err) {
		shell_print(shell,
			    "Current temp: %d, current delta: %d "
			    "target temp: %d, target delta: %d, rem time: %d",
			    rsp->current.temp, rsp->current.delta_uv, rsp->target.temp,
			    rsp->target.delta_uv, rsp->remaining_time);
	}
}

static int cmd_temp_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_CTL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctl_cli *cli = mod->rt->user_data;
	struct bt_mesh_light_temp_status rsp;

	int err = bt_mesh_light_temp_get(cli, NULL, &rsp);

	temp_print(shell, err, &rsp);
	return err;
}

static int temp_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t temp = shell_strtoul(argv[1], 0, &err);
	int16_t delta = shell_strtol(argv[2], 0, &err);
	uint32_t time = (argc >= 4) ? shell_strtoul(argv[3], 0, &err) : 0;
	uint32_t delay = (argc == 5) ? shell_strtoul(argv[4], 0, &err) : 0;

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_CTL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctl_cli *cli = mod->rt->user_data;
	struct bt_mesh_model_transition trans = { .time = time, .delay = delay };
	struct bt_mesh_light_temp_set set = {
		.params = {
			.temp = temp,
			.delta_uv = delta,
		},
		.transition = (argc > 3) ? &trans : NULL,
	};

	if (acked) {
		struct bt_mesh_light_temp_status rsp;

		err = bt_mesh_light_temp_set(cli, NULL, &set, &rsp);
		temp_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_light_temp_set_unack(cli, NULL, &set);
	}
}

static int cmd_temp_set(const struct shell *shell, size_t argc, char *argv[])
{
	return temp_set(shell, argc, argv, true);
}

static int cmd_temp_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return temp_set(shell, argc, argv, false);
}

static void default_print(const struct shell *shell, int err, struct bt_mesh_light_ctl *rsp)
{
	if (!err) {
		shell_print(shell, "Light: %d, temp: %d, delta: %d", rsp->light, rsp->temp,
			    rsp->delta_uv);
	}
}

static int cmd_ctl_default_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_CTL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctl_cli *cli = mod->rt->user_data;
	struct bt_mesh_light_ctl rsp;

	int err = bt_mesh_light_ctl_default_get(cli, NULL, &rsp);

	default_print(shell, err, &rsp);
	return err;
}

static int default_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t light = shell_strtoul(argv[1], 0, &err);
	uint16_t temp = shell_strtoul(argv[2], 0, &err);
	int16_t delta = shell_strtol(argv[3], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_CTL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctl_cli *cli = mod->rt->user_data;
	struct bt_mesh_light_ctl set = {
		.light = light,
		.temp = temp,
		.delta_uv = delta,
	};

	if (acked) {
		struct bt_mesh_light_ctl rsp;

		err = bt_mesh_light_ctl_default_set(cli, NULL, &set, &rsp);
		default_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_light_ctl_default_set_unack(cli, NULL, &set);
	}
}

static int cmd_ctl_default_set(const struct shell *shell, size_t argc, char *argv[])
{
	return default_set(shell, argc, argv, true);
}

static int cmd_ctl_default_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return default_set(shell, argc, argv, false);
}

static void range_print(const struct shell *shell, int err,
			struct bt_mesh_light_temp_range_status *rsp)
{
	if (!err) {
		shell_print(shell, "Status: %d, min: %d, max: %d", rsp->status, rsp->range.min,
			    rsp->range.max);
	}
}

static int cmd_temp_range_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_CTL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctl_cli *cli = mod->rt->user_data;
	struct bt_mesh_light_temp_range_status rsp;

	int err = bt_mesh_light_temp_range_get(cli, NULL, &rsp);

	range_print(shell, err, &rsp);
	return err;
}

static int temp_range_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t min = shell_strtoul(argv[1], 0, &err);
	uint16_t max = shell_strtoul(argv[2], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_CTL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctl_cli *cli = mod->rt->user_data;
	struct bt_mesh_light_temp_range set = {
		.min = min,
		.max = max,
	};

	if (acked) {
		struct bt_mesh_light_temp_range_status rsp;

		err = bt_mesh_light_temp_range_set(cli, NULL, &set, &rsp);
		range_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_light_temp_range_set_unack(cli, NULL, &set);
	}
}

static int cmd_temp_range_set(const struct shell *shell, size_t argc, char *argv[])
{
	return temp_range_set(shell, argc, argv, true);
}

static int cmd_temp_range_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return temp_range_set(shell, argc, argv, false);
}

static int cmd_instance_get_all(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_all(shell, BT_MESH_MODEL_ID_LIGHT_CTL_CLI);
}

static int cmd_instance_set(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t elem_idx = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	return shell_model_instance_set(shell, &mod, BT_MESH_MODEL_ID_LIGHT_CTL_CLI, elem_idx);
}

SHELL_STATIC_SUBCMD_SET_CREATE(instance_cmds,
			       SHELL_CMD_ARG(set, NULL, "<ElemIdx> ", cmd_instance_set, 2, 0),
			       SHELL_CMD_ARG(get-all, NULL, NULL, cmd_instance_get_all, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	ctl_cmds, SHELL_CMD_ARG(get, NULL, NULL, cmd_ctl_get, 1, 0),
	SHELL_CMD_ARG(set, NULL, "<Light> <Temp> <Dlt> [TransTime(ms) [Delay(ms)]]",
		      cmd_ctl_set, 4, 2),
	SHELL_CMD_ARG(set-unack, NULL, "<Light> <Temp> <Dlt> [TransTime(ms) [Delay(ms)]]",
		      cmd_ctl_set_unack, 4, 2),
	SHELL_CMD_ARG(temp-get, NULL, NULL, cmd_temp_get, 1, 0),
	SHELL_CMD_ARG(temp-set, NULL, "<Temp> <Dlt> [TransTime(ms) [Delay(ms)]]",
		      cmd_temp_set, 3, 2),
	SHELL_CMD_ARG(temp-set-unack, NULL, "<Temp> <Dlt> [TransTime(ms) [Delay(ms)]]",
		      cmd_temp_set_unack, 3, 2),
	SHELL_CMD_ARG(default-get, NULL, NULL, cmd_ctl_default_get, 1, 0),
	SHELL_CMD_ARG(default-set, NULL, "<Light> <Temp> <Dlt>", cmd_ctl_default_set, 4, 0),
	SHELL_CMD_ARG(default-set-unack, NULL, "<Light> <Temp> <Dlt>",
		      cmd_ctl_default_set_unack, 4, 0),
	SHELL_CMD_ARG(temp-range-get, NULL, NULL, cmd_temp_range_get, 1, 0),
	SHELL_CMD_ARG(temp-range-set, NULL, "<Min> <Max>", cmd_temp_range_set, 3, 0),
	SHELL_CMD_ARG(temp-range-set-unack, NULL, "<Min> <Max>", cmd_temp_range_set_unack,
		      3, 0),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", shell_model_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), ctl, &ctl_cmds, "Ctl Cli commands", shell_model_cmds_help, 1, 1);
