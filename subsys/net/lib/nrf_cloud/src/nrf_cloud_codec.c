/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf_cloud_codec.h"
#include "nrf_cloud_mem.h"

#include <stdbool.h>
#include <string.h>
#include <zephyr.h>
#include <logging/log.h>
#include <modem/modem_info.h>
#include "cJSON.h"
#include "cJSON_os.h"

LOG_MODULE_REGISTER(nrf_cloud_codec, CONFIG_NRF_CLOUD_LOG_LEVEL);

#define DUA_PIN_STR "not_associated"
#define TIMEOUT_STR "timeout"
#define PAIRED_STR "paired"

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
	[NRF_CLOUD_SENSOR_LIGHT] = "LIGHT",
};

#define MSGTYPE_VAL_DATA	"DATA"
#define FOTA_VAL_BOOT		"BOOT"
#define FOTA_VAL_APP		"APP"
#define FOTA_VAL_MODEM		"MODEM"

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

#define JSON_KEY_D2C		"d2c"
#define JSON_KEY_C2D		"c2d"

#define JSON_KEY_MSGTYPE	"messageType"
#define JSON_KEY_APPID		"appId"
#define JSON_KEY_DATA		"data"

/* --- A few wrappers for cJSON APIs --- */

static int json_add_obj_cs(cJSON *parent, const char *str, cJSON *item)
{
	if (!parent || !str || !item) {
		return -EINVAL;
	}
	return cJSON_AddItemToObjectCS(parent, str, item) ? 0 : -ENOMEM;
}

static int json_add_str_cs(cJSON *parent, const char *str, const char *item)
{
	if (!parent || !str || !item) {
		return -EINVAL;
	}

	return cJSON_AddStringToObjectCS(parent, str, item) ? 0 : -ENOMEM;
}

static int json_add_null_cs(cJSON *parent, const char *const str)
{
	if (!parent || !str) {
		return -EINVAL;
	}

	return cJSON_AddNullToObjectCS(parent, str) ? 0 : -ENOMEM;
}

static cJSON *json_object_decode(cJSON *obj, const char *str)
{
	return obj ? cJSON_GetObjectItem(obj, str) : NULL;
}

