/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/models.h>
#include <zephyr/shell/shell.h>
#include <ctype.h>

#include "shell_utils.h"

static struct bt_mesh_model *mod;

static void descriptor_print(const struct shell *shell, int err,
			     struct bt_mesh_sensor_descriptor *rsp)
{
	if (!err) {
		shell_print(shell, "{");
		shell_fprintf(shell, SHELL_NORMAL, "\ttolerance: { positive: ");
		shell_model_print_sensorval(shell, &rsp->tolerance.positive);
		shell_fprintf(shell, SHELL_NORMAL, ", negative: ");
		shell_model_print_sensorval(shell, &rsp->tolerance.negative);
		shell_print(shell, " }");
		shell_print(shell, "\tsampling type: %d", rsp->sampling_type);
		shell_print(shell, "\tperiod: %lld", rsp->period);
		shell_print(shell, "\tupdate interval: %lld", rsp->update_interval);
		shell_print(shell, "}");
	}
}

static void descriptors_print(const struct shell *shell, int err,
			      struct bt_mesh_sensor_info *rsp, uint32_t count)
{
	if (!err) {
		for (int i = 0; i < count; ++i) {
			shell_fprintf(shell, SHELL_NORMAL, "0x%04x: ", rsp[i].id);
			descriptor_print(shell, err, &rsp[i].descriptor);
		}
	}
}

static int cmd_desc_get(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SENSOR_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_sensor_cli *cli = mod->user_data;

	if (argc == 1) {
		uint32_t count = CONFIG_BT_MESH_SHELL_SENSOR_CLI_MAX_SENSORS;
		struct bt_mesh_sensor_info rsp[CONFIG_BT_MESH_SHELL_SENSOR_CLI_MAX_SENSORS];

		err = bt_mesh_sensor_cli_desc_all_get(cli, NULL, rsp, &count);

		descriptors_print(shell, err, rsp, count);
		return err;
	}

	err = 0;
	uint32_t sensor_id = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	struct bt_mesh_sensor_descriptor rsp;
	const struct bt_mesh_sensor_type *sensor_type = bt_mesh_sensor_type_get(sensor_id);

	if (sensor_type == NULL) {
		return -ENOENT;
	}

	err = bt_mesh_sensor_cli_desc_get(cli, NULL, sensor_type, &rsp);
	descriptor_print(shell, err, &rsp);
	return err;
}

static void cadence_print(const struct shell *shell, int err,
			  struct bt_mesh_sensor_cadence_status *rsp)
{
	if (!err) {
		shell_print(shell, "fast period div: %d\nmin interval: %d", rsp->fast_period_div,
			    rsp->min_int);
		shell_fprintf(shell, SHELL_NORMAL, "delta threshold: { type: %d, up: ",
			      rsp->threshold.delta.type);
		shell_model_print_sensorval(shell, &rsp->threshold.delta.up);
		shell_fprintf(shell, SHELL_NORMAL, ", down: ");
		shell_model_print_sensorval(shell, &rsp->threshold.delta.down);
		shell_fprintf(shell, SHELL_NORMAL,
			      " }\nfast cadence range: { cadence inside: %d, lower boundary: ",
			      rsp->threshold.range.cadence);
		shell_model_print_sensorval(shell, &rsp->threshold.range.low);
		shell_fprintf(shell, SHELL_NORMAL, ", upper boundary: ");
		shell_model_print_sensorval(shell, &rsp->threshold.range.high);
		shell_print(shell, " }");
	}
}

static int cmd_cadence_get(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint32_t sensor_id = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SENSOR_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_sensor_cli *cli = mod->user_data;
	struct bt_mesh_sensor_cadence_status rsp;
	const struct bt_mesh_sensor_type *sensor_type = bt_mesh_sensor_type_get(sensor_id);

	if (sensor_type == NULL) {
		return -ENOENT;
	}

	err = bt_mesh_sensor_cli_cadence_get(cli, NULL, sensor_type, &rsp);
	cadence_print(shell, err, &rsp);
	return err;
}

