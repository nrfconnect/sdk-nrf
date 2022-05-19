/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf_cloud_codec.h"
#include "nrf_cloud_mem.h"
#include "nrf_cloud_fsm.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/modem_info.h>
#include "cJSON_os.h"

LOG_MODULE_REGISTER(nrf_cloud_codec, CONFIG_NRF_CLOUD_LOG_LEVEL);

#define DUA_PIN_STR "not_associated"
#define TIMEOUT_STR "timeout"
#define PAIRED_STR "paired"

/* Modem returns RSRP and RSRQ as index values which require
 * a conversion to dBm and dB respectively. See modem AT
 * command reference guide for more information.
 */
#define RSRP_ADJ(rsrp) (rsrp - ((rsrp <= 0) ? 140 : 141))
#define RSRQ_ADJ(rsrq) (((double)rsrq * 0.5) - 19.5)

bool initialized;

#if defined(CONFIG_NRF_CLOUD_MQTT)
#ifdef CONFIG_NRF_CLOUD_GATEWAY
static gateway_state_handler_t gateway_state_handler;
#endif
#endif

static const char *const sensor_type_str[] = {
	[NRF_CLOUD_SENSOR_GPS] = NRF_CLOUD_JSON_APPID_VAL_GPS,
	[NRF_CLOUD_SENSOR_FLIP] = NRF_CLOUD_JSON_APPID_VAL_FLIP,
	[NRF_CLOUD_SENSOR_BUTTON] = NRF_CLOUD_JSON_APPID_VAL_BTN,
	[NRF_CLOUD_SENSOR_TEMP] = NRF_CLOUD_JSON_APPID_VAL_TEMP,
	[NRF_CLOUD_SENSOR_HUMID] = NRF_CLOUD_JSON_APPID_VAL_HUMID,
	[NRF_CLOUD_SENSOR_AIR_PRESS] = NRF_CLOUD_JSON_APPID_VAL_AIR_PRESS,
	[NRF_CLOUD_SENSOR_AIR_QUAL] = NRF_CLOUD_JSON_APPID_VAL_AIR_QUAL,
	[NRF_CLOUD_LTE_LINK_RSRP] = NRF_CLOUD_JSON_APPID_VAL_RSRP,
	[NRF_CLOUD_DEVICE_INFO] = NRF_CLOUD_JSON_APPID_VAL_DEVICE,
	[NRF_CLOUD_SENSOR_LIGHT] = NRF_CLOUD_JSON_APPID_VAL_LIGHT,
};
#define SENSOR_TYPE_ARRAY_SIZE (sizeof(sensor_type_str) / sizeof(*sensor_type_str))

/* Shadow keys */
#define JSON_KEY_STATE		"state"
#define JSON_KEY_REP		"reported"
#define JSON_KEY_DES		"desired"
#define JSON_KEY_DELTA		"delta"

#define JSON_KEY_DEVICE		"device"
#define JSON_KEY_SRVC_INFO	"serviceInfo"
#define JSON_KEY_SRVC_INFO_UI	"ui"
#define JSON_KEY_SRVC_INFO_FOTA	NRF_CLOUD_FOTA_VER_STR
#define JSON_KEY_CFG		"config"
#define JSON_KEY_TOPICS		"topics"
#define JSON_KEY_STAGE		"stage"
#define JSON_KEY_PAIRING	"pairing"
#define JSON_KEY_PAIR_STAT	"pairingStatus"
#define JSON_KEY_TOPIC_PRFX	"nrfcloud_mqtt_topic_prefix"
#define JSON_KEY_KEEPALIVE	"keepalive"
#define JSON_KEY_CONN		"connection"

#ifndef CONFIG_NRF_CLOUD_GATEWAY
#define JSON_KEY_DEVICE_TO_CLOUD "d2c"
#define JSON_KEY_CLOUD_TO_DEVICE "c2d"
#else
#define JSON_KEY_DEVICE_TO_CLOUD "g2c"
#define JSON_KEY_CLOUD_TO_DEVICE "c2g"
#endif

int nrf_cloud_codec_init(void)
{
	if (!initialized) {
		cJSON_Init();
		initialized = true;
	}
	return 0;
}

static int json_add_num_cs(cJSON *parent, const char *str, double item)
{
	if (!parent || !str) {
		return -EINVAL;
	}

	return cJSON_AddNumberToObjectCS(parent, str, item) ? 0 : -ENOMEM;
}

