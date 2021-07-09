/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf_cloud_codec.h"
#include "nrf_cloud_mem.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <zephyr.h>
#include <logging/log.h>
#include <modem/modem_info.h>
#include "cJSON_os.h"
#include "nrf_cloud_fota.h"

LOG_MODULE_REGISTER(nrf_cloud_codec, CONFIG_NRF_CLOUD_LOG_LEVEL);

#define PGPS_RCV_ARRAY_IDX_HOST 0
#define PGPS_RCV_ARRAY_IDX_PATH 1
#define PGPS_RCV_REST_HOST "host"
#define PGPS_RCV_REST_PATH "path"

#define DUA_PIN_STR "not_associated"
#define TIMEOUT_STR "timeout"
#define PAIRED_STR "paired"

#define RSRP_ADJ(rsrp) (rsrp - ((rsrp <= 0) ? 140 : 141))

#if defined(CONFIG_NRF_CLOUD_MQTT)
static const char *const sensor_type_str[] = {
	[NRF_CLOUD_SENSOR_GPS] = "GPS",
	[NRF_CLOUD_SENSOR_FLIP] = "FLIP",
	[NRF_CLOUD_SENSOR_BUTTON] = "BUTTON",
	[NRF_CLOUD_SENSOR_TEMP] = "TEMP",
	[NRF_CLOUD_SENSOR_HUMID] = "HUMID",
	[NRF_CLOUD_SENSOR_AIR_PRESS] = "AIR_PRESS",
	[NRF_CLOUD_SENSOR_AIR_QUAL] = "AIR_QUAL",
	[NRF_CLOUD_LTE_LINK_RSRP] = "RSRP",
	[NRF_CLOUD_DEVICE_INFO] = "DEVICE",
};
#endif

int nrf_cloud_codec_init(void)
{
	cJSON_Init();

	return 0;
}

static char *json_strdup(cJSON *const string_obj)
{
	char *dest;
	char *src = cJSON_GetStringValue(string_obj);

	if (!src) {
		return NULL;
	}

	dest = nrf_cloud_calloc(strlen(src) + 1, 1);
	if (dest) {
		strcpy(dest, src);
	}

	return dest;
}

static int get_modem_info(struct modem_param_info *const modem_info)
{
	__ASSERT_NO_MSG(modem_info != NULL);

	int err = modem_info_init();

	if (err) {
		LOG_ERR("Could not initialize modem info module, error: %d",
			err);
		return err;
	}

	err = modem_info_params_init(modem_info);
	if (err) {
		LOG_ERR("Could not initialize modem info parameters, error: %d",
			err);
		return err;
	}

	err = modem_info_params_get(modem_info);
	if (err) {
		LOG_ERR("Could not obtain cell information, error: %d",
			err);
		return err;
	}

	return 0;
}

static int json_format_modem_info_data_obj(cJSON *const data_obj,
	const struct modem_param_info *const modem_info)
{
	__ASSERT_NO_MSG(data_obj != NULL);
	__ASSERT_NO_MSG(modem_info != NULL);

	if (!cJSON_AddNumberToObject(data_obj, NRF_CLOUD_JSON_MCC_KEY,
		modem_info->network.mcc.value) ||
	    !cJSON_AddNumberToObject(data_obj, NRF_CLOUD_JSON_MNC_KEY,
		modem_info->network.mnc.value) ||
	    !cJSON_AddNumberToObject(data_obj, NRF_CLOUD_JSON_AREA_CODE_KEY,
		modem_info->network.area_code.value) ||
	    !cJSON_AddNumberToObject(data_obj, NRF_CLOUD_JSON_CELL_ID_KEY,
		(uint32_t)modem_info->network.cellid_dec) ||
	    !cJSON_AddNumberToObject(data_obj, NRF_CLOUD_JSON_PHYCID_KEY, 0)) {
		return -ENOMEM;
	}

	return 0;
}

