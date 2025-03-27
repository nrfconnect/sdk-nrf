/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf_cloud_transport.h"
#include "nrf_cloud_mem.h"
#include "nrf_cloud_client_id.h"
#include "nrf_cloud_credentials.h"
#if defined(CONFIG_NRF_CLOUD_FOTA)
#include "nrf_cloud_fota.h"
#endif

#include "nrf_cloud_dns.h"

#include <zephyr/kernel.h>
#include <stdio.h>
#include <fcntl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/settings/settings.h>
#include <zephyr/net/socket.h>

LOG_MODULE_REGISTER(nrf_cloud_transport, CONFIG_NRF_CLOUD_LOG_LEVEL);

#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME)
BUILD_ASSERT((sizeof(CONFIG_NRF_CLOUD_CLIENT_ID) - 1) <= NRF_CLOUD_CLIENT_ID_MAX_LEN,
	"CONFIG_NRF_CLOUD_CLIENT_ID must not exceed NRF_CLOUD_CLIENT_ID_MAX_LEN");
#endif

#define NRF_CLOUD_HOSTNAME CONFIG_NRF_CLOUD_HOST_NAME
#define NRF_CLOUD_PORT CONFIG_NRF_CLOUD_PORT

/* nRF Cloud's custom update topic (device PUB).
 * Functionally identical to the AWS shadow update topic.
 */
#define NCT_SHDW_TPC_UPDATE		"%s/shadow/update"

/* nRF Cloud's custom rejected topic (device SUB).
 * Functionally identical to the AWS get/rejected topic.
 */
#define NCT_SHDW_TPC_GET_REJECT		"%s/shadow/get/rejected"

/* nRF Cloud's custom delta update topic (device SUB).
 * Functionally identical to the AWS update/delta topic.
 */
#define NCT_SHDW_TPC_DELTA_FULL		"%s/shadow/update/delta/full"

/* nRF Cloud's custom trimmed shadow request topic (device PUB).
 * Request only the shadow data needed (no metadata) for nrf_cloud library functionality.
 * Using the default AWS shadow topic to request the shadow can often result in the cloud
 * sending more data than the modem can receive (2kB for TLS). This is avoided by using
 * a custom trimmed shadow.
 */
#define NCT_SHDW_TPC_GET_TRIM		"%s/shadow/get/trim"

/* nRF Cloud's custom trimmed shadow topic (device SUB).
 * Receives the trimmed shadow data.
 */
#define NCT_SHDW_TPC_GET_ACCEPT_TRIM	"%s/shadow/get/accepted/trim"

/* nRF Cloud's custom trimmed delta topic (device SUB).
 * Returns the delta without the metadata.
 * Not currently used by the nrf_cloud library.
 */
#define NCT_SHDW_TPC_DELTA_TRIM		"%s/shadow/update/delta/trim"

/* nRF Cloud's custom shadow transform (JSONata expression) request topic (device PUB).
 * Request shadow data according to the provided transform.
 */
#define NCT_SHDW_TPC_GET_TF		"%s/shadow/get/tf"

/* nRF Cloud's custom shadow transform topic (device SUB).
 * Receives the shadow data from a requested transform.
 */
#define NCT_SHDW_TPC_GET_ACCEPT_TF	"%s/shadow/get/accepted/tf"


/* Buffers to hold stage and tenant strings. */
static char stage[NRF_CLOUD_STAGE_ID_MAX_LEN];
static char tenant[NRF_CLOUD_TENANT_ID_MAX_LEN];

/* Null-terminated MQTT client ID */
static const char *client_id_ptr;

static bool mqtt_client_initialized;
static bool persistent_session;

static int nct_settings_set(const char *key, size_t len_rd,
			    settings_read_cb read_cb, void *cb_arg);

#define SETTINGS_NAME "nrf_cloud"
#define SETTINGS_KEY_PERSISTENT_SESSION "p_sesh"
#define SETTINGS_FULL_PERSISTENT_SESSION SETTINGS_NAME \
					 "/" \
					 SETTINGS_KEY_PERSISTENT_SESSION

SETTINGS_STATIC_HANDLER_DEFINE(nrf_cloud, SETTINGS_NAME, NULL, nct_settings_set,
			       NULL, NULL);

/* Forward declaration of the event handler registered with MQTT. */
static void nct_mqtt_evt_handler(struct mqtt_client *client,
				 const struct mqtt_evt *evt);

/* nrf_cloud transport instance. */
static struct nct {
	struct mqtt_sec_config tls_config;
	struct mqtt_client client;
	struct nct_cc_endpoints cc;
	struct nct_dc_endpoints dc;
	uint16_t message_id;
	uint8_t rx_buf[CONFIG_NRF_CLOUD_MQTT_MESSAGE_BUFFER_LEN];
	uint8_t tx_buf[CONFIG_NRF_CLOUD_MQTT_MESSAGE_BUFFER_LEN];
	uint8_t payload_buf[CONFIG_NRF_CLOUD_MQTT_PAYLOAD_BUFFER_LEN + 1];
} nct;

#if defined(CONFIG_NRF_CLOUD_MQTT_SHADOW_TRANSFORMS)
#define CC_RX_LIST_CNT 4
#define CC_TX_LIST_CNT 3
#else
#define CC_RX_LIST_CNT 3
#define CC_TX_LIST_CNT 2
#endif
static uint32_t const nct_cc_rx_opcode_map[CC_RX_LIST_CNT] = {
	NCT_CC_OPCODE_UPDATE_ACCEPTED,
	NCT_CC_OPCODE_UPDATE_REJECTED,
	NCT_CC_OPCODE_UPDATE_DELTA,
#if defined(CONFIG_NRF_CLOUD_MQTT_SHADOW_TRANSFORMS)
	NCT_CC_OPCODE_TRANSFORM
#endif
};
static struct mqtt_topic nct_cc_rx_list[CC_RX_LIST_CNT];

