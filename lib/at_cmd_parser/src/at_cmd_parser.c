/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zephyr.h>
#include <zephyr/types.h>

#include <at_cmd_parser.h>
#include <at_utils.h>

#define AT_CMD_PARAM_SEPARATOR ','
#define AT_CMD_SEPARATOR ';'

/* Internal function. Parameters cannot be null. String must be null terminated.
 */
static int at_parse_param_u32(char *at_str, u32_t *val,
				size_t *consumed)
{
	u64_t value = 0;
	u64_t check_value;
	bool negative = false;

	*consumed = 0;

	if (*at_str == '-' && isdigit(*(at_str + 1))) {
		at_str++;
		(*consumed)++;
		negative = true;
	} else if (!isdigit(*at_str)) {
		return -EINVAL;
	}

	while (isdigit(*at_str)) {
		if (value > UINT32_MAX - 1) {
			return -EINVAL;
		}
		check_value = value * 10;
		value = check_value + ((*at_str) - '0');

		if (value >= UINT32_MAX) {
			return -EINVAL;
		}
		at_str++;
		(*consumed)++;
	}

	if (negative) {
		value = (u32_t)(0 - value);
	}

	*val = value;
	return 0;
}


/* Internal function. Parameters cannot be null. String must be null terminated.
 */
static int at_parse_param_numeric(char *at_str,
				  struct at_param_list *list,
				  size_t index, size_t *consumed)
{
	size_t num_spaces = at_params_space_count_get(&at_str);

	u32_t val;
	size_t consumed_bytes;
	int err = at_parse_param_u32(at_str, &val, &consumed_bytes);

	if (err) {
		*consumed = 0;
		return -EINVAL;
	}

	if (val <= USHRT_MAX) {
		err = at_params_short_put(list, index, (u16_t)(val));
	} else {
		err = at_params_int_put(list, index, val);
	}

	*consumed = consumed_bytes + num_spaces;
	return 0;
}


/* Internal function. Parameters cannot be null. String must be null terminated.
 */
static int at_parse_param_string(char *at_str,
				 struct at_param_list *list,
				 size_t index, size_t *consumed)
{
	u16_t str_len;
	bool in_double_quotes = false;
	char *param_value_start = 0;

	if (at_str == NULL || *at_str == '\0') {
		return 0;
	}

	size_t spaces = at_params_space_count_get(&at_str);

	if (*at_str == '\"') {
		at_str++;
		in_double_quotes = true;
	}

	param_value_start = at_str;

	while ((*at_str != '\0') && (*at_str != '\r')
		&& ((!in_double_quotes
		&& ((*at_str != AT_CMD_SEPARATOR)
		|| (*at_str != AT_CMD_PARAM_SEPARATOR)))
		|| (in_double_quotes && (*at_str != '\"')))) {
		at_str++;
	}

	if (in_double_quotes &&
	    ((*at_str == AT_CMD_SEPARATOR) || (*at_str == '\0'))) {
		return -EINVAL;
	}

	str_len = at_str - param_value_start;
	*consumed = str_len + spaces;

	if (in_double_quotes && (*at_str == '\"')) {
		at_str++;
		*consumed += 2;
	}

	return at_params_string_put(list, index, param_value_start, str_len);
}


/* Internal function. Parameters cannot be null. String must be null terminated.
 */
static int at_parse_param(char *at_params_str,
			  struct at_param_list *list, size_t index,
			  size_t *consumed)
{
	if (at_params_str == NULL || *at_params_str == '\0' ||
	    *at_params_str == AT_CMD_PARAM_SEPARATOR ||
	    *at_params_str == AT_CMD_SEPARATOR) {
		*consumed = 0;
		(void)at_params_clear(list, index);
		return 0;
	}

	int err = at_parse_param_numeric(at_params_str, list, index,
					 consumed);

	if (err) {
		err = at_parse_param_string(at_params_str, list, index,
					    consumed);
	}

	return err;
}


int at_parser_params_from_str(char *at_params_str,
			      struct at_param_list *list)
{
	return at_parser_max_params_from_str(at_params_str, list,
					     list->param_count);
}


int at_parser_max_params_from_str(char *str,
				  struct at_param_list *list,
				  size_t max_params_count)
{
	if (str == NULL || list == NULL || list->params == NULL) {
		return -EINVAL;
	}

	at_params_list_clear(list);

	max_params_count = MIN(max_params_count, list->param_count);

	(void)at_params_space_count_get(&str);

	for (size_t i = 0; i < max_params_count; ++i) {
		size_t consumed;
		int err = at_parse_param(str, list, i, &consumed);

		if (err) {
			return err;
		}

		str += consumed;

		if (i < (max_params_count - 1) && *str != '\0') {
			if (*str == AT_CMD_PARAM_SEPARATOR) {
				str++;
			} else if ((*str == '\r') || (*str == '\n')) {
				return 0;
			} else {
				return -EINVAL;
			}
		}
	}

	return 0;
}