int nrf_cloud_json_add_modem_info(cJSON *const data_obj)
{
	__ASSERT_NO_MSG(data_obj != NULL);

	struct modem_param_info modem_info = {0};
	int err;

	err = get_modem_info(&modem_info);
	if (err) {
		return err;
	}

	return json_format_modem_info_data_obj(data_obj, &modem_info);
}

static int json_add_obj(cJSON *parent, const char *str, cJSON *item)
{
	return cJSON_AddItemToObject(parent, str, item) ? 0 : -ENOMEM;
}

static int json_add_num(cJSON *parent, const char *str, double item)
{
	cJSON *json_num;

	json_num = cJSON_CreateNumber(item);
	if (json_num == NULL) {
		return -ENOMEM;
	}

	if (json_add_obj(parent, str, json_num)) {
		cJSON_Delete(json_num);
		return -ENOMEM;
	}

	return 0;
}

#if defined(CONFIG_NRF_CLOUD_MQTT)
static int json_add_null(cJSON *parent, const char *str)
{
	cJSON *json_null;

	json_null = cJSON_CreateNull();
	if (json_null == NULL) {
		return -ENOMEM;
	}

	if (json_add_obj(parent, str, json_null)) {
		cJSON_Delete(json_null);
		return -ENOMEM;
	}

	return 0;
}

static cJSON *json_object_decode(cJSON *obj, const char *str)
{
	return obj ? cJSON_GetObjectItem(obj, str) : NULL;
}

static int json_decode_and_alloc(cJSON *obj, struct nrf_cloud_data *data)
{
	if (!data || !cJSON_IsString(obj)) {
		return -EINVAL;
	}

	data->ptr = json_strdup(obj);

	if (data->ptr == NULL) {
		return -ENOMEM;
	}

	data->len = strlen(data->ptr);

	return 0;
}

static int json_add_str(cJSON *parent, const char *str, const char *item)
{
	cJSON *json_str;

	json_str = cJSON_CreateString(item);
	if (json_str == NULL) {
		return -ENOMEM;
	}

	if (json_add_obj(parent, str, json_str)) {
		cJSON_Delete(json_str);
		return -ENOMEM;
	}

	return 0;
}

static bool compare(const char *s1, const char *s2)
{
	return !strncmp(s1, s2, strlen(s2));
}

static void nrf_cloud_decode_desired_obj(cJSON *const root_obj,
					 cJSON **desired_obj)
{
	cJSON *state_obj;

	if ((root_obj != NULL) && (desired_obj != NULL)) {
		/* On initial pairing, a shadow delta event is sent */
		/* which does not include the "desired" JSON key, */
		/* "state" is used instead */
		state_obj = json_object_decode(root_obj, "state");
		if (state_obj == NULL) {
			*desired_obj = json_object_decode(root_obj, "desired");
		} else {
			*desired_obj = state_obj;
		}
	}
}

int nrf_cloud_encode_shadow_data(const struct nrf_cloud_sensor_data *sensor,
				 struct nrf_cloud_data *output)
{
	int ret;
	char *buffer;

	__ASSERT_NO_MSG(sensor != NULL);
	__ASSERT_NO_MSG(sensor->data.ptr != NULL);
	__ASSERT_NO_MSG(sensor->data.len != 0);
	__ASSERT_NO_MSG(output != NULL);

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *reported_obj = cJSON_CreateObject();

	if (root_obj == NULL || state_obj == NULL || reported_obj == NULL) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(reported_obj);
		return -ENOMEM;
	}

	ret = json_add_obj(reported_obj, sensor_type_str[sensor->type],
			   (cJSON *)sensor->data.ptr);
	ret += json_add_obj(state_obj, "reported", reported_obj);
	ret += json_add_obj(root_obj, "state", state_obj);

	if (ret != 0) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(reported_obj);
	}

	buffer = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);

	output->ptr = buffer;
	output->len = strlen(buffer);

	return 0;
}

int nrf_cloud_encode_sensor_data(const struct nrf_cloud_sensor_data *sensor,
				 struct nrf_cloud_data *output)
{
	int ret;

