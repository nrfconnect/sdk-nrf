/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include "service_info.h"

#define SERVICE_INFO_JSON_NAME "serviceInfo"
#define UI_JSON_NAME "ui"
#define FOTAS_JSON_NAME "fota_v"
#define FOTAS_JSON_NAME_SIZE (sizeof(FOTAS_JSON_NAME) + 5)

static int add_array_obj(const char * const items[], const uint32_t item_cnt,
			 const char * const item_name, cJSON * const obj)
{
	cJSON *obj_to_add = NULL;
	cJSON *str = NULL;

	if ((obj == NULL) || (item_name == NULL)) {
		return -EINVAL;
	}

	obj_to_add = cJSON_CreateArray();
	if (obj_to_add == NULL) {
		return -ENOMEM;
	}

	for (uint32_t cnt = 0; cnt < item_cnt; ++cnt) {
		if (items[cnt] != NULL) {
			str = cJSON_CreateString(items[cnt]);
			if (str == NULL) {
				cJSON_Delete(obj_to_add);
				return -ENOMEM;
			}
			cJSON_AddItemToArray(obj_to_add, str);
		}
	}

	/* if no strings were added, use NULL object */
	if (cJSON_GetArraySize(obj_to_add) == 0) {
		obj_to_add->type = cJSON_NULL;
	}

	cJSON_AddItemToObject(obj, item_name, obj_to_add);

	return 0;
}

int service_info_json_object_encode(
	const char * const ui[], const uint32_t ui_count, const char * const fota[],
	const uint32_t fota_count, const uint16_t fota_version, cJSON * const obj_out)
{
	int err = 0;
	cJSON *service_info_obj = NULL;
	char fota_name[FOTAS_JSON_NAME_SIZE];

	if ((obj_out == NULL) || ((ui == NULL) && ui_count) ||
	    ((fota == NULL) && fota_count)) {
		return -EINVAL;
	}

	service_info_obj = cJSON_CreateObject();
	if (service_info_obj == NULL) {
		return -ENOMEM;
	}

	if (!err) {
		err = add_array_obj(ui, ui_count, UI_JSON_NAME,
				    service_info_obj);
	}

	if (!err) {
		snprintf(fota_name, sizeof(fota_name), "%s%hu", FOTAS_JSON_NAME,
			 fota_version);
		err = add_array_obj(fota, fota_count, fota_name,
				    service_info_obj);
		if (fota_version > 1) {
			/* Clear previous fota version in the shadow */
			snprintf(fota_name, sizeof(fota_name), "%s%hu",
				 FOTAS_JSON_NAME, fota_version - 1);
			cJSON_AddNullToObject(service_info_obj, fota_name);
		}
	}

	if (!err) {
		cJSON_AddItemToObject(obj_out, SERVICE_INFO_JSON_NAME,
				      service_info_obj);
	} else {
		cJSON_Delete(service_info_obj);
	}

	return err;
}
