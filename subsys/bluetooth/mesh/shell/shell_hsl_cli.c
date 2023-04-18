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

static struct bt_mesh_model *mod;

static void hsl_print(const struct shell *shell, int err, struct bt_mesh_light_hsl_status *rsp)
{
	if (!err) {
		shell_print(shell,
			    "Current light: %d, current hue: %d "
			    "current saturation: %d, rem time: %d",
			    rsp->params.lightness, rsp->params.hue, rsp->params.saturation,
			    rsp->remaining_time);
	}
}

static int cmd_hsl_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_HSL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_hsl_cli *cli = mod->user_data;
	struct bt_mesh_light_hsl_status rsp;

	int err = bt_mesh_light_hsl_get(cli, NULL, &rsp);

	hsl_print(shell, err, &rsp);
	return err;
}

static int hsl_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t light = shell_strtoul(argv[1], 0, &err);
	uint16_t hue = shell_strtoul(argv[2], 0, &err);
	uint16_t saturation = shell_strtoul(argv[3], 0, &err);
	uint32_t time = (argc >= 5) ? shell_strtoul(argv[4], 0, &err) : 0;
	uint32_t delay = (argc == 6) ? shell_strtoul(argv[5], 0, &err) : 0;

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_HSL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_hsl_cli *cli = mod->user_data;
	struct bt_mesh_model_transition trans = { .time = time, .delay = delay };
	struct bt_mesh_light_hsl_params set = {
		.params = {
			.lightness = light,
			.hue = hue,
			.saturation = saturation,
		},
		.transition = (argc > 4) ? &trans : NULL,
	};

	if (acked) {
		struct bt_mesh_light_hsl_status rsp;

		err = bt_mesh_light_hsl_set(cli, NULL, &set, &rsp);
		hsl_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_light_hsl_set_unack(cli, NULL, &set);
	}
}

static int cmd_hsl_set(const struct shell *shell, size_t argc, char *argv[])
{
	return hsl_set(shell, argc, argv, true);
}

static int cmd_hsl_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return hsl_set(shell, argc, argv, false);
}

static int cmd_hsl_target_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_HSL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_hsl_cli *cli = mod->user_data;
	struct bt_mesh_light_hsl_status rsp;

	int err = bt_mesh_light_hsl_target_get(cli, NULL, &rsp);

	if (!err) {
		shell_print(shell,
			    "Target light: %d, target hue: %d "
			    "target saturation: %d, rem time: %d",
			    rsp.params.lightness, rsp.params.hue, rsp.params.saturation,
			    rsp.remaining_time);
	}

	return err;
}

static void default_print(const struct shell *shell, int err, struct bt_mesh_light_hsl *rsp)
{
	if (!err) {
		shell_print(shell,
			    "Default light: %d, default hue: %d "
			    "default saturation: %d",
			    rsp->lightness, rsp->hue, rsp->saturation);
	}
}

static int cmd_hsl_default_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_HSL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_hsl_cli *cli = mod->user_data;
	struct bt_mesh_light_hsl rsp;

	int err = bt_mesh_light_hsl_default_get(cli, NULL, &rsp);

	default_print(shell, err, &rsp);
	return err;
}

static int hsl_default_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t light = shell_strtoul(argv[1], 0, &err);
	uint16_t hue = shell_strtoul(argv[2], 0, &err);
	uint16_t saturation = shell_strtoul(argv[3], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_HSL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_hsl_cli *cli = mod->user_data;
	struct bt_mesh_light_hsl set = {
		.lightness = light,
		.hue = hue,
		.saturation = saturation,
	};

	if (acked) {
		struct bt_mesh_light_hsl rsp;

		err = bt_mesh_light_hsl_default_set(cli, NULL, &set, &rsp);
		default_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_light_hsl_default_set_unack(cli, NULL, &set);
	}
}

static int cmd_hsl_default_set(const struct shell *shell, size_t argc, char *argv[])
{
	return hsl_default_set(shell, argc, argv, true);
}

static int cmd_hsl_default_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return hsl_default_set(shell, argc, argv, false);
}