	__ASSERT_NO_MSG(sensor != NULL);
	__ASSERT_NO_MSG(sensor->data.ptr != NULL);
	__ASSERT_NO_MSG(sensor->data.len != 0);
	__ASSERT_NO_MSG(output != NULL);

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		return -ENOMEM;
	}

	ret = json_add_str(root_obj, "appId", sensor_type_str[sensor->type]);
	ret += json_add_str(root_obj, "data", sensor->data.ptr);
	ret += json_add_str(root_obj, "messageType", "DATA");

	if (ret != 0) {
		cJSON_Delete(root_obj);
		return -ENOMEM;
	}

	char *buffer;

	buffer = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);

	output->ptr = buffer;
	output->len = strlen(buffer);

	return 0;
}

int nrf_cloud_decode_requested_state(const struct nrf_cloud_data *input,
				     enum nfsm_state *requested_state)
{
	__ASSERT_NO_MSG(requested_state != NULL);
	__ASSERT_NO_MSG(input != NULL);
	__ASSERT_NO_MSG(input->ptr != NULL);
	__ASSERT_NO_MSG(input->len != 0);

	cJSON *root_obj;
	cJSON *desired_obj;
	cJSON *pairing_obj;
	cJSON *pairing_state_obj;
	cJSON *topic_prefix_obj;

	root_obj = cJSON_Parse(input->ptr);
	if (root_obj == NULL) {
		LOG_ERR("cJSON_Parse failed: %s",
			log_strdup((char *)input->ptr));
		return -ENOENT;
	}

	nrf_cloud_decode_desired_obj(root_obj, &desired_obj);

	topic_prefix_obj =
		json_object_decode(desired_obj, "nrfcloud_mqtt_topic_prefix");
	if (topic_prefix_obj != NULL) {
		(*requested_state) = STATE_UA_PIN_COMPLETE;
		cJSON_Delete(root_obj);
		return 0;
	}

	pairing_obj = json_object_decode(desired_obj, "pairing");
	pairing_state_obj = json_object_decode(pairing_obj, "state");

	if (!pairing_state_obj || pairing_state_obj->type != cJSON_String) {
		if (cJSON_HasObjectItem(desired_obj, "config") == false) {
			LOG_WRN("Unhandled data received from nRF Cloud.");
			LOG_INF("Ensure device firmware is up to date.");
			LOG_INF("Delete and re-add device to nRF Cloud if problem persists.");
		}
		cJSON_Delete(root_obj);
		return -ENOENT;
	}

	const char *state_str = pairing_state_obj->valuestring;

	if (compare(state_str, DUA_PIN_STR)) {
		(*requested_state) = STATE_UA_PIN_WAIT;
	} else {
		LOG_ERR("Deprecated state. Delete device from nRF Cloud and update device with JITP certificates.");
		cJSON_Delete(root_obj);
		return -ENOTSUP;
	}

	cJSON_Delete(root_obj);

	return 0;
}

