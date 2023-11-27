/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <zephyr/shell/shell.h>

struct sensor_value shell_model_strtosensorval(const char *str, int *err);

void shell_model_print_sensorval(const struct shell *shell, struct sensor_value *value);

double shell_model_strtodbl(const char *str, int *err);

bool shell_model_first_get(uint16_t id, const struct bt_mesh_model **mod);

bool shell_vnd_model_first_get(uint16_t cid, uint16_t id, const struct bt_mesh_model **mod);

int shell_model_instance_set(const struct shell *shell, const struct bt_mesh_model **mod,
			      uint16_t mod_id, uint8_t elem_idx);

int shell_vnd_model_instance_set(const struct shell *shell, const struct bt_mesh_model **mod,
				 uint16_t mod_id, uint16_t cid, uint8_t elem_idx);

int shell_model_instances_get_all(const struct shell *shell, uint16_t mod_id);

int shell_vnd_model_instances_get_all(const struct shell *shell, uint16_t mod_id, uint16_t cid);

int shell_model_cmds_help(const struct shell *shell, size_t argc, char **argv);