static int cadence_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;

	uint32_t sensor_id = shell_strtoul(argv[1], 0, &err);
	struct bt_mesh_sensor_cadence_status cadence = {
		.fast_period_div = shell_strtoul(argv[2], 0, &err),
		.min_int = shell_strtoul(argv[3], 0, &err),
		.threshold = { .delta = { .type = shell_strtol(argv[4], 0, &err),
					  .up = shell_model_strtosensorval(argv[5], &err),
					  .down = shell_model_strtosensorval(argv[6], &err) },
			       .range = { .cadence = shell_strtol(argv[7], 0, &err),
					  .low = shell_model_strtosensorval(argv[8], &err),
					  .high = shell_model_strtosensorval(argv[9], &err) } }
	};

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SENSOR_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_sensor_cli *cli = mod->user_data;
	const struct bt_mesh_sensor_type *sensor_type = bt_mesh_sensor_type_get(sensor_id);

	if (sensor_type == NULL) {
		return -ENOENT;
	}

	if (acked) {
		struct bt_mesh_sensor_cadence_status rsp;

		err = bt_mesh_sensor_cli_cadence_set(cli, NULL, sensor_type, &cadence, &rsp);
		cadence_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_sensor_cli_cadence_set_unack(cli, NULL, sensor_type, &cadence);
	}
}

static int cmd_cadence_set(const struct shell *shell, size_t argc, char *argv[])
{
	return cadence_set(shell, argc, argv, true);
}

static int cmd_cadence_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return cadence_set(shell, argc, argv, false);
}

static void settings_print(const struct shell *shell, int err, uint16_t *ids, uint32_t count)
{
	if (!err) {
		shell_fprintf(shell, SHELL_NORMAL, "[");
		for (uint32_t i = 0; i < count; ++i) {
			shell_fprintf(shell, SHELL_NORMAL, "0x%04x, ", ids[i]);
		}
		shell_print(shell, "]");
	}
}

static int cmd_settings_get(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint32_t sensor_id = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SENSOR_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_sensor_cli *cli = mod->user_data;
	uint32_t count = CONFIG_BT_MESH_SHELL_SENSOR_CLI_MAX_SETTINGS;
	uint16_t ids[CONFIG_BT_MESH_SHELL_SENSOR_CLI_MAX_SETTINGS];

	const struct bt_mesh_sensor_type *sensor_type = bt_mesh_sensor_type_get(sensor_id);

	if (sensor_type == NULL) {
		return -ENOENT;
	}

	err = bt_mesh_sensor_cli_settings_get(cli, NULL, sensor_type, ids, &count);
	settings_print(shell, err, ids, count);
	return err;
}

static void values_print(const struct shell *shell, int err, struct sensor_value *values,
			 const struct bt_mesh_sensor_type *sensor_type)
{
	if (!err) {
		shell_fprintf(shell, SHELL_NORMAL, "{ ");
		for (uint32_t channel = 0; channel < sensor_type->channel_count; ++channel) {
			shell_fprintf(shell, SHELL_NORMAL, "channel %d: ", channel);
			shell_model_print_sensorval(shell, &values[channel]);
			shell_fprintf(shell, SHELL_NORMAL, ", ");
		}
		shell_print(shell, "}");
	}
}

static void setting_print(const struct shell *shell, int err,
			  struct bt_mesh_sensor_setting_status *rsp)
{
	values_print(shell, err, rsp->value, rsp->type);
}