int nrf_cloud_encode_config_response(struct nrf_cloud_data const *const input,
				     struct nrf_cloud_data *const output,
				     bool *const has_config)
{
	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(input != NULL);

	char *buffer = NULL;
	cJSON *root_obj = NULL;
	cJSON *desired_obj = NULL;
	cJSON *null_obj = NULL;
	cJSON *reported_obj = NULL;
	cJSON *state_obj = NULL;
	cJSON *config_obj = NULL;
	cJSON *input_obj = input ? cJSON_Parse(input->ptr) : NULL;

	if (input_obj == NULL) {
		return -ESRCH; /* invalid input or no JSON parsed */
	}

	/* A delta update will have the config inside of state */
	state_obj = cJSON_DetachItemFromObject(input_obj, "state");
	config_obj = cJSON_DetachItemFromObject(
		state_obj ? state_obj : input_obj, "config");
	cJSON_Delete(input_obj);

	if (has_config) {
		*has_config = (config_obj != NULL);
	}

	/* If this is not a delta update, no response data is required */
	if ((state_obj == NULL) || (config_obj == NULL)) {
		cJSON_Delete(state_obj);
		cJSON_Delete(config_obj);

		output->ptr = NULL;
		output->len = 0;
		return 0;
	}

	/* Prepare JSON response for the delta */
	root_obj = cJSON_CreateObject();
	desired_obj = cJSON_CreateObject();
	null_obj = cJSON_CreateNull();
	reported_obj = cJSON_CreateObject();

	if ((root_obj == NULL) || (desired_obj == NULL) || (null_obj == NULL) ||
		(reported_obj == NULL)) {
		cJSON_Delete(root_obj);
		cJSON_Delete(desired_obj);
		cJSON_Delete(null_obj);
		cJSON_Delete(reported_obj);
		cJSON_Delete(config_obj);
		cJSON_Delete(state_obj);
		return -ENOMEM;
	}

	/* Add delta config to reported */
	(void)json_add_obj(reported_obj, "config", config_obj);
	(void)json_add_obj(root_obj, "reported", reported_obj);

	/* Add a null config to desired */
	(void)json_add_obj(desired_obj, "config", null_obj);
	(void)json_add_obj(root_obj, "desired", desired_obj);

	/* Cleanup received state obj and re-use for the response */
	cJSON_Delete(state_obj);
	state_obj = cJSON_CreateObject();
	(void)json_add_obj(state_obj, "state", root_obj);
	buffer = cJSON_PrintUnformatted(state_obj);
	cJSON_Delete(state_obj);

	if (buffer == NULL) {
		return -ENOMEM;
	}

	output->ptr = buffer;
	output->len = strlen(buffer);

	return 0;
}

int nrf_cloud_encode_state(uint32_t reported_state, struct nrf_cloud_data *output)
{
	int ret;

	__ASSERT_NO_MSG(output != NULL);

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *reported_obj = cJSON_CreateObject();
	cJSON *pairing_obj = cJSON_CreateObject();
	cJSON *connection_obj = cJSON_CreateObject();

	if ((root_obj == NULL) || (state_obj == NULL) ||
	    (reported_obj == NULL) || (pairing_obj == NULL) ||
	    (connection_obj == NULL)) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(reported_obj);
		cJSON_Delete(pairing_obj);
		cJSON_Delete(connection_obj);

		return -ENOMEM;
	}

	ret = 0;

	switch (reported_state) {
	case STATE_UA_PIN_WAIT: {
		ret += json_add_str(pairing_obj, "state", DUA_PIN_STR);
		ret += json_add_null(pairing_obj, "topics");
		ret += json_add_null(pairing_obj, "config");
		ret += json_add_null(reported_obj, "stage");
		ret += json_add_null(reported_obj,
				     "nrfcloud_mqtt_topic_prefix");
		ret += json_add_null(connection_obj, "keepalive");
		break;
	}
	case STATE_UA_PIN_COMPLETE: {
		struct nrf_cloud_data rx_endp;
		struct nrf_cloud_data tx_endp;
		struct nrf_cloud_data m_endp;

		/* Get the endpoint information. */
		nct_dc_endpoint_get(&tx_endp, &rx_endp, &m_endp);
		ret += json_add_str(reported_obj, "nrfcloud_mqtt_topic_prefix",
				    m_endp.ptr);

		/* Clear pairing config and pairingStatus fields. */
		ret += json_add_str(pairing_obj, "state", PAIRED_STR);
		ret += json_add_null(pairing_obj, "config");
		ret += json_add_null(reported_obj, "pairingStatus");

		/* Report keepalive value. */
		if (cJSON_AddNumberToObject(connection_obj, "keepalive",
					    CONFIG_MQTT_KEEPALIVE) == NULL) {
			ret = -ENOMEM;
		}

		/* Report pairing topics. */
		cJSON *topics_obj = cJSON_CreateObject();

		if (topics_obj == NULL) {
			cJSON_Delete(root_obj);
			cJSON_Delete(state_obj);
			cJSON_Delete(reported_obj);
			cJSON_Delete(pairing_obj);
			cJSON_Delete(connection_obj);

			return -ENOMEM;
		}

		ret += json_add_str(topics_obj, "d2c", tx_endp.ptr);
		ret += json_add_str(topics_obj, "c2d", rx_endp.ptr);
		ret += json_add_obj(pairing_obj, "topics", topics_obj);

		if (ret != 0) {
			cJSON_Delete(topics_obj);
		}
		break;
	}
	default: {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(reported_obj);
		cJSON_Delete(pairing_obj);
		cJSON_Delete(connection_obj);
		return -ENOTSUP;
	}
	}

	ret += json_add_obj(reported_obj, "pairing", pairing_obj);
	ret += json_add_obj(reported_obj, "connection", connection_obj);
	ret += json_add_obj(state_obj, "reported", reported_obj);
	ret += json_add_obj(root_obj, "state", state_obj);

	if (ret != 0) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(reported_obj);
		cJSON_Delete(pairing_obj);
		cJSON_Delete(connection_obj);

		return -ENOMEM;
	}

	char *buffer;

	buffer = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);

	if (buffer == NULL) {
		return -ENOMEM;
	}

	output->ptr = buffer;
	output->len = strlen(buffer);

	return 0;
}

