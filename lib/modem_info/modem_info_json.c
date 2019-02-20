/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <string.h>
#include <stdlib.h>
#include <cJSON.h>
#include <modem_info.h>
#include <at_params.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(modem_info_json);

struct lte_info {
	enum modem_info info;
};

static const struct lte_info modem_info_band = {
	.info = MODEM_INFO_BAND,
};

static const struct lte_info modem_info_mode = {
	.info = MODEM_INFO_MODE,
};

static const struct lte_info modem_info_operator = {
	.info = MODEM_INFO_OPERATOR,
};

static const struct lte_info modem_info_cellid = {
	.info = MODEM_INFO_CELLID,
};

static const struct lte_info modem_info_ip_address = {
	.info = MODEM_INFO_IP_ADDRESS,
};

static const struct lte_info modem_info_uicc = {
	.info = MODEM_INFO_UICC,
};

static const struct lte_info modem_info_battery = {
	.info = MODEM_INFO_BATTERY,
};

static const struct lte_info modem_info_fw = {
	.info = MODEM_INFO_FW_VERSION,
};

static const struct lte_info *const modem_information[] = {
	&modem_info_band,
	&modem_info_mode,
	&modem_info_operator,
	&modem_info_cellid,
	&modem_info_ip_address,
	&modem_info_uicc,
	&modem_info_battery,
	&modem_info_fw,
};

static int json_add_obj(cJSON *parent, const char *str, cJSON *item)
{
	cJSON_AddItemToObject(parent, str, item);

	return 0;
}

static int json_add_num(cJSON *parent, const char *str, double num)
{
	cJSON *json_num;

	json_num = cJSON_CreateNumber(num);
	if (json_num == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_num);
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

int modem_info_json_string_get(char *buf)
{
	u16_t param_value;
	size_t len = 0;
	size_t total_len = 0;
	int ret = 0;
	char data_buffer[MODEM_INFO_MAX_RESPONSE_SIZE] = {0};
	char data_name[MODEM_INFO_MAX_RESPONSE_SIZE];
	cJSON *data_obj = cJSON_CreateObject();
	enum at_param_type data_type;

	if (data_obj == NULL) {
		return -ENOMEM;
	}

	for (size_t i = 0; i < ARRAY_SIZE(modem_information); i++) {
		memset(data_name, 0, MODEM_INFO_MAX_RESPONSE_SIZE);
		len = modem_info_name_get(modem_information[i]->info,
					  data_name);
		if (len < 0) {
			LOG_DBG("Data name not obtained: %d\n", len);
			continue;
		}

		data_type = modem_info_type_get(modem_information[i]->info);

		if (data_type == AT_PARAM_TYPE_STRING) {
			len = modem_info_string_get(modem_information[i]->info,
						    data_buffer);
			if (len < 0) {
				LOG_DBG("Link data not obtained: %d\n", len);
				continue;
			}

			total_len += len;
			ret += json_add_str(data_obj, data_name, data_buffer);
		} else if (data_type == AT_PARAM_TYPE_NUM_SHORT) {
			len = modem_info_short_get(modem_information[i]->info,
						   &param_value);
			if (len < 0) {
				LOG_DBG("Link data not obtained: %d\n", len);
				continue;
			}

			total_len += len;
			ret += json_add_num(data_obj, data_name, param_value);
		}
	}

	if (ret != 0) {
		cJSON_Delete(data_obj);
		return -ENOMEM;
	}

	memcpy(buf,
		cJSON_PrintUnformatted(data_obj),
		MODEM_INFO_JSON_STRING_SIZE);

	cJSON_Delete(data_obj);

	return total_len;
}