cJSON *json_create_req_obj(const char *const app_id, const char *const msg_type)
{
	__ASSERT_NO_MSG(app_id != NULL);
	__ASSERT_NO_MSG(msg_type != NULL);

	nrf_cloud_codec_init();

	cJSON *req_obj = cJSON_CreateObject();

	if (!cJSON_AddStringToObject(req_obj, NRF_CLOUD_JSON_APPID_KEY, app_id) ||
	    !cJSON_AddStringToObject(req_obj, NRF_CLOUD_JSON_MSG_TYPE_KEY, msg_type)) {
		cJSON_Delete(req_obj);
		req_obj = NULL;
	}

	return req_obj;
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

	if (json_add_num_cs(data_obj, NRF_CLOUD_JSON_MCC_KEY,
		modem_info->network.mcc.value) ||
	    json_add_num_cs(data_obj, NRF_CLOUD_JSON_MNC_KEY,
		modem_info->network.mnc.value) ||
	    json_add_num_cs(data_obj, NRF_CLOUD_JSON_AREA_CODE_KEY,
		modem_info->network.area_code.value) ||
	    json_add_num_cs(data_obj, NRF_CLOUD_JSON_CELL_ID_KEY,
		(uint32_t)modem_info->network.cellid_dec) ||
	    json_add_num_cs(data_obj, NRF_CLOUD_CELL_POS_JSON_KEY_RSRP,
		RSRP_ADJ(modem_info->network.rsrp.value))) {
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

static int json_add_obj_cs(cJSON *parent, const char *str, cJSON *item)
{
	if (!parent || !str || !item) {
		return -EINVAL;
	}
	return cJSON_AddItemToObjectCS(parent, str, item) ? 0 : -ENOMEM;
}

static int json_add_null_cs(cJSON *parent, const char *const str)
{
	if (!parent || !str) {
		return -EINVAL;
	}

	return cJSON_AddNullToObjectCS(parent, str) ? 0 : -ENOMEM;
}

static int get_error_code_value(cJSON *const obj, enum nrf_cloud_error * const err)
{
	cJSON *err_obj;

	err_obj = cJSON_GetObjectItem(obj, NRF_CLOUD_JSON_ERR_KEY);
	if (!err_obj) {
		return -ENOMSG;
	}

	if (!cJSON_IsNumber(err_obj)) {
		LOG_WRN("Invalid JSON data type for error value");
		return -EBADMSG;
	}

	*err = (enum nrf_cloud_error)cJSON_GetNumberValue(err_obj);

	return 0;
}

#if defined(CONFIG_NRF_CLOUD_MQTT)
static int json_add_str_cs(cJSON *parent, const char *str, const char *item)
{
	if (!parent || !str || !item) {
		return -EINVAL;
	}

	return cJSON_AddStringToObjectCS(parent, str, item) ? 0 : -ENOMEM;
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
		state_obj = json_object_decode(root_obj, JSON_KEY_STATE);
		if (state_obj == NULL) {
			*desired_obj = json_object_decode(root_obj, JSON_KEY_DES);
		} else {
			*desired_obj = state_obj;
		}
	}
}

int nrf_cloud_encode_shadow_data(const struct nrf_cloud_sensor_data *sensor,
				 struct nrf_cloud_data *output)
{
	int ret;
	char *buffer = NULL;

	__ASSERT_NO_MSG(sensor != NULL);
	__ASSERT_NO_MSG(sensor->data.ptr != NULL);
	__ASSERT_NO_MSG(sensor->data.len != 0);
	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(sensor->type < SENSOR_TYPE_ARRAY_SIZE);

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_AddObjectToObjectCS(root_obj, JSON_KEY_STATE);
	cJSON *reported_obj = cJSON_AddObjectToObjectCS(state_obj, JSON_KEY_REP);
	cJSON *input_obj = cJSON_ParseWithLength(sensor->data.ptr, sensor->data.len);

	ret = json_add_obj_cs(reported_obj, sensor_type_str[sensor->type], input_obj);

	if (ret == 0) {
		buffer = cJSON_PrintUnformatted(root_obj);
	} else {
		cJSON_Delete(input_obj);
	}

	if (buffer) {
		output->ptr = buffer;
		output->len = strlen(buffer);
	} else {
		ret = -ENOMEM;
	}

	cJSON_Delete(root_obj);
	return ret;
}

int nrf_cloud_encode_sensor_data(const struct nrf_cloud_sensor_data *sensor,
				 struct nrf_cloud_data *output)
{
	int ret;

	__ASSERT_NO_MSG(sensor != NULL);
	__ASSERT_NO_MSG(sensor->data.ptr != NULL);
	__ASSERT_NO_MSG(sensor->data.len != 0);
	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(sensor->type < SENSOR_TYPE_ARRAY_SIZE);

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		return -ENOMEM;
	}

	ret = json_add_str_cs(root_obj, NRF_CLOUD_JSON_APPID_KEY, sensor_type_str[sensor->type]);
	ret += json_add_str_cs(root_obj, NRF_CLOUD_JSON_DATA_KEY, sensor->data.ptr);
	ret += json_add_str_cs(root_obj, NRF_CLOUD_JSON_MSG_TYPE_KEY,
			       NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA);

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

#ifdef CONFIG_NRF_CLOUD_GATEWAY
void nrf_cloud_register_gateway_state_handler(gateway_state_handler_t handler)
{
	gateway_state_handler = handler;
}
#endif

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

#ifdef CONFIG_NRF_CLOUD_GATEWAY
	int ret;

	if (gateway_state_handler) {
		ret = gateway_state_handler(root_obj);
		if (ret != 0) {
			LOG_ERR("Error from gateway_state_handler: %d", ret);
			cJSON_Delete(root_obj);
			return ret;
		}
	} else {
		LOG_ERR("No gateway state handler registered");
		cJSON_Delete(root_obj);
		return -EINVAL;
	}
#endif /* CONFIG_NRF_CLOUD_GATEWAY */

	nrf_cloud_decode_desired_obj(root_obj, &desired_obj);

	topic_prefix_obj =
		json_object_decode(desired_obj, JSON_KEY_TOPIC_PRFX);
	if (topic_prefix_obj != NULL) {
		nct_set_topic_prefix(topic_prefix_obj->valuestring);
		(*requested_state) = STATE_UA_PIN_COMPLETE;
		cJSON_Delete(root_obj);
		return 0;
	}

	pairing_obj = json_object_decode(desired_obj, JSON_KEY_PAIRING);
	pairing_state_obj = json_object_decode(pairing_obj, JSON_KEY_STATE);

	if (!pairing_state_obj || pairing_state_obj->type != cJSON_String) {
#ifndef CONFIG_NRF_CLOUD_GATEWAY
		if (cJSON_HasObjectItem(desired_obj, JSON_KEY_CFG) == false) {
			LOG_WRN("Unhandled data received from nRF Cloud.");
			LOG_INF("Ensure device firmware is up to date.");
			LOG_INF("Delete and re-add device to nRF Cloud if problem persists.");
		}
#endif
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
	cJSON *reported_obj = NULL;
	cJSON *state_obj = NULL;
	cJSON *config_obj = NULL;
	cJSON *input_obj = input ? cJSON_Parse(input->ptr) : NULL;

	if (input_obj == NULL) {
		return -ESRCH; /* invalid input or no JSON parsed */
	}

	/* A delta update will have the config inside of state */
	state_obj = cJSON_DetachItemFromObject(input_obj, JSON_KEY_STATE);
	config_obj = cJSON_DetachItemFromObject(
		state_obj ? state_obj : input_obj, JSON_KEY_CFG);
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
	desired_obj = cJSON_AddObjectToObjectCS(root_obj, JSON_KEY_DES);
	reported_obj = cJSON_AddObjectToObjectCS(root_obj, JSON_KEY_REP);

	/* Add a null config to desired and add the delta config to reported */
	if (json_add_null_cs(desired_obj, JSON_KEY_CFG) ||
	    json_add_obj_cs(reported_obj, JSON_KEY_CFG, config_obj)) {
		cJSON_Delete(root_obj);
		cJSON_Delete(config_obj);
		cJSON_Delete(state_obj);
		return -ENOMEM;
	}

	/* Cleanup received state obj and re-use for the response */
	cJSON_Delete(state_obj);
	state_obj = cJSON_CreateObject();
	if (state_obj) {
		(void)json_add_obj_cs(state_obj, JSON_KEY_STATE, root_obj);
		buffer = cJSON_PrintUnformatted(state_obj);
		cJSON_Delete(state_obj);
	} else {
		cJSON_Delete(root_obj);
	}

	if (buffer == NULL) {
		return -ENOMEM;
	}

	output->ptr = buffer;
	output->len = strlen(buffer);

	return 0;
}

int nrf_cloud_encode_state(uint32_t reported_state, struct nrf_cloud_data *output)
{
	__ASSERT_NO_MSG(output != NULL);

	char *buffer;
	int ret = 0;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_AddObjectToObjectCS(root_obj, JSON_KEY_STATE);
	cJSON *reported_obj = cJSON_AddObjectToObjectCS(state_obj, JSON_KEY_REP);
	cJSON *pairing_obj = cJSON_AddObjectToObjectCS(reported_obj, JSON_KEY_PAIRING);
	cJSON *connection_obj = cJSON_AddObjectToObjectCS(reported_obj, JSON_KEY_CONN);

	if (!pairing_obj || !connection_obj) {
		cJSON_Delete(root_obj);
		return -ENOMEM;
	}

	switch (reported_state) {
	case STATE_UA_PIN_WAIT: {
		ret += json_add_str_cs(pairing_obj, JSON_KEY_STATE, DUA_PIN_STR);
		ret += json_add_null_cs(pairing_obj, JSON_KEY_TOPICS);
		ret += json_add_null_cs(pairing_obj, JSON_KEY_CFG);
		ret += json_add_null_cs(reported_obj, JSON_KEY_STAGE);
		ret += json_add_null_cs(reported_obj, JSON_KEY_TOPIC_PRFX);
		ret += json_add_null_cs(connection_obj, JSON_KEY_KEEPALIVE);
		break;
	}
	case STATE_UA_PIN_COMPLETE: {
		struct nrf_cloud_data rx_endp;
		struct nrf_cloud_data tx_endp;
		struct nrf_cloud_data m_endp;

		/* Get the endpoint information. */
		nct_dc_endpoint_get(&tx_endp, &rx_endp, NULL, &m_endp);
		ret += json_add_str_cs(reported_obj, JSON_KEY_TOPIC_PRFX, m_endp.ptr);

		/* Clear pairing config and pairingStatus fields. */
		ret += json_add_str_cs(pairing_obj, JSON_KEY_STATE, PAIRED_STR);
		ret += json_add_null_cs(pairing_obj, JSON_KEY_CFG);
		ret += json_add_null_cs(reported_obj, JSON_KEY_PAIR_STAT);

		/* Report keepalive value. */
		ret += json_add_num_cs(connection_obj, JSON_KEY_KEEPALIVE,
				       CONFIG_NRF_CLOUD_MQTT_KEEPALIVE);

		/* Report pairing topics. */
		cJSON *topics_obj = cJSON_AddObjectToObjectCS(pairing_obj, JSON_KEY_TOPICS);

		ret += json_add_str_cs(topics_obj, JSON_KEY_DEVICE_TO_CLOUD, tx_endp.ptr);
		ret += json_add_str_cs(topics_obj, JSON_KEY_CLOUD_TO_DEVICE, rx_endp.ptr);

		if (ret != 0) {
			cJSON_Delete(root_obj);
			return -ENOMEM;
		}
		break;
	}
	default: {
		cJSON_Delete(root_obj);
		return -ENOTSUP;
	}
	}

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
				   struct nrf_cloud_data *bulk_endpoint,
				   struct nrf_cloud_data *m_endpoint)
{
	__ASSERT_NO_MSG(input != NULL);
	__ASSERT_NO_MSG(input->ptr != NULL);
	__ASSERT_NO_MSG(input->len != 0);
	__ASSERT_NO_MSG(tx_endpoint != NULL);
	__ASSERT_NO_MSG(rx_endpoint != NULL);
	__ASSERT_NO_MSG(bulk_endpoint != NULL);

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

	cJSON *pairing_obj = json_object_decode(desired_obj, JSON_KEY_PAIRING);
	cJSON *pairing_state_obj = json_object_decode(pairing_obj, JSON_KEY_STATE);
	cJSON *topic_obj = json_object_decode(pairing_obj, JSON_KEY_TOPICS);

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

	cJSON *tx_obj = json_object_decode(topic_obj, JSON_KEY_DEVICE_TO_CLOUD);

	err = json_decode_and_alloc(tx_obj, tx_endpoint);
	if (err) {
		cJSON_Delete(root_obj);
		LOG_ERR("could not decode topic for %s", JSON_KEY_DEVICE_TO_CLOUD);
		return err;
	}

	/* Populate bulk endpoint topic by copying and appending /bulk to the parsed
	 * tx endpoint (d2c) topic.
	 */
	size_t bulk_ep_len_temp = tx_endpoint->len + sizeof(NRF_CLOUD_BULK_MSG_TOPIC);

	bulk_endpoint->ptr = nrf_cloud_calloc(bulk_ep_len_temp, 1);
	if (bulk_endpoint->ptr == NULL) {
		cJSON_Delete(root_obj);
		LOG_ERR("Could not allocate memory for bulk topic");
		return -ENOMEM;
	}

	bulk_endpoint->len = snprintk((char *)bulk_endpoint->ptr, bulk_ep_len_temp, "%s%s",
				       (char *)tx_endpoint->ptr,
				       NRF_CLOUD_BULK_MSG_TOPIC);

	cJSON *rx_obj = json_object_decode(topic_obj, JSON_KEY_CLOUD_TO_DEVICE);

	err = json_decode_and_alloc(rx_obj, rx_endpoint);
	if (err) {
		cJSON_Delete(root_obj);
		LOG_ERR("could not decode topic for %s", JSON_KEY_CLOUD_TO_DEVICE);
		return err;
	}

	cJSON_Delete(root_obj);

	return err;
}

int json_send_to_cloud(cJSON *const request)
{
	__ASSERT_NO_MSG(request != NULL);

	if (nfsm_get_current_state() != STATE_DC_CONNECTED) {
		return -EACCES;
	}

	char *msg_string;
	int err;

	msg_string = cJSON_PrintUnformatted(request);
	if (!msg_string) {
		LOG_ERR("Could not allocate memory for request message");
		return -ENOMEM;
	}

	LOG_DBG("Created request: %s", log_strdup(msg_string));

	struct nct_dc_data msg = {
		.data.ptr = msg_string,
		.data.len = strlen(msg_string)
	};

	err = nct_dc_send(&msg);
	if (err) {
		LOG_ERR("Failed to send request, error: %d", err);
	} else {
		LOG_DBG("Request sent to cloud");
	}

	k_free(msg_string);

	return err;
}
#endif /* CONFIG_NRF_CLOUD_MQTT */

static int encode_info_item_cs(const enum nrf_cloud_shadow_info inf, const char *const inf_name,
			    cJSON *const inf_obj, cJSON *const root_obj)
{
	cJSON *move_obj;

	switch (inf) {
	case NRF_CLOUD_INFO_SET:
		move_obj = cJSON_DetachItemFromObject(inf_obj, inf_name);

		if (!move_obj) {
			LOG_ERR("Info item \"%s\" not found", log_strdup(inf_name));
			return -ENOMSG;
		}

		if (json_add_obj_cs(root_obj, inf_name, move_obj)) {
			cJSON_Delete(move_obj);
			LOG_ERR("Failed to add info item \"%s\"", log_strdup(inf_name));
			return -ENOMEM;
		}
		break;
	case NRF_CLOUD_INFO_CLEAR:
		if (json_add_null_cs(root_obj, inf_name)) {
			LOG_ERR("Failed to create NULL item for \"%s\"", log_strdup(inf_name));
			return -ENOMEM;
		}
		break;
	case NRF_CLOUD_INFO_NO_CHANGE:
	default:
		break;
	}

	return 0;
}

static int nrf_cloud_encode_service_info_fota(const struct nrf_cloud_svc_info_fota *const fota,
					      cJSON *const svc_inf_obj)
{
	if (!svc_inf_obj) {
		return -EINVAL;
	}

	if (fota == NULL ||
	    (IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) && !IS_ENABLED(CONFIG_NRF_CLOUD_FOTA))) {
		if (fota && (fota->application || fota->modem || fota->bootloader)) {
			LOG_WRN("CONFIG_NRF_CLOUD_FOTA not enabled, setting FOTA array to 'null'");
		}

		if (json_add_null_cs(svc_inf_obj, JSON_KEY_SRVC_INFO_FOTA) != 0) {
			return -ENOMEM;
		}
	} else if (fota) {
		int item_cnt = 0;
		cJSON *array = cJSON_AddArrayToObjectCS(svc_inf_obj, JSON_KEY_SRVC_INFO_FOTA);

		if (!array) {
			return -ENOMEM;
		}
		if (fota->bootloader) {
			cJSON_AddItemToArray(array, cJSON_CreateString(NRF_CLOUD_FOTA_TYPE_BOOT));
			++item_cnt;
		}
		if (fota->modem) {
			cJSON_AddItemToArray(array,
					     cJSON_CreateString(NRF_CLOUD_FOTA_TYPE_MODEM_DELTA));
			++item_cnt;
		}
		if (fota->application) {
			cJSON_AddItemToArray(array, cJSON_CreateString(NRF_CLOUD_FOTA_TYPE_APP));
			++item_cnt;
		}
		if (fota->modem_full) {
			cJSON_AddItemToArray(array,
					     cJSON_CreateString(NRF_CLOUD_FOTA_TYPE_MODEM_FULL));
			++item_cnt;
		}

		if (cJSON_GetArraySize(array) != item_cnt) {
			cJSON_DeleteItemFromObject(svc_inf_obj, JSON_KEY_SRVC_INFO_FOTA);
			return -ENOMEM;
		}
	}

	return 0;
}