BUILD_ASSERT(ARRAY_SIZE(nct_cc_rx_opcode_map) == ARRAY_SIZE(nct_cc_rx_list),
	"nct_cc_rx_opcode_map should be the same size as nct_cc_rx_list");

static struct mqtt_topic nct_cc_tx_list[CC_TX_LIST_CNT];

/* Internal routine to reset data endpoint information. */
static void dc_endpoint_reset(void)
{
	memset(&nct.dc.e, 0, sizeof(nct.dc.e));
}

/* Get the next unused message id. */
static uint16_t get_next_message_id(void)
{
	if (nct.message_id < NCT_MSG_ID_INCREMENT_BEGIN ||
	    nct.message_id == NCT_MSG_ID_INCREMENT_END) {
		nct.message_id = NCT_MSG_ID_INCREMENT_BEGIN;
	} else {
		++nct.message_id;
	}

	return nct.message_id;
}

static uint16_t get_message_id(const uint16_t requested_id)
{
	if (requested_id != NCT_MSG_ID_USE_NEXT_INCREMENT) {
		return requested_id;
	}

	return get_next_message_id();
}

/* Free memory allocated for the data endpoint and reset the endpoint.
 *
 * Casting away const for the endpoints is OK because the
 * nct_dc_endpoint_set() caller gets the buffers from
 * json_decode_and_alloc(), which uses nrf_cloud_malloc() to call
 * k_malloc().
 */
static void dc_endpoint_free(void)
{
	for (int cnt = 0; cnt < DC__COUNT; ++cnt) {
		nrf_cloud_free((void *)nct.dc.e[cnt].utf8);
	}
	dc_endpoint_reset();

#if defined(CONFIG_NRF_CLOUD_FOTA)
	nrf_cloud_fota_endpoint_clear();
#endif
}

static int endp_send(const struct nct_dc_data *dc_data, struct mqtt_utf8 *endp, enum mqtt_qos qos)
{
	if (dc_data == NULL) {
		LOG_DBG("Passed in structure cannot be NULL");
		return -EINVAL;
	}

	if ((qos != MQTT_QOS_0_AT_MOST_ONCE) && (qos != MQTT_QOS_1_AT_LEAST_ONCE)) {
		LOG_DBG("Unsupported MQTT QoS level");
		return -EINVAL;
	}

	struct mqtt_publish_param publish = {
		.message_id = 0,
		.message.topic.qos = qos,
		.message.topic.topic.size = endp->size,
		.message.topic.topic.utf8 = endp->utf8,
	};

	/* Populate payload. */
	if ((dc_data->data.len != 0) && (dc_data->data.ptr != NULL)) {
		publish.message.payload.data = (uint8_t *)dc_data->data.ptr;
		publish.message.payload.len = dc_data->data.len;
	} else {
		LOG_DBG("Payload is empty!");
	}

	if (qos != MQTT_QOS_0_AT_MOST_ONCE) {
		publish.message_id = get_message_id(dc_data->message_id);
	}
	LOG_DBG("mqtt_publish: id = %d len = %d, topic: %.*s", publish.message_id,
		dc_data->data.len,
		publish.message.topic.topic.size,
		publish.message.topic.topic.utf8);
	LOG_DBG("payload: %.*s", publish.message.payload.len,
	     publish.message.payload.data ? (const char *)publish.message.payload.data : "");
	return mqtt_publish(&nct.client, &publish);
}

/* Verify if the RX topic is a control channel topic or not. */
static bool nrf_cloud_cc_rx_topic_decode(const struct mqtt_topic *topic, enum nct_cc_opcode *opcode)
{
	const uint32_t list_size = ARRAY_SIZE(nct_cc_rx_list);
	const char *const topic_str = topic->topic.utf8;
	const uint32_t topic_sz = topic->topic.size;

	for (uint32_t index = 0; index < list_size; ++index) {
		if (topic_sz != nct_cc_rx_list[index].topic.size) {
			continue;
		}

		/* Compare incoming topic with the entry in the RX topic list */
		if (strncmp(topic_str, (char *)nct_cc_rx_list[index].topic.utf8, topic_sz) == 0) {
			*opcode = nct_cc_rx_opcode_map[index];
			return true;
		}
	}

	/* Not a control channel topic */
	return false;
}

/* Function to set/generate the MQTT client ID */
static int nct_client_id_set(const char * const client_id)
{
	int err = 0;

	if (client_id) {
		if (!IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME)) {
			LOG_WRN("Not configured for runtime client ID, ignoring");
		} else {
			err = nrf_cloud_client_id_runtime_set(client_id);
			if (err) {
				LOG_ERR("Failed to set runtime client ID, error: %d", err);
				return err;
			}
		}
	}

	err = nrf_cloud_client_id_ptr_get(&client_id_ptr);
	if (err) {
		LOG_ERR("Failed to get client ID, error %d", err);
		return err;
	}

	LOG_DBG("client_id = %s", client_id_ptr);

	return 0;
}

int nct_stage_get(char *cur_stage, const int cur_stage_len)
{
	int len = strlen(stage);

	if (cur_stage_len <= len) {
		return -EMSGSIZE;
	} else if ((cur_stage != NULL) && len) {
		strncpy(cur_stage, stage, cur_stage_len);
		return 0;
	}
	return -EINVAL;
}

