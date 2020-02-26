/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <string.h>
#include <stdlib.h>
#include <cJSON.h>
#include <cJSON_os.h>
#include <modem_info.h>
#include <modem/at_params.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(modem_info_json);

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

	if (parent == NULL || str == NULL || item == NULL) {
		return -EINVAL;
	}

	json_str = cJSON_CreateString(item);
	if (json_str == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_str);
}

static int json_add_data(struct lte_param *param, cJSON *json_obj)
{
	char data_name[MODEM_INFO_MAX_RESPONSE_SIZE];
	enum at_param_type data_type;
	int total_len = 0;
	int ret;

	if (param == NULL || json_obj == NULL) {
		return -EINVAL;
	}

	memset(data_name, 0, MODEM_INFO_MAX_RESPONSE_SIZE);
	ret = modem_info_name_get(param->type,
				data_name);
	if (ret < 0) {
		LOG_DBG("Data name not obtained: %d", ret);
		return -EINVAL;
	}

	data_type = modem_info_type_get(param->type);
	if (data_type < 0) {
		return -EINVAL;
	}

	if (data_type == AT_PARAM_TYPE_STRING &&
	    param->type != MODEM_INFO_AREA_CODE) {
		total_len += strlen(param->value_string);
		ret += json_add_str(json_obj, data_name, param->value_string);
	} else {
		total_len += sizeof(u16_t);
		ret += json_add_num(json_obj, data_name, param->value);
	}

	if (ret < 0) {
		return ret;
	}

	return total_len;
}

static int network_data_add(struct network_param *network, cJSON *json_obj)
{
	char data_name[MODEM_INFO_MAX_RESPONSE_SIZE];
	int total_len;
	int ret;
	int len;

	static const char lte_string[]	 = "LTE-M";
	static const char nbiot_string[] = "NB-IoT";
	static const char gps_string[]	 = " GPS";

	if (network == NULL || json_obj == NULL) {
		return -EINVAL;
	}

	total_len = json_add_data(&network->current_band, json_obj);
	total_len += json_add_data(&network->sup_band, json_obj);
	total_len += json_add_data(&network->area_code, json_obj);
	total_len += json_add_data(&network->current_operator, json_obj);
	total_len += json_add_data(&network->ip_address, json_obj);
	total_len += json_add_data(&network->ue_mode, json_obj);

	len = modem_info_name_get(network->cellid_hex.type, data_name);
	data_name[len] =  '\0';
	ret = json_add_num(json_obj, data_name, network->cellid_dec);

	if (ret) {
		LOG_DBG("Unable to add the cell ID.");
	} else {
		total_len += sizeof(double);
	}

	if (network->lte_mode.value == 1) {
		strcat(network->network_mode, lte_string);
		total_len += sizeof(lte_string);
	} else if (network->nbiot_mode.value == 1) {
		strcat(network->network_mode, nbiot_string);
		total_len += sizeof(nbiot_string);
	}

	if (network->gps_mode.value == 1) {
		strcat(network->network_mode, gps_string);
		total_len += sizeof(gps_string);
	}

	ret = json_add_str(json_obj, "networkMode", network->network_mode);

	if (ret) {
		printk("Unable to add the network mode");
	}

	return total_len;
}

static int sim_data_add(struct sim_param *sim, cJSON *json_obj)
{
	int total_len;

	if (sim == NULL || json_obj == NULL) {
		return -EINVAL;
	}

	total_len = json_add_data(&sim->uicc, json_obj);
	total_len += json_add_data(&sim->iccid, json_obj);
	total_len += json_add_data(&sim->imsi, json_obj);

	return total_len;
}

static int device_data_add(struct device_param *device, cJSON *json_obj)
{
	int total_len;

	if (device == NULL || json_obj == NULL) {
		return -EINVAL;
	}

	total_len = json_add_data(&device->modem_fw, json_obj);
	total_len += json_add_data(&device->battery, json_obj);
	total_len += json_add_data(&device->imei, json_obj);
	total_len += json_add_str(json_obj, "board", device->board);
	total_len += json_add_str(json_obj, "appVersion", device->app_version);
	total_len += json_add_str(json_obj, "appName", device->app_name);

	return total_len;
}

int modem_info_json_object_encode(struct modem_param_info *modem,
				  cJSON *root_obj)
{
	if (root_obj == NULL || modem == NULL) {
		return -EINVAL;
	}

	int obj_count = cJSON_GetArraySize(root_obj);

	cJSON *network_obj	= cJSON_CreateObject();
	cJSON *sim_obj		= cJSON_CreateObject();
	cJSON *device_obj	= cJSON_CreateObject();

	if (network_obj == NULL || sim_obj == NULL || device_obj == NULL) {
		obj_count = -ENOMEM;
		goto delete_object;
	}

	if (IS_ENABLED(CONFIG_MODEM_INFO_ADD_NETWORK) &&
	    (network_data_add(&modem->network, network_obj) > 0)) {

		json_add_obj(root_obj, "networkInfo", network_obj);
		network_obj = NULL;
	}

	if (IS_ENABLED(CONFIG_MODEM_INFO_ADD_SIM) &&
	    (sim_data_add(&modem->sim, sim_obj) > 0)) {

		json_add_obj(root_obj, "simInfo", sim_obj);
		sim_obj = NULL;
	}

	if (IS_ENABLED(CONFIG_MODEM_INFO_ADD_DEVICE) &&
	    (device_data_add(&modem->device, device_obj) > 0)) {

		json_add_obj(root_obj, "deviceInfo", device_obj);
		device_obj = NULL;
	}

delete_object:
	cJSON_Delete(network_obj);
	cJSON_Delete(sim_obj);
	cJSON_Delete(device_obj);

	if (obj_count >= 0) {
		obj_count = cJSON_GetArraySize(root_obj) - obj_count;
	}

	return obj_count;
}

int modem_info_json_string_encode(struct modem_param_info *modem,
				  char *buf)
{
	int total_len = 0;
	int ret;

	cJSON *root_obj		= cJSON_CreateObject();
	cJSON *network_obj	= cJSON_CreateObject();
	cJSON *sim_obj		= cJSON_CreateObject();
	cJSON *device_obj	= cJSON_CreateObject();

	if (root_obj == NULL || network_obj == NULL || buf == NULL ||
	    sim_obj == NULL || device_obj == NULL) {
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_MODEM_INFO_ADD_NETWORK)) {
		ret = network_data_add(&modem->network, network_obj);
		if (ret < 0) {
			total_len = ret;
			goto delete_object;
		}

		total_len += ret;
		json_add_obj(root_obj, "networkInfo", network_obj);
	}

	if (IS_ENABLED(CONFIG_MODEM_INFO_ADD_SIM)) {
		ret = sim_data_add(&modem->sim, sim_obj);
		if (ret < 0) {
			total_len = ret;
			goto delete_object;
		}

		total_len += ret;
		json_add_obj(root_obj, "simInfo", sim_obj);
	}

	if (IS_ENABLED(CONFIG_MODEM_INFO_ADD_DEVICE)) {
		ret = device_data_add(&modem->device, device_obj);
		if (ret < 0) {
			total_len = ret;
			goto delete_object;
		}

		total_len += ret;
		json_add_obj(root_obj, "deviceInfo", device_obj);
	}

	if (total_len > 0) {
		char *root_obj_string = cJSON_PrintUnformatted(root_obj);

		memcpy(buf, root_obj_string,
		       MODEM_INFO_JSON_STRING_SIZE);
		cJSON_FreeString(root_obj_string);
	}

delete_object:
	cJSON_Delete(root_obj);

	return total_len;
}
