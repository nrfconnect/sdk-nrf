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

static void model_instances_get(uint16_t id, struct shell_model_instance *arr, uint8_t len)
{
	const struct bt_mesh_comp *comp = bt_mesh_comp_get();
	struct bt_mesh_elem *elem;
	struct bt_mesh_model *mod;

	for (int i = 0; i < len; i++) {
		elem = bt_mesh_elem_find(comp->elem[i].addr);
		mod = bt_mesh_model_find(elem, id);
		if (mod) {
			arr[i].addr = comp->elem[i].addr;
			arr[i].elem_idx = mod->elem_idx;
		}
	}
}

static uint8_t str2u8(const char *str)
{
	if (isdigit((unsigned char)str[0])) {
		return strtoul(str, NULL, 0);
	}

	return (!strcmp(str, "on") || !strcmp(str, "enable"));
}

bool shell_model_str2bool(const char *str)
{
	return str2u8(str);
}


int shell_model_str2sensorval(const char *str, struct sensor_value *out)
{
	int32_t sign = 1;
	char buf[18]; /* 10 digits + decimal point + 6 digits + terminator */

	if (*str == '-') {
		sign = -1;
		str++;
	}

	if (strlen(str) > 17 || strlen(str) == 0) {
		return -EINVAL;
	}

	strcpy(buf, str);

	char *dp = strchr(buf, '.');

	if (dp != NULL) {
		*dp = 0;
		dp++;
	}

	errno = 0;
	uint32_t val = strtoul(buf, NULL, 10);

	if (errno || (sign == 1 && val > INT32_MAX) || (sign == -1 && val > INT32_MAX + 1ul)) {
		return -EINVAL;
	}
	out->val1 = sign * val;
	if (dp == NULL || *dp == 0) {
		out->val2 = 0;
		return 0;
	}

	int32_t factor = 100000;

	val = 0;

	do {
		if (isdigit(*dp) && factor > 0) {
			val += (*dp - '0') * factor;
			factor /= 10;
		} else {
			return -EINVAL;
		}
	} while (*(++dp));
	out->val2 = val * sign;
	return 0;
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

static bool hex_str_check(char *str, uint8_t str_len)
{
	if (str_len % 2) {
		return false;
	}

	for (int i = 0; i < str_len; i++) {
		if (!isxdigit((int)str[i])) {
			return false;
		}
	}
	return true;
}

uint8_t shell_model_hexstr2num(const struct shell *shell, char *str, uint8_t *buf, uint8_t buf_len)
{
	char str_num[3] = { 0 };
	int str_len = strlen(str);

	if (!hex_str_check(str, str_len)) {
		shell_error(shell, "Invalid hex string format");
		return 0;
	}

	if ((str_len / 2) > buf_len) {
		shell_error(shell, "Hex value is too large");
		return 0;
	}

	for (int i = 0; i < str_len / 2; i++) {
		strncpy(str_num, str + (i * 2), 2);
		buf[i] = strtol(str_num, NULL, 16);
	}

	return str_len / 2;
}

double shell_model_str2dbl(const struct shell *shell, const char *str)
{
	char *point;
	char frac_buf[10] = {0};
	double decimal, frac;
	int len;

	decimal = (double)strtol(str, &point, 0);

	if ((decimal <= LONG_MIN) || (decimal >= LONG_MAX)) {
		shell_warn(shell, "Passed input value is too small/large. Returning zero");
		return 0;
	}

	if (!strlen(point)) {
		return decimal;
	}

	len = MIN((strlen(point) - 1), sizeof(frac_buf) - 1);
	strncpy(frac_buf, point + 1, len);
	frac = (double)strtol(frac_buf, NULL, 0);

	for (int i = 0; i < len; i++) {
		frac /= 10;
	}

	return (decimal < 0) ? (decimal - frac) : (decimal + frac);
}

bool shell_model_first_get(uint16_t id, struct bt_mesh_model **mod)
{
	const struct bt_mesh_comp *comp = bt_mesh_comp_get();

	for (int i = 0; i < comp->elem_count; i++) {
		*mod = bt_mesh_model_find(&comp->elem[i], id);
		if (*mod) {
			return true;
		}
	}

	return false;
}

int shell_model_instance_set(const struct shell *shell, struct bt_mesh_model **mod,
			      uint16_t mod_id, uint8_t elem_idx)
{
	struct bt_mesh_model *mod_temp;
	const struct bt_mesh_comp *comp = bt_mesh_comp_get();

	if (elem_idx >= comp->elem_count) {
		shell_error(shell, "Invalid element index");
		return -EINVAL;
	}

	mod_temp = bt_mesh_model_find(&comp->elem[elem_idx], mod_id);

	if (mod_temp) {
		*mod = mod_temp;
	} else {
		shell_error(shell, "Unable to find model instance for element index %d", elem_idx);
		return -ENODEV;
	}

	return 0;
}

int shell_model_instances_get_all(const struct shell *shell, uint16_t mod_id)
{
	uint8_t elem_cnt = bt_mesh_elem_count();
	struct shell_model_instance mod_arr[elem_cnt];

	memset(mod_arr, 0, sizeof(mod_arr));
	model_instances_get(mod_id, mod_arr, ARRAY_SIZE(mod_arr));

	for (int i = 0; i < ARRAY_SIZE(mod_arr); i++) {
		if (mod_arr[i].addr) {
			shell_print(shell,
				    "Client model instance found at addr 0x%.4X. Element index: %d",
				    mod_arr[i].addr, mod_arr[i].elem_idx);
		}
	}

	return 0;
}

int shell_model_cmds_help(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return 1;
	}

	shell_error(shell, "%s unknown command: %s", argv[0], argv[1]);
	return -EINVAL;
}