int nct_tenant_id_get(char *cur_tenant, const int cur_tenant_len)
{
	int len = strlen(tenant);

	if (cur_tenant_len <= len) {
		return -EMSGSIZE;
	} else if ((cur_tenant != NULL) && len) {
		strncpy(cur_tenant, tenant, cur_tenant_len);
		return 0;
	}
	return -EINVAL;
}

void nct_set_topic_prefix(const char *topic_prefix)
{
	char *end_of_stage = strchr(topic_prefix, '/');
	int len;

	if (end_of_stage) {
		len = end_of_stage - topic_prefix;
		if (len >= sizeof(stage)) {
			LOG_WRN("Truncating copy of stage string length "
				"from %d to %zd",
				len, sizeof(stage));
			len = sizeof(stage) - 1;
		}
		memcpy(stage, topic_prefix, len);
		stage[len] = '\0';
		len = strlen(topic_prefix) - len - 2; /* skip both / */
		if (len >= sizeof(tenant)) {
			LOG_WRN("Truncating copy of tenant id string length "
				"from %d to %zd",
				len, sizeof(tenant));
			len = sizeof(tenant) - 1;
		}
		memcpy(tenant, end_of_stage + 1, len);
		tenant[len] = '\0';
	}
}

static int allocate_and_format_topic(struct mqtt_utf8 *const topic,
				     const char * const topic_template)
{
	int ret;
	size_t topic_sz;
	char *topic_str;
	const size_t client_sz = strlen(client_id_ptr);

	topic->size = 0;
	topic->utf8 = NULL;

	topic_sz = client_sz + strlen(topic_template) - 1;
	topic_str = nrf_cloud_calloc(topic_sz, 1);

	if (!topic_str) {
		return -ENOMEM;
	}

	ret = snprintk(topic_str, topic_sz, topic_template, client_id_ptr);
	if (ret <= 0 || ret >= topic_sz) {
		nrf_cloud_free(topic_str);
		return -EIO;
	}

	topic->utf8 = (uint8_t *)topic_str;
	topic->size = strlen(topic_str);

	return 0;
}

static void nct_reset_topics(void)
{
	/* Reset the topics */
	for (int cnt = 0; cnt < CC__COUNT; ++cnt) {
		nrf_cloud_free((void *)nct.cc.e[cnt].utf8);
	}
	memset(&nct.cc.e, 0, sizeof(nct.cc.e));

	/* Reset the lists */
	memset(nct_cc_rx_list, 0, sizeof(nct_cc_rx_list[0]) * CC_RX_LIST_CNT);
	memset(nct_cc_tx_list, 0, sizeof(nct_cc_tx_list[0]) * CC_TX_LIST_CNT);
}

static void nct_topic_lists_populate(void)
{
	/* Add RX (subscribe) topics, aligning with opcode list */
	for (int idx = 0; idx < CC_RX_LIST_CNT; ++idx) {
		switch (nct_cc_rx_opcode_map[idx]) {
		case NCT_CC_OPCODE_UPDATE_ACCEPTED:
			nct_cc_rx_list[idx].qos = MQTT_QOS_1_AT_LEAST_ONCE;
			nct_cc_rx_list[idx].topic = nct.cc.e[CC_RX_ACCEPT];
			break;
		case NCT_CC_OPCODE_UPDATE_REJECTED:
			nct_cc_rx_list[idx].qos = MQTT_QOS_1_AT_LEAST_ONCE;
			nct_cc_rx_list[idx].topic = nct.cc.e[CC_RX_REJECT];
			break;
		case NCT_CC_OPCODE_UPDATE_DELTA:
			nct_cc_rx_list[idx].qos = MQTT_QOS_1_AT_LEAST_ONCE;
			nct_cc_rx_list[idx].topic = nct.cc.e[CC_RX_DELTA];
			break;
#if defined(CONFIG_NRF_CLOUD_MQTT_SHADOW_TRANSFORMS)
		case NCT_CC_OPCODE_TRANSFORM:
			nct_cc_rx_list[idx].qos = MQTT_QOS_1_AT_LEAST_ONCE;
			nct_cc_rx_list[idx].topic = nct.cc.e[CC_RX_ACCEPT_TF];
			break;
#endif
		default:
			__ASSERT(false, "Op code not added to RX list");
			break;
		}
	}

	/* Add TX topics */
	nct_cc_tx_list[NCT_CC_OPCODE_GET_REQ].qos = MQTT_QOS_1_AT_LEAST_ONCE;
	nct_cc_tx_list[NCT_CC_OPCODE_GET_REQ].topic = nct.cc.e[CC_TX_GET];

	nct_cc_tx_list[NCT_CC_OPCODE_UPDATE_ACCEPTED].qos = MQTT_QOS_1_AT_LEAST_ONCE;
	nct_cc_tx_list[NCT_CC_OPCODE_UPDATE_ACCEPTED].topic = nct.cc.e[CC_TX_UPDATE];

#if defined(CONFIG_NRF_CLOUD_MQTT_SHADOW_TRANSFORMS)
	nct_cc_tx_list[NCT_CC_OPCODE_TRANSFORM].qos = MQTT_QOS_1_AT_LEAST_ONCE;
	nct_cc_tx_list[NCT_CC_OPCODE_TRANSFORM].topic = nct.cc.e[CC_TX_GET_TF];
#endif
}