static int cmd_setting_get(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint32_t sensor_id = shell_strtoul(argv[1], 0, &err);
	uint32_t setting_id = shell_strtoul(argv[2], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SENSOR_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_sensor_cli *cli = mod->user_data;
	struct bt_mesh_sensor_setting_status rsp;
	const struct bt_mesh_sensor_type *sensor_type = bt_mesh_sensor_type_get(sensor_id);
	const struct bt_mesh_sensor_type *setting_type = bt_mesh_sensor_type_get(setting_id);

	if (sensor_type == NULL || setting_type == NULL) {
		return -ENOENT;
	}

	err = bt_mesh_sensor_cli_setting_get(cli, NULL, sensor_type, setting_type, &rsp);
	setting_print(shell, err, &rsp);
	return err;
}

static int setting_set(const struct shell *shell, size_t argc, char *argv[], bool acked)
{
	int err = 0;
	uint32_t sensor_id = shell_strtoul(argv[1], 0, &err);
	uint32_t setting_id = shell_strtoul(argv[2], 0, &err);
	struct sensor_value value = shell_model_strtosensorval(argv[3], &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SENSOR_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_sensor_cli *cli = mod->user_data;
	const struct bt_mesh_sensor_type *sensor_type = bt_mesh_sensor_type_get(sensor_id);
	const struct bt_mesh_sensor_type *setting_type = bt_mesh_sensor_type_get(setting_id);

	if (sensor_type == NULL || setting_type == NULL) {
		return -ENOENT;
	}

	if (acked) {
		struct bt_mesh_sensor_setting_status rsp;

		err = bt_mesh_sensor_cli_setting_set(cli, NULL, sensor_type, setting_type,
						     &value, &rsp);
		setting_print(shell, err, &rsp);
		return err;
	} else {
		return bt_mesh_sensor_cli_setting_set_unack(cli, NULL, sensor_type, setting_type,
							    &value);
	}
}

static int cmd_setting_set(const struct shell *shell, size_t argc, char *argv[])
{
	return setting_set(shell, argc, argv, true);
}

static int cmd_setting_set_unack(const struct shell *shell, size_t argc, char *argv[])
{
	return setting_set(shell, argc, argv, false);
}

static void sensors_print(const struct shell *shell, int err, struct bt_mesh_sensor_data *sensors,
			  uint32_t count)
{
	if (!err) {
		for (uint32_t i = 0; i < count; ++i) {
			shell_fprintf(shell, SHELL_NORMAL, "0x%04x: ", sensors[i].type->id);
			values_print(shell, err, sensors[i].value, sensors[i].type);
		}
	}
}

static int cmd_get(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SENSOR_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_sensor_cli *cli = mod->user_data;

	if (argc == 1) {
		uint32_t count = CONFIG_BT_MESH_SHELL_SENSOR_CLI_MAX_SENSORS;
		struct bt_mesh_sensor_data rsp[CONFIG_BT_MESH_SHELL_SENSOR_CLI_MAX_SENSORS];

		err = bt_mesh_sensor_cli_all_get(cli, NULL, rsp, &count);

		sensors_print(shell, err, rsp, count);
		return err;
	}

	err = 0;
	uint32_t sensor_id = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	struct sensor_value rsp[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
	const struct bt_mesh_sensor_type *sensor_type = bt_mesh_sensor_type_get(sensor_id);

	if (sensor_type == NULL) {
		return -ENOENT;
	}

	err = bt_mesh_sensor_cli_get(cli, NULL, sensor_type, rsp);

	values_print(shell, err, rsp, sensor_type);
	return err;
}

static void series_entry_print(const struct shell *shell, int err,
			       struct bt_mesh_sensor_series_entry *entry,
			       const struct bt_mesh_sensor_type *sensor_type)
{
	if (!err) {
		shell_fprintf(shell, SHELL_NORMAL, "[");
		shell_model_print_sensorval(shell, &entry->column.start);
		shell_fprintf(shell, SHELL_NORMAL, " to ");
		shell_model_print_sensorval(shell, &entry->column.end);
		shell_fprintf(shell, SHELL_NORMAL, "]: ");
		values_print(shell, err, entry->value, sensor_type);
	}
}

static int cmd_series_entry_get(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint32_t sensor_id = shell_strtoul(argv[1], 0, &err);
	struct bt_mesh_sensor_column column = {.start = shell_model_strtosensorval(argv[2], &err)};

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SENSOR_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_sensor_cli *cli = mod->user_data;
	const struct bt_mesh_sensor_type *sensor_type = bt_mesh_sensor_type_get(sensor_id);

	if (sensor_type == NULL) {
		return -ENOENT;
	}

	struct bt_mesh_sensor_series_entry rsp;

	err = bt_mesh_sensor_cli_series_entry_get(cli, NULL, sensor_type, &column, &rsp);
	series_entry_print(shell, err, &rsp, sensor_type);
	return err;
}

static void series_entries_print(const struct shell *shell, int err,
				 struct bt_mesh_sensor_series_entry *rsp, uint32_t count,
				 const struct bt_mesh_sensor_type *sensor_type)
{
	if (!err) {
		for (uint32_t i = 0; i < count; ++i) {
			series_entry_print(shell, err, &rsp[i], sensor_type);
		}
	}
}

static int cmd_series_entries_get(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint32_t sensor_id = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	if (!mod && !shell_model_first_get(BT_MESH_MODEL_ID_SENSOR_CLI, &mod)) {
		return -ENODEV;
	}

	struct bt_mesh_sensor_cli *cli = mod->user_data;

	const struct bt_mesh_sensor_type *sensor_type = bt_mesh_sensor_type_get(sensor_id);

	if (sensor_type == NULL) {
		return -ENOENT;
	}

	struct bt_mesh_sensor_series_entry rsp[CONFIG_BT_MESH_SHELL_SENSOR_CLI_MAX_COLUMNS];
	uint32_t count = CONFIG_BT_MESH_SHELL_SENSOR_CLI_MAX_COLUMNS;

	if (argc == 4) {
		struct bt_mesh_sensor_column range = {
			.start = shell_model_strtosensorval(argv[2], &err),
			.end = shell_model_strtosensorval(argv[3], &err)
		};

		if (err) {
			shell_warn(shell, "Unable to parse input string arg");
			return err;
		}

		err = bt_mesh_sensor_cli_series_entries_get(cli, NULL, sensor_type, &range, rsp,
							    &count);
	} else if (argc == 2) {
		err = bt_mesh_sensor_cli_series_entries_get(cli, NULL, sensor_type, NULL, rsp,
							    &count);
	} else {
		return -ENOEXEC;
	}

	series_entries_print(shell, err, rsp, count, sensor_type);
	return err;
}

static int cmd_instance_get_all(const struct shell *shell, size_t argc, char *argv[])
{
	return shell_model_instances_get_all(shell, BT_MESH_MODEL_ID_SENSOR_CLI);
}

static int cmd_instance_set(const struct shell *shell, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t elem_idx = shell_strtoul(argv[1], 0, &err);

	if (err) {
		shell_warn(shell, "Unable to parse input string arg");
		return err;
	}

	return shell_model_instance_set(shell, &mod, BT_MESH_MODEL_ID_SENSOR_CLI, elem_idx);
}

SHELL_STATIC_SUBCMD_SET_CREATE(instance_cmds,
			       SHELL_CMD_ARG(set, NULL, "<ElemIdx> ", cmd_instance_set, 2, 0),
			       SHELL_CMD_ARG(get-all, NULL, NULL, cmd_instance_get_all, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sensor_cmds,
	SHELL_CMD_ARG(desc-get, NULL, "[SensorID]", cmd_desc_get, 1, 1),
	SHELL_CMD_ARG(cadence-get, NULL, "<SensorID>", cmd_cadence_get, 2, 0),
	SHELL_CMD_ARG(cadence-set, NULL, "<SensorID> <FastPerDiv> <MinInt> <DltType> "
		      "<DltUp> <DltDown> <CadInside> <RngLow> <RngHigh>",
		      cmd_cadence_set, 10, 0),
	SHELL_CMD_ARG(cadence-set-unack, NULL, "<SensorID> <FastPerDiv> <MinInt> "
		      "<DltType> <DltUp> <DltDown> <CadInside> <RngLow> "
		      "<RngHigh>", cmd_cadence_set_unack, 10, 0),
	SHELL_CMD_ARG(settings-get, NULL, "<SensorID>", cmd_settings_get, 2, 0),
	SHELL_CMD_ARG(setting-get, NULL, "<SensorID> <SettingID>", cmd_setting_get,
		      3, 0),
	SHELL_CMD_ARG(setting-set, NULL, "<SensorID> <SettingID> <Value>",
		      cmd_setting_set, 4, 0),
	SHELL_CMD_ARG(setting-set-unack, NULL, "<SensorID> <SettingID> <Value>",
		      cmd_setting_set_unack, 4, 0),
	SHELL_CMD_ARG(get, NULL, "[SensorID]", cmd_get, 1, 1),
	SHELL_CMD_ARG(series-entry-get, NULL, "<SensorID> <Column>",
		      cmd_series_entry_get, 3, 0),
	SHELL_CMD_ARG(series-entries-get, NULL, "<SensorID> [<RngStart> <RngEnd>]",
		      cmd_series_entries_get, 2, 2),
	SHELL_CMD(instance, &instance_cmds, "Instance commands", shell_model_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), sensor, &sensor_cmds, "Sensor Cli commands", shell_model_cmds_help,
		 1, 1);