static int nrf_cloud_encode_service_info_ui(const struct nrf_cloud_svc_info_ui *const ui,
					    cJSON *const svc_inf_obj)
{
	if (!svc_inf_obj) {
		return -EINVAL;
	}

	if (ui == NULL) {
		if (json_add_null_cs(svc_inf_obj, JSON_KEY_SRVC_INFO_UI) != 0) {
			return -ENOMEM;
		}
	} else {
		int item_cnt = 0;
		cJSON *array = cJSON_AddArrayToObjectCS(svc_inf_obj, JSON_KEY_SRVC_INFO_UI);

		if (!array) {
			return -ENOMEM;
		}
		if (ui->air_pressure) {
			cJSON_AddItemToArray(array,
				cJSON_CreateString(sensor_type_str[NRF_CLOUD_SENSOR_AIR_PRESS]));
			++item_cnt;
		}
		if (ui->gps) {
			cJSON_AddItemToArray(array,
				cJSON_CreateString(sensor_type_str[NRF_CLOUD_SENSOR_GPS]));
			++item_cnt;
		}
		if (ui->flip) {
			cJSON_AddItemToArray(array,
				cJSON_CreateString(sensor_type_str[NRF_CLOUD_SENSOR_FLIP]));
			++item_cnt;
		}
		if (ui->button) {
			cJSON_AddItemToArray(array,
				cJSON_CreateString(sensor_type_str[NRF_CLOUD_SENSOR_BUTTON]));
			++item_cnt;
		}
		if (ui->temperature) {
			cJSON_AddItemToArray(array,
				cJSON_CreateString(sensor_type_str[NRF_CLOUD_SENSOR_TEMP]));
			++item_cnt;
		}
		if (ui->humidity) {
			cJSON_AddItemToArray(array,
				cJSON_CreateString(sensor_type_str[NRF_CLOUD_SENSOR_HUMID]));
			++item_cnt;
		}
		if (ui->light_sensor) {
			cJSON_AddItemToArray(array,
				cJSON_CreateString(sensor_type_str[NRF_CLOUD_SENSOR_LIGHT]));
			++item_cnt;
		}
		if (ui->rsrp) {
			cJSON_AddItemToArray(array,
				cJSON_CreateString(sensor_type_str[NRF_CLOUD_LTE_LINK_RSRP]));
			++item_cnt;
		}

		if (cJSON_GetArraySize(array) != item_cnt) {
			cJSON_DeleteItemFromObject(svc_inf_obj, JSON_KEY_SRVC_INFO_UI);
			return -ENOMEM;
		}
	}

	return 0;
}