static void range_print(const struct shell *shell, int err,
			struct bt_mesh_light_hsl_range_status *rsp)
{
	if (!err) {
		shell_print(shell,
			    "Status: %d, hue min val: %d, hue max val: %d "
			    "saturation min val: %d, saturation max val: %d",
			    rsp->status_code, rsp->range.min.hue, rsp->range.max.hue,
			    rsp->range.min.saturation, rsp->range.max.saturation);
	}
}

static int cmd_hsl_range_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_HSL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_hsl_cli *cli = mod->user_data;
	struct bt_mesh_light_hsl_range_status rsp;

	int err = bt_mesh_light_hsl_range_get(cli, NULL, &rsp);

	range_print(shell, err, &rsp);
	return err;
}

static int hsl_range_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t hue_min = shell_strtoul(argv[1], 0, &err);
	uint16_t hue_max = shell_strtoul(argv[2], 0, &err);
	uint16_t sat_min = shell_strtoul(argv[3], 0, &err);
	uint16_t sat_max = shell_strtoul(argv[4], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_HSL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_hsl_cli *cli = mod->user_data;
	struct bt_mesh_light_hue_sat_range set = {
		.min.hue = hue_min,
		.max.hue = hue_max,
		.min.saturation = sat_min,
		.max.saturation = sat_max,
	};

	if (acked) {
		struct bt_mesh_light_hsl_range_status rsp;

		err = bt_mesh_light_hsl_range_set(cli, NULL, &set, &rsp);
		range_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_light_hsl_range_set_unack(cli, NULL, &set);
	}
}

static int cmd_hsl_range_set(const struct shell *shell, size_t argc, char *argv[])
{
	return hsl_range_set(shell, argc, argv, true);
}

static int cmd_hsl_range_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return hsl_range_set(shell, argc, argv, false);
}

static void hue_print(const struct shell *shell, int err, struct bt_mesh_light_hue_status *rsp)
{
	if (!err) {
		shell_print(shell, "Current level: %d, target level: %d, rem time: %d",
			    rsp->current, rsp->target, rsp->remaining_time);
	}
}

static int cmd_hue_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_HSL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_hsl_cli *cli = mod->user_data;
	struct bt_mesh_light_hue_status rsp;

	int err = bt_mesh_light_hue_get(cli, NULL, &rsp);

	hue_print(shell, err, &rsp);
	return err;
}

static int hue_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t lvl = shell_strtoul(argv[1], 0, &err);
	uint32_t time = (argc >= 3) ? shell_strtoul(argv[2], 0, &err) : 0;
	uint32_t delay = (argc == 4) ? shell_strtoul(argv[3], 0, &err) : 0;

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_HSL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_hsl_cli *cli = mod->user_data;
	struct bt_mesh_model_transition trans = { .time = time, .delay = delay };
	struct bt_mesh_light_hue set = { .lvl = lvl, .transition = (argc > 2) ? &trans : NULL };

	if (acked) {
		struct bt_mesh_light_hue_status rsp;

		err = bt_mesh_light_hue_set(cli, NULL, &set, &rsp);
		hue_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_light_hue_set_unack(cli, NULL, &set);
	}
}

static int cmd_hue_set(const struct shell *shell, size_t argc, char *argv[])
{
	return hue_set(shell, argc, argv, true);
}

static int cmd_hue_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return hue_set(shell, argc, argv, true);
}

static void saturation_print(const struct shell *shell, int err,
			     struct bt_mesh_light_sat_status *rsp)
{
	if (!err) {
		shell_print(shell, "Current val: %d, target val: %d, rem time: %d", rsp->current,
			    rsp->target, rsp->remaining_time);
	}
}

static int cmd_saturation_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_HSL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_hsl_cli *cli = mod->user_data;
	struct bt_mesh_light_sat_status rsp;

	int err = bt_mesh_light_saturation_get(cli, NULL, &rsp);

	saturation_print(shell, err, &rsp);
	return err;
}

