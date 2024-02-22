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

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_shell_prop_cli);

#include "common/bt_str.h"

static const struct bt_mesh_model *mod;

static void props_print(const struct shell *shell, int err, struct bt_mesh_prop_list *rsp)
{
	if (!err) {
		shell_print(shell, "ID count: %d", rsp->count);
		for (uint8_t i = 0; i < rsp->count; i++) {
			shell_print(shell, "ID %d val: %d", i, rsp->ids[i]);
		}
	}
}

static int cmd_prop_client_props_get(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint16_t id = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_PROP_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_prop_cli *cli = mod->rt->user_data;
	uint16_t ids[CONFIG_BT_MESH_PROP_MAXCOUNT];
	struct bt_mesh_prop_list rsp = { .ids = &ids[0], .count = CONFIG_BT_MESH_PROP_MAXCOUNT };

	err = bt_mesh_prop_cli_client_props_get(cli, NULL, id, &rsp);
	props_print(shell, err, &rsp);
	return err;
}

static int cmd_prop_props_get(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	enum bt_mesh_prop_srv_kind kind =
		(enum bt_mesh_prop_srv_kind)shell_strtol(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_PROP_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_prop_cli *cli = mod->rt->user_data;
	uint16_t ids[CONFIG_BT_MESH_PROP_MAXCOUNT];
	struct bt_mesh_prop_list rsp = { .ids = &ids[0], .count = CONFIG_BT_MESH_PROP_MAXCOUNT };

	err = bt_mesh_prop_cli_props_get(cli, NULL, kind, &rsp);
	props_print(shell, err, &rsp);
	return err;
}

static void prop_val_print(const struct shell *shell, int err, struct bt_mesh_prop_val *rsp)
{
	if (!err) {
		shell_print(shell, "Property value: %s", bt_hex(rsp->value, rsp->size));
	}
}

static int cmd_prop_prop_get(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	enum bt_mesh_prop_srv_kind kind =
		(enum bt_mesh_prop_srv_kind)shell_strtol(argv[1], 0, &err);
	uint16_t id = shell_strtoul(argv[2], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_PROP_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_prop_cli *cli = mod->rt->user_data;
	uint8_t value[CONFIG_BT_MESH_PROP_MAXSIZE];
	struct bt_mesh_prop_val rsp = { .value = &value[0], .size = CONFIG_BT_MESH_PROP_MAXSIZE };

	err = bt_mesh_prop_cli_prop_get(cli, NULL, kind, id, &rsp);
	prop_val_print(shell, err, &rsp);
	return err;
}

static int user_prop_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t id = shell_strtoul(argv[1], 0, &err);
	uint8_t val[CONFIG_BT_MESH_PROP_MAXSIZE] = { 0 };
	uint8_t len = hex2bin(argv[2], strlen(argv[2]), val, sizeof(val));

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_PROP_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_prop_cli *cli = mod->rt->user_data;
	struct bt_mesh_prop_val set = {
		.meta.id = id,
		.value = val,
		.size = len,
	};

	if (acked) {
		uint8_t rsp_val[CONFIG_BT_MESH_PROP_MAXSIZE];
		struct bt_mesh_prop_val rsp = { .value = &rsp_val[0],
						.size = CONFIG_BT_MESH_PROP_MAXSIZE };

		err = bt_mesh_prop_cli_user_prop_set(cli, NULL, &set, &rsp);
		prop_val_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_prop_cli_user_prop_set_unack(cli, NULL, &set);
	}
}

static int cmd_prop_user_prop_set(const struct shell *shell, size_t argc, char *argv[])
{
	return user_prop_set(shell, argc, argv, true);
}

static int cmd_prop_user_prop_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return user_prop_set(shell, argc, argv, false);
}

static int admin_prop_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t id = shell_strtoul(argv[1], 0, &err);
	enum bt_mesh_prop_access access = (enum bt_mesh_prop_access)shell_strtol(argv[2], 0, &err);
	uint8_t val[CONFIG_BT_MESH_PROP_MAXSIZE] = { 0 };
	uint8_t len = hex2bin(argv[3], strlen(argv[3]), val, sizeof(val));

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_PROP_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_prop_cli *cli = mod->rt->user_data;
	struct bt_mesh_prop_val set = {
		.meta.id = id,
		.meta.user_access = access,
		.value = val,
		.size = len,
	};

	if (acked) {
		uint8_t rsp_val[CONFIG_BT_MESH_PROP_MAXSIZE];
		struct bt_mesh_prop_val rsp = { .value = &rsp_val[0],
						.size = CONFIG_BT_MESH_PROP_MAXSIZE };

		err = bt_mesh_prop_cli_admin_prop_set(cli, NULL, &set, &rsp);
		prop_val_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_prop_cli_admin_prop_set_unack(cli, NULL, &set);
	}
}