static int nct_topics_populate(void)
{
	__ASSERT_NO_MSG(client_id_ptr != NULL);

	int ret;

	nct_reset_topics();

	ret = allocate_and_format_topic(&nct.cc.e[CC_RX_ACCEPT], NCT_SHDW_TPC_GET_ACCEPT_TRIM);
	if (ret) {
		goto err_cleanup;
	}
	ret = allocate_and_format_topic(&nct.cc.e[CC_RX_REJECT], NCT_SHDW_TPC_GET_REJECT);
	if (ret) {
		goto err_cleanup;
	}
	ret = allocate_and_format_topic(&nct.cc.e[CC_RX_DELTA], NCT_SHDW_TPC_DELTA_FULL);
	if (ret) {
		goto err_cleanup;
	}
	ret = allocate_and_format_topic(&nct.cc.e[CC_TX_UPDATE], NCT_SHDW_TPC_UPDATE);
	if (ret) {
		goto err_cleanup;
	}
	ret = allocate_and_format_topic(&nct.cc.e[CC_TX_GET], NCT_SHDW_TPC_GET_TRIM);
	if (ret) {
		goto err_cleanup;
	}

#if defined(CONFIG_NRF_CLOUD_MQTT_SHADOW_TRANSFORMS)
	ret = allocate_and_format_topic(&nct.cc.e[CC_TX_GET_TF], NCT_SHDW_TPC_GET_TF);
	if (ret) {
		goto err_cleanup;
	}
	ret = allocate_and_format_topic(&nct.cc.e[CC_RX_ACCEPT_TF], NCT_SHDW_TPC_GET_ACCEPT_TF);
	if (ret) {
		goto err_cleanup;
	}
#endif

	LOG_DBG("CC topics:");
	for (int cnt = 0; cnt < CC__COUNT; ++cnt) {
		LOG_DBG("%s", (char *)nct.cc.e[cnt].utf8);
	}

	/* Populate RX and TX topic lists */
	nct_topic_lists_populate();

	return 0;

err_cleanup:
	LOG_ERR("Failed to format MQTT topics, err: %d", ret);
	nct_reset_topics();
	return ret;
}

/* Provisions root CA certificate using modem_key_mgmt API */
static int nct_provision(void)
{
	static sec_tag_t sec_tag;
	int err = 0;

	sec_tag = nrf_cloud_sec_tag_get();

	nct.tls_config.peer_verify = 2;
	nct.tls_config.cipher_count = 0;
	nct.tls_config.cipher_list = NULL;
	nct.tls_config.sec_tag_count = 1;
	nct.tls_config.sec_tag_list = &sec_tag;
	nct.tls_config.hostname = NRF_CLOUD_HOSTNAME;

#if defined(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES)
		err = nrf_cloud_credentials_provision();

		if (err) {
			return err;
		}
#endif

	return err;
}

static int nct_settings_set(const char *key, size_t len_rd,
			    settings_read_cb read_cb, void *cb_arg)
{
	if (!key) {
		return -EINVAL;
	}

	int read_val;

	LOG_DBG("Settings key: %s, size: %d", key, len_rd);

	if (!strncmp(key, SETTINGS_KEY_PERSISTENT_SESSION,
		     strlen(SETTINGS_KEY_PERSISTENT_SESSION)) &&
	    (len_rd == sizeof(read_val))) {
		if (read_cb(cb_arg, (void *)&read_val, len_rd) == len_rd) {
#if !defined(CONFIG_MQTT_CLEAN_SESSION)
			persistent_session = (bool)read_val;
#endif
			LOG_DBG("Read setting val: %d", read_val);
			return 0;
		}
	}
	return -ENOTSUP;
}

int nct_save_session_state(const int session_valid)
{
	int ret = 0;

#if !defined(CONFIG_MQTT_CLEAN_SESSION)
	LOG_DBG("Setting session state: %d", session_valid);
	persistent_session = (bool)session_valid;
	ret = settings_save_one(SETTINGS_FULL_PERSISTENT_SESSION,
				&session_valid, sizeof(session_valid));
#endif
	return ret;
}

int nct_get_session_state(void)
{
	return persistent_session;
}

static int nct_settings_init(void)
{
	int ret = 0;

#if !defined(CONFIG_MQTT_CLEAN_SESSION) || defined(CONFIG_NRF_CLOUD_FOTA)
	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("Settings init failed: %d", ret);
		return ret;
	}
#if !defined(CONFIG_MQTT_CLEAN_SESSION)
	ret = settings_load_subtree(settings_handler_nrf_cloud.name);
	if (ret) {
		LOG_ERR("Cannot load settings: %d", ret);
	}
#endif
#else
	ARG_UNUSED(settings_handler_nrf_cloud);
#endif

	return ret;
}

