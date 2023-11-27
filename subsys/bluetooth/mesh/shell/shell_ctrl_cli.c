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

static void mode_print(const struct shell *shell, int err, bool rsp)
{
	if (!err) {
		shell_print(shell, "Mode: %s", rsp ? "enabled" : "disabled");
	}
}

static int cmd_mode_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_LC_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;
	bool rsp;

	int err = bt_mesh_light_ctrl_cli_mode_get(cli, NULL, &rsp);

	mode_print(shell, err, rsp);
	return err;
}

static int mode_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	bool enabled = shell_strtobool(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_LC_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;

	if (acked) {
		bool rsp;

		err = bt_mesh_light_ctrl_cli_mode_set(cli, NULL, enabled, &rsp);
		mode_print(shell, err, rsp);
		return err;
	} else {
		return bt_mesh_light_ctrl_cli_mode_set_unack(cli, NULL, enabled);
	}
}

static int cmd_mode_set(const struct shell *shell, size_t argc, char *argv[])
{
	return mode_set(shell, argc, argv, true);
}

static int cmd_mode_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return mode_set(shell, argc, argv, false);
}

static void occ_enable_print(const struct shell *shell, int err, bool rsp)
{
	if (!err) {
		shell_print(shell, "Occupancy mode: %s", rsp ? "enabled" : "disabled");
	}
}

static int cmd_occupancy_enabled_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_LC_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;
	bool rsp;

	int err = bt_mesh_light_ctrl_cli_occupancy_enabled_get(cli, NULL, &rsp);

	occ_enable_print(shell, err, rsp);
	return err;
}

static int occupancy_enabled_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	bool enabled = shell_strtobool(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_LC_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;

	if (acked) {
		bool rsp;

		err = bt_mesh_light_ctrl_cli_occupancy_enabled_set(cli, NULL, enabled, &rsp);
		occ_enable_print(shell, err, rsp);
		return err;
	} else {
		return bt_mesh_light_ctrl_cli_occupancy_enabled_set_unack(cli, NULL, enabled);
	}
}

static int cmd_occupancy_enabled_set(const struct shell *shell, size_t argc, char *argv[])
{
	return occupancy_enabled_set(shell, argc, argv, true);
}

static int cmd_occupancy_enabled_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return occupancy_enabled_set(shell, argc, argv, false);
}

static void onoff_print(const struct shell *shell, int err, struct bt_mesh_onoff_status *rsp)
{
	if (!err) {
		shell_print(shell, "Present val: %d, target val: %d, rem time: %d",
			    rsp->present_on_off, rsp->target_on_off, rsp->remaining_time);
	}
}

static int cmd_light_onoff_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_LC_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;
	struct bt_mesh_onoff_status rsp;

	int err = bt_mesh_light_ctrl_cli_light_onoff_get(cli, NULL, &rsp);

	onoff_print(shell, err, &rsp);
	return err;
}

static int light_onoff_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	bool on_off = shell_strtobool(argv[1], 0, &err);
	uint32_t time = (argc >= 3) ? shell_strtoul(argv[2], 0, &err) : 0;
	uint32_t delay = (argc == 4) ? shell_strtoul(argv[3], 0, &err) : 0;

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_LC_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;
	struct bt_mesh_model_transition trans = { .time = time, .delay = delay };
	struct bt_mesh_onoff_set set = { .on_off = on_off,
					 .transition = (argc > 2) ? &trans : NULL };

	if (acked) {
		struct bt_mesh_onoff_status rsp;

		err = bt_mesh_light_ctrl_cli_light_onoff_set(cli, NULL, &set, &rsp);
		onoff_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_light_ctrl_cli_light_onoff_set_unack(cli, NULL, &set);
	}
}

static int cmd_light_onoff_set(const struct shell *shell, size_t argc, char *argv[])
{
	return light_onoff_set(shell, argc, argv, true);
}

static int cmd_light_onoff_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return light_onoff_set(shell, argc, argv, false);
}

static void prop_print(const struct shell *shell, int err, struct sensor_value *rsp)
{
	if (!err) {
		shell_fprintf(shell, SHELL_NORMAL, "Property value: ");
		shell_model_print_sensorval(shell, rsp);
		shell_print(shell, "");
	}
}

static int cmd_prop_get(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	enum bt_mesh_light_ctrl_prop id =
		(enum bt_mesh_light_ctrl_prop)shell_strtol(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_LC_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;
	struct sensor_value rsp;

	err = bt_mesh_light_ctrl_cli_prop_get(cli, NULL, id, &rsp);
	prop_print(shell, err, &rsp);
	return err;
}

static int prop_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	enum bt_mesh_light_ctrl_prop id =
		(enum bt_mesh_light_ctrl_prop)shell_strtol(argv[1], 0, &err);
	struct sensor_value set = shell_model_strtosensorval(argv[2], &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_LC_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;

	if (acked) {
		struct sensor_value rsp;
		err = bt_mesh_light_ctrl_cli_prop_set(cli, NULL, id, &set, &rsp);

		prop_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_light_ctrl_cli_prop_set_unack(cli, NULL, id, &set);
	}
}

static int cmd_prop_set(const struct shell *shell, size_t argc, char *argv[])
{
	return prop_set(shell, argc, argv, true);
}

static int cmd_prop_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return prop_set(shell, argc, argv, false);
}

