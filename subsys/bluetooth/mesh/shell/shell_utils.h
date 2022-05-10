/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <zephyr/shell/shell.h>

bool shell_model_str2bool(const char *str);

int shell_model_str2sensorval(const char *str, struct sensor_value *out);

void shell_model_print_sensorval(const struct shell *shell, struct sensor_value *value);

uint8_t shell_model_hexstr2num(const struct shell *shell, char *str, uint8_t *buf, uint8_t buf_len);

double shell_model_str2dbl(const struct shell *shell, const char *str);

bool shell_model_first_get(uint16_t id, struct bt_mesh_model **mod);

int shell_model_instance_set(const struct shell *shell, struct bt_mesh_model **mod,
			      uint16_t mod_id, uint8_t elem_idx);

int shell_model_instances_get_all(const struct shell *shell, uint16_t mod_id);

int shell_model_cmds_help(const struct shell *shell, size_t argc, char **argv);
