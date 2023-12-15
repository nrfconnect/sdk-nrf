/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <modem/at_params.h>

/* Internal function. Parameter cannot be null. */
static void at_param_init(struct at_param *param)
{
	__ASSERT(param != NULL, "Parameter cannot be NULL.");

	/* Initialize to default. Empty parameter with null value. */
	memset(param, 0, sizeof(struct at_param));
}

/* Internal function. Parameter cannot be null. */
static void at_param_clear(struct at_param *param)
{
	__ASSERT(param != NULL, "Parameter cannot be NULL.");

	if ((param->type == AT_PARAM_TYPE_STRING) ||
	    (param->type == AT_PARAM_TYPE_ARRAY)) {
		k_free(param->value.str_val);
	}

	param->value.int_val = 0;
}

/* Internal function. Parameter cannot be null. */
static struct at_param *at_params_get(const struct at_param_list *list,
				      size_t index)
{
	__ASSERT(list != NULL, "Parameter list cannot be NULL.");

	if (index >= list->param_count) {
		return NULL;
	}

	struct at_param *param = list->params;

	return &param[index];
}

/* Internal function. Parameter cannot be null. */
static size_t at_param_size(const struct at_param *param)
{
	__ASSERT(param != NULL, "Parameter cannot be NULL.");

	if (param->type == AT_PARAM_TYPE_NUM_INT) {
		return sizeof(uint64_t);
	} else if ((param->type == AT_PARAM_TYPE_STRING) ||
		   (param->type == AT_PARAM_TYPE_ARRAY)) {
		return param->size;
	}

	return 0;
}

int at_params_list_init(struct at_param_list *list, size_t max_params_count)
{
	if (list == NULL) {
		return -EINVAL;
	}

	/* Array initialized with empty parameters. */
	list->params = k_calloc(max_params_count, sizeof(struct at_param));
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

	for (size_t i = 0; i < list->param_count; ++i) {
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

int at_params_empty_put(const struct at_param_list *list, size_t index)
{
	if (list == NULL || list->params == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	at_param_clear(param);

	param->type = AT_PARAM_TYPE_EMPTY;
	param->value.int_val = 0;

	return 0;
}

int at_params_int_put(const struct at_param_list *list, size_t index, int64_t value)
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

	char *param_value = (char *)k_malloc(str_len + 1);

	if (param_value == NULL) {
		return -ENOMEM;
	}

	memcpy(param_value, str, str_len);

	at_param_clear(param);
	param->size = str_len;
	param->type = AT_PARAM_TYPE_STRING;
	param->value.str_val = param_value;

	return 0;
}

int at_params_array_put(const struct at_param_list *list, size_t index,
			const uint32_t *array, size_t array_len)
{
	if (list == NULL || list->params == NULL || array == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	uint32_t *param_value = (uint32_t *)k_malloc(array_len);

	if (param_value == NULL) {
		return -ENOMEM;
	}

	memcpy(param_value, array, array_len);

	at_param_clear(param);
	param->size = array_len;
	param->type = AT_PARAM_TYPE_ARRAY;
	param->value.array_val = param_value;

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
			int16_t *value)
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

	if ((param->value.int_val > INT16_MAX) || (param->value.int_val < INT16_MIN)) {
		return -EINVAL;
	}

	*value = (int16_t)param->value.int_val;
	return 0;
}

int at_params_unsigned_short_get(const struct at_param_list *list, size_t index,
			uint16_t *value)
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

	if ((param->value.int_val > UINT16_MAX) || (param->value.int_val < 0)) {
		return -EINVAL;
	}

	*value = (uint16_t)param->value.int_val;
	return 0;
}

int at_params_int_get(const struct at_param_list *list, size_t index,
		      int32_t *value)
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

	if ((param->value.int_val > INT32_MAX) || (param->value.int_val < INT32_MIN)) {
		return -EINVAL;
	}

	*value = (int32_t)param->value.int_val;
	return 0;
}

int at_params_unsigned_int_get(const struct at_param_list *list, size_t index, uint32_t *value)
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

	if ((param->value.int_val > UINT32_MAX) || (param->value.int_val < 0)) {
		return -EINVAL;
	}

	*value = (uint32_t)param->value.int_val;
	return 0;
}

int at_params_int64_get(const struct at_param_list *list, size_t index, int64_t *value)
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

	if ((param->value.int_val > INT64_MAX) || (param->value.int_val < INT64_MIN)) {
		return -EINVAL;
	}

	*value = param->value.int_val;
	return 0;
}

int at_params_string_get(const struct at_param_list *list, size_t index,
			 char *value, size_t *len)
{
	if (list == NULL || list->params == NULL || value == NULL || len == NULL) {
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

	if (*len < param_len) {
		return -ENOMEM;
	}

	memcpy(value, param->value.str_val, param_len);
	*len = param_len;

	return 0;
}

int at_params_string_ptr_get(const struct at_param_list *list, size_t index, const char **at_param,
			     size_t *len)
{
	if (list == NULL || list->params == NULL || at_param == NULL || len == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL || param->type != AT_PARAM_TYPE_STRING) {
		return -EINVAL;
	}

	*at_param = param->value.str_val;
	*len = at_param_size(param);

	return 0;
}

int at_params_array_get(const struct at_param_list *list, size_t index,
			uint32_t *array, size_t *len)
{
	if (list == NULL || list->params == NULL || array == NULL ||
	    len == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	if (param->type != AT_PARAM_TYPE_ARRAY) {
		return -EINVAL;
	}

	size_t param_len = at_param_size(param);

	if (*len < param_len) {
		return -ENOMEM;
	}

	memcpy(array, param->value.array_val, param_len);
	*len = param_len;

	return 0;
}

uint32_t at_params_valid_count_get(const struct at_param_list *list)
{
	if (list == NULL || list->params == NULL) {
		return -EINVAL;
	}

	size_t valid_i = 0;
	struct at_param *param = at_params_get(list, valid_i);

	while (param != NULL && param->type != AT_PARAM_TYPE_INVALID) {
		valid_i += 1;
		param = at_params_get(list, valid_i);
	}

	return valid_i;
}

enum at_param_type at_params_type_get(const struct at_param_list *list,
				      size_t index)
{
	if (list == NULL || list->params == NULL) {
		return AT_PARAM_TYPE_INVALID;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return AT_PARAM_TYPE_INVALID;
	}

	return param->type;
}