static int cmd_prop_admin_prop_set(const struct shell *shell, size_t argc, char *argv[])
{
	return admin_prop_set(shell, argc, argv, true);
}

static int cmd_prop_admin_prop_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return admin_prop_set(shell, argc, argv, false);
}

static int mfr_prop_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint16_t id = shell_strtoul(argv[1], 0, &err);
	enum bt_mesh_prop_access access = (enum bt_mesh_prop_access)shell_strtol(argv[2], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_GEN_PROP_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_prop_cli *cli = mod->rt->user_data;
	struct bt_mesh_prop set = {
		.id = id,
		.user_access = access,
	};

	if (acked) {
		uint8_t rsp_val[CONFIG_BT_MESH_PROP_MAXSIZE];
		struct bt_mesh_prop_val rsp = { .value = &rsp_val[0],
						.size = CONFIG_BT_MESH_PROP_MAXSIZE };

		err = bt_mesh_prop_cli_mfr_prop_set(cli, NULL, &set, &rsp);
		prop_val_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_prop_cli_mfr_prop_set_unack(cli, NULL, &set);
	}
}

static int cmd_prop_mfr_prop_set(const struct shell *shell, size_t argc, char *argv[])
{
	return mfr_prop_set(shell, argc, argv, true);
}

static int cmd_prop_mfr_prop_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return mfr_prop_set(shell, argc, argv, false);
}

static int cmd_instance_get_all(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_all(shell, BT_MESH_MODEL_ID_GEN_PROP_CLI);
}

static int cmd_instance_set(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t elem_idx = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	return shell_model_instance_set(shell, &mod, BT_MESH_MODEL_ID_GEN_PROP_CLI, elem_idx);
}

SHELL_STATIC_SUBCMD_SET_CREATE(instance_cmds,
			       SHELL_CMD_ARG(set, NULL, "<ElemIdx> ", cmd_instance_set, 2, 0),
			       SHELL_CMD_ARG(get-all, NULL, NULL, cmd_instance_get_all, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	prop_cmds,
	SHELL_CMD_ARG(cli-props-get, NULL, "<ID>", cmd_prop_client_props_get, 2, 0),
	SHELL_CMD_ARG(props-get, NULL, "<Kind>", cmd_prop_props_get, 2, 0),
	SHELL_CMD_ARG(prop-get, NULL, "<Kind> <ID>", cmd_prop_prop_get, 3, 0),
	SHELL_CMD_ARG(user-prop-set, NULL, "<ID> <HexStrVal>",
		      cmd_prop_user_prop_set, 3, (CONFIG_BT_MESH_PROP_MAXSIZE - 1)),
	SHELL_CMD_ARG(user-prop-set-unack, NULL, "<ID> <HexStrVal>",
		      cmd_prop_user_prop_set_unack, 3, (CONFIG_BT_MESH_PROP_MAXSIZE - 1)),
	SHELL_CMD_ARG(admin-prop-set, NULL,
		      "<ID> <Access> <HexStrVal>",
		      cmd_prop_admin_prop_set, 4, (CONFIG_BT_MESH_PROP_MAXSIZE - 1)),
	SHELL_CMD_ARG(admin-prop-set-unack, NULL,
		      "<ID> <Access> <HexStrVal>",
		      cmd_prop_admin_prop_set_unack, 4, (CONFIG_BT_MESH_PROP_MAXSIZE - 1)),
	SHELL_CMD_ARG(mfr-prop-set, NULL, "<ID> <Access>", cmd_prop_mfr_prop_set, 3, 0),
	SHELL_CMD_ARG(mfr-prop-set-unack, NULL, "<ID> <Access>",
		      cmd_prop_mfr_prop_set_unack, 3, 0),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", shell_model_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), prop, &prop_cmds, "Property Cli commands", shell_model_cmds_help,
		 1, 1);