static int json_decode_and_alloc(cJSON *obj, struct nrf_cloud_data *data)
{
	if (obj == NULL || (obj->type != cJSON_String)) {
		data->ptr = NULL;
		return -ENOENT;
	}

	data->len = strlen(obj->valuestring);
	data->ptr = nrf_cloud_malloc(data->len + 1);

	if (data->ptr == NULL) {
		return -ENOMEM;
	}

	strncpy((char *)data->ptr, obj->valuestring, data->len + 1);

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

int nrf_codec_init(void)
{
	cJSON_Init();

	return 0;
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

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_AddObjectToObjectCS(root_obj, JSON_KEY_STATE);
	cJSON *reported_obj = cJSON_AddObjectToObjectCS(state_obj, JSON_KEY_REP);

	ret = json_add_obj_cs(reported_obj, sensor_type_str[sensor->type],
			   (cJSON *)sensor->data.ptr);

	if (ret == 0) {
		buffer = cJSON_PrintUnformatted(root_obj);
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

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		return -ENOMEM;
	}

	ret = json_add_str_cs(root_obj, JSON_KEY_APPID, sensor_type_str[sensor->type]);
	ret += json_add_str_cs(root_obj, JSON_KEY_DATA, sensor->data.ptr);
	ret += json_add_str_cs(root_obj, JSON_KEY_MSGTYPE, MSGTYPE_VAL_DATA);

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
		json_object_decode(desired_obj, JSON_KEY_TOPIC_PRFX);
	if (topic_prefix_obj != NULL) {
		(*requested_state) = STATE_UA_PIN_COMPLETE;
		cJSON_Delete(root_obj);
		return 0;
	}

	pairing_obj = json_object_decode(desired_obj, JSON_KEY_PAIRING);
	pairing_state_obj = json_object_decode(pairing_obj, JSON_KEY_STATE);

	if (!pairing_state_obj || pairing_state_obj->type != cJSON_String) {
		if (cJSON_HasObjectItem(desired_obj, JSON_KEY_CFG) == false) {
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
		nct_dc_endpoint_get(&tx_endp, &rx_endp, &m_endp);
		ret += json_add_str_cs(reported_obj, JSON_KEY_TOPIC_PRFX, m_endp.ptr);

		/* Clear pairing config and pairingStatus fields. */
		ret += json_add_str_cs(pairing_obj, JSON_KEY_STATE, PAIRED_STR);
		ret += json_add_null_cs(pairing_obj, JSON_KEY_CFG);
		ret += json_add_null_cs(reported_obj, JSON_KEY_PAIR_STAT);

		/* Report keepalive value. */
		if (cJSON_AddNumberToObjectCS(connection_obj, JSON_KEY_KEEPALIVE,
					    CONFIG_NRF_CLOUD_MQTT_KEEPALIVE) == NULL) {
			ret = -ENOMEM;
		}

		/* Report pairing topics. */
		cJSON *topics_obj = cJSON_AddObjectToObjectCS(pairing_obj, JSON_KEY_TOPICS);

		ret += json_add_str_cs(topics_obj, JSON_KEY_D2C, tx_endp.ptr);
		ret += json_add_str_cs(topics_obj, JSON_KEY_C2D, rx_endp.ptr);

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

	cJSON *tx_obj = json_object_decode(topic_obj, JSON_KEY_D2C);

	err = json_decode_and_alloc(tx_obj, tx_endpoint);
	if (err) {
		cJSON_Delete(root_obj);
		return err;
	}

	cJSON *rx_obj = json_object_decode(topic_obj, JSON_KEY_C2D);

	err = json_decode_and_alloc(rx_obj, rx_endpoint);
	if (err) {
		cJSON_Delete(root_obj);
		return err;
	}

	cJSON_Delete(root_obj);

	return err;
}

static int nrf_cloud_encode_service_info_fota(const struct nrf_cloud_svc_info_fota *const fota,
					      cJSON *const svc_inf_obj)
{
	if (!svc_inf_obj) {
		return -EINVAL;
	}

	if (fota == NULL || !IS_ENABLED(CONFIG_NRF_CLOUD_FOTA)) {
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
			cJSON_AddItemToArray(array, cJSON_CreateString(FOTA_VAL_BOOT));
			++item_cnt;
		}
		if (fota->modem) {
			cJSON_AddItemToArray(array, cJSON_CreateString(FOTA_VAL_MODEM));
			++item_cnt;
		}
		if (fota->application) {
			cJSON_AddItemToArray(array, cJSON_CreateString(FOTA_VAL_APP));
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
		if (ui->air_pressure) {
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

static int encode_info_item(const enum nrf_cloud_shadow_info inf, const char *const inf_name,
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

		if (!cJSON_AddItemToObject(root_obj, inf_name, move_obj)) {
			cJSON_Delete(move_obj);
			LOG_ERR("Failed to add info item \"%s\"", log_strdup(inf_name));
			return -ENOMEM;
		}
		break;
	case NRF_CLOUD_INFO_CLEAR:
		if (json_add_null_cs(root_obj, inf_name) < 0) {
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

	if (encode_info_item(mod_inf->device, MODEM_INFO_JSON_KEY_DEV_INF, tmp, mod_inf_obj) ||
	    encode_info_item(mod_inf->network, MODEM_INFO_JSON_KEY_NET_INF, tmp, mod_inf_obj) ||
	    encode_info_item(mod_inf->sim, MODEM_INFO_JSON_KEY_SIM_INF, tmp, mod_inf_obj)) {
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
	struct nrf_cloud_data * const output)
{
	if (!dev_status || !output) {
		return -EINVAL;
	}

	int err = 0;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_AddObjectToObjectCS(root_obj, JSON_KEY_STATE);
	cJSON *reported_obj = cJSON_AddObjectToObjectCS(state_obj, JSON_KEY_REP);
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
