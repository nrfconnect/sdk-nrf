/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/models.h>
#include <zephyr/shell/shell.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "mesh/net.h"
#include "mesh/access.h"
#include "shell_utils.h"

extern int errno;

struct shell_model_instance {
	uint16_t addr;
	uint8_t elem_idx;
};

struct sensor_value shell_model_strtosensorval(const char *str, int *err)
{
	int temp_err = 0;
	struct sensor_value out;

	double val = shell_model_strtodbl(str, &temp_err);

	if (temp_err) {
		*err = temp_err;
		return out;
	}

	out.val1 = (int)val;
	out.val2 = (val - out.val1) * 1000000;

	return out;
}

void shell_model_print_sensorval(const struct shell *shell, struct sensor_value *value)
{
	shell_fprintf(shell, SHELL_NORMAL, "%s%d", (value->val1 < 0 || value->val2 < 0) ? "-" : "",
		      abs(value->val1));
	if (value->val2) {
		int32_t val = abs(value->val2);
		int digits = 6;

		while (!(val % 10)) {
			val /= 10;
			digits--;
		}
		shell_fprintf(shell, SHELL_NORMAL, ".%0*d", digits, val);
	}
}

static size_t whitespace_trim(char *out, size_t len, const char *str)
{
	if (len == 0) {
		return 0;
	}
	const char *end;
	size_t out_size;

	while (str[0] == ' ') {
		str++;
	}
	if (*str == 0) {
		*out = 0;
		return 1;
	}
	end = str + strlen(str) - 1;
	while (end > str && (end[0] == ' ')) {
		end--;
	}
	end++;
	out_size = (end - str) + 1;
	if (out_size > len) {
		return 0;
	}
	memcpy(out, str, out_size - 1);
	out[out_size - 1] = 0;
	return out_size;
}

double shell_model_strtodbl(const char *str, int *err)
{
	char trimmed_buf[22] = { 0 };
	long intgr;
	unsigned long frac;
	double frac_dbl;
	int temp_err = 0;
	size_t len = whitespace_trim(trimmed_buf, sizeof(trimmed_buf), str);

	if (len < 2) {
		*err = -EINVAL;
		return 0;
	}

	int comma_idx = strcspn(trimmed_buf, ".");
	int frac_len = strlen(trimmed_buf + comma_idx + 1);
	/* Covers corner case "." input */
	if (strlen(trimmed_buf) < 2 && trimmed_buf[comma_idx] != 0) {
		*err = -EINVAL;
		return 0;
	}
	trimmed_buf[comma_idx] = 0;
	/* Avoid fractional overflow by losing one precision point */
	if (frac_len > 9) {
		trimmed_buf[comma_idx + 10] = 0;
		frac_len = 9;
	}

	/* Avoid doing str2long if intgr part is empty or only single sign char*/
	if (trimmed_buf[0] == '\0' ||
	    (trimmed_buf[1] == '\0' && (trimmed_buf[0] == '+' || trimmed_buf[0] == '-'))) {
		intgr = 0;
	} else {
		intgr = shell_strtol(trimmed_buf, 10, &temp_err);
		if (temp_err) {
			*err = temp_err;
			return 0;
		}
	}
	/* Avoid doing str2ulong if fractional part is empty */
	if ((trimmed_buf + comma_idx + 1)[0] == '\0') {
		frac = 0;
	} else {
		frac = shell_strtoul(trimmed_buf + comma_idx + 1, 10, &temp_err);

		if (temp_err) {
			*err = temp_err;
			return 0;
		}
	}
	frac_dbl = (double)frac;
	for (int i = 0; i < frac_len; i++) {
		frac_dbl /= 10;
	}

	return (trimmed_buf[0] == '-') ? ((double)intgr - frac_dbl) : ((double)intgr + frac_dbl);
}

static bool model_first_get(uint16_t id, struct bt_mesh_model **mod, uint16_t *cid)
{
	const struct bt_mesh_comp *comp = bt_mesh_comp_get();

	for (int i = 0; i < comp->elem_count; i++) {
		if (cid) {
			*mod = bt_mesh_model_find_vnd(&comp->elem[i], *cid, id);
		} else {
			*mod = bt_mesh_model_find(&comp->elem[i], id);
		}

		if (*mod) {
			return true;
		}
	}

	return false;
}