#if defined(CONFIG_NRF_CLOUD_FOTA)
static void nrf_cloud_fota_cb_handler(const struct nrf_cloud_fota_evt
				      * const evt)
{
	if (!evt) {
		LOG_ERR("Received NULL FOTA event");
		return;
	}

	switch (evt->id) {
	case NRF_CLOUD_FOTA_EVT_START: {
		struct nrf_cloud_evt cloud_evt = {
			.type = NRF_CLOUD_EVT_FOTA_START
		};

		LOG_DBG("NRF_CLOUD_FOTA_EVT_START");
		cloud_evt.data.ptr = (const void *)&evt->type;
		cloud_evt.data.len = sizeof(evt->type);
		nct_send_event(&cloud_evt);
		break;
	}
	case NRF_CLOUD_FOTA_EVT_DONE: {
		struct nrf_cloud_evt cloud_evt = {
			.type = NRF_CLOUD_EVT_FOTA_DONE,
		};

		LOG_DBG("NRF_CLOUD_FOTA_EVT_DONE");
		cloud_evt.data.ptr = (const void *)&evt->type;
		cloud_evt.data.len = sizeof(evt->type);
		nct_send_event(&cloud_evt);
		break;
	}
	case NRF_CLOUD_FOTA_EVT_ERROR: {
		LOG_ERR("NRF_CLOUD_FOTA_EVT_ERROR");
		struct nrf_cloud_evt cloud_evt = {
			.type = NRF_CLOUD_EVT_FOTA_ERROR
		};

		nct_send_event(&cloud_evt);
		break;
	}
	case NRF_CLOUD_FOTA_EVT_ERASE_PENDING: {
		LOG_DBG("NRF_CLOUD_FOTA_EVT_ERASE_PENDING");
		break;
	}
	case NRF_CLOUD_FOTA_EVT_ERASE_TIMEOUT: {
		LOG_DBG("NRF_CLOUD_FOTA_EVT_ERASE_TIMEOUT");
		break;
	}
	case NRF_CLOUD_FOTA_EVT_ERASE_DONE: {
		LOG_DBG("NRF_CLOUD_FOTA_EVT_ERASE_DONE");
		break;
	}
	case NRF_CLOUD_FOTA_EVT_DL_PROGRESS: {
		LOG_DBG("NRF_CLOUD_FOTA_EVT_DL_PROGRESS");
		break;
	}
	case NRF_CLOUD_FOTA_EVT_JOB_RCVD: {
		struct nrf_cloud_evt cloud_evt = {
			.type = NRF_CLOUD_EVT_FOTA_JOB_AVAILABLE
		};

		LOG_DBG("NRF_CLOUD_EVT_FOTA_JOB_AVAILABLE");
		cloud_evt.data.ptr = (const void *)&evt->type;
		cloud_evt.data.len = sizeof(evt->type);
		nct_send_event(&cloud_evt);
		break;
	}
	default: {
		break;
	}
	}
}
#endif

static int publish_get_payload(struct mqtt_client *client, size_t length)
{
	if (length > (sizeof(nct.payload_buf) - 1)) {
		LOG_ERR("Length specified:%zd larger than payload_buf:%zd",
			length, sizeof(nct.payload_buf));
		return -EMSGSIZE;
	}

	int ret = mqtt_readall_publish_payload(client, nct.payload_buf, length);

	/* Ensure buffer is always NULL-terminated */
	nct.payload_buf[length] = 0;

	return ret;
}

static int translate_mqtt_connack_result(const int mqtt_result)
{
	switch (mqtt_result) {
	case MQTT_CONNECTION_ACCEPTED:
		return NRF_CLOUD_ERR_STATUS_NONE;
	case MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
		return NRF_CLOUD_ERR_STATUS_MQTT_CONN_BAD_PROT_VER;
	case MQTT_IDENTIFIER_REJECTED:
		return NRF_CLOUD_ERR_STATUS_MQTT_CONN_ID_REJECTED;
	case MQTT_SERVER_UNAVAILABLE:
		return NRF_CLOUD_ERR_STATUS_MQTT_CONN_SERVER_UNAVAIL;
	case MQTT_BAD_USER_NAME_OR_PASSWORD:
		return NRF_CLOUD_ERR_STATUS_MQTT_CONN_BAD_USR_PWD;
	case MQTT_NOT_AUTHORIZED:
		return NRF_CLOUD_ERR_STATUS_MQTT_CONN_NOT_AUTH;
	default:
		return NRF_CLOUD_ERR_STATUS_MQTT_CONN_FAIL;
	}
}

static int translate_mqtt_suback_result(const int mqtt_result)
{
	switch (mqtt_result) {
	case MQTT_SUBACK_SUCCESS_QoS_0:
	case MQTT_SUBACK_SUCCESS_QoS_1:
	case MQTT_SUBACK_SUCCESS_QoS_2:
		return NRF_CLOUD_ERR_STATUS_NONE;
	case MQTT_SUBACK_FAILURE:
	default:
		return NRF_CLOUD_ERR_STATUS_MQTT_SUB_FAIL;
	}
}

