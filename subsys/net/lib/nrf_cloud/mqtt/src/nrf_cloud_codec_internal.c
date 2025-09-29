/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cJSON.h>
#include <net/nrf_cloud_codec.h>
#include "nrf_cloud_fsm.h"
#include "nrf_cloud_codec_internal.h"
#include "nrf_cloud_mqtt_internal.h"
#include <zephyr/logging/log.h>
#include "nrf_cloud_mem.h"

LOG_MODULE_REGISTER(nrf_cloud_codec_internal_mqtt, CONFIG_NRF_CLOUD_LOG_LEVEL);

#define STRLEN_TOPIC_VAL_C2D (sizeof(NRF_CLOUD_JSON_VAL_TOPIC_C2D) - 1)

#define TOPIC_VAL_RCV_WILDCARD (NRF_CLOUD_JSON_VAL_TOPIC_WILDCARD NRF_CLOUD_JSON_VAL_TOPIC_RCV)
#define TOPIC_VAL_RCV_AGNSS    (NRF_CLOUD_JSON_VAL_TOPIC_AGNSS NRF_CLOUD_JSON_VAL_TOPIC_RCV)
#define TOPIC_VAL_RCV_PGPS     (NRF_CLOUD_JSON_VAL_TOPIC_PGPS NRF_CLOUD_JSON_VAL_TOPIC_RCV)
#define TOPIC_VAL_RCV_C2D      (NRF_CLOUD_JSON_VAL_TOPIC_C2D NRF_CLOUD_JSON_VAL_TOPIC_RCV)
#define TOPIC_VAL_RCV_GND_FIX  (NRF_CLOUD_JSON_VAL_TOPIC_GND_FIX NRF_CLOUD_JSON_VAL_TOPIC_RCV)

static cJSON *json_object_decode(cJSON *obj, const char *str)
{
	return obj ? cJSON_GetObjectItem(obj, str) : NULL;
}

static int json_decode_and_alloc(cJSON *obj, struct mqtt_utf8 *const ep)
{
	char *src = cJSON_GetStringValue(obj);

	if (!ep || !src) {
		return -EINVAL;
	}

	ep->utf8 = nrf_cloud_calloc(strlen(src) + 1, 1);

	if (ep->utf8 == NULL) {
		return -ENOMEM;
	}

	strcpy((char *)ep->utf8, src);

	ep->size = (uint32_t)strlen(ep->utf8);

	return 0;
}

int nrf_cloud_sensor_data_encode(const struct nrf_cloud_sensor_data *sensor,
				 struct nrf_cloud_data *output)
{
	int ret;
	const char *sensor_type_str = nrf_cloud_get_sensor_type_str_internal(sensor->type);

