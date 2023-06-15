/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf_cloud_codec_internal.h"
#include "nrf_cloud_mem.h"
#include "nrf_cloud_fsm.h"
#include <net/nrf_cloud_codec.h>
#include "nrf_cloud_log_internal.h"
#include <net/nrf_cloud_location.h>
#include <net/nrf_cloud_alert.h>
#include <net/nrf_cloud_log.h>
#include <zephyr/logging/log_output.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <version.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <modem/modem_info.h>
#include "cJSON_os.h"

LOG_MODULE_REGISTER(nrf_cloud_codec_internal, CONFIG_NRF_CLOUD_LOG_LEVEL);

bool initialized;
static const char *application_version;

#if defined(CONFIG_MODEM_INFO)
static struct modem_param_info modem_inf;
static bool modem_inf_initd;
static int init_modem_info(void);
#endif

#if defined(CONFIG_NRF_CLOUD_MQTT)
#ifdef CONFIG_NRF_CLOUD_GATEWAY
static gateway_state_handler_t gateway_state_handler;
#endif
#endif

static const char *const sensor_type_str[] = {
	[NRF_CLOUD_SENSOR_GNSS] = NRF_CLOUD_JSON_APPID_VAL_GNSS,
	[NRF_CLOUD_SENSOR_FLIP] = NRF_CLOUD_JSON_APPID_VAL_FLIP,
	[NRF_CLOUD_SENSOR_BUTTON] = NRF_CLOUD_JSON_APPID_VAL_BTN,
	[NRF_CLOUD_SENSOR_TEMP] = NRF_CLOUD_JSON_APPID_VAL_TEMP,
	[NRF_CLOUD_SENSOR_HUMID] = NRF_CLOUD_JSON_APPID_VAL_HUMID,
	[NRF_CLOUD_SENSOR_AIR_PRESS] = NRF_CLOUD_JSON_APPID_VAL_AIR_PRESS,
	[NRF_CLOUD_SENSOR_AIR_QUAL] = NRF_CLOUD_JSON_APPID_VAL_AIR_QUAL,
	[NRF_CLOUD_LTE_LINK_RSRP] = NRF_CLOUD_JSON_APPID_VAL_RSRP,
	[NRF_CLOUD_LOG] = NRF_CLOUD_JSON_APPID_VAL_LOG,
	[NRF_CLOUD_DICTIONARY_LOG] = NRF_CLOUD_JSON_APPID_VAL_DICTIONARY_LOG,
	[NRF_CLOUD_DEVICE_INFO] = NRF_CLOUD_JSON_APPID_VAL_DEVICE,
	[NRF_CLOUD_SENSOR_LIGHT] = NRF_CLOUD_JSON_APPID_VAL_LIGHT,
};
#define SENSOR_TYPE_ARRAY_SIZE (sizeof(sensor_type_str) / sizeof(*sensor_type_str))

#define STRLEN_TOPIC_VAL_C2D	(sizeof(NRF_CLOUD_JSON_VAL_TOPIC_C2D) - 1)

#define TOPIC_VAL_RCV_WILDCARD	(NRF_CLOUD_JSON_VAL_TOPIC_WILDCARD NRF_CLOUD_JSON_VAL_TOPIC_RCV)
#define TOPIC_VAL_RCV_AGPS	(NRF_CLOUD_JSON_VAL_TOPIC_AGPS     NRF_CLOUD_JSON_VAL_TOPIC_RCV)
#define TOPIC_VAL_RCV_PGPS	(NRF_CLOUD_JSON_VAL_TOPIC_PGPS     NRF_CLOUD_JSON_VAL_TOPIC_RCV)
#define TOPIC_VAL_RCV_C2D	(NRF_CLOUD_JSON_VAL_TOPIC_C2D      NRF_CLOUD_JSON_VAL_TOPIC_RCV)
#define TOPIC_VAL_RCV_GND_FIX	(NRF_CLOUD_JSON_VAL_TOPIC_GND_FIX  NRF_CLOUD_JSON_VAL_TOPIC_RCV)

/* Max length of a NRF_CLOUD_JSON_MSG_TYPE_VAL_DISCONNECT message */
#define NRF_CLOUD_JSON_MSG_MAX_LEN_DISCONNECT	200

int nrf_cloud_codec_init(struct nrf_cloud_os_mem_hooks *hooks)
{
	if (!initialized) {
		if (hooks == NULL) {
			/* Use OS defaults */
			cJSON_Init();
		} else {
			cJSON_Hooks cjson_hooks = {
				.free_fn = hooks->free_fn,
				.malloc_fn = hooks->malloc_fn,
			};

			cJSON_InitHooks(&cjson_hooks);
		}
#if defined(CONFIG_MODEM_INFO)
		init_modem_info();
#endif
		initialized = true;
	}
	return 0;
}

/* remove static to eliminate warning if not using CONFIG_NRF_CLOUD_MQTT */
int json_add_bool_cs(cJSON *parent, const char *str, bool item)
{
	if (!parent || !str) {
		return -EINVAL;
	}

	return cJSON_AddBoolToObjectCS(parent, str, item) ? 0 : -ENOMEM;
}

void nrf_cloud_set_app_version(const char * const app_ver)
{
	application_version = app_ver;
}

static int json_add_num_cs(cJSON *parent, const char *str, double item)
{
	if (!parent || !str) {
		return -EINVAL;
	}

	return cJSON_AddNumberToObjectCS(parent, str, item) ? 0 : -ENOMEM;
}

static int json_add_str_cs(cJSON *parent, const char *str, const char *item)
{
	if (!parent || !str || !item) {
		return -EINVAL;
	}

	return cJSON_AddStringToObjectCS(parent, str, item) ? 0 : -ENOMEM;
}

