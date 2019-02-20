/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <at_params.h>
#include <kernel.h>

/* Internal function. */
static void at_param_init(struct at_param *param)
{
	__ASSERT(param != NULL, "Parameter pointer cannot be NULL.\n");

	memset(param, 0, sizeof(struct at_param));
}


/* Internal function. */
static void at_param_clear(struct at_param *param)
{
	__ASSERT(param != NULL, "Parameter pointer cannot be NULL.\n");

	if (param->type == AT_PARAM_TYPE_STRING) {
		k_free(param->value.str_val);
	} else if (param->type == AT_PARAM_TYPE_NUM_INT) {
		param->value.int_val = 0;
	} else if (param->type == AT_PARAM_TYPE_NUM_SHORT) {
		param->value.short_val = 0;
	}
}


/* Internal function. */
static struct at_param *at_params_get(const struct at_param_list *list,
				size_t index)
{
	__ASSERT(list != NULL, "Parameter list cannot be NULL.\n");

	if (index >= list->param_count) {
		return NULL;
	}

	struct at_param *param = list->params;

	return &param[index];
}


/* Internal function. Parameter cannot be null. */
static size_t at_param_size(const struct at_param *param)
{
	if (param->type == AT_PARAM_TYPE_NUM_SHORT) {
		return sizeof(u16_t);
	} else if (param->type == AT_PARAM_TYPE_NUM_INT) {
		return sizeof(u32_t);
	} else if (param->type == AT_PARAM_TYPE_STRING) {
		return strlen(param->value.str_val);
	}

	return 0;
}


int at_params_list_init(struct at_param_list *list,
			size_t max_params_count)
{
	if (list == NULL) {
		return -EINVAL;
	}

	if (list->params != NULL) {
		return -EACCES;
	}

	list->params = k_calloc(max_params_count,
				sizeof(struct at_param));
	if (list->params == NULL) {
		return -ENOMEM;
	}

	list->param_count = max_params_count;

	return 0;
}


void at_params_list_clear(struct at_param_list *list)
{
	if (list == NULL || list->params == NULL) {
		return;
	}

	for (size_t i = 0; i < list->param_count;
		 ++i) {
		struct at_param *params = list->params;

		at_param_clear(&params[i]);
		at_param_init(&params[i]);
	}
}


void at_params_list_free(struct at_param_list *list)
{
	if (list == NULL || list->params == NULL) {
		return;
	}

	at_params_list_clear(list);

	list->param_count = 0;
	k_free(list->params);
	list->params = NULL;
}


int at_params_clear(struct at_param_list *list, size_t index)
{
	if (list == NULL || list->params == NULL) {
		return -EINVAL;
	}


	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	at_param_clear(param);
	at_param_init(param);
	return 0;
}


int at_params_short_put(const struct at_param_list *list, size_t index,
			u16_t value)
{
	if (list == NULL || list->params == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	at_param_clear(param);

	param->type = AT_PARAM_TYPE_NUM_SHORT;
	param->value.short_val = (value & USHRT_MAX);
	return 0;
}


int at_params_int_put(const struct at_param_list *list, size_t index,
			u32_t value)
{
	if (list == NULL || list->params == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	at_param_clear(param);

	param->type = AT_PARAM_TYPE_NUM_INT;
	param->value.int_val = value;
	return 0;
}


int at_params_string_put(const struct at_param_list *list, size_t index,
			 const char *str, size_t str_len)
{
	if (list == NULL || list->params == NULL || str == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	char *param_value = k_malloc(str_len + 1);

	if (param_value == NULL) {
		return -ENOMEM;
	}

	memcpy(param_value, str, str_len);
	param_value[str_len] = '\0';

	at_param_clear(param);
	param->type = AT_PARAM_TYPE_STRING;
	param->value.str_val =	param_value;

	return 0;
}


int at_params_size_get(const struct at_param_list *list, size_t index,
			size_t *len)
{
	if (list == NULL || list->params == NULL || len == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	*len = at_param_size(param);
	return 0;
}


int at_params_short_get(const struct at_param_list *list, size_t index,
			u16_t *value)
{
	if (list == NULL || list->params == NULL || value == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	if (param->type != AT_PARAM_TYPE_NUM_SHORT) {
		return -EINVAL;
	}

	*value = param->value.short_val;
	return 0;
}


int at_params_int_get(const struct at_param_list *list, size_t index,
			u32_t *value)
{
	if (list == NULL || list->params == NULL || value == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	if (param->type != AT_PARAM_TYPE_NUM_INT) {
		return -EINVAL;
	}

	*value = param->value.int_val;
	return 0;
}


int at_params_string_get(const struct at_param_list *list, size_t index,
			char *value, size_t len)
{
	if (list == NULL || list->params == NULL || value == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	if (param->type != AT_PARAM_TYPE_STRING) {
		return -EINVAL;
	}

	size_t param_len = at_param_size(param);

	if (len < param_len) {
		return -ENOMEM;
	}

	memcpy(value, param->value.str_val, param_len);
	return param_len;
}


u32_t at_params_valid_count_get(const struct at_param_list *list)
{
	if (list == NULL || list->params == NULL) {
		return 0;
	}

	size_t valid_i = 0;
	struct at_param *param = at_params_get(list, valid_i);

	while (param != NULL && param->type != AT_PARAM_TYPE_EMPTY) {
		valid_i += 1;
		param = at_params_get(list, valid_i);
	}

	return valid_i;
}
