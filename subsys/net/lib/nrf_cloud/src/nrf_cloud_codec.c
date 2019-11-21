/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "nrf_cloud_codec.h"
#include "nrf_cloud_mem.h"

#include <stdbool.h>
#include <string.h>
#include <zephyr.h>
#include <logging/log.h>
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

static int json_add_null(cJSON *parent, const char *str)
{
	cJSON *json_null;

	json_null = cJSON_CreateNull();
	if (json_null == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_null);
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
		/* On initial pairing there is no "desired" JSON key, */
		/* "state" is used instead */
		state_obj = json_object_decode(root_obj, "state");
		if (state_obj == NULL) {
			*desired_obj = json_object_decode(root_obj, "desired");
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
		LOG_DBG("No valid state found!");
		cJSON_Delete(root_obj);
		return -ENOENT;
	}

	const char *state_str = pairing_state_obj->valuestring;

	if (compare(state_str, DUA_PIN_STR)) {
		(*requested_state) = STATE_UA_PIN_WAIT;
	} else {
		LOG_ERR("Deprecated state. Delete device from nrfCloud and update device with JITP certificates.");
		cJSON_Delete(root_obj);
		return -ENOTSUP;
	}

	cJSON_Delete(root_obj);

	return 0;
}

int nrf_cloud_encode_state(u32_t reported_state, struct nrf_cloud_data *output)
{
	int ret;

	__ASSERT_NO_MSG(output != NULL);

	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_CreateObject();
	cJSON *reported_obj = cJSON_CreateObject();
	cJSON *pairing_obj = cJSON_CreateObject();

	if ((root_obj == NULL) || (state_obj == NULL) ||
	    (reported_obj == NULL) || (pairing_obj == NULL)) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(reported_obj);
		cJSON_Delete(pairing_obj);

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

		/* Report pairing topics. */
		cJSON *topics_obj = cJSON_CreateObject();

		if (topics_obj == NULL) {
			cJSON_Delete(root_obj);
			cJSON_Delete(state_obj);
			cJSON_Delete(reported_obj);
			cJSON_Delete(pairing_obj);

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
		return -ENOTSUP;
	}
	}

	ret += json_add_obj(reported_obj, "pairing", pairing_obj);
	ret += json_add_obj(state_obj, "reported", reported_obj);
	ret += json_add_obj(root_obj, "state", state_obj);

	if (ret != 0) {
		cJSON_Delete(root_obj);
		cJSON_Delete(state_obj);
		cJSON_Delete(reported_obj);
		cJSON_Delete(pairing_obj);

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