/**
 * @brief Decodes data endpoint information.
 *
 * @param[in] input Input to be decoded.
 *
 * @retval 0 or an error code indicating reason for failure
 */
int nrf_cloud_decode_data_endpoint(const struct nrf_cloud_data *input,
				   struct nrf_cloud_data *tx_endpoint,
				   struct nrf_cloud_data *rx_endpoint,
				   struct nrf_cloud_data *m_endpoint)
{
	__ASSERT_NO_MSG(input != NULL);
	__ASSERT_NO_MSG(input->ptr != NULL);
	__ASSERT_NO_MSG(input->len != 0);
	__ASSERT_NO_MSG(tx_endpoint != NULL);
	__ASSERT_NO_MSG(rx_endpoint != NULL);

	int err;
	cJSON *root_obj;
	cJSON *m_endpoint_obj = NULL;
	cJSON *desired_obj = NULL;

	root_obj = cJSON_Parse(input->ptr);
	if (root_obj == NULL) {
		return -ENOENT;
	}

	nrf_cloud_decode_desired_obj(root_obj, &desired_obj);

	if (m_endpoint != NULL) {
		m_endpoint_obj = json_object_decode(
			desired_obj, "nrfcloud_mqtt_topic_prefix");
	}

	cJSON *pairing_obj = json_object_decode(desired_obj, "pairing");
	cJSON *pairing_state_obj = json_object_decode(pairing_obj, "state");
	cJSON *topic_obj = json_object_decode(pairing_obj, "topics");

	if ((pairing_state_obj == NULL) || (topic_obj == NULL) ||
	    (pairing_state_obj->type != cJSON_String)) {
		cJSON_Delete(root_obj);
		return -ENOENT;
	}

	const char *state_str = pairing_state_obj->valuestring;

	if (!compare(state_str, PAIRED_STR)) {
		cJSON_Delete(root_obj);
		return -ENOENT;
	}

	if (m_endpoint_obj != NULL) {
		err = json_decode_and_alloc(m_endpoint_obj, m_endpoint);
		if (err) {
			cJSON_Delete(root_obj);
			return err;
		}
	}

	cJSON *tx_obj = json_object_decode(topic_obj, "d2c");

	err = json_decode_and_alloc(tx_obj, tx_endpoint);
	if (err) {
		cJSON_Delete(root_obj);
		return err;
	}

	cJSON *rx_obj = json_object_decode(topic_obj, "c2d");

	err = json_decode_and_alloc(rx_obj, rx_endpoint);
	if (err) {
		cJSON_Delete(root_obj);
		return err;
	}

	cJSON_Delete(root_obj);

	return err;
}
#endif /* CONFIG_NRF_CLOUD_MQTT */

int nrf_cloud_parse_cell_pos_json(const cJSON *const cell_loc_obj,
	struct nrf_cloud_cell_pos_result *const location_out)
{
	if (!cell_loc_obj || !location_out) {
		return -EINVAL;
	}