/* Handle MQTT events. */
static void nct_mqtt_evt_handler(struct mqtt_client *const mqtt_client,
				 const struct mqtt_evt *_mqtt_evt)
{
	int err;
	struct nct_evt evt = { .status = _mqtt_evt->result };
	struct nct_cc_data cc;
	struct nct_dc_data dc;
	bool event_notify = false;

#if defined(CONFIG_NRF_CLOUD_FOTA)
	err = nrf_cloud_fota_mqtt_evt_handler(_mqtt_evt);
	if (err == 0) {
		return;
	} else if (err < 0) {
		LOG_ERR("nrf_cloud_fota_mqtt_evt_handler: Failed! %d", err);
		return;
	}
#endif

	switch (_mqtt_evt->type) {
	case MQTT_EVT_CONNACK: {
		const struct mqtt_connack_param *p = &_mqtt_evt->param.connack;

		LOG_DBG("MQTT_EVT_CONNACK: result %d", _mqtt_evt->result);

		evt.param.flag = (p->session_present_flag != 0) && persistent_session;

		if (persistent_session && (p->session_present_flag == 0)) {
			/* Session not present, clear saved state */
			nct_save_session_state(0);
		}

		evt.status = translate_mqtt_connack_result(_mqtt_evt->result);
		evt.type = NCT_EVT_CONNECTED;
		event_notify = true;
		break;
	}
	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *p = &_mqtt_evt->param.publish;

		LOG_DBG("MQTT_EVT_PUBLISH: id = %d len = %d, topic = %.*s",
			p->message_id,
			p->message.payload.len,
			p->message.topic.topic.size,
			p->message.topic.topic.utf8);

		int err = publish_get_payload(mqtt_client,
					      p->message.payload.len);

		if (err < 0) {
			LOG_ERR("publish_get_payload: failed %d", err);
			(void)nrf_cloud_disconnect();
			event_notify = false;
			break;
		}

		/* Determine if this is a control channel or data channel topic event */
		if (nrf_cloud_cc_rx_topic_decode(&p->message.topic, &cc.opcode)) {
			cc.message_id = p->message_id;
			cc.data.ptr = nct.payload_buf;
			cc.data.len = p->message.payload.len;
			cc.topic.len = p->message.topic.topic.size;
			cc.topic.ptr = p->message.topic.topic.utf8;

			evt.type = NCT_EVT_CC_RX_DATA;
			evt.param.cc = &cc;
			event_notify = true;
		} else {
			/* Try to match it with one of the data topics. */
			dc.message_id = p->message_id;
			dc.data.ptr = nct.payload_buf;
			dc.data.len = p->message.payload.len;
			dc.topic.len = p->message.topic.topic.size;
			dc.topic.ptr = p->message.topic.topic.utf8;

			evt.type = NCT_EVT_DC_RX_DATA;
			evt.param.dc = &dc;
			event_notify = true;
		}

		if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			const struct mqtt_puback_param ack = {
				.message_id = p->message_id
			};

			/* Send acknowledgment. */
			mqtt_publish_qos1_ack(mqtt_client, &ack);
		}
		break;
	}
	case MQTT_EVT_SUBACK: {
		LOG_DBG("MQTT_EVT_SUBACK: id = %d result = %d",
			_mqtt_evt->param.suback.message_id, _mqtt_evt->result);

		evt.status = translate_mqtt_suback_result(_mqtt_evt->result);

		if (_mqtt_evt->param.suback.message_id == NCT_MSG_ID_CC_SUB) {
			evt.type = NCT_EVT_CC_CONNECTED;
			event_notify = true;
		}
		if (_mqtt_evt->param.suback.message_id == NCT_MSG_ID_DC_SUB) {
			evt.type = NCT_EVT_DC_CONNECTED;
			event_notify = true;

			/* Subscribing complete, session is now valid */
			err = nct_save_session_state(1);
			if (err) {
				LOG_ERR("Failed to save session state: %d",
					err);
			}
#if defined(CONFIG_NRF_CLOUD_FOTA)
			err = nrf_cloud_fota_subscribe();
			if (err) {
				LOG_ERR("FOTA MQTT subscribe failed: %d", err);
			}
#endif
		}
		break;
	}
	case MQTT_EVT_UNSUBACK: {
		LOG_DBG("MQTT_EVT_UNSUBACK");

		if (_mqtt_evt->param.suback.message_id == NCT_MSG_ID_CC_UNSUB) {
			evt.type = NCT_EVT_CC_DISCONNECTED;
			event_notify = true;
		}
		break;
	}
	case MQTT_EVT_PUBACK: {
		LOG_DBG("MQTT_EVT_PUBACK: id = %d result = %d",
			_mqtt_evt->param.puback.message_id, _mqtt_evt->result);

		evt.type = NCT_EVT_CC_TX_DATA_ACK;
		evt.param.message_id = _mqtt_evt->param.puback.message_id;
		event_notify = true;
		break;
	}
	case MQTT_EVT_PINGRESP: {
		LOG_DBG("MQTT_EVT_PINGRESP");

		evt.type = NCT_EVT_PINGRESP;
		event_notify = true;
		break;
	}
	case MQTT_EVT_DISCONNECT: {
		LOG_DBG("MQTT_EVT_DISCONNECT: result = %d", _mqtt_evt->result);

		evt.type = NCT_EVT_DISCONNECTED;
		event_notify = true;
		break;
	}
	default:
		break;
	}

	if (event_notify) {
		err = nct_input(&evt);

		if (err != 0) {
			LOG_ERR("nct_input: failed %d", err);
		}
	}
}

int nct_initialize(const struct nrf_cloud_init_param *param)
{
	int err;

	/* Perform settings and FOTA init first so that pending updates
	 * can be completed
	 */
	err = nct_settings_init();
	if (err) {
		return err;
	}

#if defined(CONFIG_NRF_CLOUD_FOTA)
	struct nrf_cloud_fota_init_param fota_init = {
		.evt_cb = nrf_cloud_fota_cb_handler,
		.smp_reset_cb = param->smp_reset_cb
	};

	err = nrf_cloud_fota_init(&fota_init);
	if (err < 0) {
		return err;
	} else if (err && persistent_session) {
		/* After a completed FOTA, use clean session */
		nct_save_session_state(0);
	}
#endif

	err = nct_client_id_set(param->client_id);
	if (err) {
		return err;
	}

	dc_endpoint_reset();

	err = nct_topics_populate();
	if (err) {
		return err;
	}

	return nct_provision();
}

void nct_uninit(void)
{
	LOG_DBG("Uninitializing nRF Cloud transport");
	dc_endpoint_free();
	nct_reset_topics();

	memset(&nct, 0, sizeof(nct));
	mqtt_client_initialized = false;
}