int nrf_cloud_modem_info_json_encode(const struct nrf_cloud_modem_info *const mod_inf,
				     cJSON *const mod_inf_obj)
{
	if (!mod_inf_obj || !mod_inf) {
		return -EINVAL;
	}

	if (!IS_ENABLED(CONFIG_MODEM_INFO) &&
	    (mod_inf->device == NRF_CLOUD_INFO_SET ||
	     mod_inf->sim == NRF_CLOUD_INFO_SET ||
	     mod_inf->network == NRF_CLOUD_INFO_SET)) {
		LOG_ERR("CONFIG_MODEM_INFO is not enabled, unable to set device info");
		return -EACCES;
	} else if ((!IS_ENABLED(CONFIG_MODEM_INFO_ADD_DEVICE)) &&
		   (mod_inf->device == NRF_CLOUD_INFO_SET)) {
		LOG_ERR("CONFIG_MODEM_INFO_ADD_DEVICE is not enabled, unable to add device info");
		return -EACCES;
	} else if ((!IS_ENABLED(CONFIG_MODEM_INFO_ADD_NETWORK)) &&
		   (mod_inf->network == NRF_CLOUD_INFO_SET)) {
		LOG_ERR("CONFIG_MODEM_INFO_ADD_NETWORK is not enabled, unable to add network info");
		return -EACCES;
	} else if ((!IS_ENABLED(CONFIG_MODEM_INFO_ADD_SIM)) &&
		   (mod_inf->sim == NRF_CLOUD_INFO_SET)) {
		LOG_ERR("CONFIG_MODEM_INFO_ADD_SIM is not enabled, unable to add SIM info");
		return -EACCES;
	}

	int err = 0;
	cJSON *tmp = cJSON_CreateObject();

	if (!tmp) {
		err = -ENOMEM;
		goto cleanup;
	}

#ifdef CONFIG_MODEM_INFO
	struct modem_param_info *mpi = (struct modem_param_info *)mod_inf->mpi;
	struct modem_param_info fetched_mod_inf;

	if (!mpi) {
		err = modem_info_init();
		if (err) {
			LOG_ERR("modem_info_init() failed: %d", err);
			goto cleanup;
		}

		err = modem_info_params_init(&fetched_mod_inf);
		if (err) {
			LOG_ERR("modem_info_params_init() failed: %d", err);
			goto cleanup;
		}

		err = modem_info_params_get(&fetched_mod_inf);
		if (err < 0) {
			LOG_ERR("modem_info_params_get() failed: %d", err);
			goto cleanup;
		}
		mpi = &fetched_mod_inf;
	}

	err = modem_info_json_object_encode(mpi, tmp);
	if (err < 0) {
		LOG_ERR("Failed to encode modem info: %d", err);
		goto cleanup;
	}
	err = 0;
#endif

	if (encode_info_item_cs(mod_inf->device, MODEM_INFO_JSON_KEY_DEV_INF, tmp, mod_inf_obj) ||
	    encode_info_item_cs(mod_inf->network, MODEM_INFO_JSON_KEY_NET_INF, tmp, mod_inf_obj) ||
	    encode_info_item_cs(mod_inf->sim, MODEM_INFO_JSON_KEY_SIM_INF, tmp, mod_inf_obj)) {
		LOG_ERR("Failed to encode modem info");
		err = -EIO;
		goto cleanup;
	}

cleanup:
	cJSON_Delete(tmp);
	return err;
}