	cJSON *lat, *lon, *unc;
	cJSON *loc_obj;

	/* TODO: new payload is slightly different... handle both for now */
	loc_obj = cJSON_GetObjectItem(cell_loc_obj,
				  NRF_CLOUD_CELL_POS_JSON_KEY_LOC);
	if (loc_obj) {
		lat = cJSON_GetObjectItem(loc_obj,
				  NRF_CLOUD_CELL_POS_JSON_KEY_LAT);
		lon = cJSON_GetObjectItem(loc_obj,
					NRF_CLOUD_CELL_POS_JSON_KEY_LNG);
		unc = cJSON_GetObjectItem(cell_loc_obj,
					NRF_CLOUD_CELL_POS_JSON_KEY_ACC);
	} else {
		lat = cJSON_GetObjectItem(cell_loc_obj,
				  NRF_CLOUD_CELL_POS_JSON_KEY_LAT);
		lon = cJSON_GetObjectItem(cell_loc_obj,
					NRF_CLOUD_CELL_POS_JSON_KEY_LON);
		unc = cJSON_GetObjectItem(cell_loc_obj,
					NRF_CLOUD_CELL_POS_JSON_KEY_UNCERT);
	}


	if (!cJSON_IsNumber(lat) || !cJSON_IsNumber(lon) ||
	    !cJSON_IsNumber(unc)) {
		LOG_DBG("Expected items not found in cell-pos msg");
		return -EBADMSG;
	}

	location_out->lat = lat->valuedouble;
	location_out->lon = lon->valuedouble;
	location_out->unc = (uint32_t)unc->valueint;

	return 0;
}

int nrf_cloud_parse_cell_pos(const char *const response,
	struct nrf_cloud_cell_pos_result *const location_out)
{
	cJSON *resp_obj = cJSON_Parse(response);

	int ret = nrf_cloud_parse_cell_pos_json(resp_obj, location_out);

	cJSON_Delete(resp_obj);

	return ret;
}

void nrf_cloud_fota_job_free(struct nrf_cloud_fota_job_info *const job)
{
	if (!job) {
		return;
	}

	if (job->id) {
		nrf_cloud_free(job->id);
		job->id = NULL;
	}

	if (job->host) {
		nrf_cloud_free(job->host);
		job->host = NULL;
	}

	if (job->path) {
		nrf_cloud_free(job->path);
		job->path = NULL;
	}
}

int nrf_cloud_rest_fota_execution_parse(const char *const response,
	struct nrf_cloud_fota_job_info *const job)
{
	if (!response || !job) {
		return -EINVAL;
	}

	int ret = 0;
	char *type;
	cJSON *path_obj;
	cJSON *host_obj;
	cJSON *type_obj;
	cJSON *size_obj;

	cJSON *resp_obj = cJSON_Parse(response);
	cJSON *job_doc  = cJSON_GetObjectItem(resp_obj, NRF_CLOUD_FOTA_REST_KEY_JOB_DOC);
	cJSON *id_obj   = cJSON_GetObjectItem(resp_obj, NRF_CLOUD_FOTA_REST_KEY_JOB_ID);

	memset(job, 0, sizeof(*job));

	if (!job_doc || !id_obj) {
		ret = -EBADMSG;
		goto err_cleanup;
	}

	path_obj = cJSON_GetObjectItem(job_doc, NRF_CLOUD_FOTA_REST_KEY_PATH);
	host_obj = cJSON_GetObjectItem(job_doc, NRF_CLOUD_FOTA_REST_KEY_HOST);
	type_obj = cJSON_GetObjectItem(job_doc, NRF_CLOUD_FOTA_REST_KEY_TYPE);
	size_obj = cJSON_GetObjectItem(job_doc, NRF_CLOUD_FOTA_REST_KEY_SIZE);

	if (!id_obj || !path_obj || !host_obj || !type_obj || !size_obj) {
		ret = -EFTYPE;
		goto err_cleanup;
	}