static int saturation_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t lvl = shell_strtoul(argv[1], 0, &err);
	uint32_t time = (argc >= 3) ? shell_strtoul(argv[2], 0, &err) : 0;
	uint32_t delay = (argc == 4) ? shell_strtoul(argv[3], 0, &err) : 0;

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_HSL_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_hsl_cli *cli = mod->user_data;
	struct bt_mesh_model_transition trans = { .time = time, .delay = delay };
	struct bt_mesh_light_sat set = { .lvl = lvl, .transition = (argc > 2) ? &trans : NULL };

	if (acked) {
		struct bt_mesh_light_sat_status rsp;

		err = bt_mesh_light_saturation_set(cli, NULL, &set, &rsp);
		saturation_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_light_saturation_set_unack(cli, NULL, &set);
	}
}

static int cmd_saturation_set(const struct shell *shell, size_t argc, char *argv[])
{
	return saturation_set(shell, argc, argv, true);
}

static int cmd_saturation_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return saturation_set(shell, argc, argv, false);
}

static int cmd_instance_get_all(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_all(shell, BT_MESH_MODEL_ID_LIGHT_HSL_CLI);
}

static int cmd_instance_set(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t elem_idx = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	return shell_model_instance_set(shell, &mod, BT_MESH_MODEL_ID_LIGHT_HSL_CLI, elem_idx);
}

SHELL_STATIC_SUBCMD_SET_CREATE(instance_cmds,
			       SHELL_CMD_ARG(set, NULL, "<elem_idx> ", cmd_instance_set, 2, 0),
			       SHELL_CMD_ARG(get-all, NULL, NULL, cmd_instance_get_all, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	hsl_cmds, SHELL_CMD_ARG(get, NULL, NULL, cmd_hsl_get, 1, 0),
	SHELL_CMD_ARG(set, NULL, "<light> <hue> <saturation> [transition_time(ms) [delay(ms)]]",
		      cmd_hsl_set, 4, 2),
	SHELL_CMD_ARG(set-unack, NULL,
		      "<light> <hue> <saturation> [transition_time(ms) [delay(ms)]]",
		      cmd_hsl_set_unack, 4, 2),
	SHELL_CMD_ARG(target-get, NULL, NULL, cmd_hsl_target_get, 1, 0),
	SHELL_CMD_ARG(default-get, NULL, NULL, cmd_hsl_default_get, 1, 0),
	SHELL_CMD_ARG(default-set, NULL, "<light> <hue> <saturation>",
		      cmd_hsl_default_set, 4, 0),
	SHELL_CMD_ARG(default-set-unack, NULL, "<light> <hue> <saturation>",
		      cmd_hsl_default_set_unack, 4, 0),
	SHELL_CMD_ARG(range-get, NULL, NULL, cmd_hsl_range_get, 1, 0),
	SHELL_CMD_ARG(range-set, NULL, "<hue_min> <hue_max> <sat_min> <sat_max>",
		      cmd_hsl_range_set, 5, 0),
	SHELL_CMD_ARG(range-set-unack, NULL, "<hue_min> <hue_max> <sat_min> <sat_max>",
		      cmd_hsl_range_set_unack, 5, 0),
	SHELL_CMD_ARG(hue-get, NULL, NULL, cmd_hue_get, 1, 0),
	SHELL_CMD_ARG(hue-set, NULL, "<lvl> [transition_time(ms) [delay(ms)]]", cmd_hue_set, 2,
		      2),
	SHELL_CMD_ARG(hue-set-unack, NULL, "<lvl> [transition_time(ms) [delay(ms)]]",
		      cmd_hue_set_unack, 2, 2),
	SHELL_CMD_ARG(saturation-get, NULL, NULL, cmd_saturation_get, 1, 0),
	SHELL_CMD_ARG(saturation-set, NULL, "<lvl> [transition_time(ms) [delay(ms)]]",
		      cmd_saturation_set, 2, 2),
	SHELL_CMD_ARG(saturation-set-unack, NULL, "<lvl> [transition_time(ms) [delay(ms)]]",
		      cmd_saturation_set_unack, 2, 2),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", shell_model_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), hsl, &hsl_cmds, "HSL Cli commands", shell_model_cmds_help, 1, 1);
