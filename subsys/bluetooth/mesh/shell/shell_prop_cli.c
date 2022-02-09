/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <bluetooth/mesh/models.h>
#include <shell/shell.h>

#include "mesh/net.h"
#include "mesh/access.h"
#include "shell_utils.h"
#include "common/log.h"

static struct bt_mesh_model *mod;

static int shell_prop_client_props_get(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t id = (uint16_t)strtol(argv[1], NULL, 0);

	if (!mod) {
		if (!shell_model_first_get(BT_MESH_MODEL_ID_GEN_PROP_CLI, &mod)) {
			return -ENODEV;
		}
	}

	struct bt_mesh_prop_cli *cli = mod->user_data;
	struct bt_mesh_prop_list rsp;

	int err = bt_mesh_prop_cli_client_props_get(cli, NULL, id, &rsp);

	if (!err) {
		shell_print(shell, "ID count: %d", rsp.count);
		for (size_t i = 0; i < rsp.count; i++) {
			shell_print(shell, "ID %d val: %d", i, rsp.ids[i]);
		}
	}

	return err;
}

static int shell_prop_props_get(const struct shell *shell, size_t argc, char *argv[])
{
	enum bt_mesh_prop_srv_kind kind = (enum bt_mesh_prop_srv_kind)strtol(argv[1], NULL, 0);

	if (!mod) {
		if (!shell_model_first_get(BT_MESH_MODEL_ID_GEN_PROP_CLI, &mod)) {
			return -ENODEV;
		}
	}

	struct bt_mesh_prop_cli *cli = mod->user_data;
	struct bt_mesh_prop_list rsp;

	int err = bt_mesh_prop_cli_props_get(cli, NULL, kind, &rsp);

	if (!err) {
		shell_print(shell, "ID count: %d", rsp.count);
		for (size_t i = 0; i < rsp.count; i++) {
			shell_print(shell, "ID %d val: %d", i, rsp.ids[i]);
		}
	}

	return err;
}

static int shell_prop_prop_get(const struct shell *shell, size_t argc, char *argv[])
{
	enum bt_mesh_prop_srv_kind kind = (enum bt_mesh_prop_srv_kind)strtol(argv[1], NULL, 0);
	uint16_t id = (uint16_t)strtol(argv[2], NULL, 0);

	if (!mod) {
		if (!shell_model_first_get(BT_MESH_MODEL_ID_GEN_PROP_CLI, &mod)) {
			return -ENODEV;
		}
	}

	struct bt_mesh_prop_cli *cli = mod->user_data;
	struct bt_mesh_prop_val rsp;

	int err = bt_mesh_prop_cli_prop_get(cli, NULL, kind, id, &rsp);

	if (!err) {
		shell_print(shell, "Property value: %s", bt_hex(rsp.value, rsp.size));
	}

	return err;
}

static int user_prop_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	uint16_t id = (uint16_t)strtol(argv[1], NULL, 0);
	uint8_t val[CONFIG_BT_MESH_PROP_MAXSIZE] = { 0 };
	uint16_t i;

	for (i = 0; i < CONFIG_BT_MESH_PROP_MAXSIZE; i++) {
		if (argc == (2 + i)) {
			break;
		}

		val[i] = (uint8_t)strtol(argv[2 + i], NULL, 0);
	}

	if (!mod) {
		if (!shell_model_first_get(BT_MESH_MODEL_ID_GEN_PROP_CLI, &mod)) {
			return -ENODEV;
		}
	}

	struct bt_mesh_prop_cli *cli = mod->user_data;
	struct bt_mesh_prop_val set = {
		.meta.id = id,
		.value = val,
		.size = i,
	};

	if (acked) {
		struct bt_mesh_prop_val rsp;
		int err = bt_mesh_prop_cli_user_prop_set(cli, NULL, &set, &rsp);

		if (!err) {
			shell_print(shell, "Property value: %s", bt_hex(rsp.value, rsp.size));
		}

		return err;
	} else {
		return bt_mesh_prop_cli_user_prop_set_unack(cli, NULL, &set);
	}
}

static int shell_prop_user_prop_set(const struct shell *shell, size_t argc, char *argv[])
{
	return user_prop_set(shell, argc, argv, true);
}

static int shell_prop_user_prop_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return user_prop_set(shell, argc, argv, false);
}