bool shell_model_first_get(uint16_t id, struct bt_mesh_model **mod)
{
	return model_first_get(id, mod, NULL);
}

bool shell_vnd_model_first_get(uint16_t cid, uint16_t id, struct bt_mesh_model **mod)
{
	return model_first_get(id, mod, &cid);
}

static int instance_set(const struct shell *shell, struct bt_mesh_model **mod, uint16_t mod_id,
			uint16_t *cid, uint8_t elem_idx)
{
	struct bt_mesh_model *mod_temp;
	const struct bt_mesh_comp *comp = bt_mesh_comp_get();

	if (elem_idx >= comp->elem_count) {
		shell_error(shell, "Invalid element index");
		return -EINVAL;
	}

	if (cid) {
		mod_temp = bt_mesh_model_find_vnd(&comp->elem[elem_idx], *cid, mod_id);
	} else {
		mod_temp = bt_mesh_model_find(&comp->elem[elem_idx], mod_id);
	}

	if (mod_temp) {
		*mod = mod_temp;
	} else {
		shell_error(shell, "Unable to find model instance for element index %d", elem_idx);
		return -ENODEV;
	}

	return 0;
}

int shell_model_instance_set(const struct shell *shell, struct bt_mesh_model **mod, uint16_t mod_id,
			     uint8_t elem_idx)
{
	return instance_set(shell, mod, mod_id, NULL, elem_idx);
}

int shell_vnd_model_instance_set(const struct shell *shell, struct bt_mesh_model **mod,
				 uint16_t mod_id, uint16_t cid, uint8_t elem_idx)
{
	return instance_set(shell, mod, mod_id, &cid, elem_idx);
}

static void model_instances_get(uint16_t id, uint16_t *cid, struct shell_model_instance *arr,
				uint8_t len)
{
	const struct bt_mesh_comp *comp = bt_mesh_comp_get();
	struct bt_mesh_elem *elem;
	struct bt_mesh_model *mod;

	for (int i = 0; i < len; i++) {
		elem = bt_mesh_elem_find(comp->elem[i].addr);

		if (cid) {
			mod = bt_mesh_model_find_vnd(elem, *cid, id);
		} else {
			mod = bt_mesh_model_find(elem, id);
		}

		if (mod) {
			arr[i].addr = comp->elem[i].addr;
			arr[i].elem_idx = mod->elem_idx;
		}
	}
}

static int instances_get_all(const struct shell *shell, uint16_t mod_id, uint16_t *cid)
{
	uint8_t elem_cnt = bt_mesh_elem_count();
	struct shell_model_instance mod_arr[elem_cnt];

	memset(mod_arr, 0, sizeof(mod_arr));
	model_instances_get(mod_id, cid, mod_arr, ARRAY_SIZE(mod_arr));

	for (int i = 0; i < ARRAY_SIZE(mod_arr); i++) {
		if (mod_arr[i].addr) {
			shell_print(shell,
				    "Model instance found at addr 0x%.4X. Element index: %d",
				    mod_arr[i].addr, mod_arr[i].elem_idx);
		}
	}

	return 0;
}

int shell_model_instances_get_all(const struct shell *shell, uint16_t mod_id)
{
	return instances_get_all(shell, mod_id, NULL);
}

int shell_vnd_model_instances_get_all(const struct shell *shell, uint16_t mod_id, uint16_t cid)
{
	return instances_get_all(shell, mod_id, &cid);
}

int shell_model_cmds_help(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(
		shell,
		"\nFor a detailed description of the commands and arguments in this shell module, "
		"please visit:\ndeveloper.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries"
		"/bluetooth_services/mesh/models.html.\nFrom there you can navigate to the specific"
		" model type.\n");

	if (argc == 1) {
		shell_help(shell);
		return 1;
	}

	shell_error(shell, "%s unknown command: %s", argv[0], argv[1]);
	return -EINVAL;
}