int nrf_cloud_service_info_json_encode(const struct nrf_cloud_svc_info *const svc_inf,
	cJSON *const svc_inf_obj)
{
	if (!svc_inf || !svc_inf_obj) {
		return -EINVAL;
	}

	int err = nrf_cloud_encode_service_info_fota(svc_inf->fota, svc_inf_obj);

	if (err) {
		return err;
	}

	return nrf_cloud_encode_service_info_ui(svc_inf->ui, svc_inf_obj);
}

void nrf_cloud_device_status_free(struct nrf_cloud_data *status)
{
	if (status && status->ptr) {
		cJSON_free((void *)status->ptr);
		status->ptr = NULL;
		status->len = 0;
	}
}

int nrf_cloud_device_status_encode(const struct nrf_cloud_device_status *const dev_status,
	struct nrf_cloud_data * const output, const bool include_state)
{
	if (!dev_status || !output) {
		return -EINVAL;
	}

	int err = 0;
	cJSON *state_obj = NULL;
	cJSON *reported_obj = NULL;
	cJSON *root_obj = cJSON_CreateObject();

	if (include_state) {
		state_obj = cJSON_AddObjectToObjectCS(root_obj, JSON_KEY_STATE);
		reported_obj = cJSON_AddObjectToObjectCS(state_obj, JSON_KEY_REP);
	} else {
		reported_obj = cJSON_AddObjectToObjectCS(root_obj, JSON_KEY_REP);
	}