static int admin_prop_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	uint16_t id = (uint16_t)strtol(argv[1], NULL, 0);
	enum bt_mesh_prop_access access = (enum bt_mesh_prop_access)strtol(argv[2], NULL, 0);
	uint8_t val[CONFIG_BT_MESH_PROP_MAXSIZE] = { 0 };
	uint16_t i;

	for (i = 0; i < CONFIG_BT_MESH_PROP_MAXSIZE; i++) {
		if (argc == (3 + i)) {
			break;
		}

		val[i] = (uint8_t)strtol(argv[3 + i], NULL, 0);
	}

	if (!mod) {
		if (!shell_model_first_get(BT_MESH_MODEL_ID_GEN_PROP_CLI, &mod)) {
			return -ENODEV;
		}
	}

	struct bt_mesh_prop_cli *cli = mod->user_data;
	struct bt_mesh_prop_val set = {
		.meta.id = id,
		.meta.user_access = access,
		.value = val,
		.size = i,
	};

	if (acked) {
		struct bt_mesh_prop_val rsp;
		int err = bt_mesh_prop_cli_admin_prop_set(cli, NULL, &set, &rsp);

		if (!err) {
			shell_print(shell, "Property value: %s", bt_hex(rsp.value, rsp.size));
		}

		return err;
	} else {
		return bt_mesh_prop_cli_admin_prop_set_unack(cli, NULL, &set);
	}
}

static int shell_prop_admin_prop_set(const struct shell *shell, size_t argc, char *argv[])
{
	return admin_prop_set(shell, argc, argv, true);
}

static int shell_prop_admin_prop_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return admin_prop_set(shell, argc, argv, false);
}

static int mfr_prop_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	uint16_t id = (uint16_t)strtol(argv[1], NULL, 0);
	enum bt_mesh_prop_access access = (enum bt_mesh_prop_access)strtol(argv[3], NULL, 0);

	if (!mod) {
		if (!shell_model_first_get(BT_MESH_MODEL_ID_GEN_PROP_CLI, &mod)) {
			return -ENODEV;
		}
	}

	struct bt_mesh_prop_cli *cli = mod->user_data;
	struct bt_mesh_prop set = {
		.id = id,
		.user_access = access,
	};

	if (acked) {
		struct bt_mesh_prop_val rsp;
		int err = bt_mesh_prop_cli_mfr_prop_set(cli, NULL, &set, &rsp);

		if (!err) {
			shell_print(shell, "Property value: %s", bt_hex(rsp.value, rsp.size));
		}

		return err;
	} else {
		return bt_mesh_prop_cli_mfr_prop_set_unack(cli, NULL, &set);
	}
}

static int shell_prop_mfr_prop_set(const struct shell *shell, size_t argc, char *argv[])
{
	return mfr_prop_set(shell, argc, argv, true);
}

static int shell_prop_mfr_prop_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return mfr_prop_set(shell, argc, argv, false);
}

static int shell_instance_get_all(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_all(shell, BT_MESH_MODEL_ID_GEN_PROP_CLI);
}

static int shell_instance_get_curr(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_curr(shell, mod);
}

static int shell_instance_set(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t elem_idx = (uint8_t)strtol(argv[1], NULL, 0);

	return shell_model_instance_set(shell, &mod, BT_MESH_MODEL_ID_GEN_PROP_CLI, elem_idx);
}

SHELL_STATIC_SUBCMD_SET_CREATE(instance_cmds,
			       SHELL_CMD_ARG(set, NULL, "<elem_idx> ", shell_instance_set, 2, 0),
			       SHELL_CMD_ARG(get-curr, NULL, NULL, shell_instance_get_curr, 1, 0),
			       SHELL_CMD_ARG(get-all, NULL, NULL, shell_instance_get_all, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	prop_cmds,
	SHELL_CMD_ARG(cli-props-get, NULL, "<id>", shell_prop_client_props_get, 2, 0),
	SHELL_CMD_ARG(props-get, NULL, "<kind>", shell_prop_props_get, 2, 0),
	SHELL_CMD_ARG(prop-get, NULL, "<kind> <id>", shell_prop_prop_get, 3, 0),
	SHELL_CMD_ARG(user-prop-set, NULL, "<id> <val_0> [val_1 [val_2 ... [val_max]]",
		      shell_prop_user_prop_set, 3, (CONFIG_BT_MESH_PROP_MAXSIZE - 1)),
	SHELL_CMD_ARG(user-prop-set-unack, NULL, "<id> <val_0> [val_1 [val_2 ... [val_max]]",
		      shell_prop_user_prop_set_unack, 3, (CONFIG_BT_MESH_PROP_MAXSIZE - 1)),
	SHELL_CMD_ARG(admin-prop-set, NULL,
		      "<id> <access> <val_0> [val_1 [val_2 ... [val_max]]",
		      shell_prop_admin_prop_set, 4, (CONFIG_BT_MESH_PROP_MAXSIZE - 1)),
	SHELL_CMD_ARG(admin-prop-set-unack, NULL,
		      "<id> <access> <val_0> [val_1 [val_2 ... [val_max]]",
		      shell_prop_admin_prop_set_unack, 4, (CONFIG_BT_MESH_PROP_MAXSIZE - 1)),
	SHELL_CMD_ARG(mfr-prop-set, NULL, "<id> <access>", shell_prop_mfr_prop_set, 3, 0),
	SHELL_CMD_ARG(mfr-prop-set-unack, NULL, "<id> <access>",
		      shell_prop_mfr_prop_set_unack, 3, 0),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", shell_model_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(mesh_prop, &prop_cmds, "Property Cli commands", shell_model_cmds_help, 1, 1);
