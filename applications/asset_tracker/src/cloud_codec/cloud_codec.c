/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */


#include <stdbool.h>
#include <string.h>
#include <zephyr.h>
#include <zephyr/types.h>

#include "cJSON.h"
#include "cJSON_os.h"
#include "cloud_codec.h"


static const char * const sensor_type_str[] = {
	[CLOUD_SENSOR_GPS] = "GPS",
	[CLOUD_SENSOR_FLIP] = "FLIP",
	[CLOUD_SENSOR_BUTTON] = "BUTTON",
	[CLOUD_SENSOR_TEMP] = "TEMP",
	[CLOUD_SENSOR_HUMID] = "HUMID",
	[CLOUD_SENSOR_AIR_PRESS] = "AIR_PRESS",
	[CLOUD_SENSOR_AIR_QUAL] = "AIR_QUAL",
	[CLOUD_LTE_LINK_RSRP] = "RSRP",
	[CLOUD_DEVICE_INFO] = "DEVICE",
};

/* --- A few wrappers for cJSON APIs --- */

static int json_add_obj(cJSON *parent, const char *str, cJSON *item)
{
	cJSON_AddItemToObject(parent, str, item);

	return 0;
}

static int json_add_str(cJSON *parent, const char *str, const char *item)
{
	cJSON *json_str;

	json_str = cJSON_CreateString(item);
	if (json_str == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_str);
}

int cloud_encode_sensor_data(const struct cloud_sensor_data *const sensor,
				 struct cloud_data *const output)
{
	int ret;

	__ASSERT_NO_MSG(sensor != NULL);
	__ASSERT_NO_MSG(sensor->data.buf != NULL);
	__ASSERT_NO_MSG(sensor->data.len != 0);
	__ASSERT_NO_MSG(output != NULL);

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		return -ENOMEM;
	}

	ret = json_add_str(root_obj, "appId", sensor_type_str[sensor->type]);
	ret += json_add_str(root_obj, "data", sensor->data.buf);
	ret += json_add_str(root_obj, "messageType", "DATA");

	if (ret != 0) {
		cJSON_Delete(root_obj);
		return -ENOMEM;
	}

	char *buffer;

	buffer = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);

	output->buf = buffer;
	output->len = strlen(buffer);

	return 0;
}