	cJSON *device_obj = cJSON_AddObjectToObjectCS(reported_obj, JSON_KEY_DEVICE);
	cJSON *svc_inf_obj = cJSON_AddObjectToObjectCS(device_obj, JSON_KEY_SRVC_INFO);

	if (svc_inf_obj == NULL) {
		err = -ENOMEM;
		goto cleanup;
	}

	if (dev_status->modem) {
		err = nrf_cloud_modem_info_json_encode(dev_status->modem, device_obj);
		if (err) {
			goto cleanup;
		}
	}

	if (dev_status->svc) {
		err = nrf_cloud_service_info_json_encode(dev_status->svc, svc_inf_obj);
		if (err) {
			goto cleanup;
		}
	}

	output->ptr = cJSON_PrintUnformatted(root_obj);
	if (output->ptr) {
		output->len = strlen(output->ptr);
	} else {
		err = -ENOMEM;
	}

cleanup:
	cJSON_Delete(root_obj);

	if (err) {
		output->ptr = NULL;
		output->len = 0;
	}

	return err;
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

	if (!strcmp(type, NRF_CLOUD_FOTA_TYPE_MODEM_DELTA)) {
		job->type = NRF_CLOUD_FOTA_MODEM_DELTA;
	} else if (!strcmp(type, NRF_CLOUD_FOTA_TYPE_MODEM_FULL)) {
		job->type = NRF_CLOUD_FOTA_MODEM_FULL;
	} else if (!strcmp(type, NRF_CLOUD_FOTA_TYPE_BOOT)) {
		job->type = NRF_CLOUD_FOTA_BOOTLOADER;
	} else if (!strcmp(type, NRF_CLOUD_FOTA_TYPE_APP)) {
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

#if defined(CONFIG_NRF_CLOUD_PGPS)
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
		if (get_string_from_array(rsp_obj, NRF_CLOUD_PGPS_RCV_ARRAY_IDX_HOST, &host_ptr) ||
		    get_string_from_array(rsp_obj, NRF_CLOUD_PGPS_RCV_ARRAY_IDX_PATH, &path_ptr)) {
			LOG_ERR("Invalid P-GPS array response format");
			err = -EFTYPE;
			goto cleanup;
		}
	} else if (get_string_from_obj(rsp_obj, NRF_CLOUD_PGPS_RCV_REST_HOST, &host_ptr) ||
		   get_string_from_obj(rsp_obj, NRF_CLOUD_PGPS_RCV_REST_PATH, &path_ptr)) {
		enum nrf_cloud_error nrf_err;

		/* Check for a potential P-GPS JSON error message from nRF Cloud */
		err = nrf_cloud_handle_error_message(response, NRF_CLOUD_JSON_APPID_VAL_PGPS,
						     NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA, &nrf_err);
		if (!err) {
			LOG_ERR("nRF Cloud returned P-GPS error: %d", nrf_err);
			err = -EFAULT;
		} else {
			LOG_ERR("Invalid P-GPS response format");
			err = -EFTYPE;
		}

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
#endif /* CONFIG_NRF_CLOUD_PGPS */

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

int nrf_cloud_format_single_cell_pos_req_json(cJSON *const req_obj_out)
{
	int err = 0;
	cJSON *lte_array = cJSON_AddArrayToObjectCS(req_obj_out, NRF_CLOUD_CELL_POS_JSON_KEY_LTE);
	cJSON *lte_obj = cJSON_CreateObject();

	if (!cJSON_AddItemToArray(lte_array, lte_obj)) {
		cJSON_Delete(lte_obj);
		err = -ENOMEM;
	} else {
		err = nrf_cloud_json_add_modem_info(lte_obj);
	}

	if (err) {
		cJSON_DeleteItemFromObject(req_obj_out, NRF_CLOUD_CELL_POS_JSON_KEY_LTE);
	}

	return err;
}

int nrf_cloud_format_cell_pos_req_json(struct lte_lc_cells_info const *const inf,
	size_t inf_cnt, cJSON *const req_obj_out)
{
	if (!inf || !inf_cnt || !req_obj_out) {
		return -EINVAL;
	}

	cJSON *lte_obj = NULL;
	cJSON *ncell_obj = NULL;
	cJSON *lte_array = NULL;
	cJSON *nmr_array = NULL;

	lte_array = cJSON_AddArrayToObjectCS(req_obj_out, NRF_CLOUD_CELL_POS_JSON_KEY_LTE);
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
		if (json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_ECI, cur->id) ||
		    json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_MCC, cur->mcc) ||
		    json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_MNC, cur->mnc) ||
		    json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_TAC, cur->tac)) {
			goto cleanup;
		}

		/* optional */
		if ((cur->earfcn != NRF_CLOUD_CELL_POS_OMIT_EARFCN) &&
		    json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_EARFCN, cur->earfcn)) {
			goto cleanup;
		}

		if ((cur->rsrp != NRF_CLOUD_CELL_POS_OMIT_RSRP) &&
		    json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_RSRP,
				    RSRP_ADJ(cur->rsrp))) {
			goto cleanup;
		}

		if ((cur->rsrq != NRF_CLOUD_CELL_POS_OMIT_RSRQ) &&
		    json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_RSRQ,
				    RSRQ_ADJ(cur->rsrq))) {
			goto cleanup;
		}

		if (cur->timing_advance != NRF_CLOUD_CELL_POS_OMIT_TIME_ADV) {
			uint16_t t_adv = cur->timing_advance;

			if (t_adv > NRF_CLOUD_CELL_POS_TIME_ADV_MAX) {
				t_adv = NRF_CLOUD_CELL_POS_TIME_ADV_MAX;
			}

			if (json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_T_ADV, t_adv)) {
				goto cleanup;
			}
		}

		/* Add an array for neighbor cell data if there are any */
		if (lte->ncells_count) {
			if (lte->neighbor_cells == NULL) {
				LOG_WRN("Neighbor cell count is %u, but buffer is NULL",
					lte->ncells_count);
				return 0;
			}

			nmr_array = cJSON_AddArrayToObjectCS(lte_obj,
							     NRF_CLOUD_CELL_POS_JSON_KEY_NBORS);
			if (!nmr_array) {
				goto cleanup;
			}
		}

		for (uint8_t j = 0; nmr_array && (j < lte->ncells_count); ++j) {
			struct lte_lc_ncell *ncell = lte->neighbor_cells + j;

			if (ncell == NULL) {
				break;
			}

			ncell_obj = cJSON_CreateObject();

			if (!ncell_obj) {
				goto cleanup;
			}

			if (!cJSON_AddItemToArray(nmr_array, ncell_obj)) {
				cJSON_Delete(ncell_obj);
				goto cleanup;
			}

			/* required items */
			if (json_add_num_cs(ncell_obj, NRF_CLOUD_CELL_POS_JSON_KEY_EARFCN,
					    ncell->earfcn) ||
			    json_add_num_cs(ncell_obj, NRF_CLOUD_CELL_POS_JSON_KEY_PCI,
					    ncell->phys_cell_id)) {
				goto cleanup;
			}

			/* optional */
			if ((ncell->rsrp != NRF_CLOUD_CELL_POS_OMIT_RSRP) &&
			    json_add_num_cs(ncell_obj, NRF_CLOUD_CELL_POS_JSON_KEY_RSRP,
					    RSRP_ADJ(ncell->rsrp))) {
				goto cleanup;
			}
			if ((ncell->rsrq != NRF_CLOUD_CELL_POS_OMIT_RSRQ) &&
			    json_add_num_cs(ncell_obj, NRF_CLOUD_CELL_POS_JSON_KEY_RSRQ,
					    RSRQ_ADJ(ncell->rsrq))) {
				goto cleanup;
			}
		}
	}

	return 0;

