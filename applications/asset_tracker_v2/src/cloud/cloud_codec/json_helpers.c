/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <sys/printk.h>

#include "json_helpers.h"
#include "cJSON.h"
#include "cJSON_os.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(json_helpers, CONFIG_CLOUD_CODEC_LOG_LEVEL);

void json_add_obj(cJSON *parent, const char *str, cJSON *item)
{
	if (!cJSON_AddItemToObject(parent, str, item)) {
		LOG_ERR("Failed adding object");
		cJSON_Delete(item);
	}
}

void json_add_obj_array(cJSON *parent, cJSON *item)
{
	if (!cJSON_AddItemToArray(parent, item)) {
		LOG_ERR("Failed adding array item");
		cJSON_Delete(item);
	}
}

int json_add_number(cJSON *parent, const char *str, double item)
{
	cJSON *json_num;

	json_num = cJSON_CreateNumber(item);
	if (json_num == NULL) {
		return -ENOMEM;
	}

	json_add_obj(parent, str, json_num);

	return 0;
}

int json_add_bool(cJSON *parent, const char *str, int item)
{
	cJSON *json_bool;

	json_bool = cJSON_CreateBool(item);
	if (json_bool == NULL) {
		return -ENOMEM;
	}

	json_add_obj(parent, str, json_bool);

	return 0;
}

cJSON *json_object_decode(cJSON *obj, const char *str)
{
	return obj ? cJSON_GetObjectItem(obj, str) : NULL;
}

int json_add_str(cJSON *parent, const char *str, const char *item)
{
	cJSON *json_str;

	json_str = cJSON_CreateString(item);
	if (json_str == NULL) {
		return -ENOMEM;
	}

	json_add_obj(parent, str, json_str);

	return 0;
}

void json_print_obj(const char *prefix, const cJSON *obj)
{
	char *string = cJSON_Print(obj);

	if (string == NULL) {
		LOG_ERR("Failed to print object content");
		return;
	}

	printk("%s%s\n", prefix, string);

	cJSON_FreeString(string);
}
