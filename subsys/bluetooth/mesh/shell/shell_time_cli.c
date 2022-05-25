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

static void time_print(const struct shell *shell, int err, struct bt_mesh_time_status *rsp)
{
	if (!err) {
		shell_print(shell,
			    "Sec: %lld, subsec: %d, uncertainty: %lld, "
			    "tai utc delta: %d, time zone offset: %d, is authority: %d",
			    (uint64_t)rsp->tai.sec, rsp->tai.subsec, rsp->uncertainty,
			    rsp->tai_utc_delta, rsp->time_zone_offset, rsp->is_authority);
	}
}

static int cmd_time_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_TIME_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_time_cli *cli = mod->user_data;
	struct bt_mesh_time_status rsp;

	int err = bt_mesh_time_cli_time_get(cli, NULL, &rsp);

	time_print(shell, err, &rsp);
	return err;
}

static int cmd_time_set(const struct shell *shell, size_t argc, char *argv[])
{
	uint64_t sec = (uint64_t)strtoul(argv[1], NULL, 0);
	uint8_t subsec = (uint8_t)strtol(argv[2], NULL, 0);
	uint64_t uncertainty = (uint64_t)strtoul(argv[3], NULL, 0);
	int16_t tai_utc_delta = (int16_t)strtol(argv[4], NULL, 0);
	int16_t time_zone_offset = (int16_t)strtol(argv[5], NULL, 0);
	bool is_authority = (bool)strtol(argv[6], NULL, 0);

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_TIME_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_time_cli *cli = mod->user_data;
	struct bt_mesh_time_status set = { .tai.sec = sec,
					   .tai.subsec = subsec,
					   .uncertainty = uncertainty,
					   .tai_utc_delta = tai_utc_delta,
					   .time_zone_offset = time_zone_offset,
					   .is_authority = is_authority };

	struct bt_mesh_time_status rsp;
	int err = bt_mesh_time_cli_time_set(cli, NULL, &set, &rsp);

	time_print(shell, err, &rsp);
	return err;
}

static void zone_print(const struct shell *shell, int err, struct bt_mesh_time_zone_status *rsp)
{
	if (!err) {
		shell_print(shell, "Current offset: %d, new offset: %d, timestamp: %lld",
			    rsp->current_offset, rsp->time_zone_change.new_offset,
			    rsp->time_zone_change.timestamp);
	}
}

static int cmd_zone_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_TIME_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_time_cli *cli = mod->user_data;
	struct bt_mesh_time_zone_status rsp;

	int err = bt_mesh_time_cli_zone_get(cli, NULL, &rsp);

	zone_print(shell, err, &rsp);
	return err;
}

static int cmd_zone_set(const struct shell *shell, size_t argc, char *argv[])
{
	int16_t new_offset = (int16_t)strtol(argv[1], NULL, 0);
	uint64_t ts = (uint64_t)strtoul(argv[2], NULL, 0);

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_TIME_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_time_cli *cli = mod->user_data;
	struct bt_mesh_time_zone_change set = { .new_offset = new_offset, .timestamp = ts };

	struct bt_mesh_time_zone_status rsp;
	int err = bt_mesh_time_cli_zone_set(cli, NULL, &set, &rsp);

	zone_print(shell, err, &rsp);
	return err;
}

static void tai_utc_print(const struct shell *shell, int err,
			  struct bt_mesh_time_tai_utc_delta_status *rsp)
{
	if (!err) {
		shell_print(shell, "Delta current: %d, delta new: %d, timestamp: %lld",
			    rsp->delta_current, rsp->tai_utc_change.delta_new,
			    rsp->tai_utc_change.timestamp);
	}
}

static int cmd_tai_utc_delta_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_TIME_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_time_cli *cli = mod->user_data;
	struct bt_mesh_time_tai_utc_delta_status rsp;

	int err = bt_mesh_time_cli_tai_utc_delta_get(cli, NULL, &rsp);

	tai_utc_print(shell, err, &rsp);
	return err;
}

static int cmd_tai_utc_delta_set(const struct shell *shell, size_t argc, char *argv[])
{
	int16_t delta_new = (int16_t)strtol(argv[1], NULL, 0);
	uint64_t ts = (uint64_t)strtoul(argv[2], NULL, 0);

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_TIME_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_time_cli *cli = mod->user_data;
	struct bt_mesh_time_tai_utc_change set = { .delta_new = delta_new, .timestamp = ts };

	struct bt_mesh_time_tai_utc_delta_status rsp;
	int err = bt_mesh_time_cli_tai_utc_delta_set(cli, NULL, &set, &rsp);

	tai_utc_print(shell, err, &rsp);
	return err;
}

static void role_print(const struct shell *shell, int err, uint8_t rsp)
{
	if (!err) {
		shell_print(shell, "Status: %d", rsp);
	}
}

static int cmd_role_get(const struct shell *shell, size_t argc, char *argv[])
{
	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_TIME_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_time_cli *cli = mod->user_data;
	uint8_t rsp;

	int err = bt_mesh_time_cli_role_get(cli, NULL, &rsp);

	role_print(shell, err, rsp);
	return err;
}

static int cmd_role_set(const struct shell *shell, size_t argc, char *argv[])
{
	enum bt_mesh_time_role role = (enum bt_mesh_time_role)strtol(argv[1], NULL, 0);

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_TIME_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_time_cli *cli = mod->user_data;
	uint8_t rsp;

	int err = bt_mesh_time_cli_role_set(cli, NULL, &role, &rsp);

	role_print(shell, err, rsp);
	return err;
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

SHELL_STATIC_SUBCMD_SET_CREATE(
	time_cmds, SHELL_CMD_ARG(time-get, NULL, NULL, cmd_time_get, 1, 0),
	SHELL_CMD_ARG(time-set, NULL,
		      "<sec> <subsec> <uncertainty> <tai_utc_delta> "
		      "<time_zone_offset> <is_authority>",
		      cmd_time_set, 7, 0),
	SHELL_CMD_ARG(zone-get, NULL, NULL, cmd_zone_get, 1, 0),
	SHELL_CMD_ARG(zone-set, NULL, "<new_offset> <timestamp>", cmd_zone_set, 3, 0),
	SHELL_CMD_ARG(tai-utc-delta-get, NULL, NULL, cmd_tai_utc_delta_get, 1, 0),
	SHELL_CMD_ARG(tai-utc-delta-set, NULL, "<delta_new> <timestamp>",
		      cmd_tai_utc_delta_set, 3, 0),
	SHELL_CMD_ARG(role-get, NULL, NULL, cmd_role_get, 1, 0),
	SHELL_CMD_ARG(role-set, NULL, "<role>", cmd_role_set, 2, 0),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", shell_model_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(mdl_time, &time_cmds, "Time Cli commands", shell_model_cmds_help, 1, 1);