cleanup:
	/* Only need to delete the lte_array since all items (if any) were added to it */
	cJSON_DeleteItemFromObject(req_obj_out, NRF_CLOUD_CELL_POS_JSON_KEY_LTE);
	LOG_ERR("Failed to format location request, out of memory");
	return -ENOMEM;
}

int nrf_cloud_format_cell_pos_req(struct lte_lc_cells_info const *const inf,
	size_t inf_cnt, char **string_out)
{
	if (!inf || !inf_cnt || !string_out) {
		return -EINVAL;
	}

	int err = 0;
	cJSON *req_obj = cJSON_CreateObject();

	err = nrf_cloud_format_cell_pos_req_json(inf, inf_cnt, req_obj);
	if (err) {
		goto cleanup;
	}

	*string_out = cJSON_PrintUnformatted(req_obj);
	if (*string_out == NULL) {
		err = -ENOMEM;
	}

cleanup:
	cJSON_Delete(req_obj);

	return err;
}

static bool json_item_string_exists(const cJSON *const obj, const char *const key,
				    const char *const val)
{
	__ASSERT_NO_MSG(obj != NULL);
	__ASSERT_NO_MSG(key != NULL);

	char *str_val;
	cJSON *item = cJSON_GetObjectItem(obj, key);

	if (!item) {
		return false;
	}

	if (!val) {
		return cJSON_IsNull(item);
	}

	str_val = cJSON_GetStringValue(item);
	if (!str_val) {
		return false;
	}

	return (strcmp(str_val, val) == 0);
}

static int nrf_cloud_parse_cell_pos_json(const cJSON *const cell_pos_obj,
	struct nrf_cloud_cell_pos_result *const location_out)
{
	if (!cell_pos_obj || !location_out) {
		return -EINVAL;
	}

	cJSON *lat, *lon, *unc;
	char *type;

	lat = cJSON_GetObjectItem(cell_pos_obj,
				  NRF_CLOUD_CELL_POS_JSON_KEY_LAT);
	lon = cJSON_GetObjectItem(cell_pos_obj,
				NRF_CLOUD_CELL_POS_JSON_KEY_LON);
	unc = cJSON_GetObjectItem(cell_pos_obj,
				NRF_CLOUD_CELL_POS_JSON_KEY_UNCERT);

	if (!cJSON_IsNumber(lat) || !cJSON_IsNumber(lon) ||
	    !cJSON_IsNumber(unc)) {
		return -EBADMSG;
	}

	location_out->lat = lat->valuedouble;
	location_out->lon = lon->valuedouble;
	location_out->unc = (uint32_t)unc->valueint;

	location_out->type = CELL_POS_TYPE__INVALID;

	if (!get_string_from_obj(cell_pos_obj, NRF_CLOUD_JSON_FULFILL_KEY, &type)) {
		if (!strcmp(type, NRF_CLOUD_CELL_POS_TYPE_VAL_MCELL)) {
			location_out->type = CELL_POS_TYPE_MULTI;
		} else if (!strcmp(type, NRF_CLOUD_CELL_POS_TYPE_VAL_SCELL)) {
			location_out->type = CELL_POS_TYPE_SINGLE;
		} else {
			LOG_WRN("Unhandled cellular positioning type: %s", log_strdup(type));
		}
	} else {
		LOG_WRN("Cellular positioning type not found in message");
	}

	return 0;
}

int nrf_cloud_handle_error_message(const char *const buf,
				   const char *const app_id,
				   const char *const msg_type,
				   enum nrf_cloud_error * const err)
{
	if (!buf || !err) {
		return -EINVAL;
	}

	int ret;
	cJSON *root_obj;

	*err = NRF_CLOUD_ERROR_NONE;

	root_obj = cJSON_Parse(buf);
	if (!root_obj) {
		LOG_DBG("No JSON found");
		return -ENODATA;
	}

	ret = get_error_code_value(root_obj, err);
	if (ret) {
		goto clean_up;
	}