static void coeff_print(const struct shell *shell, int err, float rsp)
{
	if (!err) {
		shell_print(shell, "Regulator Coefficient: %f", rsp);
	}
}

static int cmd_coeff_get(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	enum bt_mesh_light_ctrl_coeff id =
		(enum bt_mesh_light_ctrl_coeff)shell_strtol(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_LC_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;
	float rsp;

	err = bt_mesh_light_ctrl_cli_coeff_get(cli, NULL, id, &rsp);
	coeff_print(shell, err, rsp);
	return err;
}

static int coeff_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	enum bt_mesh_light_ctrl_coeff id =
		(enum bt_mesh_light_ctrl_coeff)shell_strtol(argv[1], 0, &err);
	float val = shell_model_strtodbl(argv[2], &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_LIGHT_LC_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;

	if (acked) {
		float rsp;

		err = bt_mesh_light_ctrl_cli_coeff_set(cli, NULL, id, val, &rsp);
		coeff_print(shell, err, rsp);
		return err;
	} else {
		return bt_mesh_light_ctrl_cli_coeff_set_unack(cli, NULL, id, val);
	}
}

static int cmd_coeff_set(const struct shell *shell, size_t argc, char *argv[])
{
	return coeff_set(shell, argc, argv, true);
}

static int cmd_coeff_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return coeff_set(shell, argc, argv, false);
}

static int cmd_instance_get_all(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_all(shell, BT_MESH_MODEL_ID_LIGHT_LC_CLI);
}

static int cmd_instance_set(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t elem_idx = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	return shell_model_instance_set(shell, &mod, BT_MESH_MODEL_ID_LIGHT_LC_CLI, elem_idx);
}

SHELL_STATIC_SUBCMD_SET_CREATE(instance_cmds,
			       SHELL_CMD_ARG(set, NULL, "<ElemIdx> ", cmd_instance_set, 2, 0),
			       SHELL_CMD_ARG(get-all, NULL, NULL, cmd_instance_get_all, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	lc_cmds, SHELL_CMD_ARG(mode-get, NULL, NULL, cmd_mode_get, 1, 0),
	SHELL_CMD_ARG(mode-set, NULL, "Enable(off, on)", cmd_mode_set, 2, 0),
	SHELL_CMD_ARG(mode-set-unack, NULL, "Enable(off, on)", cmd_mode_set_unack, 2, 0),
	SHELL_CMD_ARG(occupancy-enabled-get, NULL, NULL, cmd_occupancy_enabled_get, 1, 0),
	SHELL_CMD_ARG(occupancy-enabled-set, NULL, "Enable(off, on)",
		      cmd_occupancy_enabled_set, 2, 0),
	SHELL_CMD_ARG(occupancy-enabled-set-unack, NULL, "Enable(off, on)",
		      cmd_occupancy_enabled_set_unack, 2, 0),
	SHELL_CMD_ARG(light-onoff-get, NULL, NULL, cmd_light_onoff_get, 1, 0),
	SHELL_CMD_ARG(light-onoff-set, NULL, "<OnOff> [TransTime(ms) [Delay(ms)]]",
		      cmd_light_onoff_set, 2, 2),
	SHELL_CMD_ARG(light-onoff-set-unack, NULL, "<OnOff> [TransTime(ms) [Delay(ms)]]",
		      cmd_light_onoff_set_unack, 2, 2),
	SHELL_CMD_ARG(prop-get, NULL, "<ID>", cmd_prop_get, 2, 0),
	SHELL_CMD_ARG(prop-set, NULL, "<ID> <Val>", cmd_prop_set, 3, 0),
	SHELL_CMD_ARG(prop-set-unack, NULL, "<ID> <Val>", cmd_prop_set_unack, 3, 0),
	SHELL_CMD_ARG(coeff-get, NULL, "<ID>", cmd_coeff_get, 2, 0),
	SHELL_CMD_ARG(coeff-set, NULL, "<ID> <Val>", cmd_coeff_set, 3, 0),
	SHELL_CMD_ARG(coeff-set-unack, NULL, "<ID> <Val>", cmd_coeff_set_unack, 3, 0),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", shell_model_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), ctrl, &lc_cmds, "Light lightness control Cli commands",
		 shell_model_cmds_help, 1, 1);