static void nct_init_mqtt(void)
{
	if (!mqtt_client_initialized) {
		mqtt_client_init(&nct.client);

		nct.client.evt_cb = nct_mqtt_evt_handler;
		nct.client.client_id.utf8 = (uint8_t *)client_id_ptr;
		nct.client.client_id.size = strlen(client_id_ptr);
		nct.client.protocol_version = MQTT_VERSION_3_1_1;
		nct.client.password = NULL;
		nct.client.user_name = NULL;
		nct.client.keepalive = CONFIG_NRF_CLOUD_MQTT_KEEPALIVE;
		nct.client.clean_session = persistent_session ? 0U : 1U;
		LOG_DBG("MQTT clean session flag: %u",
			nct.client.clean_session);

#if defined(CONFIG_MQTT_LIB_TLS)
		nct.client.transport.type = MQTT_TRANSPORT_SECURE;
		nct.client.rx_buf = nct.rx_buf;
		nct.client.rx_buf_size = sizeof(nct.rx_buf);
		nct.client.tx_buf = nct.tx_buf;
		nct.client.tx_buf_size = sizeof(nct.tx_buf);

		struct mqtt_sec_config *tls_config =
			   &nct.client.transport.tls.config;
		memcpy(tls_config, &nct.tls_config,
			   sizeof(struct mqtt_sec_config));
#else
		nct.client.transport.type = MQTT_TRANSPORT_NON_SECURE;
#endif
		mqtt_client_initialized = true;
	}
}

static int nct_connect_host_cb(struct sockaddr *const addr)
{
	int err;

	/* Initialize MQTT client if it is not already initialized. */
	nct_init_mqtt();

	/* Attempt to connect to the provided address */
	nct.client.broker = addr;
	err = mqtt_connect(&nct.client);

	if (err != 0) {
		LOG_DBG("mqtt_connect failed %d", err);
		return err;
	}

	/* Success, return the resulting socket. */
	return nct_socket_get();
}

int nct_connect(void)
{
	int err = 0;
	const char *const host_name = NRF_CLOUD_HOSTNAME;
	uint16_t port = htons(NRF_CLOUD_PORT);
	struct zsock_addrinfo hints = {
		.ai_socktype = SOCK_STREAM
	};
	int sock = nrf_cloud_connect_host(host_name, port, &hints, &nct_connect_host_cb);

	if (sock < 0) {
		LOG_ERR("Could not connect to nRF Cloud MQTT Broker %s, port: %d. err: %d",
			host_name, port, sock);
		return -ECONNREFUSED;
	}

	if (IS_ENABLED(CONFIG_NRF_CLOUD_SEND_NONBLOCKING)) {
		err = fcntl(sock, F_SETFL, O_NONBLOCK);
		if (err == -1) {
			LOG_ERR("Failed to set socket as non-blocking, err: %d",
				errno);
			LOG_WRN("Continuing with blocking socket");
			err = 0;
		} else {
			LOG_DBG("Using non-blocking socket");
		}
	}  else if (IS_ENABLED(CONFIG_NRF_CLOUD_SEND_TIMEOUT)) {
		struct timeval timeout = {
			.tv_sec = CONFIG_NRF_CLOUD_SEND_TIMEOUT_SEC
		};

		err = zsock_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
		if (err == -1) {
			LOG_ERR("Failed to set timeout, errno: %d", errno);
			err = 0;
		} else {
			LOG_DBG("Using socket send timeout of %d seconds",
				CONFIG_NRF_CLOUD_SEND_TIMEOUT_SEC);
		}
	}

	return err;
}

int nct_cc_connect(void)
{
	const struct mqtt_subscription_list subscription_list = {
		.list = (struct mqtt_topic *)&nct_cc_rx_list,
		.list_count = ARRAY_SIZE(nct_cc_rx_list),
		.message_id = NCT_MSG_ID_CC_SUB
	};

	LOG_DBG("Subscribing to:");
	for (int i = 0; i < subscription_list.list_count; i++) {
		LOG_DBG("%.*s", subscription_list.list[i].topic.size,
			(const char *)subscription_list.list[i].topic.utf8);
	}
	return mqtt_subscribe(&nct.client, &subscription_list);
}

int nct_set_keepalive(int seconds)
{
	nct.client.keepalive = seconds;
	return mqtt_ping(&nct.client);
}

int nct_cc_send(const struct nct_cc_data *cc_data)
{
	if (cc_data == NULL) {
		LOG_ERR("cc_data == NULL");
		return -EINVAL;
	}

	if (cc_data->opcode >= ARRAY_SIZE(nct_cc_tx_list)) {
		LOG_ERR("opcode = %d", cc_data->opcode);
		return -ENOTSUP;
	}

	struct mqtt_publish_param publish = {
		.message.topic.qos = nct_cc_tx_list[cc_data->opcode].qos,
		.message.topic.topic.size =
			nct_cc_tx_list[cc_data->opcode].topic.size,
		.message.topic.topic.utf8 =
			nct_cc_tx_list[cc_data->opcode].topic.utf8,
	};

	/* Populate payload. */
	if ((cc_data->data.len != 0) && (cc_data->data.ptr != NULL)) {
		publish.message.payload.data = (uint8_t *)cc_data->data.ptr,
		publish.message.payload.len = cc_data->data.len;
	}

	publish.message_id = get_message_id(cc_data->message_id);

	LOG_DBG("mqtt_publish: id = %d opcode = %d len = %d, topic: %.*s", publish.message_id,
		cc_data->opcode, cc_data->data.len,
		publish.message.topic.topic.size,
		publish.message.topic.topic.utf8);
	LOG_DBG("payload: %.*s", publish.message.payload.len,
	     publish.message.payload.data ? (const char *)publish.message.payload.data : "");
	int err = mqtt_publish(&nct.client, &publish);

	if (err) {
		LOG_ERR("mqtt_publish failed %d", err);
	}

	return err;
}