	/* If provided, check for matching app id and msg type */
	if (msg_type &&
	    !json_item_string_exists(root_obj, NRF_CLOUD_JSON_MSG_TYPE_KEY, msg_type)) {
		ret = -ENOENT;
		goto clean_up;
	}
	if (app_id &&
	    !json_item_string_exists(root_obj, NRF_CLOUD_JSON_APPID_KEY, app_id)) {
		ret = -ENOENT;
		goto clean_up;
	}

clean_up:
	cJSON_Delete(root_obj);
	return ret;
}

int nrf_cloud_parse_cell_pos_response(const char *const buf,
				      struct nrf_cloud_cell_pos_result *result)
{
	int ret = 1; /* 1: cell-based location not found */
	cJSON *cell_pos_obj;
	cJSON *data_obj;

	if ((buf == NULL) || (result == NULL)) {
		return -EINVAL;
	}

	cell_pos_obj = cJSON_Parse(buf);
	if (!cell_pos_obj) {
		LOG_DBG("No JSON found for cellular positioning");
		return 1;
	}

	/* First, check to see if this is a REST payload, which is not wrapped in
	 * an nRF Cloud MQTT message
	 */
	ret = nrf_cloud_parse_cell_pos_json(cell_pos_obj, result);
	if (ret == 0) {
		goto cleanup;
	}

	/* Clear the error flag and check for MQTT payload format */
	result->err = NRF_CLOUD_ERROR_NONE;
	ret = 1;

	/* Check for nRF Cloud MQTT message; valid appId and msgType */
	if (!json_item_string_exists(cell_pos_obj, NRF_CLOUD_JSON_MSG_TYPE_KEY,
				     NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA) ||
	    !json_item_string_exists(cell_pos_obj, NRF_CLOUD_JSON_APPID_KEY,
				     NRF_CLOUD_JSON_APPID_VAL_CELL_POS)) {
		/* Not a celluar positioning data message */
		goto cleanup;
	}

	/* MQTT payload format found, parse the data */
	data_obj = cJSON_GetObjectItem(cell_pos_obj, NRF_CLOUD_JSON_DATA_KEY);
	if (data_obj) {
		ret = nrf_cloud_parse_cell_pos_json(data_obj, result);
		if (ret) {
			LOG_ERR("Failed to parse cellular positioning data");
		}
		/* A message with "data" should not also contain an error code */
		goto cleanup;
	}

	/* Check for error code */
	ret = get_error_code_value(cell_pos_obj, &result->err);
	if (ret) {
		/* Indicate that an nRF Cloud error code was found */
		ret = -EFAULT;
	} else {
		/* No data or error was found */
		LOG_ERR("Expected data not found in cellular positioning message");
		ret = -EBADMSG;
	}

cleanup:
	cJSON_Delete(cell_pos_obj);

	if (ret < 0) {
		/* Clear data on error */
		result->lat = 0.0;
		result->lon = 0.0;
		result->unc = 0;
		result->type = CELL_POS_TYPE__INVALID;

		/* Set to unknown error if an error code was not found */
		if (result->err == NRF_CLOUD_ERROR_NONE) {
			result->err = NRF_CLOUD_ERROR_UNKNOWN;
		}
	}

	return ret;
}

int nrf_cloud_parse_rest_error(const char *const buf, enum nrf_cloud_error *const err)
{
	int ret = -ENOMSG;
	cJSON *root_obj;
	cJSON *err_obj;
	char *msg = NULL;

	if ((buf == NULL) || (err == NULL)) {
		return -EINVAL;
	}

	*err = NRF_CLOUD_ERROR_NONE;

	root_obj = cJSON_Parse(buf);
	if (!root_obj) {
		LOG_DBG("No JSON found in REST response");
		return ret;
	}

	/* Some responses are only an array of strings */
	if (cJSON_IsArray(root_obj) && (get_string_from_array(root_obj, 0, &msg) == 0)) {
		goto cleanup;
	}

	/* Check for a message string. Ignore return, just for debug printing */
	(void)get_string_from_obj(root_obj, NRF_CLOUD_REST_ERROR_MSG_KEY, &msg);

	/* Get the error code */
	err_obj = cJSON_GetObjectItem(root_obj, NRF_CLOUD_REST_ERROR_CODE_KEY);
	if (cJSON_IsNumber(err_obj)) {
		ret = 0;
		*err = (enum nrf_cloud_error)cJSON_GetNumberValue(err_obj);
	}

cleanup:
	if (msg) {
		LOG_DBG("REST error msg: %s", msg);
	}

	cJSON_Delete(root_obj);

	return ret;
}

bool nrf_cloud_detect_disconnection_request(const char *const buf)
{
	if (buf == NULL) {
		return false;
	}

	/* The candidate buffer must be a null-terminated string less than
	 * a certain length
	 */
	if (memchr(buf, '\0', NRF_CLOUD_JSON_MSG_MAX_LEN_DISCONNECT) == NULL) {
		return false;
	}

	/* Fast test to avoid parsing EVERY message with cJSON. */
	if (strstr(buf, NRF_CLOUD_JSON_APPID_VAL_DEVICE) == NULL ||
	    strstr(buf, NRF_CLOUD_JSON_MSG_TYPE_VAL_DISCONNECT) == NULL) {
		return false;
	}

	/* If the quick test passes, use cJSON to get certainty */
	bool ret = true;
	cJSON *discon_request_obj = cJSON_Parse(buf);

	/* Check for nRF Cloud disconnection request MQTT message */
	if (!json_item_string_exists(discon_request_obj, NRF_CLOUD_JSON_MSG_TYPE_KEY,
				     NRF_CLOUD_JSON_MSG_TYPE_VAL_DISCONNECT) ||
	    !json_item_string_exists(discon_request_obj, NRF_CLOUD_JSON_APPID_KEY,
				     NRF_CLOUD_JSON_APPID_VAL_DEVICE)) {
		/* Not a disconnection request message */
		ret = false;
	}

	cJSON_Delete(discon_request_obj);
	return ret;
}
