/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <bluetooth/mesh/models.h>
#include <zephyr/shell/shell.h>
#include <bluetooth/mesh/vnd/dm_cli.h>

#include "mesh/net.h"
#include "mesh/access.h"
#include "shell_utils.h"

static const struct bt_mesh_model *mod;

static int cmd_cfg(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc != 1 && argc != 4) {
		return -EINVAL;
	}

	int err = 0;
	struct bt_mesh_dm_cfg set = { 0 };

	if (argc == 4) {
		set.ttl = shell_strtoul(argv[1], 0, &err);
		set.timeout = shell_strtoul(argv[2], 0, &err);
		set.delay = shell_strtoul(argv[3], 0, &err);
	}

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_vnd_model_first_get(BT_MESH_VENDOR_COMPANY_ID,
					       BT_MESH_MODEL_ID_DM_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dm_cli *cli = mod->user_data;
	struct bt_mesh_dm_cli_cfg_status rsp;

	err = bt_mesh_dm_cli_config(cli, NULL, (argc == 1) ? NULL : &set, &rsp);

	if (!err) {
		shell_print(shell,
			    "Status: %s, In progress?: %s, Entry count: %d, "
			    "TTL: %d, Timeout: %d, Delay: %d",
			    rsp.status ? "Fail" : "Success", rsp.is_in_progress ? "Yes" : "No",
			    rsp.result_entry_cnt, rsp.def.ttl, rsp.def.timeout, rsp.def.delay);
	}

	return err;
}

static void result_print(const struct shell *shell,
			 struct bt_mesh_dm_cli_results *rsp)
{
	shell_print(shell, "Status: %s (err: %d)", rsp->status ? "Fail" : "Success", rsp->status);

	if (!rsp->status) {
		for (int i = 0; i < rsp->entry_cnt; i++) {
			shell_print(shell, "\nEntry number: %d", i + 1);
			shell_print(shell, "\tMode: %s",
				    rsp->res[i].mode == DM_RANGING_MODE_RTT ? "RTT" : "MCPD");
			shell_print(shell, "\tQuality: %d", rsp->res[i].quality);
			shell_print(shell, "\tError Occurred: %d", rsp->res[i].err_occurred);
			shell_print(shell, "\tAddress: 0X%04x", rsp->res[i].addr);

			if (rsp->res[i].mode == DM_RANGING_MODE_RTT) {
				shell_print(shell, "\tRTT distance (cm): %d", rsp->res[i].res.rtt);
			} else {
				shell_print(shell, "\tBest distance (cm): %d",
					    rsp->res[i].res.mcpd.best);
				shell_print(shell, "\tIFFT distance (cm): %d",
					    rsp->res[i].res.mcpd.ifft);
				shell_print(shell, "\tPhase slope distance (cm): %d",
					    rsp->res[i].res.mcpd.phase_slope);
				shell_print(shell, "\tRSSI open space distance (cm): %d",
					    rsp->res[i].res.mcpd.rssi_openspace);
			}
		}
	}
}

static int cmd_dm_start(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc != 3 && argc != 4 && argc != 7) {
		return -EINVAL;
	}

	int err = 0;
	struct bt_mesh_dm_cli_start set = {.reuse_transaction = false, .cfg = NULL };
	struct bt_mesh_dm_cfg cfg = { 0 };

	set.mode = shell_strtoul(argv[1], 0, &err);
	set.addr = shell_strtoul(argv[2], 0, &err);

	if (argc >= 4) {
		set.reuse_transaction = shell_strtobool(argv[3], 0, &err);
	}

	if (argc == 7) {
		cfg.ttl = shell_strtoul(argv[4], 0, &err);
		cfg.timeout = shell_strtoul(argv[5], 0, &err);
		cfg.delay = shell_strtoul(argv[6], 0, &err);
		set.cfg = &cfg;
	}

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_vnd_model_first_get(BT_MESH_VENDOR_COMPANY_ID,
					       BT_MESH_MODEL_ID_DM_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dm_cli *cli = mod->user_data;
	struct bt_mesh_dm_cli_results rsp;

	err = bt_mesh_dm_cli_measurement_start(cli, NULL, &set, &rsp);

	if (!err) {
		result_print(shell, &rsp);
	}

	return err;
}

static int cmd_result_get(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t count = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_vnd_model_first_get(BT_MESH_VENDOR_COMPANY_ID,
					       BT_MESH_MODEL_ID_DM_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_dm_cli *cli = mod->user_data;
	struct bt_mesh_dm_cli_results rsp;

	err = bt_mesh_dm_cli_results_get(cli, NULL, count, &rsp);

	if (!err) {
		result_print(shell, &rsp);
	}

	return err;
}

static int cmd_instance_get_all(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_vnd_model_instances_get_all(shell, BT_MESH_MODEL_ID_DM_CLI,
						 BT_MESH_VENDOR_COMPANY_ID);
}

static int cmd_instance_set(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t elem_idx = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	return shell_vnd_model_instance_set(shell, &mod, BT_MESH_MODEL_ID_DM_CLI,
					    BT_MESH_VENDOR_COMPANY_ID, elem_idx);
}

SHELL_STATIC_SUBCMD_SET_CREATE(instance_cmds,
			       SHELL_CMD_ARG(set, NULL, "<ElemIdx> ", cmd_instance_set, 2, 0),
			       SHELL_CMD_ARG(get-all, NULL, NULL, cmd_instance_get_all, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	dm_cmds,
	SHELL_CMD_ARG(cfg, NULL, "[<TTL> <Timeout(100ms steps)> <Delay(us)>]", cmd_cfg, 1, 3),
	SHELL_CMD_ARG(
		start, NULL,
		"<Mode> <Addr> [<ReuseTransaction> [<TTL> <Timeout(100ms steps)> <Delay(us)>]]",
		cmd_dm_start, 3, 4),
	SHELL_CMD_ARG(result - get, NULL, "<EntryCnt>", cmd_result_get, 2, 0),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", shell_model_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), dm, &dm_cmds, "Distance measurement Cli commands",
		 shell_model_cmds_help, 1, 1);
