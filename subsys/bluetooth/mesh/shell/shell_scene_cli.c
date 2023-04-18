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

#define SHELL_SENSOR_CLI_REG_CNT_MAX 32

static struct bt_mesh_model *mod;

static void scene_get_print(const struct shell *shell, int err, struct bt_mesh_scene_state *rsp)
{
	if (!err) {
		shell_print(shell,
			    "Status: %d, current scene: %d "
			    "target scene: %d, rem time: %d",
			    rsp->status, rsp->current, rsp->target,
			    rsp->remaining_time);
	}
}

static int cmd_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SCENE_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_scene_cli *cli = mod->user_data;
	struct bt_mesh_scene_state rsp;

	int err = bt_mesh_scene_cli_get(cli, NULL, &rsp);

	scene_get_print(shell, err, &rsp);
	return err;
}

static void scene_print(const struct shell *shell, int err, struct bt_mesh_scene_register *rsp)
{
	if (!err) {
		shell_print(shell, "Status: %d, current scene: %d", rsp->status, rsp->current);

		for (int i = 0; i < rsp->count; i++) {
			shell_print(shell, "Scene %d: %d", i, rsp->scenes[i]);
		}
	}
}

static int cmd_register_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SCENE_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_scene_cli *cli = mod->user_data;
	uint16_t scenes[SHELL_SENSOR_CLI_REG_CNT_MAX];
	struct bt_mesh_scene_register rsp = {
		.count = SHELL_SENSOR_CLI_REG_CNT_MAX,
		.scenes = &scenes[0],
	};

	int err = bt_mesh_scene_cli_register_get(cli, NULL, &rsp);

	scene_print(shell, err, &rsp);
	return err;
}

static int store(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t scene = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SCENE_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_scene_cli *cli = mod->user_data;

	if (acked) {
		uint16_t scenes[CONFIG_BT_MESH_SCENES_MAX];
		struct bt_mesh_scene_register rsp = {
			.count = CONFIG_BT_MESH_SCENES_MAX,
			.scenes = &scenes[0],
		};

		err = bt_mesh_scene_cli_store(cli, NULL, scene, &rsp);
		scene_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_scene_cli_store_unack(cli, NULL, scene);
	}
}

static int cmd_store(const struct shell *shell, size_t argc, char *argv[])
{
	return store(shell, argc, argv, true);
}

static int cmd_store_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return store(shell, argc, argv, false);
}

static int scene_del(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t scene = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SCENE_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_scene_cli *cli = mod->user_data;

	if (acked) {
		uint16_t scenes[CONFIG_BT_MESH_SCENES_MAX];
		struct bt_mesh_scene_register rsp = {
			.count = CONFIG_BT_MESH_SCENES_MAX,
			.scenes = &scenes[0],
		};

		err = bt_mesh_scene_cli_delete(cli, NULL, scene, &rsp);
		scene_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_scene_cli_delete_unack(cli, NULL, scene);
	}
}

static int cmd_delete(const struct shell *shell, size_t argc, char *argv[])
{
	return scene_del(shell, argc, argv, true);
}

static int cmd_delete_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return scene_del(shell, argc, argv, false);
}

static int recall(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t scene = shell_strtoul(argv[1], 0, &err);
	uint32_t time = (argc >= 3) ? shell_strtoul(argv[2], 0, &err) : 0;
	uint32_t delay = (argc == 4) ? shell_strtoul(argv[3], 0, &err) : 0;

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SCENE_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_scene_cli *cli = mod->user_data;
	struct bt_mesh_model_transition trans = { .time = time, .delay = delay };

	if (acked) {
		struct bt_mesh_scene_state rsp;

		err = bt_mesh_scene_cli_recall(cli, NULL, scene, (argc > 2) ? &trans : NULL, &rsp);
		scene_get_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_scene_cli_recall_unack(cli, NULL, scene, (argc > 2) ? &trans : NULL);
	}
}

static int cmd_recall(const struct shell *shell, size_t argc, char *argv[])
{
	return recall(shell, argc, argv, true);
}

static int cmd_recall_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return recall(shell, argc, argv, false);
}

static int cmd_instance_get_all(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_all(shell, BT_MESH_MODEL_ID_SCENE_CLI);
}

static int cmd_instance_set(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t elem_idx = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	return shell_model_instance_set(shell, &mod, BT_MESH_MODEL_ID_SCENE_CLI, elem_idx);
}

SHELL_STATIC_SUBCMD_SET_CREATE(instance_cmds,
			       SHELL_CMD_ARG(set, NULL, "<elem_idx> ", cmd_instance_set, 2, 0),
			       SHELL_CMD_ARG(get-all, NULL, NULL, cmd_instance_get_all, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	scene_cmds, SHELL_CMD_ARG(get, NULL, NULL, cmd_get, 1, 0),
	SHELL_CMD_ARG(register-get, NULL, NULL, cmd_register_get, 1, 0),
	SHELL_CMD_ARG(store, NULL, "<scene>", cmd_store, 2, 0),
	SHELL_CMD_ARG(store-unack, NULL, "<scene>", cmd_store_unack, 2, 0),
	SHELL_CMD_ARG(delete, NULL, "<scene>", cmd_delete, 2, 0),
	SHELL_CMD_ARG(delete-unack, NULL, "<scene>", cmd_delete_unack, 2, 0),
	SHELL_CMD_ARG(recall, NULL, "<scene> [transition_time(ms) [delay(ms)]]", cmd_recall, 2, 2),
	SHELL_CMD_ARG(recall-unack, NULL, "<scene> [transition_time(ms) [delay(ms)]]",
		      cmd_recall_unack, 2, 2),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", shell_model_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), scene, &scene_cmds, "Scene Cli commands", shell_model_cmds_help, 1,
		 1);