int nct_cc_disconnect(void)
{
	LOG_DBG("Unsubscribing");

	static const struct mqtt_subscription_list subscription_list = {
		.list = (struct mqtt_topic *)nct_cc_rx_list,
		.list_count = ARRAY_SIZE(nct_cc_rx_list),
		.message_id = NCT_MSG_ID_CC_UNSUB
	};

	return mqtt_unsubscribe(&nct.client, &subscription_list);
}

void nct_dc_endpoint_set(const struct nct_dc_endpoints *const eps)
{
	__ASSERT_NO_MSG(eps != NULL);

	LOG_DBG("Setting endpoints");

	/* In case the endpoint was previously set, free and reset before copying new one */
	dc_endpoint_free();

	nct.dc = *eps;

#if defined(CONFIG_NRF_CLOUD_FOTA)
	(void)nrf_cloud_fota_endpoint_set_and_report(&nct.client, client_id_ptr,
						     &nct.dc.e[DC_BASE]);
	if (persistent_session) {
		/* Check for updates since FOTA topics are already subscribed to */
		(void)nrf_cloud_fota_update_check();
	}
#endif
}

void nct_dc_endpoint_get(struct nct_dc_endpoints *const eps)
{
	__ASSERT_NO_MSG(eps != NULL);
	*eps = nct.dc;
}

int nct_dc_connect(void)
{
	struct mqtt_topic subscribe_topic = {
		.topic = nct.dc.e[DC_RX],
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = NCT_MSG_ID_DC_SUB
	};

#if defined(CONFIG_NRF_CLOUD_GATEWAY)
	/** The topic for cloud to device we receive in the shadow for
	 * BLE gateway devices is incorrect. The <device_id>/+/r suffix is invalid and
	 * must be corrected here to be <device_id>/c2g. Using it without this
	 * change results in immediate disconnection from nRF Cloud.
	 */
	char *p = (char *)subscribe_topic.topic.utf8;
	int l = subscribe_topic.topic.size;

	memcpy(&p[l - 3], NRF_CLOUD_JSON_KEY_CLOUD_TO_DEVICE, 3);
#endif

	LOG_DBG("Subscribing to:");
	for (int i = 0; i < subscription_list.list_count; i++) {
		LOG_DBG("%.*s", subscription_list.list[i].topic.size,
			(const char *)subscription_list.list[i].topic.utf8);
	}
	return mqtt_subscribe(&nct.client, &subscription_list);
}

int nct_dc_send(const struct nct_dc_data *dc_data)
{
	return endp_send(dc_data, &nct.dc.e[DC_TX], MQTT_QOS_1_AT_LEAST_ONCE);
}

int nct_dc_stream(const struct nct_dc_data *dc_data)
{
	return endp_send(dc_data, &nct.dc.e[DC_TX], MQTT_QOS_0_AT_MOST_ONCE);
}

int nct_dc_bulk_send(const struct nct_dc_data *dc_data, enum mqtt_qos qos)
{
	return endp_send(dc_data, &nct.dc.e[DC_BULK], qos);
}

int nct_dc_bin_send(const struct nct_dc_data *dc_data, enum mqtt_qos qos)
{
	return endp_send(dc_data, &nct.dc.e[DC_BIN], qos);
}

int nct_dc_disconnect(void)
{
	int ret;

	LOG_DBG("Unsubscribing");

	struct mqtt_topic subscribe_topic = {
		.topic = nct.dc.e[DC_RX],
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = NCT_MSG_ID_DC_UNSUB
	};

	ret = mqtt_unsubscribe(&nct.client, &subscription_list);

#if defined(CONFIG_NRF_CLOUD_FOTA)
	int err = nrf_cloud_fota_unsubscribe();

	if (err) {
		LOG_ERR("FOTA MQTT unsubscribe failed: %d", err);
		if (ret == 0) {
			ret = err;
		}
	}
#endif

	return ret;
}

int nct_disconnect(void)
{
	LOG_DBG("Disconnecting");

	dc_endpoint_free();
	return mqtt_disconnect(&nct.client, NULL);
}

int nct_process(void)
{
	int err;
	int ret;

	err = mqtt_input(&nct.client);
	if (err) {
		LOG_ERR("MQTT input error: %d", err);
		if (err != -ENOTCONN) {
			return err;
		}
	} else if (nct.client.unacked_ping > 1) {
		LOG_DBG("Previous MQTT ping not acknowledged");
		err = -ECONNRESET;
	} else {
		err = mqtt_live(&nct.client);
		if (err && (err != -EAGAIN)) {
			LOG_ERR("MQTT ping error: %d", err);
		} else {
			return err;
		}
	}

	ret = nct_disconnect();
	if (ret) {
		LOG_ERR("Error disconnecting from cloud: %d", ret);
	}

	struct nct_evt evt = { .status = err };

	evt.type = NCT_EVT_DISCONNECTED;
	ret = nct_input(&evt);
	if (ret) {
		LOG_ERR("Error sending event to application: %d", err);
		err = ret;
	}
	return err;
}

int nct_keepalive_time_left(void)
{
	return mqtt_keepalive_time_left(&nct.client);
}

int nct_socket_get(void)
{
	return nct.client.transport.tls.sock;
}
