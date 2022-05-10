/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <bluetooth/mesh/models.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/math_extras.h>

#include "mesh/net.h"
#include "mesh/access.h"
#include "../model_utils.h"

#include "shell_utils.h"

static struct bt_mesh_model *mod;
static struct bt_mesh_schedule_entry set_entry;

static int cmd_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SCHEDULER_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_scheduler_cli *cli = mod->user_data;
	uint16_t rsp;

	int err = bt_mesh_scheduler_cli_get(cli, NULL, &rsp);

	if (!err) {
		shell_print(shell, "Status: %d", rsp);
	}

	return err;
}

static void schedule_print(const struct shell *shell, int err, struct bt_mesh_schedule_entry *rsp)
{
	if (!err) {
		shell_print(shell,
			    "Day: %02d\nMonth: %02d\nYear: %02d\n"
			    "Hour: %d\nMinute: %d\nSecond:%d\n"
			    "Weekday: %d\nAction: %d\nTrans time: %d\nScene num: %d",
			    rsp->day, rsp->month, rsp->year, rsp->hour, rsp->minute, rsp->second,
			    rsp->day_of_week, rsp->action,
			    model_transition_decode(rsp->transition_time), rsp->scene_number);
	}
}

static int cmd_action_get(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t idx = (uint8_t)strtol(argv[1], NULL, 0);

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SCHEDULER_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_scheduler_cli *cli = mod->user_data;
	struct bt_mesh_schedule_entry rsp;

	int err = bt_mesh_scheduler_cli_action_get(cli, NULL, idx, &rsp);

	schedule_print(shell, err, &rsp);
	return err;
}

static int cmd_action_ctx_set(const struct shell *shell, size_t argc, char *argv[])
{
	set_entry.year = (uint8_t)strtol(argv[1], NULL, 0);
	set_entry.month = (uint8_t)strtol(argv[2], NULL, 0);
	set_entry.day = (uint8_t)strtol(argv[3], NULL, 0);
	set_entry.hour = (uint8_t)strtol(argv[4], NULL, 0);
	set_entry.minute = (uint8_t)strtol(argv[5], NULL, 0);
	set_entry.second = (uint8_t)strtol(argv[6], NULL, 0);
	set_entry.day_of_week = (uint8_t)strtol(argv[7], NULL, 0);
	set_entry.action = (enum bt_mesh_scheduler_action)strtol(argv[8], NULL, 0);
	set_entry.scene_number = (uint16_t)strtol(argv[10], NULL, 0);

	uint32_t transition_time = (uint32_t)strtol(argv[9], NULL, 0);

	set_entry.transition_time = model_transition_encode(transition_time);

	shell_print(shell, "Current action context:");
	schedule_print(shell, 0, &set_entry);
	return 0;
}

static int action_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	uint8_t idx = (uint8_t)strtol(argv[1], NULL, 0);

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SCHEDULER_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_scheduler_cli *cli = mod->user_data;

	if (acked) {
		struct bt_mesh_schedule_entry rsp;
		int err = bt_mesh_scheduler_cli_action_set(cli, NULL, idx, &set_entry, &rsp);

		schedule_print(shell, 0, &set_entry);
		return err;
	} else {
		return bt_mesh_scheduler_cli_action_set_unack(cli, NULL, idx, &set_entry);
	}
}

static int cmd_action_set(const struct shell *shell, size_t argc, char *argv[])
{
	return action_set(shell, argc, argv, true);
}

static int cmd_action_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return action_set(shell, argc, argv, false);
}

static int cmd_instance_get_all(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_all(shell, BT_MESH_MODEL_ID_SCHEDULER_CLI);
}

static int cmd_instance_set(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t elem_idx = (uint8_t)strtol(argv[1], NULL, 0);

	return shell_model_instance_set(shell, &mod, BT_MESH_MODEL_ID_SCHEDULER_CLI, elem_idx);
}

SHELL_STATIC_SUBCMD_SET_CREATE(instance_cmds,
			       SHELL_CMD_ARG(set, NULL, "<elem_idx> ", cmd_instance_set, 2, 0),
			       SHELL_CMD_ARG(get-all, NULL, NULL, cmd_instance_get_all, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(sched_cmds, SHELL_CMD_ARG(get, NULL, NULL, cmd_get, 1, 0),
			       SHELL_CMD_ARG(action-get, NULL, "<idx>", cmd_action_get, 2, 0),
			       SHELL_CMD_ARG(action-ctx-set, NULL,
					     "<year> <month> <day> <hour> "
					     "<minute> <second> <day_of_week> <action> "
					     "<transition_time> <scene_number>",
					     cmd_action_ctx_set, 11, 0),
			       SHELL_CMD_ARG(action-set, NULL, "<idx>", cmd_action_set, 2, 0),
			       SHELL_CMD_ARG(action-set-unack, NULL, "<idx>",
					     cmd_action_set_unack, 2, 0),
			       SHELL_CMD(instance, &instance_cmds, "Instance commands",
					 shell_model_cmds_help),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(mdl_sched, &sched_cmds, "Scheduler Cli commands", shell_model_cmds_help,
		       1, 1);