	if (!cJSON_IsNumber(size_obj)) {
		ret = -ENOMSG;
		goto err_cleanup;
	}
	job->file_size	= size_obj->valueint;

	job->id		= json_strdup(id_obj);
	job->path	= json_strdup(path_obj);
	job->host	= json_strdup(host_obj);

	if (!job->id || !job->path || !job->host) {
		ret = -ENOSTR;
		goto err_cleanup;
	}

	type = cJSON_GetStringValue(type_obj);
	if (!type) {
		ret = -ENODATA;
		goto err_cleanup;
	}

	if (!strcmp(type, NRF_CLOUD_FOTA_REST_VAL_TYPE_MODEM)) {
		job->type = NRF_CLOUD_FOTA_MODEM;
	} else if (!strcmp(type, NRF_CLOUD_FOTA_REST_VAL_TYPE_BOOT)) {
		job->type = NRF_CLOUD_FOTA_BOOTLOADER;
	} else if (!strcmp(type, NRF_CLOUD_FOTA_REST_VAL_TYPE_APP)) {
		job->type = NRF_CLOUD_FOTA_APPLICATION;
	} else {
		LOG_WRN("Unhandled FOTA type: %s", log_strdup(type));
		job->type = NRF_CLOUD_FOTA_TYPE__INVALID;
	}

	cJSON_Delete(resp_obj);

	return 0;

err_cleanup:
	if (resp_obj) {
		cJSON_Delete(resp_obj);
	}

	nrf_cloud_fota_job_free(job);
	memset(job, 0, sizeof(*job));
	job->type = NRF_CLOUD_FOTA_TYPE__INVALID;

	return ret;
}

int nrf_cloud_parse_pgps_response(const char *const response,
	struct nrf_cloud_pgps_result *const result)
{
	if (!response || !result ||
	    !result->host || !result->host_sz ||
	    !result->path || !result->path_sz) {
		return -EINVAL;
	}

	char *host_ptr = NULL;
	char *path_ptr = NULL;
	int err = 0;
	cJSON *rsp_obj = cJSON_Parse(response);

	if (!rsp_obj) {
		LOG_ERR("P-GPS response does not contain valid JSON");
		err = -EBADMSG;
		goto cleanup;
	}

	/* MQTT response is an array, REST is key/value map */
	if (cJSON_IsArray(rsp_obj)) {
		if (get_string_from_array(rsp_obj, PGPS_RCV_ARRAY_IDX_HOST, &host_ptr) ||
		    get_string_from_array(rsp_obj, PGPS_RCV_ARRAY_IDX_PATH, &path_ptr)) {
			LOG_ERR("Invalid P-GPS array response format");
			err = -EFTYPE;
			goto cleanup;
		}
	} else if (get_string_from_obj(rsp_obj, PGPS_RCV_REST_HOST, &host_ptr) ||
		   get_string_from_obj(rsp_obj, PGPS_RCV_REST_PATH, &path_ptr)) {
		LOG_ERR("Invalid P-GPS REST response format");
		err = -EFTYPE;
		goto cleanup;
	}

	if (!host_ptr || !path_ptr) {
		err = -ENOSTR;
		goto cleanup;
	}

	if ((result->host_sz <= strlen(host_ptr)) ||
	    (result->path_sz <= strlen(path_ptr))) {
		err = -ENOBUFS;
		goto cleanup;
	}

	strncpy(result->host, host_ptr, result->host_sz);
	LOG_DBG("host: %s", log_strdup(result->host));

	strncpy(result->path, path_ptr, result->path_sz);
	LOG_DBG("path: %s", log_strdup(result->path));

cleanup:
	if (rsp_obj) {
		cJSON_Delete(rsp_obj);
	}
	return err;
}

int get_string_from_array(const cJSON *const array, const int index,
			  char **string_out)
{
	__ASSERT_NO_MSG(string_out != NULL);

	cJSON *item = cJSON_GetArrayItem(array, index);

	if (!cJSON_IsString(item)) {
		return -EINVAL;
	}

	*string_out = item->valuestring;

	return 0;
}