cJSON *json_create_req_obj(const char *const app_id, const char *const msg_type)
{
	__ASSERT_NO_MSG(app_id != NULL);
	__ASSERT_NO_MSG(msg_type != NULL);

	nrf_cloud_codec_init(NULL);

	cJSON *req_obj = cJSON_CreateObject();

	if (json_add_str_cs(req_obj, NRF_CLOUD_JSON_APPID_KEY, app_id) ||
	    json_add_str_cs(req_obj, NRF_CLOUD_JSON_MSG_TYPE_KEY, msg_type)) {
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

static int info_encode(cJSON * const root_obj, const struct nrf_cloud_modem_info * const mdm_inf,
	const struct nrf_cloud_svc_info * const svc_inf)
{
	int ret = 0;

#ifdef CONFIG_MODEM_INFO
	if (mdm_inf) {
		ret = nrf_cloud_modem_info_json_encode(mdm_inf, root_obj);
		if (ret) {
			return -ENOMEM;
		}
	}
#endif

	if (svc_inf) {
		cJSON *svc_inf_obj = cJSON_AddObjectToObjectCS(root_obj,
							       NRF_CLOUD_JSON_KEY_SRVC_INFO);

		if (svc_inf_obj == NULL) {
			return -ENOMEM;
		}

		ret = nrf_cloud_service_info_json_encode(svc_inf, svc_inf_obj);
	}

	return ret;
}

#if defined(CONFIG_NRF_CLOUD_MQTT)
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

static void nrf_cloud_decode_desired_obj(cJSON *root_obj,
					 cJSON **desired_obj)
{
	cJSON *state_obj;

	if ((root_obj != NULL) && (desired_obj != NULL)) {
		/* On initial pairing, a shadow delta event is sent */
		/* which does not include the "desired" JSON key, */
		/* "state" is used instead */
		state_obj = json_object_decode(root_obj, NRF_CLOUD_JSON_KEY_STATE);
		if (state_obj == NULL) {
			*desired_obj = json_object_decode(root_obj, NRF_CLOUD_JSON_KEY_DES);
		} else {
			*desired_obj = state_obj;
		}
	}
}

int nrf_cloud_sensor_data_encode(const struct nrf_cloud_sensor_data *sensor,
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
	if (sensor->ts_ms != NRF_CLOUD_NO_TIMESTAMP) {
		ret += json_add_num_cs(root_obj, NRF_CLOUD_MSG_TIMESTAMP_KEY, sensor->ts_ms);
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

#ifdef CONFIG_NRF_CLOUD_GATEWAY
void nrf_cloud_register_gateway_state_handler(gateway_state_handler_t handler)
{
	gateway_state_handler = handler;
}
#endif

int nrf_cloud_requested_state_decode(const struct nrf_cloud_data *input,
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
		LOG_ERR("cJSON_Parse failed: %s", (char *)input->ptr);
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
		json_object_decode(desired_obj, NRF_CLOUD_JSON_KEY_TOPIC_PRFX);
	if (topic_prefix_obj != NULL) {
		nct_set_topic_prefix(topic_prefix_obj->valuestring);
		(*requested_state) = STATE_UA_PIN_COMPLETE;
		cJSON_Delete(root_obj);
		return 0;
	}

	pairing_obj = json_object_decode(desired_obj, NRF_CLOUD_JSON_KEY_PAIRING);
	pairing_state_obj = json_object_decode(pairing_obj, NRF_CLOUD_JSON_KEY_STATE);

	if (!pairing_state_obj || pairing_state_obj->type != cJSON_String) {
#ifndef CONFIG_NRF_CLOUD_GATEWAY
		if ((cJSON_HasObjectItem(desired_obj, NRF_CLOUD_JSON_KEY_CFG) == false) &&
		    (cJSON_HasObjectItem(desired_obj, NRF_CLOUD_JSON_KEY_CTRL) == false)) {
			LOG_WRN("Unhandled data received from nRF Cloud.");
			LOG_INF("Ensure device firmware is up to date.");
			LOG_INF("Delete and re-add device to nRF Cloud if problem persists.");
		}
#endif
		cJSON_Delete(root_obj);
		return -ENOENT;
	}

	const char *state_str = pairing_state_obj->valuestring;

	if (compare(state_str, NRF_CLOUD_JSON_VAL_NOT_ASSOC)) {
		(*requested_state) = STATE_UA_PIN_WAIT;
	} else {
		LOG_ERR("Deprecated state. Delete device from nRF Cloud and update device with JITP certificates.");
		cJSON_Delete(root_obj);
		return -ENOTSUP;
	}

	cJSON_Delete(root_obj);

	return 0;
}

int nrf_cloud_shadow_config_response_encode(struct nrf_cloud_data const *const input,
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
	state_obj = cJSON_DetachItemFromObject(input_obj, NRF_CLOUD_JSON_KEY_STATE);
	config_obj = cJSON_DetachItemFromObject(
		state_obj ? state_obj : input_obj, NRF_CLOUD_JSON_KEY_CFG);
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
	desired_obj = cJSON_AddObjectToObjectCS(root_obj, NRF_CLOUD_JSON_KEY_DES);
	reported_obj = cJSON_AddObjectToObjectCS(root_obj, NRF_CLOUD_JSON_KEY_REP);

	/* Add a null config to desired and add the delta config to reported */
	if (json_add_null_cs(desired_obj, NRF_CLOUD_JSON_KEY_CFG) ||
	    json_add_obj_cs(reported_obj, NRF_CLOUD_JSON_KEY_CFG, config_obj)) {
		cJSON_Delete(root_obj);
		cJSON_Delete(config_obj);
		cJSON_Delete(state_obj);
		return -ENOMEM;
	}

	/* Cleanup received state obj and re-use for the response */
	cJSON_Delete(state_obj);
	state_obj = cJSON_CreateObject();
	if (state_obj) {
		if (json_add_obj_cs(state_obj, NRF_CLOUD_JSON_KEY_STATE, root_obj)) {
			cJSON_Delete(root_obj);
		} else {
			buffer = cJSON_PrintUnformatted(state_obj);
		}
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

int nrf_cloud_shadow_control_decode(struct nrf_cloud_data const *const input,
				    enum nrf_cloud_ctrl_status *status,
				    struct nrf_cloud_ctrl_data *data)
{
	__ASSERT_NO_MSG(input != NULL);
	__ASSERT_NO_MSG(status != NULL);
	__ASSERT_NO_MSG(data != NULL);

	cJSON *state_obj = NULL;
	cJSON *control_obj = NULL;
	cJSON *alert_obj = NULL;
	cJSON *log_obj = NULL;
	cJSON *input_obj = cJSON_Parse(input->ptr);

	if (input_obj == NULL) {
		return -ESRCH; /* invalid input or no JSON parsed */
	}

	/* A delta update will have the control inside of state. */
	state_obj = cJSON_GetObjectItem(input_obj, NRF_CLOUD_JSON_KEY_STATE);
	if (state_obj) {
		control_obj = cJSON_GetObjectItem(state_obj, NRF_CLOUD_JSON_KEY_CTRL);
		if (control_obj) {
			LOG_DBG("Control inside of state");
		}
	}
	/* A shadow/get/accepted on initial connect will have control inside of desired. */
	if (control_obj == NULL) {
		state_obj = cJSON_GetObjectItem(input_obj, NRF_CLOUD_JSON_KEY_DES);
		if (state_obj) {
			control_obj = cJSON_GetObjectItem(state_obj, NRF_CLOUD_JSON_KEY_CTRL);
			if (control_obj) {
				LOG_DBG("Control inside of desired");
			}
		}
	}
	/* If there is no delta and no desired, but there is reported, use that. */
	if (control_obj == NULL) {
		state_obj = cJSON_GetObjectItem(input_obj, NRF_CLOUD_JSON_KEY_REP);
		if (state_obj) {
			control_obj = cJSON_GetObjectItem(state_obj, NRF_CLOUD_JSON_KEY_CTRL);
			if (control_obj) {
				LOG_DBG("Control inside of reported");
			}
		}
	}
	if (control_obj == NULL) {
		LOG_DBG("Shadow delta does not have control section");
		*status = NRF_CLOUD_CTRL_NOT_PRESENT;
		goto end;
	}

	bool differs = false;

	alert_obj = cJSON_GetObjectItem(control_obj, NRF_CLOUD_JSON_KEY_ALERT);
	if (alert_obj == NULL) {
		LOG_DBG(NRF_CLOUD_JSON_KEY_ALERT " not found");
	} else if (cJSON_IsBool(alert_obj)) {
		if (data->alerts_enabled != cJSON_IsTrue(alert_obj)) {
			differs = true;
			data->alerts_enabled = cJSON_IsTrue(alert_obj);
			LOG_INF("AlertsEn changed to %u", data->alerts_enabled);
		}
	} else {
		LOG_WRN(NRF_CLOUD_JSON_KEY_ALERT " is not a bool");
	}

	log_obj = cJSON_GetObjectItem(control_obj, NRF_CLOUD_JSON_KEY_LOG);
	if (log_obj == NULL) {
		LOG_DBG(NRF_CLOUD_JSON_KEY_LOG " not found");
	} else if (cJSON_IsNumber(log_obj)) {
		if (data->log_level != (int)cJSON_GetNumberValue(log_obj)) {
			differs = true;
			data->log_level = (int)cJSON_GetNumberValue(log_obj);
			LOG_INF("LogLvl changed to %u", data->log_level);
		}
	} else {
		LOG_WRN(NRF_CLOUD_JSON_KEY_LOG " is not a number");
	}

	if ((state_obj == NULL) && !differs) {
		/* If this is not a delta update, and our settings match,
		 * then no shadow update is required.
		 */
		LOG_DBG("No response needed. Got shadow: %s", (const char *)input->ptr);
		*status = NRF_CLOUD_CTRL_NO_REPLY;
	} else {
		LOG_DBG("Got delta: %s", (const char *)input->ptr);
		*status = NRF_CLOUD_CTRL_REPLY;
	}

end:
	cJSON_Delete(input_obj);
	return 0;
}

int nrf_cloud_shadow_control_response_encode(struct nrf_cloud_ctrl_data const *const data,
					     struct nrf_cloud_data *const output)
{
	__ASSERT_NO_MSG(data != NULL);
	__ASSERT_NO_MSG(output != NULL);

	char *buffer = NULL;
	int err = 0;

	/* Prepare JSON response for the delta */
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_AddObjectToObjectCS(root_obj, NRF_CLOUD_JSON_KEY_STATE);
	cJSON *reported_obj = cJSON_AddObjectToObjectCS(state_obj, NRF_CLOUD_JSON_KEY_REP);
	cJSON *control_obj = cJSON_AddObjectToObjectCS(reported_obj, NRF_CLOUD_JSON_KEY_CTRL);
	int alert_ret = json_add_bool_cs(control_obj, NRF_CLOUD_JSON_KEY_ALERT,
					 data->alerts_enabled);
	int log_ret = json_add_num_cs(control_obj, NRF_CLOUD_JSON_KEY_LOG, data->log_level);

	/* If any object could not be created, we ran out of memory */
	if (!(root_obj && state_obj && reported_obj && control_obj &&
	      !alert_ret && !log_ret)) {
		err = -ENOMEM;
		goto end;
	}
	buffer = cJSON_PrintUnformatted(root_obj);
	if (!buffer) {
		err = -ENOMEM;
		goto end;
	}
	LOG_DBG("Sending shadow back: %s", buffer);

	output->ptr = buffer;
	output->len = strlen(buffer);

end:
	cJSON_Delete(root_obj);
	return err;
}

static int device_status_encode(cJSON * const reported_obj)
{
	struct nrf_cloud_modem_info mdm_inf = {
		.device = NRF_CLOUD_INFO_SET,
		.application_version = application_version
	};
	cJSON *device_obj = cJSON_AddObjectToObjectCS(reported_obj, NRF_CLOUD_JSON_KEY_DEVICE);

	if (!device_obj) {
		return -ENOMEM;
	}

	mdm_inf.network = IS_ENABLED(CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS_NETWORK) ?
					NRF_CLOUD_INFO_SET : NRF_CLOUD_INFO_CLEAR;

	mdm_inf.sim = IS_ENABLED(CONFIG_NRF_CLOUD_SEND_DEVICE_STATUS_SIM) ?
					NRF_CLOUD_INFO_SET : NRF_CLOUD_INFO_CLEAR;

	return info_encode(device_obj, &mdm_inf, NULL);
}

int nrf_cloud_state_encode(uint32_t reported_state, const bool update_desired_topic,
			   const bool add_dev_status, struct nrf_cloud_data *output)
{
	__ASSERT_NO_MSG(output != NULL);

	char *buffer;
	int ret = 0;
	cJSON *root_obj = cJSON_CreateObject();
	cJSON *state_obj = cJSON_AddObjectToObjectCS(root_obj, NRF_CLOUD_JSON_KEY_STATE);
	cJSON *reported_obj = cJSON_AddObjectToObjectCS(state_obj, NRF_CLOUD_JSON_KEY_REP);
	cJSON *pairing_obj = cJSON_AddObjectToObjectCS(reported_obj, NRF_CLOUD_JSON_KEY_PAIRING);
	cJSON *connection_obj = cJSON_AddObjectToObjectCS(reported_obj, NRF_CLOUD_JSON_KEY_CONN);

	if (!pairing_obj || !connection_obj) {
		cJSON_Delete(root_obj);
		return -ENOMEM;
	}

	switch (reported_state) {
	case STATE_UA_PIN_WAIT: {
		ret += json_add_str_cs(pairing_obj, NRF_CLOUD_JSON_KEY_STATE,
				       NRF_CLOUD_JSON_VAL_NOT_ASSOC);
		ret += json_add_null_cs(pairing_obj, NRF_CLOUD_JSON_KEY_TOPICS);
		ret += json_add_null_cs(pairing_obj, NRF_CLOUD_JSON_KEY_CFG);
		ret += json_add_null_cs(reported_obj, NRF_CLOUD_JSON_KEY_STAGE);
		ret += json_add_null_cs(reported_obj, NRF_CLOUD_JSON_KEY_TOPIC_PRFX);
		ret += json_add_null_cs(connection_obj, NRF_CLOUD_JSON_KEY_KEEPALIVE);
		if (ret != 0) {
			cJSON_Delete(root_obj);
			return -ENOMEM;
		}
		break;
	}
	case STATE_UA_PIN_COMPLETE: {
		struct nrf_cloud_data rx_endp;
		struct nrf_cloud_data tx_endp;
		struct nrf_cloud_data m_endp;

		/* Get the endpoint information. */
		nct_dc_endpoint_get(&tx_endp, &rx_endp, NULL, NULL, &m_endp);
		ret += json_add_str_cs(reported_obj, NRF_CLOUD_JSON_KEY_TOPIC_PRFX, m_endp.ptr);

		/* Clear pairing config and pairingStatus fields. */
		ret += json_add_str_cs(pairing_obj, NRF_CLOUD_JSON_KEY_STATE,
				       NRF_CLOUD_JSON_VAL_PAIRED);
		ret += json_add_null_cs(pairing_obj, NRF_CLOUD_JSON_KEY_CFG);
		ret += json_add_null_cs(reported_obj, NRF_CLOUD_JSON_KEY_PAIR_STAT);

		/* Report keepalive value. */
		ret += json_add_num_cs(connection_obj, NRF_CLOUD_JSON_KEY_KEEPALIVE,
				       CONFIG_NRF_CLOUD_MQTT_KEEPALIVE);

		/* Report pairing topics. */
		cJSON *topics_obj = cJSON_AddObjectToObjectCS(pairing_obj,
							      NRF_CLOUD_JSON_KEY_TOPICS);

		ret += json_add_str_cs(topics_obj, NRF_CLOUD_JSON_KEY_DEVICE_TO_CLOUD,
				       tx_endp.ptr);
		ret += json_add_str_cs(topics_obj, NRF_CLOUD_JSON_KEY_CLOUD_TO_DEVICE,
				       rx_endp.ptr);

		if (update_desired_topic) {
			/* Align desired c2d topic with reported to prevent delta events */
			cJSON *des_obj = cJSON_AddObjectToObjectCS(state_obj,
								   NRF_CLOUD_JSON_KEY_DES);
			cJSON *pair_obj = cJSON_AddObjectToObjectCS(des_obj,
								    NRF_CLOUD_JSON_KEY_PAIRING);
			cJSON *topic_obj = cJSON_AddObjectToObjectCS(pair_obj,
								     NRF_CLOUD_JSON_KEY_TOPICS);

			ret += json_add_str_cs(topic_obj, NRF_CLOUD_JSON_KEY_CLOUD_TO_DEVICE,
					       rx_endp.ptr);
		}

		if (add_dev_status) {
			ret += device_status_encode(reported_obj);
		}

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
int nrf_cloud_data_endpoint_decode(const struct nrf_cloud_data *input,
				   struct nrf_cloud_data *tx_endpoint,
				   struct nrf_cloud_data *rx_endpoint,
				   struct nrf_cloud_data *bulk_endpoint,
				   struct nrf_cloud_data *bin_endpoint,
				   struct nrf_cloud_data *m_endpoint)
{
	__ASSERT_NO_MSG(input != NULL);
	__ASSERT_NO_MSG(input->ptr != NULL);
	__ASSERT_NO_MSG(input->len != 0);
	__ASSERT_NO_MSG(tx_endpoint != NULL);
	__ASSERT_NO_MSG(rx_endpoint != NULL);
	__ASSERT_NO_MSG(bulk_endpoint != NULL);
	__ASSERT_NO_MSG(bin_endpoint != NULL);

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

	cJSON *pairing_obj = json_object_decode(desired_obj, NRF_CLOUD_JSON_KEY_PAIRING);
	cJSON *pairing_state_obj = json_object_decode(pairing_obj, NRF_CLOUD_JSON_KEY_STATE);
	cJSON *topic_obj = json_object_decode(pairing_obj, NRF_CLOUD_JSON_KEY_TOPICS);

	if ((pairing_state_obj == NULL) || (topic_obj == NULL) ||
	    (pairing_state_obj->type != cJSON_String)) {
		cJSON_Delete(root_obj);
		return -ENOENT;
	}

	const char *state_str = pairing_state_obj->valuestring;

	if (!compare(state_str, NRF_CLOUD_JSON_VAL_PAIRED)) {
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

	cJSON *tx_obj = json_object_decode(topic_obj, NRF_CLOUD_JSON_KEY_DEVICE_TO_CLOUD);

	err = json_decode_and_alloc(tx_obj, tx_endpoint);
	if (err) {
		cJSON_Delete(root_obj);
		LOG_ERR("Could not decode topic for %s", NRF_CLOUD_JSON_KEY_DEVICE_TO_CLOUD);
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

	/* Populate bin endpoint topic by copying and appending /bin to the parsed
	 * tx endpoint (d2c) topic.
	 */
	size_t bin_ep_len_temp = tx_endpoint->len + sizeof(NRF_CLOUD_JSON_VAL_TOPIC_BIN);

	bin_endpoint->ptr = nrf_cloud_calloc(bin_ep_len_temp, 1);
	if (bin_endpoint->ptr == NULL) {
		cJSON_Delete(root_obj);
		LOG_ERR("Could not allocate memory for bin topic");
		return -ENOMEM;
	}

	bin_endpoint->len = snprintk((char *)bin_endpoint->ptr, bin_ep_len_temp, "%s%s",
				       (char *)tx_endpoint->ptr,
				       NRF_CLOUD_JSON_VAL_TOPIC_BIN);

	err = json_decode_and_alloc(json_object_decode(topic_obj,
		NRF_CLOUD_JSON_KEY_CLOUD_TO_DEVICE), rx_endpoint);
	if (err) {
		cJSON_Delete(root_obj);
		LOG_ERR("Failed to parse \"%s\" from JSON, error: %d",
			NRF_CLOUD_JSON_KEY_CLOUD_TO_DEVICE, err);
		return err;
	}

	cJSON_Delete(root_obj);

	return err;
}

BUILD_ASSERT(sizeof(NRF_CLOUD_JSON_VAL_TOPIC_C2D) == sizeof(TOPIC_VAL_RCV_WILDCARD),
	"NRF_CLOUD_JSON_VAL_TOPIC_C2D and TOPIC_VAL_RCV_WILDCARD are expected to be the same size");
bool nrf_cloud_set_wildcard_c2d_topic(char *const topic, size_t topic_len)
{
	if (!topic || (topic_len < STRLEN_TOPIC_VAL_C2D)) {
		return false;
	}

	char *c2d_str = &topic[topic_len - STRLEN_TOPIC_VAL_C2D];

	/* If the shadow contains the old c2d postfix, update to use new wildcard string */
	if (memcmp(c2d_str, NRF_CLOUD_JSON_VAL_TOPIC_C2D,
	    sizeof(NRF_CLOUD_JSON_VAL_TOPIC_C2D)) == 0) {
		/* The build assert above ensures the string defines are the same size */
		memcpy(c2d_str, TOPIC_VAL_RCV_WILDCARD, sizeof(NRF_CLOUD_JSON_VAL_TOPIC_C2D));
		LOG_DBG("Replaced \"%s\" with \"%s\" in c2d topic",
			NRF_CLOUD_JSON_VAL_TOPIC_C2D, TOPIC_VAL_RCV_WILDCARD);
		return true;
	}

	return false;
}

enum nrf_cloud_rcv_topic nrf_cloud_dc_rx_topic_decode(const char * const topic)
{
	if (!topic) {
		return NRF_CLOUD_RCV_TOPIC_UNKNOWN;
	}

	if (strstr(topic, TOPIC_VAL_RCV_AGPS)) {
		return NRF_CLOUD_RCV_TOPIC_AGPS;
	} else if (strstr(topic, TOPIC_VAL_RCV_PGPS)) {
		return NRF_CLOUD_RCV_TOPIC_PGPS;
	} else if (strstr(topic, TOPIC_VAL_RCV_GND_FIX)) {
		return NRF_CLOUD_RCV_TOPIC_LOCATION;
	} else if (strstr(topic, TOPIC_VAL_RCV_C2D)) {
		return NRF_CLOUD_RCV_TOPIC_GENERAL;
	} else {
		return NRF_CLOUD_RCV_TOPIC_UNKNOWN;
	}
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

	struct nct_dc_data msg = {
		.data.ptr = msg_string,
		.data.len = strlen(msg_string)
	};

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
#endif /* CONFIG_NRF_CLOUD_MQTT */

int nrf_cloud_pvt_data_encode(const struct nrf_cloud_gnss_pvt * const pvt,
			      cJSON * const pvt_data_obj)
{
	if (!pvt || !pvt_data_obj) {
		return -EINVAL;
	}

	if (json_add_num_cs(pvt_data_obj, NRF_CLOUD_JSON_GNSS_PVT_KEY_LON, pvt->lon) ||
	    json_add_num_cs(pvt_data_obj, NRF_CLOUD_JSON_GNSS_PVT_KEY_LAT, pvt->lat) ||
	    json_add_num_cs(pvt_data_obj, NRF_CLOUD_JSON_GNSS_PVT_KEY_ACCURACY, pvt->accuracy) ||
	    (pvt->has_alt &&
	     json_add_num_cs(pvt_data_obj, NRF_CLOUD_JSON_GNSS_PVT_KEY_ALTITUDE, pvt->alt)) ||
	    (pvt->has_speed &&
	     json_add_num_cs(pvt_data_obj, NRF_CLOUD_JSON_GNSS_PVT_KEY_SPEED, pvt->speed)) ||
	    (pvt->has_heading &&
	     json_add_num_cs(pvt_data_obj, NRF_CLOUD_JSON_GNSS_PVT_KEY_HEADING, pvt->heading))) {
		LOG_DBG("Failed to encode PVT data");
		return -ENOMEM;
	}

	return 0;
}

int nrf_cloud_encode_message(const char *app_id, double value, const char *str_val,
			     const char *topic, int64_t ts, struct nrf_cloud_data *output)
{
	int ret = 0;

	__ASSERT_NO_MSG(app_id != NULL);
	__ASSERT_NO_MSG(output != NULL);

	NRF_CLOUD_OBJ_JSON_DEFINE(root_obj);
	NRF_CLOUD_OBJ_JSON_DEFINE(msg_obj);

	/* Init the root object and add a topic string if provided */
	if (nrf_cloud_obj_init(&root_obj)){
		return -ENOMEM;
	}

	if (topic != NULL) {
		ret = nrf_cloud_obj_str_add(&root_obj, NRF_CLOUD_REST_TOPIC_KEY, topic, false);
	}

	/* Init the DATA message with provided app ID */
	ret += nrf_cloud_obj_msg_init(&msg_obj, app_id, NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA);

	ret += nrf_cloud_obj_ts_add(&msg_obj, ts);

	if (str_val != NULL) {
		ret += nrf_cloud_obj_str_add(&msg_obj, NRF_CLOUD_JSON_DATA_KEY, str_val, false);
	} else {
		ret += nrf_cloud_obj_num_add(&msg_obj, NRF_CLOUD_JSON_DATA_KEY, value, false);
	}

	/* Add the message to the root object */
	ret += nrf_cloud_obj_object_add(&root_obj, NRF_CLOUD_REST_MSG_KEY, &msg_obj, false);
	if (ret) {
		ret = -ENOMEM;
		goto cleanup;
	}

	/* msg_obj now belongs to root_obj */
	nrf_cloud_obj_reset(&msg_obj);

	/* Encode the root object for the cloud */
	ret = nrf_cloud_obj_cloud_encode(&root_obj);
	if (ret == 0) {
		*output = root_obj.encoded_data;
	}

cleanup:
	nrf_cloud_obj_free(&root_obj);
	nrf_cloud_obj_free(&msg_obj);
	return ret;
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

		if (json_add_null_cs(svc_inf_obj, NRF_CLOUD_JSON_KEY_SRVC_INFO_FOTA) != 0) {
			return -ENOMEM;
		}
	} else if (fota) {
		int item_cnt = 0;
		cJSON *array = cJSON_AddArrayToObjectCS(svc_inf_obj,
							NRF_CLOUD_JSON_KEY_SRVC_INFO_FOTA);

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
			cJSON_DeleteItemFromObject(svc_inf_obj, NRF_CLOUD_JSON_KEY_SRVC_INFO_FOTA);
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
		if (json_add_null_cs(svc_inf_obj, NRF_CLOUD_JSON_KEY_SRVC_INFO_UI) != 0) {
			return -ENOMEM;
		}
	} else {
		int item_cnt = 0;
		cJSON *array = cJSON_AddArrayToObjectCS(svc_inf_obj,
							NRF_CLOUD_JSON_KEY_SRVC_INFO_UI);

		if (!array) {
			return -ENOMEM;
		}
		if (ui->air_pressure) {
			cJSON_AddItemToArray(array,
				cJSON_CreateString(sensor_type_str[NRF_CLOUD_SENSOR_AIR_PRESS]));
			++item_cnt;
		}
		if (ui->gnss) {
			cJSON_AddItemToArray(array,
				cJSON_CreateString(sensor_type_str[NRF_CLOUD_SENSOR_GNSS]));
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
		if (ui->log) {
			cJSON_AddItemToArray(array,
				cJSON_CreateString(sensor_type_str[NRF_CLOUD_LOG]));
			++item_cnt;
		}
		if (ui->dictionary_log) {
			cJSON_AddItemToArray(array,
				cJSON_CreateString(sensor_type_str[NRF_CLOUD_DICTIONARY_LOG]));
			++item_cnt;
		}

		if (cJSON_GetArraySize(array) != item_cnt) {
			cJSON_DeleteItemFromObject(svc_inf_obj, NRF_CLOUD_JSON_KEY_SRVC_INFO_UI);
			return -ENOMEM;
		}
	}

	return 0;
}

static int encode_info_item_cs(const enum nrf_cloud_shadow_info inf, const char *const inf_name,
			    cJSON *const inf_obj, cJSON *const root_obj)
{
	cJSON *move_obj;

	switch (inf) {
	case NRF_CLOUD_INFO_SET:
		move_obj = cJSON_DetachItemFromObject(inf_obj, inf_name);

		if (!move_obj) {
			LOG_ERR("Info item \"%s\" not found", inf_name);
			return -ENOMSG;
		}

		if (json_add_obj_cs(root_obj, inf_name, move_obj)) {
			cJSON_Delete(move_obj);
			LOG_ERR("Failed to add info item \"%s\"", inf_name);
			return -ENOMEM;
		}
		break;
	case NRF_CLOUD_INFO_CLEAR:
		if (json_add_null_cs(root_obj, inf_name)) {
			LOG_ERR("Failed to create NULL item for \"%s\"", inf_name);
			return -ENOMEM;
		}
		break;
	case NRF_CLOUD_INFO_NO_CHANGE:
	default:
		break;
	}

	return 0;
}

#ifdef CONFIG_MODEM_INFO
static int init_modem_info(void)
{
	if (!modem_inf_initd) {
		int err;

		err = modem_info_init();
		if (err) {
			LOG_ERR("modem_info_init() failed: %d", err);
			return err;
		}

		err = modem_info_params_init(&modem_inf);
		if (err) {
			LOG_ERR("modem_info_params_init() failed: %d", err);
			return err;
		}

		modem_inf_initd = true;
	}
	return 0;
}

static int get_modem_info(void)
{
	int err = init_modem_info();

	if (err) {
		LOG_ERR("Could not initialize modem info module, error: %d", err);
		return err;
	}

	err = modem_info_params_get(&modem_inf);
	if (err) {
		LOG_ERR("Could not obtain information from modem, error: %d", err);
		return err;
	}

	return 0;
}

static int cell_info_json_encode(cJSON *const obj, const struct lte_lc_cell *const cell_inf)
{
	__ASSERT_NO_MSG(obj != NULL);
	__ASSERT_NO_MSG(cell_inf != NULL);

	if (json_add_num_cs(obj, NRF_CLOUD_JSON_MCC_KEY, cell_inf->mcc) ||
	    json_add_num_cs(obj, NRF_CLOUD_JSON_MNC_KEY, cell_inf->mnc) ||
	    json_add_num_cs(obj, NRF_CLOUD_JSON_AREA_CODE_KEY, cell_inf->tac) ||
	    json_add_num_cs(obj, NRF_CLOUD_JSON_CELL_ID_KEY, cell_inf->id) ||
	    json_add_num_cs(obj, NRF_CLOUD_JSON_RSRP_KEY, RSRP_IDX_TO_DBM(cell_inf->rsrp))) {
		return -ENOMEM;
	}

	return 0;
}

int nrf_cloud_cell_info_json_encode(cJSON *const data_obj, const struct lte_lc_cell *const cell_inf)
{
	__ASSERT_NO_MSG(data_obj != NULL);
	int err;
	struct lte_lc_cell cell;

	/* Get cell info if not provided */
	if (!cell_inf) {
		err = nrf_cloud_get_single_cell_modem_info(&cell);
		if (err) {
			return err;
		}
	}

	return cell_info_json_encode(data_obj, (cell_inf ? cell_inf : &cell));
}

int nrf_cloud_get_single_cell_modem_info(struct lte_lc_cell *const cell_inf)
{
	__ASSERT_NO_MSG(cell_inf != NULL);
	int err;

	err = get_modem_info();
	if (err) {
		return err;
	}

	cell_inf->mcc	= modem_inf.network.mcc.value;
	cell_inf->mnc	= modem_inf.network.mnc.value;
	cell_inf->tac	= modem_inf.network.area_code.value;
	cell_inf->id	= modem_inf.network.cellid_dec;
	cell_inf->rsrp	= modem_inf.network.rsrp.value;

	return 0;
}

static int add_modem_info_data(struct lte_param *param, cJSON *json_obj)
{
	char data_name[MODEM_INFO_MAX_RESPONSE_SIZE];
	enum at_param_type data_type;
	int ret;

	__ASSERT_NO_MSG(param != NULL);
	__ASSERT_NO_MSG(json_obj != NULL);

	memset(data_name, 0, ARRAY_SIZE(data_name));
	ret = modem_info_name_get(param->type, data_name);
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
		if (cJSON_AddStringToObject(json_obj, data_name, param->value_string) == NULL) {
			return -ENOMEM;
		}
	} else {
		if (cJSON_AddNumberToObject(json_obj, data_name, param->value) == NULL) {
			return -ENOMEM;
		}
	}

	return 0;
}

static int encode_modem_info_network(struct network_param *network, cJSON *json_obj)
{
	char network_mode[12] = {0};
	char data_name[MODEM_INFO_MAX_RESPONSE_SIZE] = {0};
	int ret;

	__ASSERT_NO_MSG(network != NULL);
	__ASSERT_NO_MSG(json_obj != NULL);

	ret = add_modem_info_data(&network->current_band, json_obj);
	if (ret) {
		return ret;
	}

	ret = add_modem_info_data(&network->sup_band, json_obj);
	if (ret) {
		return ret;
	}

	ret = add_modem_info_data(&network->area_code, json_obj);
	if (ret) {
		return ret;
	}

	ret = add_modem_info_data(&network->current_operator, json_obj);
	if (ret) {
		return ret;
	}

	ret = add_modem_info_data(&network->ip_address, json_obj);
	if (ret) {
		return ret;
	}

	ret = add_modem_info_data(&network->ue_mode, json_obj);
	if (ret) {
		return ret;
	}

	ret = modem_info_name_get(network->cellid_hex.type, data_name);
	if (ret < 0) {
		return ret;
	}

	if (cJSON_AddNumberToObject(json_obj, data_name, network->cellid_dec) == NULL) {
		return -EINVAL;
	}

	if (network->lte_mode.value == 1) {
		strcat(network_mode, "LTE-M");
	} else if (network->nbiot_mode.value == 1) {
		strcat(network_mode, "NB-IoT");
	}
	if (network->gps_mode.value == 1) {
		strcat(network_mode, " GPS");
	}

	if (cJSON_AddStringToObject(json_obj, "networkMode", network_mode) == NULL) {
		return -EINVAL;
	}

	return 0;
}

static int encode_modem_info_sim(struct sim_param *sim, cJSON *json_obj)
{
	int ret;

	__ASSERT_NO_MSG(sim != NULL);
	__ASSERT_NO_MSG(json_obj != NULL);

	ret = add_modem_info_data(&sim->uicc, json_obj);
	if (ret) {
		return ret;
	}

	ret = add_modem_info_data(&sim->iccid, json_obj);
	if (ret) {
		LOG_DBG("sim_param object does not contain an ICCID");
	}

	ret = add_modem_info_data(&sim->imsi, json_obj);
	if (ret) {
		LOG_DBG("sim_param object does not contain an IMSI");
	}

	return 0;
}

static int encode_modem_info_device(struct device_param *device, cJSON *json_obj)
{
	__ASSERT_NO_MSG(device != NULL);
	__ASSERT_NO_MSG(json_obj != NULL);

	int ret;
	char hw_ver[40] = {0};
#ifdef BUILD_VERSION
	const char * const zver = STRINGIFY(BUILD_VERSION);
#else
	const char * const zver = "N/A"
#endif

	ret = add_modem_info_data(&device->modem_fw, json_obj);
	if (ret) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_NRF_CLOUD_DEVICE_STATUS_ENCODE_VOLTAGE)) {
		ret = add_modem_info_data(&device->battery, json_obj);
		if (ret) {
			return ret;
		}
	}

	ret = add_modem_info_data(&device->imei, json_obj);
	if (ret) {
		return ret;
	}

	if (json_add_str_cs(json_obj, "board", device->board)) {
		return -ENOMEM;
	}

	if (json_add_str_cs(json_obj, "sdkVer", device->app_version)) {
		return -ENOMEM;
	}

	if (json_add_str_cs(json_obj, "appName", device->app_name)) {
		return -ENOMEM;
	}

	if (json_add_str_cs(json_obj, "zephyrVer", zver)) {
		return -ENOMEM;
	}

	ret = modem_info_get_hw_version(hw_ver, sizeof(hw_ver) - 1);
	if (json_add_str_cs(json_obj, "hwVer", ((ret == 0) ? hw_ver : "N/A"))) {
		return -ENOMEM;
	}

	return 0;
}

static int encode_modem_info_json_object(struct modem_param_info *modem, cJSON *root_obj,
	const char * const app_ver)
{
	int ret;

	__ASSERT_NO_MSG(root_obj != NULL);
	__ASSERT_NO_MSG(modem != NULL);

	if (IS_ENABLED(CONFIG_MODEM_INFO_ADD_NETWORK)) {
		cJSON *network_obj = cJSON_CreateObject();

		if (network_obj == NULL) {
			return -ENOMEM;
		}
		ret = encode_modem_info_network(&modem->network, network_obj);
		if (!ret) {
			ret = json_add_obj_cs(root_obj,
					      NRF_CLOUD_DEVICE_JSON_KEY_NET_INF, network_obj);
			if (ret) {
				cJSON_Delete(network_obj);
				return -ENOMEM;
			}
		} else {
			cJSON_Delete(network_obj);
		}
	}

	if (IS_ENABLED(CONFIG_MODEM_INFO_ADD_SIM)) {
		cJSON *sim_obj = cJSON_CreateObject();

		if (sim_obj == NULL) {
			return -ENOMEM;
		}
		ret = encode_modem_info_sim(&modem->sim, sim_obj);
		if (!ret) {
			ret = json_add_obj_cs(root_obj,
					      NRF_CLOUD_DEVICE_JSON_KEY_SIM_INF, sim_obj);
			if (ret) {
				cJSON_Delete(sim_obj);
				return -ENOMEM;
			}
		} else {
			cJSON_Delete(sim_obj);
		}
	}

	if (IS_ENABLED(CONFIG_MODEM_INFO_ADD_DEVICE)) {
		cJSON *device_obj = cJSON_CreateObject();

		if (device_obj == NULL) {
			return -ENOMEM;
		}

		if (app_ver) {
			ret = json_add_str_cs(device_obj, NRF_CLOUD_JSON_KEY_APP_VER, app_ver);
		}

		if (ret) {
			cJSON_Delete(device_obj);
			return -ENOMEM;
		}

		ret = encode_modem_info_device(&modem->device, device_obj);
		if (!ret) {
			ret = json_add_obj_cs(root_obj,
					      NRF_CLOUD_DEVICE_JSON_KEY_DEV_INF, device_obj);
			if (ret) {
				cJSON_Delete(device_obj);
				return -ENOMEM;
			}
		} else {
			cJSON_Delete(device_obj);
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

	if ((!IS_ENABLED(CONFIG_MODEM_INFO_ADD_DEVICE)) &&
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

	struct modem_param_info *mpi = (struct modem_param_info *)mod_inf->mpi;

	if (!mpi) {
		/* No modem info provided, use local */
		err = get_modem_info();
		if (err < 0) {
			LOG_ERR("modem_info_params_get() failed: %d", err);
			goto cleanup;
		}
		mpi = &modem_inf;
	}

	err = encode_modem_info_json_object(mpi, tmp, mod_inf->application_version);
	if (err) {
		LOG_ERR("Failed to encode modem info: %d", err);
		goto cleanup;
	}

	if (encode_info_item_cs(mod_inf->device,
				NRF_CLOUD_DEVICE_JSON_KEY_DEV_INF, tmp, mod_inf_obj) ||
	    encode_info_item_cs(mod_inf->network,
				NRF_CLOUD_DEVICE_JSON_KEY_NET_INF, tmp, mod_inf_obj) ||
	    encode_info_item_cs(mod_inf->sim,
				NRF_CLOUD_DEVICE_JSON_KEY_SIM_INF, tmp, mod_inf_obj)) {
		LOG_ERR("Failed to encode modem info");
		err = -EIO;
		goto cleanup;
	}

cleanup:
	cJSON_Delete(tmp);
	return err;
}
#else
int nrf_cloud_modem_info_json_encode(const struct nrf_cloud_modem_info *const mod_inf,
				     cJSON *const mod_inf_obj)
{
	cJSON *tmp = cJSON_CreateObject();

	if (encode_info_item_cs(mod_inf->device,
				NRF_CLOUD_DEVICE_JSON_KEY_DEV_INF, tmp, mod_inf_obj) ||
	    encode_info_item_cs(mod_inf->network,
				NRF_CLOUD_DEVICE_JSON_KEY_NET_INF, tmp, mod_inf_obj) ||
	    encode_info_item_cs(mod_inf->sim,
				NRF_CLOUD_DEVICE_JSON_KEY_SIM_INF, tmp, mod_inf_obj)) {
		LOG_ERR("Failed to encode modem info");
		cJSON_Delete(tmp);
		return -EIO;
	}
	cJSON_Delete(tmp);
	return 0;
}
#endif /* CONFIG_MODEM_INFO */

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

int nrf_cloud_shadow_dev_status_encode(const struct nrf_cloud_device_status *const dev_status,
	struct nrf_cloud_data * const output, const bool include_state, const bool include_reported)
{
	if (!dev_status || !output || (include_state && !include_reported)) {
		return -EINVAL;
	}

	int err = 0;
	cJSON *state_obj = NULL;
	cJSON *reported_obj = NULL;
	cJSON *root_obj = cJSON_CreateObject();

	if (!include_reported) {
		reported_obj = root_obj;
	} else if (include_state) {
		state_obj = cJSON_AddObjectToObjectCS(root_obj, NRF_CLOUD_JSON_KEY_STATE);
		reported_obj = cJSON_AddObjectToObjectCS(state_obj, NRF_CLOUD_JSON_KEY_REP);
	} else {
		reported_obj = cJSON_AddObjectToObjectCS(root_obj, NRF_CLOUD_JSON_KEY_REP);
	}

	cJSON *device_obj = cJSON_AddObjectToObjectCS(reported_obj, NRF_CLOUD_JSON_KEY_DEVICE);

	if (device_obj == NULL) {
		err = -ENOMEM;
		goto cleanup;
	}

	err = info_encode(device_obj, dev_status->modem, dev_status->svc);
	if (err) {
		goto cleanup;
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

int nrf_cloud_shadow_data_encode(const struct nrf_cloud_sensor_data *sensor,
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
	cJSON *state_obj = cJSON_AddObjectToObjectCS(root_obj, NRF_CLOUD_JSON_KEY_STATE);
	cJSON *reported_obj = cJSON_AddObjectToObjectCS(state_obj, NRF_CLOUD_JSON_KEY_REP);
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

int nrf_cloud_dev_status_json_encode(const struct nrf_cloud_device_status *const dev_status,
	const int64_t timestamp, cJSON * const msg_obj_out)
{
	if (!dev_status || !msg_obj_out) {
		return -EINVAL;
	}

	int err = 0;
	cJSON *data_obj = cJSON_AddObjectToObject(msg_obj_out, NRF_CLOUD_JSON_DATA_KEY);

	if (!data_obj) {
		return -ENOMEM;
	}

	if (json_add_str_cs(msg_obj_out, NRF_CLOUD_JSON_APPID_KEY,
			    NRF_CLOUD_JSON_APPID_VAL_DEVICE) ||
	    json_add_str_cs(msg_obj_out, NRF_CLOUD_JSON_MSG_TYPE_KEY,
			    NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA)) {
		return -ENOMEM;
	}

	if (timestamp > 0) {
		if (json_add_num_cs(msg_obj_out, NRF_CLOUD_MSG_TIMESTAMP_KEY, timestamp)) {
			return -ENOMEM;
		}
	}

	err = info_encode(data_obj, dev_status->modem, dev_status->svc);

	return err;
}

void nrf_cloud_fota_job_free(struct nrf_cloud_fota_job_info *const job)
{
	if (!job) {
		return;
	}

	nrf_cloud_free(job->id);
	job->id = NULL;

	nrf_cloud_free(job->host);
	job->host = NULL;

	nrf_cloud_free(job->path);
	job->path = NULL;
}

/* FOTA job format via MQTT:
 * ["jobExecutionId",firmwareType,fileSize,"host","path"]
 * ["BLE ID","jobExecutionId",firmwareType,fileSize,"host","path"]
 *
 * Examples:
 * ["abcd1234",0,1234,"nrfcloud.com","v1/firmwares/appfw.bin"]
 * ["12:AB:34:CD:56:EF","efgh5678",0,321,"nrfcloud.com", "v1/firmwares/ble.bin"]
 */
enum rcv_item_idx {
	RCV_ITEM_IDX_BLE_ID,
	RCV_ITEM_IDX_JOB_ID,
	RCV_ITEM_IDX_FW_TYPE,
	RCV_ITEM_IDX_FILE_SIZE,
	RCV_ITEM_IDX_FILE_HOST,
	RCV_ITEM_IDX_FILE_PATH,
	RCV_ITEM_IDX__SIZE,
};

int nrf_cloud_fota_job_decode(struct nrf_cloud_fota_job_info *const job_info,
			      bt_addr_t *const ble_id,
			      const struct nrf_cloud_data *const input)
{
	if (!job_info || !input || (!input->ptr)) {
		return -EINVAL;
	}

	int err = -ENOMSG;
	size_t job_id_len;
	int offset = !ble_id ? 1 : 0;
	cJSON *array = cJSON_Parse(input->ptr);

	if (!array || !cJSON_IsArray(array)) {
		LOG_ERR("Invalid JSON array");
		err = -EINVAL;
		goto cleanup;
	}

	if (IS_ENABLED(CONFIG_NRF_CLOUD_LOG_LEVEL_DGB)) {
		char *temp = cJSON_PrintUnformatted(array);

		if (temp) {
			LOG_DBG("JSON array: %s", temp);
			cJSON_FreeString(temp);
		}
	}

	memset(job_info, 0, sizeof(*job_info));

	/* Get the job ID separately, it may be needed to reject an invalid job */
	job_info->id = json_strdup(cJSON_GetArrayItem(array, RCV_ITEM_IDX_JOB_ID - offset));
	if (job_info->id == NULL) {
		LOG_ERR("FOTA job ID not found");
		goto cleanup;
	}

	/* Check that the job ID is a valid size */
	job_id_len = strlen(job_info->id);
	if (job_id_len > (NRF_CLOUD_FOTA_JOB_ID_SIZE - 1)) {
		LOG_ERR("Job ID length: %d, exceeds allowed length: %d",
			job_id_len, NRF_CLOUD_FOTA_JOB_ID_SIZE - 1);
		goto cleanup;
	}

#if CONFIG_NRF_CLOUD_FOTA_BLE_DEVICES
	if (ble_id) {
		char *ble_str;

		/* Get the BLE ID string and copy to bt_addr_t structure */
		if (json_array_str_get(array, RCV_ITEM_IDX_BLE_ID,
					  &ble_str)) {
			LOG_ERR("Failed to get BLE ID from job");
			goto cleanup;
		}

		if (bt_addr_from_str(ble_str, ble_id)) {
			err = -EADDRNOTAVAIL;
			LOG_ERR("Invalid BLE ID: %s", ble_str);
			goto cleanup;
		}
	}
#endif

	/* Get and allocate host and path strings */
	job_info->host = json_strdup(cJSON_GetArrayItem(array, RCV_ITEM_IDX_FILE_HOST - offset));
	job_info->path = json_strdup(cJSON_GetArrayItem(array, RCV_ITEM_IDX_FILE_PATH - offset));

	/* Get type and file size */
	if ((job_info->host == NULL) || (job_info->path == NULL) ||
	    json_array_num_get(array, RCV_ITEM_IDX_FW_TYPE - offset, (int *)&job_info->type) ||
	    json_array_num_get(array, RCV_ITEM_IDX_FILE_SIZE - offset, &job_info->file_size)) {
		LOG_ERR("Error parsing job info");
		goto cleanup;
	}

	/* Check that the FOTA type is valid */
	if (job_info->type < NRF_CLOUD_FOTA_TYPE__FIRST ||
	    job_info->type >= NRF_CLOUD_FOTA_TYPE__INVALID) {
		LOG_ERR("Invalid FOTA type: %d", job_info->type);
		goto cleanup;
	}

	/* Success */
	err = 0;

cleanup:
	if (array) {
		cJSON_Delete(array);
	}

	if (err) {
		/* On error, leave the job ID so that the job can be cancelled */
		nrf_cloud_free(job_info->host);
		job_info->host = NULL;
		nrf_cloud_free(job_info->path);
		job_info->path = NULL;

		job_info->type = NRF_CLOUD_FOTA_TYPE__INVALID;
	}

	return err;
}

static const char * const get_error_string(const enum nrf_cloud_fota_error err)
{
	switch (err) {
	case NRF_CLOUD_FOTA_ERROR_DOWNLOAD_START:
		return "Failed to start download";
	case NRF_CLOUD_FOTA_ERROR_DOWNLOAD:
		return "Error during download";
	case NRF_CLOUD_FOTA_ERROR_UNABLE_TO_VALIDATE:
		return "FOTA update not validated";
	case NRF_CLOUD_FOTA_ERROR_APPLY_FAIL:
		return "Error applying update";
	case NRF_CLOUD_FOTA_ERROR_MISMATCH:
		return "FW file does not match specified FOTA type";
	case NRF_CLOUD_FOTA_ERROR_BAD_JOB_INFO:
		return "Job info is invalid";
	case NRF_CLOUD_FOTA_ERROR_BAD_TYPE:
		return "FOTA type not supported";
	case NRF_CLOUD_FOTA_ERROR_NONE:
		__fallthrough;
	default:
		return "";
	}
}

int nrf_cloud_obj_fota_job_update_create(struct nrf_cloud_obj *const obj,
					 const struct nrf_cloud_fota_job * const job)
{
	if (!job || !obj) {
		return -EINVAL;
	}
	if (!NRF_CLOUD_OBJ_TYPE_VALID(obj)) {
		return -EBADF;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		bool result;

		obj->json = cJSON_CreateArray();
		if (!obj->json) {
			LOG_ERR("Failed to create JSON array");
			return -ENOMEM;
		}

		result = (json_array_str_add(obj->json, job->info.id) == 0) &&
			 (json_array_num_add(obj->json, job->status) == 0);

		if (job->status == NRF_CLOUD_FOTA_DOWNLOADING) {
			result &= (json_array_num_add(obj->json, job->dl_progress) == 0);
		} else {
			result &= (json_array_str_add(obj->json,
						      get_error_string(job->error)) == 0);
		}

		if (!result) {
			LOG_ERR("Failed to update JSON array");
			(void)nrf_cloud_obj_free(obj);
			return -ENOMEM;
		}

		break;
	}
	default:
		return -ENOTSUP;
	}

	return 0;
}

#if defined(CONFIG_NRF_CLOUD_FOTA_BLE_DEVICES)
int nrf_cloud_obj_fota_ble_job_request_create(struct nrf_cloud_obj *const obj,
					       const bt_addr_t *const ble_id)
{
	if (!ble_id || !obj) {
		return -EINVAL;
	}
	if (!NRF_CLOUD_OBJ_TYPE_VALID(obj)) {
		return -EBADF;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		char ble_id_str[BT_ADDR_LE_STR_LEN];
		int ret;

		ret = bt_addr_to_str(ble_id, ble_id_str, BT_ADDR_LE_STR_LEN);
		if (ret < 0 || ret >= BT_ADDR_LE_STR_LEN) {
			return -EADDRNOTAVAIL;
		}

		obj->json = cJSON_CreateArray();
		if (!obj->json) {
			return -ENOMEM;
		}

		if (json_array_str_add(obj->json, ble_id_str) != 0) {
			nrf_cloud_obj_free(obj);
			return -ENOMEM;
		}

		break;
	}
	default:
		return -ENOTSUP;
	}

	return 0;
}

int nrf_cloud_obj_fota_ble_job_update_create(struct nrf_cloud_obj *const obj,
					     const struct nrf_cloud_fota_ble_job *const ble_job,
					     const enum nrf_cloud_fota_status status)
{
	if (!ble_job || !obj) {
		return -EINVAL;
	}
	if (!NRF_CLOUD_OBJ_TYPE_VALID(obj)) {
		return -EBADF;
	}

	switch (obj->type) {
	case NRF_CLOUD_OBJ_TYPE_JSON:
	{
		char ble_id_str[BT_ADDR_LE_STR_LEN];
		bool result;
		int ret;

		obj->json = cJSON_CreateArray();
		if (!obj->json) {
			return -ENOMEM;
		}

		ret = bt_addr_to_str(&ble_job->ble_id, ble_id_str, BT_ADDR_LE_STR_LEN);
		if (ret < 0 || ret >= BT_ADDR_LE_STR_LEN) {
			return -EADDRNOTAVAIL;
		}

		result = (json_array_str_add(obj->json, ble_id_str) == 0) &&
			 (json_array_str_add(obj->json, ble_job->info.id) == 0) &&
			 (json_array_num_add(obj->json, status) == 0);

		if (status == NRF_CLOUD_FOTA_DOWNLOADING) {
			result &= (json_array_num_add(obj->json, ble_job->dl_progress) == 0);
		} else {
			result &= (json_array_str_add(obj->json,
						      get_error_string(ble_job->error)) == 0);
		}

		if (!result) {
			(void)nrf_cloud_obj_free(obj);
			return -ENOMEM;
		}

		break;
	}
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

int nrf_cloud_rest_fota_execution_decode(const char *const response,
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
		ret = -EPROTO;
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
		LOG_WRN("Unhandled FOTA type: %s", type);
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

int json_array_str_get(const cJSON *const array, const int index, char **string_out)
{
	__ASSERT_NO_MSG(string_out != NULL);

	if (!array) {
		return -ENOENT;
	}

	cJSON *item = cJSON_GetArrayItem(array, index);

	if (!cJSON_IsString(item)) {
		return item ? -ENOMSG : -ENODEV;
	}

	*string_out = item->valuestring;

	return 0;
}

int json_array_num_get(const cJSON *const array, const int index, int *number_out)
{
	__ASSERT_NO_MSG(number_out != NULL);

	cJSON *item = cJSON_GetArrayItem(array, index);

	if (!cJSON_IsNumber(item)) {
		return -EINVAL;
	}

	*number_out = item->valueint;

	return 0;
}

int json_array_str_add(cJSON *const array, const char *const string)
{
	__ASSERT_NO_MSG(array != NULL);

	cJSON *item = cJSON_CreateString(string);

	if (item) {
		return cJSON_AddItemToArray(array, item) ? 0 : -EIO;
	}

	return -ENOMEM;
}

int json_array_num_add(cJSON *const array, const int number)
{
	__ASSERT_NO_MSG(array != NULL);

	cJSON *item = cJSON_CreateNumber(number);

	if (item) {
		return cJSON_AddItemToArray(array, item) ? 0 : -EIO;
	}

	return -ENOMEM;
}

int get_string_from_obj(const cJSON *const obj, const char *const key,
			char **string_out)
{
	__ASSERT_NO_MSG(string_out != NULL);

	if (!obj) {
		return -ENOENT;
	}

	cJSON *item = cJSON_GetObjectItem(obj, key);

	if (!cJSON_IsString(item)) {
		return item ? -ENOMSG : -ENODEV;
	}

	*string_out = item->valuestring;

	return 0;
}

int get_num_from_obj(const cJSON *const obj, const char *const key,
		     double *num_out)
{
	__ASSERT_NO_MSG(num_out != NULL);

	if (!obj) {
		return -ENOENT;
	}

	cJSON *item = cJSON_GetObjectItem(obj, key);

	if (!cJSON_IsNumber(item)) {
		return item ? -ENOMSG : -ENODEV;
	}

	*num_out = item->valuedouble;

	return 0;
}

static int add_ncells(cJSON * const lte_obj, const uint8_t ncells_count,
	const struct lte_lc_ncell *const neighbor_cells)
{
	if (!lte_obj) {
		return -EINVAL;
	}

	if (!ncells_count || !neighbor_cells) {
		return -ENODATA;
	}

	cJSON * nmr_array = cJSON_AddArrayToObjectCS(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_NBORS);

	if (!nmr_array) {
		return -ENOMEM;
	}

	for (uint8_t i = 0; i < ncells_count; ++i) {
		const struct lte_lc_ncell *ncell = neighbor_cells + i;
		cJSON *ncell_obj = cJSON_CreateObject();

		if (!ncell_obj) {
			return -ENOMEM;
		}

		if (!cJSON_AddItemToArray(nmr_array, ncell_obj)) {
			cJSON_Delete(ncell_obj);
			return -ENOMEM;
		}

		/* Required parameters for the API call */
		if (json_add_num_cs(ncell_obj, NRF_CLOUD_CELL_POS_JSON_KEY_EARFCN,
					ncell->earfcn) ||
			json_add_num_cs(ncell_obj, NRF_CLOUD_CELL_POS_JSON_KEY_PCI,
					ncell->phys_cell_id)) {
			return -ENOMEM;
		}

		/* Optional parameters for the API call */
		if ((ncell->rsrp != NRF_CLOUD_LOCATION_CELL_OMIT_RSRP) &&
			json_add_num_cs(ncell_obj, NRF_CLOUD_CELL_POS_JSON_KEY_RSRP,
					RSRP_IDX_TO_DBM(ncell->rsrp))) {
			return -ENOMEM;
		}
		if ((ncell->rsrq != NRF_CLOUD_LOCATION_CELL_OMIT_RSRQ) &&
			json_add_num_cs(ncell_obj, NRF_CLOUD_CELL_POS_JSON_KEY_RSRQ,
					RSRQ_IDX_TO_DB(ncell->rsrq))) {
			return -ENOMEM;
		}
		if ((ncell->time_diff != LTE_LC_CELL_TIME_DIFF_INVALID) &&
			json_add_num_cs(ncell_obj, NRF_CLOUD_CELL_POS_JSON_KEY_TDIFF,
					ncell->time_diff)) {
			return -ENOMEM;
		}
	}

	return 0;
}

static cJSON *add_lte_inf(cJSON *const lte_array, struct lte_lc_cell const *const inf)
{
	cJSON *lte_obj = cJSON_CreateObject();

	if (!lte_obj) {
		return NULL;
	}

	if (!cJSON_AddItemToArray(lte_array, lte_obj)) {
		cJSON_Delete(lte_obj);
		return NULL;
	}

	/* Required parameters for the API call */
	if (json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_ECI, inf->id) ||
	    json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_MCC, inf->mcc) ||
	    json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_MNC, inf->mnc) ||
	    json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_TAC, inf->tac)) {
		return NULL;
	}

	/* Optional parameters for the API call */
	if ((inf->earfcn != NRF_CLOUD_LOCATION_CELL_OMIT_EARFCN) &&
	    json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_EARFCN, inf->earfcn)) {
		return NULL;
	}

	if ((inf->rsrp != NRF_CLOUD_LOCATION_CELL_OMIT_RSRP) &&
	    json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_RSRP,
				RSRP_IDX_TO_DBM(inf->rsrp))) {
		return NULL;
	}

	if ((inf->rsrq != NRF_CLOUD_LOCATION_CELL_OMIT_RSRQ) &&
	    json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_RSRQ,
				RSRQ_IDX_TO_DB(inf->rsrq))) {
		return NULL;
	}

	if (inf->timing_advance != NRF_CLOUD_LOCATION_CELL_OMIT_TIME_ADV) {
		uint16_t t_adv = inf->timing_advance;

		if (t_adv > NRF_CLOUD_LOCATION_CELL_TIME_ADV_MAX) {
			t_adv = NRF_CLOUD_LOCATION_CELL_TIME_ADV_MAX;
		}

		if (json_add_num_cs(lte_obj, NRF_CLOUD_CELL_POS_JSON_KEY_T_ADV, t_adv)) {
			return NULL;
		}
	}

	return lte_obj;
}

int nrf_cloud_cell_pos_req_json_encode(struct lte_lc_cells_info const *const inf,
	cJSON *const req_obj_out)
{
	if (!inf || !req_obj_out) {
		return -EINVAL;
	}

	LOG_DBG("Encoding lte_lc_cells_info with ncells_count: %u and gci_cells_count: %u",
		inf->ncells_count, inf->gci_cells_count);

	int err;
	cJSON *lte_array;
	cJSON *lte_obj;

	lte_array = cJSON_AddArrayToObjectCS(req_obj_out, NRF_CLOUD_CELL_POS_JSON_KEY_LTE);
	if (!lte_array) {
		err = -ENOMEM;
		goto cleanup;
	}

	/* Add the current cell to the array; if using a GCI search type, sometimes
	 * there is no current cell.
	 */
	if (inf->current_cell.id != LTE_LC_CELL_EUTRAN_ID_INVALID) {
		lte_obj = add_lte_inf(lte_array, &inf->current_cell);
		if (!lte_obj) {
			err = -ENOMEM;
			goto cleanup;
		}

		/* Add neighbor cells if present */
		err = add_ncells(lte_obj, inf->ncells_count, inf->neighbor_cells);
		if ((err == -EINVAL) || (err == -ENOMEM)) {
			goto cleanup;
		}
	}

	/* Skip GCI cells if not present */
	if (!inf->gci_cells_count || !inf->gci_cells) {
		if (inf->current_cell.id == LTE_LC_CELL_EUTRAN_ID_INVALID) {
			err = -ENODATA;
			goto cleanup;
		} else {
			return 0;
		}
	}

	/* Add GCI cells */
	for (uint8_t i = 0; i < inf->gci_cells_count; ++i) {
		const struct lte_lc_cell *gci = inf->gci_cells + i;

		lte_obj = add_lte_inf(lte_array, gci);
		if (!lte_obj) {
			err = -ENOMEM;
			goto cleanup;
		}
	}

	return 0;

cleanup:
	/* Only need to delete the lte_array since all items (if any) were added to it */
	cJSON_DeleteItemFromObject(req_obj_out, NRF_CLOUD_CELL_POS_JSON_KEY_LTE);
	LOG_ERR("Failed to format location request: %d", err);
	return err;
}

int nrf_cloud_wifi_req_json_encode(struct wifi_scan_info const *const wifi,
	cJSON *const req_obj_out)
{
	if (!wifi || !req_obj_out || !wifi->ap_info || !wifi->cnt) {
		return -EINVAL;
	}

	LOG_DBG("Encoding wifi_scan_info with count: %u", wifi->cnt);

	cJSON *wifi_obj = NULL;
	cJSON *ap_array = NULL;

	wifi_obj = cJSON_AddObjectToObjectCS(req_obj_out, NRF_CLOUD_LOCATION_JSON_KEY_WIFI);
	ap_array = cJSON_AddArrayToObjectCS(wifi_obj, NRF_CLOUD_LOCATION_JSON_KEY_APS);
	if (!ap_array) {
		goto cleanup;
	}

	for (uint8_t cnt = 0; cnt < wifi->cnt; ++cnt) {
		char str_buf[MAX(WIFI_MAC_ADDR_STR_LEN, WIFI_SSID_MAX_LEN) + 1];
		struct wifi_scan_result const *const ap = (wifi->ap_info + cnt);
		cJSON *ap_obj = cJSON_CreateObject();
		int ret;

		if (!cJSON_AddItemToArray(ap_array, ap_obj)) {
			cJSON_Delete(ap_obj);
			goto cleanup;
		}

		/* MAC address is the only required parameter for the API call */
		ret = snprintk(str_buf, sizeof(str_buf),
			       WIFI_MAC_ADDR_TEMPLATE,
			       ap->mac[0], ap->mac[1], ap->mac[2],
			       ap->mac[3], ap->mac[4], ap->mac[5]);
		if ((ret != WIFI_MAC_ADDR_STR_LEN) ||
		    json_add_str_cs(ap_obj, NRF_CLOUD_LOCATION_JSON_KEY_WIFI_MAC, str_buf)) {
			goto cleanup;
		}

		/* Optional parameters for the API call */
		memset(str_buf, 0, sizeof(str_buf));
		if ((ap->ssid_length > 0) && (ap->ssid_length <= WIFI_SSID_MAX_LEN)) {
			memcpy(str_buf, ap->ssid, ap->ssid_length);
		}

		if ((str_buf[0] != '\0') &&
		    json_add_str_cs(ap_obj, NRF_CLOUD_LOCATION_JSON_KEY_WIFI_SSID, str_buf)) {
			goto cleanup;
		}

		if ((ap->rssi != NRF_CLOUD_LOCATION_WIFI_OMIT_RSSI) &&
		    json_add_num_cs(ap_obj, NRF_CLOUD_LOCATION_JSON_KEY_WIFI_RSSI,
					ap->rssi)) {
			goto cleanup;
		}

		if ((ap->channel != NRF_CLOUD_LOCATION_WIFI_OMIT_CHAN) &&
		    json_add_num_cs(ap_obj, NRF_CLOUD_LOCATION_JSON_KEY_WIFI_CH,
					ap->channel)) {
			goto cleanup;
		}
	}

	return 0;

cleanup:
	/* Only need to delete the WiFi object since all items (if any) were added to it */
	cJSON_DeleteItemFromObject(req_obj_out, NRF_CLOUD_LOCATION_JSON_KEY_WIFI);
	LOG_ERR("Failed to format WiFi location request, out of memory");
	return -ENOMEM;
}

int nrf_cloud_location_req_json_encode(struct lte_lc_cells_info const *const cell_info,
	struct wifi_scan_info const *const wifi_info, char **string_out)
{
	if ((!cell_info && !wifi_info) || !string_out) {
		return -EINVAL;
	}

	int err = 0;
	cJSON *req_obj = cJSON_CreateObject();

	if (cell_info) {
		err = nrf_cloud_cell_pos_req_json_encode(cell_info, req_obj);
		if (err) {
			goto cleanup;
		}
	}

	if (wifi_info) {
		err = nrf_cloud_wifi_req_json_encode(wifi_info, req_obj);
		if (err) {
			goto cleanup;
		}
	}

	*string_out = cJSON_PrintUnformatted(req_obj);
	if (*string_out == NULL) {
		err = -ENOMEM;
	} else {
		LOG_DBG("JSON: %s", *string_out);
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

static int nrf_cloud_parse_location_json(const cJSON *const loc_obj,
	struct nrf_cloud_location_result *const location_out)
{
	if (!loc_obj || !location_out) {
		return -EINVAL;
	}

	cJSON *lat, *lon, *unc;
	char *type;

	lat = cJSON_GetObjectItem(loc_obj,
				  NRF_CLOUD_LOCATION_JSON_KEY_LAT);
	lon = cJSON_GetObjectItem(loc_obj,
				  NRF_CLOUD_LOCATION_JSON_KEY_LON);
	unc = cJSON_GetObjectItem(loc_obj,
				  NRF_CLOUD_LOCATION_JSON_KEY_UNCERT);

	if (!cJSON_IsNumber(lat) || !cJSON_IsNumber(lon) || !cJSON_IsNumber(unc)) {
		return -EBADMSG;
	}

	location_out->lat = lat->valuedouble;
	location_out->lon = lon->valuedouble;
	location_out->unc = (uint32_t)unc->valueint;

	location_out->type = LOCATION_TYPE__INVALID;

	if (!get_string_from_obj(loc_obj, NRF_CLOUD_JSON_FULFILL_KEY, &type)) {
		if (!strcmp(type, NRF_CLOUD_LOCATION_TYPE_VAL_MCELL)) {
			location_out->type = LOCATION_TYPE_MULTI_CELL;
		} else if (!strcmp(type, NRF_CLOUD_LOCATION_TYPE_VAL_SCELL)) {
			location_out->type = LOCATION_TYPE_SINGLE_CELL;
		} else if (!strcmp(type, NRF_CLOUD_LOCATION_TYPE_VAL_WIFI)) {
			location_out->type = LOCATION_TYPE_WIFI;
		} else {
			LOG_WRN("Unhandled location type: %s", type);
		}
	} else {
		LOG_WRN("Location type not found in message");
	}

	return 0;
}

int nrf_cloud_error_msg_decode(const char *const buf,
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
		/* No JSON found, not an error message */
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

int nrf_cloud_location_response_decode(const char *const buf,
				       struct nrf_cloud_location_result *result)
{
	int ret;
	cJSON *loc_obj;
	cJSON *data_obj;

	if ((buf == NULL) || (result == NULL)) {
		return -EINVAL;
	}

	loc_obj = cJSON_Parse(buf);
	if (!loc_obj) {
		LOG_DBG("No JSON found for location");
		return 1;
	}

	/* First, check to see if this is a REST payload, which is not wrapped in
	 * an nRF Cloud MQTT message
	 */
	ret = nrf_cloud_parse_location_json(loc_obj, result);
	if (ret == 0) {
		goto cleanup;
	}

	/* Clear the error flag and check for MQTT payload format */
	result->err = NRF_CLOUD_ERROR_NONE;
	ret = 1;

	/* Check for nRF Cloud MQTT message; valid appId and msgType */
	if (!json_item_string_exists(loc_obj, NRF_CLOUD_JSON_MSG_TYPE_KEY,
				     NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA) ||
	    !json_item_string_exists(loc_obj, NRF_CLOUD_JSON_APPID_KEY,
				     NRF_CLOUD_JSON_APPID_VAL_LOCATION)) {
		/* Not a location data message */
		goto cleanup;
	}

	/* MQTT payload format found, parse the data */
	data_obj = cJSON_GetObjectItem(loc_obj, NRF_CLOUD_JSON_DATA_KEY);
	if (data_obj) {
		ret = nrf_cloud_parse_location_json(data_obj, result);
		if (ret) {
			LOG_ERR("Failed to parse location data");
		}
		/* A message with "data" should not also contain an error code */
		goto cleanup;
	}

	/* Check for error code */
	ret = get_error_code_value(loc_obj, &result->err);
	if (ret) {
		/* Indicate that an nRF Cloud error code was found */
		ret = -EFAULT;
	} else {
		/* No data or error was found */
		LOG_ERR("Expected data not found in location message");
		ret = -EBADMSG;
	}

cleanup:
	cJSON_Delete(loc_obj);

	if (ret < 0) {
		/* Clear data on error */
		result->lat = 0.0;
		result->lon = 0.0;
		result->unc = 0;
		result->type = LOCATION_TYPE__INVALID;

		/* Set to unknown error if an error code was not found */
		if (result->err == NRF_CLOUD_ERROR_NONE) {
			result->err = NRF_CLOUD_ERROR_UNKNOWN;
		}
	}

	return ret;
}

int nrf_cloud_rest_error_decode(const char *const buf, enum nrf_cloud_error *const err)
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
	if (cJSON_IsArray(root_obj) && (json_array_str_get(root_obj, 0, &msg) == 0)) {
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
		LOG_ERR("REST error msg: %s", msg);
	}

	cJSON_Delete(root_obj);

	return ret;
}

bool nrf_cloud_disconnection_request_decode(const char *const buf)
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

int nrf_cloud_gnss_msg_json_encode(const struct nrf_cloud_gnss_data * const gnss,
				   cJSON * const gnss_msg_obj)
{
	if (!gnss || !gnss_msg_obj) {
		return -EINVAL;
	}

	int ret;

	/* Add the app ID, message type, and timestamp */
	if (json_add_str_cs(gnss_msg_obj,
			    NRF_CLOUD_JSON_APPID_KEY,
			    NRF_CLOUD_JSON_APPID_VAL_GNSS) ||
	    json_add_str_cs(gnss_msg_obj,
			    NRF_CLOUD_JSON_MSG_TYPE_KEY,
			    NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA) ||
	    ((gnss->ts_ms != NRF_CLOUD_NO_TIMESTAMP) &&
	     json_add_num_cs(gnss_msg_obj, NRF_CLOUD_MSG_TIMESTAMP_KEY, gnss->ts_ms))) {
		ret = -ENOMEM;
		goto cleanup;
	}

	/* Add the specified GNSS data type */
	switch (gnss->type) {
	case NRF_CLOUD_GNSS_TYPE_MODEM_PVT:
	case NRF_CLOUD_GNSS_TYPE_PVT:
	{
		cJSON * const data_obj =
			cJSON_AddObjectToObject(gnss_msg_obj, NRF_CLOUD_JSON_DATA_KEY);

		/* Add PVT to the data object */
		if (gnss->type == NRF_CLOUD_GNSS_TYPE_PVT) {
			ret = nrf_cloud_pvt_data_encode(&gnss->pvt, data_obj);
		} else {
#if defined(CONFIG_NRF_MODEM)
			ret = nrf_cloud_modem_pvt_data_encode(gnss->mdm_pvt, data_obj);
#else
			ret = -ENOSYS;
#endif
		}

		if (ret) {
			goto cleanup;
		}

		break;
	}
	case NRF_CLOUD_GNSS_TYPE_MODEM_NMEA:
	case NRF_CLOUD_GNSS_TYPE_NMEA:
	{
		const char *nmea = NULL;

		if (gnss->type == NRF_CLOUD_GNSS_TYPE_MODEM_NMEA) {
#if defined(CONFIG_NRF_MODEM)
			if (gnss->mdm_nmea) {
				nmea = gnss->mdm_nmea->nmea_str;
			}
#endif
		} else {
			nmea = gnss->nmea.sentence;
		}

		if (nmea == NULL) {
			ret = -EINVAL;
			goto cleanup;
		}

		if (memchr(nmea, '\0', NRF_MODEM_GNSS_NMEA_MAX_LEN) == NULL) {
			ret = -EFBIG;
			goto cleanup;
		}

		/* Add the NMEA sentence to the message */
		if (cJSON_AddStringToObject(gnss_msg_obj, NRF_CLOUD_JSON_DATA_KEY, nmea) == NULL) {
			ret = -ENOMEM;
			goto cleanup;
		}

		break;
	}
	default:
		ret = -EPROTO;
		goto cleanup;
	}

	return 0;

cleanup:
	/* On failure, remove any items added to the provided object */
	cJSON_DeleteItemFromObject(gnss_msg_obj, NRF_CLOUD_JSON_APPID_KEY);
	cJSON_DeleteItemFromObject(gnss_msg_obj, NRF_CLOUD_JSON_MSG_TYPE_KEY);
	cJSON_DeleteItemFromObject(gnss_msg_obj, NRF_CLOUD_MSG_TIMESTAMP_KEY);
	cJSON_DeleteItemFromObject(gnss_msg_obj, NRF_CLOUD_JSON_DATA_KEY);

	return ret;
}

int nrf_cloud_alert_encode(const struct nrf_cloud_alert_info *alert, struct nrf_cloud_data *output)
{
#if defined(CONFIG_NRF_CLOUD_ALERT)
	int ret;

	__ASSERT_NO_MSG(alert != NULL);
	__ASSERT_NO_MSG(output != NULL);

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		return -ENOMEM;
	}

	ret = json_add_str_cs(root_obj, NRF_CLOUD_JSON_APPID_KEY, NRF_CLOUD_JSON_APPID_VAL_ALERT);
	ret += json_add_num_cs(root_obj, NRF_CLOUD_JSON_ALERT_TYPE, alert->type);
	if (alert->value != NRF_CLOUD_ALERT_UNUSED_VALUE) {
		ret += json_add_num_cs(root_obj, NRF_CLOUD_JSON_ALERT_VALUE, alert->value);
	}
	if (alert->ts_ms > NRF_CLOUD_NO_TIMESTAMP) {
		ret += json_add_num_cs(root_obj, NRF_CLOUD_MSG_TIMESTAMP_KEY, alert->ts_ms);
	}
	if ((alert->ts_ms <= NRF_CLOUD_NO_TIMESTAMP) ||
	    IS_ENABLED(CONFIG_NRF_CLOUD_ALERT_SEQ_ALWAYS)) {
		ret += json_add_num_cs(root_obj, NRF_CLOUD_JSON_ALERT_SEQUENCE, alert->sequence);
	}
	if (alert->description != NULL) {
		ret += json_add_str_cs(root_obj, NRF_CLOUD_JSON_ALERT_DESCRIPTION,
				       alert->description);
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
#else
	ARG_UNUSED(alert);
	output->ptr = NULL;
	output->len = 0;
#endif /* CONFIG_NRF_CLOUD_ALERT */
	return 0;
}

static int agps_types_array_json_encode(cJSON * const obj,
					const enum nrf_cloud_agps_type * const types,
					const size_t type_count)
{
	if (!obj || !types || !type_count) {
		return -EINVAL;
	}

	int err = 0;
	cJSON *array = cJSON_CreateArray();

	if (!array) {
		return -ENOMEM;
	}

	for (size_t i = 0; i < type_count; ++i) {
		if ((types[i] <= NRF_CLOUD_AGPS__TYPE_INVALID) ||
		    (types[i] > NRF_CLOUD_AGPS__LAST)) {
			LOG_INF("Ignoring unknown A-GPS type: %d", types[i]);
			continue;
		}

		cJSON *num = cJSON_CreateNumber(types[i]);

		if (!cJSON_AddItemToArray(array, num)) {
			cJSON_Delete(num);
			err = -EIO;
			break;
		}
	}

	if (!err) {
		err = cJSON_AddItemToObjectCS(obj, NRF_CLOUD_JSON_KEY_AGPS_TYPES, array) ? 0 : -EIO;
	}

	if (err) {
		cJSON_Delete(array);
	}

	return err;
}

int nrf_cloud_agps_req_data_json_encode(const enum nrf_cloud_agps_type * const types,
					const size_t type_count,
					const struct lte_lc_cell * const cell_inf,
					const bool fetch_cell_inf,
					const bool filtered_ephem, const uint8_t mask_angle,
					cJSON * const data_obj_out)
{
	if (!types || !type_count || !data_obj_out) {
		return -EINVAL;
	}

	int err;

	if (filtered_ephem) {
		if ((json_add_bool_cs(data_obj_out, NRF_CLOUD_JSON_FILTERED_KEY, true)) ||
		    (json_add_num_cs(data_obj_out, NRF_CLOUD_JSON_KEY_ELEVATION_MASK,
		    mask_angle))) {
			err = -ENOMEM;
			goto cleanup;
		}
		LOG_DBG("Requesting filtered ephemerides with elevation mask angle = %u degrees",
			mask_angle);
	}

	/* Add the cell info if provided or fetch flag set */
	if (cell_inf || fetch_cell_inf) {
		err = nrf_cloud_cell_info_json_encode(data_obj_out, cell_inf);
		if (err) {
			LOG_ERR("Failed to add cellular network info to A-GPS request: %d", err);
			goto cleanup;
		}
	}

	/* Add the requested types */
	err = agps_types_array_json_encode(data_obj_out, types, type_count);
	if (err) {
		LOG_ERR("Failed to add types array to A-GPS request %d", err);
		goto cleanup;
	}

	return 0;

cleanup:
	/* On failure, remove any items added to the provided object */
	cJSON_DeleteItemFromObject(data_obj_out, NRF_CLOUD_JSON_FILTERED_KEY);
	cJSON_DeleteItemFromObject(data_obj_out, NRF_CLOUD_JSON_KEY_ELEVATION_MASK);
	return err;
}

#if defined(CONFIG_NRF_MODEM)
int nrf_cloud_modem_pvt_data_encode(const struct nrf_modem_gnss_pvt_data_frame	* const mdm_pvt,
				    cJSON * const pvt_data_obj)
{
	if (!mdm_pvt || !pvt_data_obj) {
		return -EINVAL;
	}

	struct nrf_cloud_gnss_pvt pvt = {
		.lon =		mdm_pvt->longitude,
		.lat =		mdm_pvt->latitude,
		.accuracy =	mdm_pvt->accuracy,
		.alt =		mdm_pvt->altitude,
		.has_alt =	1,
		.speed =	mdm_pvt->speed,
		.has_speed =	1,
		.heading =	mdm_pvt->heading,
		.has_heading =	1
	};

	return nrf_cloud_pvt_data_encode(&pvt, pvt_data_obj);
}
#endif /* CONFIG_NRF_MODEM */

#if defined(CONFIG_NRF_CLOUD_AGPS) || defined(CONFIG_NRF_CLOUD_PGPS)
int nrf_cloud_agps_req_json_encode(const struct nrf_modem_gnss_agps_data_frame * const request,
				   cJSON * const agps_req_obj_out)
{
	if (!agps_req_obj_out || !request) {
		return -EINVAL;
	}

	int err;
	cJSON *data_obj = NULL;
	enum nrf_cloud_agps_type types[NRF_CLOUD_AGPS__LAST];
	uint8_t mask_angle = NRF_CLOUD_AGPS_MASK_ANGLE_NONE;
	int type_count = nrf_cloud_agps_type_array_get(request, types, ARRAY_SIZE(types));

	if (type_count < 0) {
		if (type_count == -ENODATA) {
			LOG_INF("No A-GPS data types requested");
		}
		return type_count;
	}

	/* Create request JSON containing a data object */
	if (json_add_str_cs(agps_req_obj_out,
			    NRF_CLOUD_JSON_APPID_KEY,
			    NRF_CLOUD_JSON_APPID_VAL_AGPS) ||
	    json_add_str_cs(agps_req_obj_out,
			    NRF_CLOUD_JSON_MSG_TYPE_KEY,
			    NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA)) {
		err = -ENOMEM;
		goto cleanup;
	}

	data_obj = cJSON_AddObjectToObject(agps_req_obj_out, NRF_CLOUD_JSON_DATA_KEY);
	if (!data_obj) {
		err = -ENOMEM;
		goto cleanup;
	}

#if defined(CONFIG_NRF_CLOUD_AGPS_FILTERED)
	mask_angle = CONFIG_NRF_CLOUD_AGPS_ELEVATION_MASK;
#endif

	/* Populate the request payload */
	err = nrf_cloud_agps_req_data_json_encode(types, type_count, NULL, true,
						  IS_ENABLED(CONFIG_NRF_CLOUD_AGPS_FILTERED),
						  mask_angle, data_obj);
	if (!err) {
		return 0;
	}

cleanup:
	/* On failure, remove any items added to the provided object */
	cJSON_DeleteItemFromObject(agps_req_obj_out, NRF_CLOUD_JSON_APPID_KEY);
	cJSON_DeleteItemFromObject(agps_req_obj_out, NRF_CLOUD_JSON_MSG_TYPE_KEY);
	cJSON_DeleteItemFromObject(agps_req_obj_out, NRF_CLOUD_JSON_DATA_KEY);

	return err;
}

int nrf_cloud_agps_type_array_get(const struct nrf_modem_gnss_agps_data_frame * const request,
				  enum nrf_cloud_agps_type *array, const size_t array_size)
{
	if (!request || !array || !array_size) {
		return -EINVAL;
	}
	if (array_size < NRF_CLOUD_AGPS__LAST) {
		LOG_ERR("Array size (%d) too small, must be >= %d",
			array_size, NRF_CLOUD_AGPS__LAST);
		return -ERANGE;
	}

	int cnt = 0;

	memset((void *)array, NRF_CLOUD_AGPS__TYPE_INVALID, array_size * sizeof(*array));

	if (request->data_flags & NRF_MODEM_GNSS_AGPS_GPS_UTC_REQUEST) {
		array[cnt++] = NRF_CLOUD_AGPS_UTC_PARAMETERS;
	}

	if (request->sv_mask_ephe) {
		array[cnt++] = NRF_CLOUD_AGPS_EPHEMERIDES;
	}

	if (request->sv_mask_alm) {
		array[cnt++] = NRF_CLOUD_AGPS_ALMANAC;
	}

	if (request->data_flags & NRF_MODEM_GNSS_AGPS_KLOBUCHAR_REQUEST) {
		array[cnt++] = NRF_CLOUD_AGPS_KLOBUCHAR_CORRECTION;
	}

	if (request->data_flags & NRF_MODEM_GNSS_AGPS_NEQUICK_REQUEST) {
		array[cnt++] = NRF_CLOUD_AGPS_NEQUICK_CORRECTION;
	}

	if (request->data_flags & NRF_MODEM_GNSS_AGPS_SYS_TIME_AND_SV_TOW_REQUEST) {
		array[cnt++] = NRF_CLOUD_AGPS_GPS_TOWS;
		array[cnt++] = NRF_CLOUD_AGPS_GPS_SYSTEM_CLOCK;
	}

	if (request->data_flags & NRF_MODEM_GNSS_AGPS_POSITION_REQUEST) {
		array[cnt++] = NRF_CLOUD_AGPS_LOCATION;
	}

	if (request->data_flags & NRF_MODEM_GNSS_AGPS_INTEGRITY_REQUEST) {
		array[cnt++] = NRF_CLOUD_AGPS_INTEGRITY;
	}

	if (cnt == 0) {
		return -ENODATA;
	}

	return cnt;
}
#endif /* CONFIG_NRF_CLOUD_AGPS */

#if defined(CONFIG_NRF_CLOUD_PGPS)
int nrf_cloud_pgps_response_decode(const char *const response,
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
		if (json_array_str_get(rsp_obj, NRF_CLOUD_PGPS_RCV_ARRAY_IDX_HOST, &host_ptr) ||
		    json_array_str_get(rsp_obj, NRF_CLOUD_PGPS_RCV_ARRAY_IDX_PATH, &path_ptr)) {
			LOG_ERR("Invalid P-GPS array response format");
			err = -EPROTO;
			goto cleanup;
		}
	} else if (get_string_from_obj(rsp_obj, NRF_CLOUD_PGPS_RCV_REST_HOST, &host_ptr) ||
		   get_string_from_obj(rsp_obj, NRF_CLOUD_PGPS_RCV_REST_PATH, &path_ptr)) {
		enum nrf_cloud_error nrf_err;

		/* Check for a potential P-GPS JSON error message from nRF Cloud */
		err = nrf_cloud_error_msg_decode(response, NRF_CLOUD_JSON_APPID_VAL_PGPS,
						     NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA, &nrf_err);
		if (!err) {
			LOG_ERR("nRF Cloud returned P-GPS error: %d", nrf_err);
			err = -EFAULT;
		} else {
			LOG_ERR("Invalid P-GPS response format");
			err = -EPROTO;
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
	LOG_DBG("host: %s", result->host);

	strncpy(result->path, path_ptr, result->path_sz);
	LOG_DBG("path: %s", result->path);

cleanup:
	if (rsp_obj) {
		cJSON_Delete(rsp_obj);
	}
	return err;
}

int nrf_cloud_pgps_req_data_json_encode(const struct gps_pgps_request * const request,
					cJSON * const data_obj_out)
{
	if (!request || !data_obj_out) {
		return -EINVAL;
	}

	if ((request->prediction_count != NRF_CLOUD_PGPS_REQ_NO_COUNT) &&
	    json_add_num_cs(data_obj_out, NRF_CLOUD_JSON_PGPS_PRED_COUNT,
			    request->prediction_count)) {
		goto cleanup;
	}

	if ((request->prediction_period_min != NRF_CLOUD_PGPS_REQ_NO_INTERVAL) &&
	    json_add_num_cs(data_obj_out, NRF_CLOUD_JSON_PGPS_INT_MIN,
			    request->prediction_period_min)) {
		goto cleanup;
	}

	if ((request->gps_day != NRF_CLOUD_PGPS_REQ_NO_GPS_DAY) &&
	    json_add_num_cs(data_obj_out, NRF_CLOUD_JSON_PGPS_GPS_DAY,
			    request->gps_day)) {
		goto cleanup;
	}

	if ((request->gps_time_of_day != NRF_CLOUD_PGPS_REQ_NO_GPS_TOD) &&
	    json_add_num_cs(data_obj_out, NRF_CLOUD_JSON_PGPS_GPS_TIME,
			    request->gps_time_of_day)) {
		goto cleanup;
	}

	return 0;

cleanup:
	/* On failure, remove any items added to the provided object */
	cJSON_DeleteItemFromObject(data_obj_out, NRF_CLOUD_JSON_PGPS_PRED_COUNT);
	cJSON_DeleteItemFromObject(data_obj_out, NRF_CLOUD_JSON_PGPS_INT_MIN);
	cJSON_DeleteItemFromObject(data_obj_out, NRF_CLOUD_JSON_PGPS_GPS_DAY);
	cJSON_DeleteItemFromObject(data_obj_out, NRF_CLOUD_JSON_PGPS_GPS_TIME);

	return -ENOMEM;
}
#endif /* CONFIG_NRF_CLOUD_PGPS */

int nrf_cloud_json_to_url_params_convert(char *const buf, const size_t buf_size,
					 const cJSON *const obj)
{
	if (!obj || !buf || !buf_size) {
		return -EINVAL;
	}

	int ret = 0;
	size_t pos = 0;
	size_t remain = buf_size;
	cJSON *child = NULL;

	cJSON_ArrayForEach(child, obj) {
		char prefix = ((pos == 0) ? '?' : '&');

		/* Only handle bool, number (int), string, and (int) array types */
		if (cJSON_IsBool(child)) {
			ret = snprintk(&buf[pos], remain, "%c%s=%s", prefix, child->string,
				       cJSON_IsTrue(child) ? "true" : "false");
		} else if (cJSON_IsNumber(child)) {
			ret = snprintk(&buf[pos], remain, "%c%s=%d", prefix, child->string,
				       child->valueint);
		} else if (cJSON_IsString(child)) {
			/* Assume that the string is URL-compatible */
			ret = snprintk(&buf[pos], remain, "%c%s=%s", prefix, child->string,
				       child->valuestring);
		} else if (cJSON_IsArray(child)) {
			int cnt = 0;
			cJSON *array_item = NULL;

			cJSON_ArrayForEach(array_item, child) {
				if (!cJSON_IsNumber(array_item)) {
					return -ENOTSUP;
				}

				if (cnt == 0) {
					/* On the first item, add the key string */
					ret = snprintk(&buf[pos], remain, "%c%s=",
						       prefix, child->string);
				} else {
					/* For additional items, add a comma */
					ret = snprintk(&buf[pos], remain, "%c", ',');
				}

				if ((ret > 0) && (ret < remain)) {
					remain -= ret;
					pos += ret;
				} else {
					return -E2BIG;
				}

				/* Add the integer value */
				ret = snprintk(&buf[pos], remain, "%d", array_item->valueint);

				if ((ret > 0) && (ret < remain)) {
					remain -= ret;
					pos += ret;
					++cnt;
				} else {
					return -E2BIG;
				}
			}

			/* Check next child item, pos and remain values already updated */
			continue;
		} else {
			return -ENOTSUP;
		}

		if ((ret > 0) && (ret < remain)) {
			remain -= ret;
			pos += ret;
		} else if (ret == 0) {
			return -EIO;
		} else {
			return -E2BIG;
		}
	}

	/* Return an error if no data was added */
	if (remain == buf_size) {
		return -ENOMSG;
	}

	return 0;
}

static int encode_json_log(struct nrf_cloud_log_context *ctx, uint8_t *buf, size_t size,
			   struct nrf_cloud_data *output)
{
	int ret;
	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		return -ENOMEM;
	}

	ret = json_add_str_cs(root_obj, NRF_CLOUD_JSON_APPID_KEY, NRF_CLOUD_JSON_APPID_VAL_LOG);
	if (ctx != NULL) {
		ret += json_add_num_cs(root_obj, NRF_CLOUD_JSON_LOG_KEY_DOMAIN, ctx->dom_id);
		ret += json_add_num_cs(root_obj, NRF_CLOUD_JSON_LOG_KEY_LEVEL, ctx->level);
		if (ctx->src_name != NULL) {
			ret += json_add_str_cs(root_obj, NRF_CLOUD_JSON_LOG_KEY_SOURCE,
					       ctx->src_name);
		}
		if (ctx->ts > 0) {
			ret += json_add_num_cs(root_obj, NRF_CLOUD_MSG_TIMESTAMP_KEY, ctx->ts);
		}
		if (!ctx->ts || IS_ENABLED(CONFIG_NRF_CLOUD_LOG_SEQ_ALWAYS)) {
			ret += json_add_num_cs(root_obj, NRF_CLOUD_JSON_LOG_KEY_SEQUENCE,
					       ctx->sequence);
		}
	}

	ret += json_add_str_cs(root_obj, NRF_CLOUD_JSON_LOG_KEY_MESSAGE, (const char *)buf);
	if (ret != 0) {
		cJSON_Delete(root_obj);
		return -ENOMEM;
	}

	char *buffer = cJSON_PrintUnformatted(root_obj);

	cJSON_Delete(root_obj);

	if (buffer == NULL) {
		return -ENOMEM;
	}

	output->ptr = buffer;
	output->len = strlen(buffer);
	return 0;
}

int nrf_cloud_log_json_encode(struct nrf_cloud_log_context *ctx, uint8_t *buf, size_t size,
			 struct nrf_cloud_data *output)
{
	__ASSERT_NO_MSG(ctx != NULL);
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(output != NULL);

	return encode_json_log(ctx, buf, size, output);
}