	__ASSERT_NO_MSG(sensor != NULL);
	__ASSERT_NO_MSG(sensor->data.ptr != NULL);
	__ASSERT_NO_MSG(sensor->data.len != 0);
	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(sensor_type_str != NULL);

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		return -ENOMEM;
	}

	ret = !cJSON_AddStringToObjectCS(root_obj, NRF_CLOUD_JSON_APPID_KEY, sensor_type_str);
	ret += !cJSON_AddStringToObjectCS(root_obj, NRF_CLOUD_JSON_DATA_KEY, sensor->data.ptr);
	ret += !cJSON_AddStringToObjectCS(root_obj, NRF_CLOUD_JSON_MSG_TYPE_KEY,
					  NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA);
	if (sensor->ts_ms != NRF_CLOUD_NO_TIMESTAMP) {
		ret += !cJSON_AddNumberToObjectCS(root_obj, NRF_CLOUD_MSG_TIMESTAMP_KEY,
						  sensor->ts_ms);
	}

	if (ret != 0) {
		cJSON_Delete(root_obj);
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

int nrf_cloud_state_encode(uint32_t reported_state, const bool update_desired_topic,
			   const bool add_info_sections, struct nrf_cloud_data *output)
{
	__ASSERT_NO_MSG(output != NULL);

	/* The state only needs to be reported on initial association/connection */
	if ((reported_state != STATE_UA_PIN_WAIT) && (reported_state != STATE_UA_PIN_COMPLETE)) {
		return -ENOTSUP;
	}

	char *buffer = NULL;
	int ret = 0;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_AddObjectToObjectCS(root_obj, NRF_CLOUD_JSON_KEY_STATE);
	cJSON *reported_obj = cJSON_AddObjectToObjectCS(state_obj, NRF_CLOUD_JSON_KEY_REP);
	cJSON *pairing_obj = cJSON_AddObjectToObjectCS(reported_obj, NRF_CLOUD_JSON_KEY_PAIRING);
	cJSON *connection_obj = cJSON_AddObjectToObjectCS(reported_obj, NRF_CLOUD_JSON_KEY_CONN);
	static bool disassociated_state_sent;

	if (!pairing_obj || !connection_obj) {
		cJSON_Delete(root_obj);
		return -ENOMEM;
	}

	if ((reported_state == STATE_UA_PIN_WAIT) && !disassociated_state_sent) {
		disassociated_state_sent = true;
		LOG_DBG("Clearing state; device is not associated");
		/* This is a state used during JITP
		 * or if the user exercises the deprecated DissociateDevice API.
		 * The device exists in nRF Cloud but is not associated to an account.
		 */
		ret += !cJSON_AddStringToObjectCS(pairing_obj, NRF_CLOUD_JSON_KEY_STATE,
						  NRF_CLOUD_JSON_VAL_NOT_ASSOC);
		/* Clear the topics */
		ret += !cJSON_AddNullToObjectCS(pairing_obj, NRF_CLOUD_JSON_KEY_TOPICS);
		/* Clear topic prefix */
		ret += !cJSON_AddNullToObjectCS(reported_obj, NRF_CLOUD_JSON_KEY_TOPIC_PRFX);
		/* Clear the keepalive value */
		ret += !cJSON_AddNullToObjectCS(connection_obj, NRF_CLOUD_JSON_KEY_KEEPALIVE);

		/* Clear deprecated fields */
		ret += !cJSON_AddNullToObjectCS(pairing_obj, NRF_CLOUD_JSON_KEY_CFG);
		ret += !cJSON_AddNullToObjectCS(reported_obj, NRF_CLOUD_JSON_KEY_PAIR_STAT);
		ret += !cJSON_AddNullToObjectCS(reported_obj, NRF_CLOUD_JSON_KEY_STAGE);

	} else if (reported_state == STATE_UA_PIN_COMPLETE) {
		struct nct_dc_endpoints eps;
		struct nrf_cloud_ctrl_data device_ctrl = {0};

		disassociated_state_sent = false;

		/* Associated */
		ret += !cJSON_AddStringToObjectCS(pairing_obj, NRF_CLOUD_JSON_KEY_STATE,
						  NRF_CLOUD_JSON_VAL_PAIRED);

		/* Report keepalive value. */
		ret += !cJSON_AddNumberToObjectCS(connection_obj, NRF_CLOUD_JSON_KEY_KEEPALIVE,
						  CONFIG_NRF_CLOUD_MQTT_KEEPALIVE);

		/* Create the topics object. */
		cJSON *topics_obj =
			cJSON_AddObjectToObjectCS(pairing_obj, NRF_CLOUD_JSON_KEY_TOPICS);

		/* Get the endpoint information and add topics */
		nct_dc_endpoint_get(&eps);
		ret += !cJSON_AddStringToObjectCS(reported_obj, NRF_CLOUD_JSON_KEY_TOPIC_PRFX,
						  (char *)eps.e[DC_BASE].utf8);
		ret += !cJSON_AddStringToObjectCS(topics_obj, NRF_CLOUD_JSON_KEY_DEVICE_TO_CLOUD,
						  (char *)eps.e[DC_TX].utf8);
		ret += !cJSON_AddStringToObjectCS(topics_obj, NRF_CLOUD_JSON_KEY_CLOUD_TO_DEVICE,
						  (char *)eps.e[DC_RX].utf8);

		if (update_desired_topic) {
			/* Align desired c2d topic with reported to prevent delta events */
			cJSON *des_obj =
				cJSON_AddObjectToObjectCS(state_obj, NRF_CLOUD_JSON_KEY_DES);
			cJSON *pair_obj =
				cJSON_AddObjectToObjectCS(des_obj, NRF_CLOUD_JSON_KEY_PAIRING);
			cJSON *topic_obj =
				cJSON_AddObjectToObjectCS(pair_obj, NRF_CLOUD_JSON_KEY_TOPICS);

			ret += !cJSON_AddStringToObjectCS(topic_obj,
							  NRF_CLOUD_JSON_KEY_CLOUD_TO_DEVICE,
							  (char *)eps.e[DC_RX].utf8);
		}

		/* Add reported control section */
		nrf_cloud_device_control_get(&device_ctrl);
		nrf_cloud_device_control_encode_internal(reported_obj, &device_ctrl);

		if (add_info_sections) {
			int err;

			err = nrf_cloud_enabled_info_sections_json_encode(
				reported_obj, nrf_cloud_get_app_version());
			if ((err != 0) && (err != -ENODEV)) {
				ret = err;
			}
		}
	} else {
		goto out;
	}

	if (ret == 0) {
		buffer = cJSON_PrintUnformatted(root_obj);
	}

out:
	cJSON_Delete(root_obj);
	output->ptr = buffer;
	output->len = (buffer ? strlen(buffer) : 0);

	return ret;
}

BUILD_ASSERT(
	sizeof(NRF_CLOUD_JSON_VAL_TOPIC_C2D) == sizeof(TOPIC_VAL_RCV_WILDCARD),
	"NRF_CLOUD_JSON_VAL_TOPIC_C2D and TOPIC_VAL_RCV_WILDCARD are expected to be the same size");
bool nrf_cloud_set_wildcard_c2d_topic(char *const topic, size_t topic_len)
{
	if (!topic || (topic_len < STRLEN_TOPIC_VAL_C2D)) {
		return false;
	}

	char *c2d_str = &topic[topic_len - STRLEN_TOPIC_VAL_C2D];

	/* If the shadow contains the old c2d postfix, update to use new wildcard string */
	if (memcmp(c2d_str, NRF_CLOUD_JSON_VAL_TOPIC_C2D, sizeof(NRF_CLOUD_JSON_VAL_TOPIC_C2D)) ==
	    0) {
		/* The build assert above ensures the string defines are the same size */
		memcpy(c2d_str, TOPIC_VAL_RCV_WILDCARD, sizeof(NRF_CLOUD_JSON_VAL_TOPIC_C2D));
		LOG_DBG("Replaced \"%s\" with \"%s\" in c2d topic", NRF_CLOUD_JSON_VAL_TOPIC_C2D,
			TOPIC_VAL_RCV_WILDCARD);
		return true;
	}

	return false;
}

enum nrf_cloud_rcv_topic nrf_cloud_dc_rx_topic_decode(const char *const topic)
{
	if (!topic) {
		return NRF_CLOUD_RCV_TOPIC_UNKNOWN;
	}

	if (strstr(topic, TOPIC_VAL_RCV_AGNSS)) {
		return NRF_CLOUD_RCV_TOPIC_AGNSS;
	} else if (strstr(topic, TOPIC_VAL_RCV_PGPS)) {
		return NRF_CLOUD_RCV_TOPIC_PGPS;
	} else if (strstr(topic, TOPIC_VAL_RCV_GND_FIX)) {
		return NRF_CLOUD_RCV_TOPIC_LOCATION;
	} else if (strstr(topic, TOPIC_VAL_RCV_C2D)) {
		return NRF_CLOUD_RCV_TOPIC_GENERAL;
	}

	return NRF_CLOUD_RCV_TOPIC_UNKNOWN;
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

	struct nct_dc_data msg = {.data.ptr = msg_string, .data.len = strlen(msg_string)};

	LOG_DBG("Created request: %s (size: %u)", (char *)msg.data.ptr, msg.data.len);

	err = nct_dc_send(&msg);
	if (err) {
		LOG_ERR("Failed to send request, error: %d", err);
	} else {
		LOG_DBG("Request sent to cloud");
	}

	nrf_cloud_free(msg_string);

	return err;
}

int nrf_cloud_obj_endpoint_decode(const struct nrf_cloud_obj *const desired_obj,
				  struct nct_dc_endpoints *const eps)
{
	__ASSERT_NO_MSG(desired_obj != NULL);
	__ASSERT_NO_MSG(desired_obj->json != NULL);
	__ASSERT_NO_MSG(desired_obj->type == NRF_CLOUD_OBJ_TYPE_JSON);
	__ASSERT_NO_MSG(eps != NULL);

	int err;
	size_t len_tmp;
	cJSON *endpoint_obj = json_object_decode(desired_obj->json, NRF_CLOUD_JSON_KEY_TOPIC_PRFX);
	cJSON *pairing_obj = json_object_decode(desired_obj->json, NRF_CLOUD_JSON_KEY_PAIRING);
	cJSON *pairing_state_obj = json_object_decode(pairing_obj, NRF_CLOUD_JSON_KEY_STATE);
	cJSON *topic_obj = json_object_decode(pairing_obj, NRF_CLOUD_JSON_KEY_TOPICS);

	if ((pairing_state_obj == NULL) || (topic_obj == NULL) ||
	    (pairing_state_obj->type != cJSON_String)) {
		return -ENOENT;
	}

	const char *state_str = pairing_state_obj->valuestring;

	if (strcmp(state_str, NRF_CLOUD_JSON_VAL_PAIRED)) {
		return -ENOENT;
	}

	if (endpoint_obj != NULL) {
		err = json_decode_and_alloc(endpoint_obj, &eps->e[DC_BASE]);
		if (err) {
			return err;
		}
	}

	cJSON *tx_obj = json_object_decode(topic_obj, NRF_CLOUD_JSON_KEY_DEVICE_TO_CLOUD);

	err = json_decode_and_alloc(tx_obj, &eps->e[DC_TX]);
	if (err) {
		LOG_ERR("Could not decode topic for %s", NRF_CLOUD_JSON_KEY_DEVICE_TO_CLOUD);
		return err;
	}

	/* Populate bulk endpoint topic by copying and appending /bulk to the parsed
	 * tx endpoint (d2c) topic.
	 */
	len_tmp = eps->e[DC_TX].size + sizeof(NRF_CLOUD_BULK_MSG_TOPIC);
	eps->e[DC_BULK].utf8 = (uint8_t *)nrf_cloud_calloc(len_tmp, 1);
	if (eps->e[DC_BULK].utf8 == NULL) {
		LOG_ERR("Could not allocate memory for bulk topic");
		return -ENOMEM;
	}

	eps->e[DC_BULK].size = snprintk((char *)eps->e[DC_BULK].utf8, len_tmp, "%s%s",
					(char *)eps->e[DC_TX].utf8, NRF_CLOUD_BULK_MSG_TOPIC);

	/* Populate bin endpoint topic by copying and appending /bin to the parsed
	 * tx endpoint (d2c) topic.
	 */
	len_tmp = eps->e[DC_TX].size + sizeof(NRF_CLOUD_JSON_VAL_TOPIC_BIN);
	eps->e[DC_BIN].utf8 = (uint8_t *)nrf_cloud_calloc(len_tmp, 1);
	if (eps->e[DC_BIN].utf8 == NULL) {
		LOG_ERR("Could not allocate memory for bin topic");
		return -ENOMEM;
	}

	eps->e[DC_BIN].size = snprintk((char *)eps->e[DC_BIN].utf8, len_tmp, "%s%s",
				       (char *)eps->e[DC_TX].utf8, NRF_CLOUD_JSON_VAL_TOPIC_BIN);

	err = json_decode_and_alloc(
		json_object_decode(topic_obj, NRF_CLOUD_JSON_KEY_CLOUD_TO_DEVICE), &eps->e[DC_RX]);
	if (err) {
		LOG_ERR("Failed to parse \"%s\" from JSON, error: %d",
			NRF_CLOUD_JSON_KEY_CLOUD_TO_DEVICE, err);
		return err;
	}

	return err;
}

int nrf_cloud_shadow_data_state_decode(const struct nrf_cloud_obj_shadow_data *const input,
				       enum nfsm_state *const requested_state)
{
	__ASSERT_NO_MSG(requested_state != NULL);
	__ASSERT_NO_MSG(input != NULL);

	if (input->type == NRF_CLOUD_OBJ_SHADOW_TYPE_TF) {
		return -ENOMSG;
	}

	cJSON *desired_obj = NULL;
	cJSON *pairing_obj = NULL;
	cJSON *pairing_state_obj = NULL;
	cJSON *topic_prefix_obj = NULL;

	if (input->type == NRF_CLOUD_OBJ_SHADOW_TYPE_ACCEPTED) {
		desired_obj = input->accepted->desired.json;
	} else if (input->type == NRF_CLOUD_OBJ_SHADOW_TYPE_DELTA) {
		desired_obj = input->delta->state.json;
	} else {
		return -ENOTSUP;
	}

	topic_prefix_obj = json_object_decode(desired_obj, NRF_CLOUD_JSON_KEY_TOPIC_PRFX);
	pairing_obj = json_object_decode(desired_obj, NRF_CLOUD_JSON_KEY_PAIRING);

	if (topic_prefix_obj != NULL) {
		/* If the topic prefix is found, association is complete */
		nct_set_topic_prefix(topic_prefix_obj->valuestring);
		(*requested_state) = STATE_UA_PIN_COMPLETE;
		return 0;
	}

	/* If no topic prefix, check if pairing state is "not associated" or already "paired" */
	pairing_state_obj = json_object_decode(pairing_obj, NRF_CLOUD_JSON_KEY_STATE);

	if (!pairing_state_obj || (pairing_state_obj->type != cJSON_String)) {
		/* This shadow data does not contain the necessary data */
		return -ENODATA;
	}

	const char *state_str = pairing_state_obj->valuestring;

	if (!strcmp(state_str, NRF_CLOUD_JSON_VAL_NOT_ASSOC)) {
		(*requested_state) = STATE_UA_PIN_WAIT;
	} else if (!strcmp(state_str, NRF_CLOUD_JSON_VAL_PAIRED)) {
		/* Already paired, ignore */
		return -EALREADY;
	}

	LOG_ERR("Deprecated device/cloud state.");
	LOG_INF("Delete the device from your nRF Cloud account...");
	LOG_INF("Install new credentials and then re-add the device to your account");
	return -ENOTSUP;
}