int get_string_from_obj(const cJSON *const obj, const char *const key,
			char **string_out)
{
	__ASSERT_NO_MSG(string_out != NULL);

	cJSON *item = cJSON_GetObjectItem(obj, key);

	if (!cJSON_IsString(item)) {
		return -EINVAL;
	}

	*string_out = item->valuestring;

	return 0;
}

char *const nrf_cloud_format_cell_pos_req_payload(struct lte_lc_cells_info const *const inf,
	size_t inf_cnt)
{
	if (!inf || !inf_cnt) {
		return NULL;
	}

	char *payload = NULL;
	cJSON *lte_obj;
	cJSON *ncell_obj;
	cJSON *lte_array;
	cJSON *nmr_array = NULL;
	cJSON *payload_obj = cJSON_CreateObject();

	lte_array = cJSON_AddArrayToObject(payload_obj, NRF_CLOUD_CELL_POS_JSON_KEY_LTE);
	if (!lte_array) {
		goto cleanup;
	}

	for (size_t i = 0; i < inf_cnt; ++i) {
		struct lte_lc_cells_info const *const lte = (inf + i);
		struct lte_lc_cell const *const cur = &lte->current_cell;

		lte_obj = cJSON_CreateObject();

		if (!lte_obj) {
			goto cleanup;
		}

		if (!cJSON_AddItemToArray(lte_array, lte_obj)) {
			cJSON_Delete(lte_obj);
			goto cleanup;
		}

		/* required items */
		if (json_add_num(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_ECI, cur->id) ||
		    json_add_num(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_MCC, cur->mcc) ||
		    json_add_num(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_MNC, cur->mnc)) {
			goto cleanup;
		}

		/* optional */
		if (json_add_num(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_TAC, cur->tac) ||
		    json_add_num(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_EARFCN, cur->earfcn) ||
		    json_add_num(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_RSRP, RSRP_ADJ(cur->rsrp)) ||
		    json_add_num(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_RSRQ, cur->rsrq)) {
			goto cleanup;
		}

		if ((cur->timing_advance != 65535) &&
		    (json_add_num(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_T_ADV,
		     cur->timing_advance))) {
			goto cleanup;
		}

		/* Add an array for neighbor cell data if there are any */
		if (lte->ncells_count) {
			nmr_array = cJSON_AddArrayToObject(lte_obj,
							   NRF_CLOUD_CELL_POS_JSON_KEY_NBORS);
			if (!nmr_array) {
				goto cleanup;
			}
		}

		for (uint8_t j = 0; nmr_array && (j < lte->ncells_count); ++j) {
			struct lte_lc_ncell *ncell = lte->neighbor_cells + j;

			ncell_obj = cJSON_CreateObject();

			if (!ncell_obj) {
				goto cleanup;
			}

			if (!cJSON_AddItemToArray(nmr_array, ncell_obj)) {
				cJSON_Delete(ncell_obj);
				goto cleanup;
			}

			/* required items */
			if (json_add_num(ncell_obj, NRF_CLOUD_CELL_POS_JSON_KEY_EARFCN,
					 ncell->earfcn) ||
			    json_add_num(ncell_obj, NRF_CLOUD_CELL_POS_JSON_KEY_PCI,
					 ncell->phys_cell_id)) {
				goto cleanup;
			}

			/* optional */
			if (json_add_num(ncell_obj, NRF_CLOUD_CELL_POS_JSON_KEY_RSRP,
					 RSRP_ADJ(ncell->rsrp)) ||
			    json_add_num(ncell_obj, NRF_CLOUD_CELL_POS_JSON_KEY_RSRQ,
					 ncell->rsrq)) {
				goto cleanup;
			}
		}

	}
	payload = cJSON_PrintUnformatted(payload_obj);
	if (!payload) {
		goto cleanup;
	}

	return payload;

cleanup:
	if (payload_obj) {
		cJSON_Delete(payload_obj);
	}

	LOG_ERR("Failed to format location request, out of memory");

	return NULL;
}
