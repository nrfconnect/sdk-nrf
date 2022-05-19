/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf_cloud_transport.h"
#include "nrf_cloud_mem.h"
#include "nrf_cloud_client_id.h"
#if defined(CONFIG_NRF_CLOUD_FOTA)
#include "nrf_cloud_fota.h"
#endif

#include <zephyr/kernel.h>
#include <stdio.h>
#include <fcntl.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/settings/settings.h>
#if defined(CONFIG_NRF_MODEM_LIB)
#if defined(CONFIG_POSIX_API)
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/sys/socket.h>
#else
#include <zephyr/net/socket.h>
#endif
#endif /* defined(CONFIG_NRF_MODEM_LIB) */

LOG_MODULE_REGISTER(nrf_cloud_transport, CONFIG_NRF_CLOUD_LOG_LEVEL);

#if defined(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES)
#include CONFIG_NRF_CLOUD_CERTIFICATES_FILE
#if defined(CONFIG_MODEM_KEY_MGMT)
#include <modem/modem_key_mgmt.h>
#endif
#endif /* defined(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES) */

#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME)
BUILD_ASSERT((sizeof(CONFIG_NRF_CLOUD_CLIENT_ID) - 1) <= NRF_CLOUD_CLIENT_ID_MAX_LEN,
	"CONFIG_NRF_CLOUD_CLIENT_ID must not exceed NRF_CLOUD_CLIENT_ID_MAX_LEN");
#endif

#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI)
#define CGSN_RESPONSE_LENGTH 19
#define NRF_IMEI_LEN 15
#define IMEI_CLIENT_ID_LEN (sizeof(CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX) \
			    - 1 + NRF_IMEI_LEN)
BUILD_ASSERT(IMEI_CLIENT_ID_LEN <= NRF_CLOUD_CLIENT_ID_MAX_LEN,
	"NRF_CLOUD_CLIENT_ID_PREFIX plus IMEI must not exceed NRF_CLOUD_CLIENT_ID_MAX_LEN");
#endif

#define NRF_CLOUD_HOSTNAME CONFIG_NRF_CLOUD_HOST_NAME
#define NRF_CLOUD_PORT CONFIG_NRF_CLOUD_PORT

#if defined(CONFIG_NRF_CLOUD_IPV6)
#define NRF_CLOUD_AF_FAMILY AF_INET6
#else
#define NRF_CLOUD_AF_FAMILY AF_INET
#endif /* defined(CONFIG_NRF_CLOUD_IPV6) */

#define AWS "$aws/things/"
/*
 * Note that this topic is intentionally not using the AWS Shadow get/accepted
 * topic ("$aws/things/<deviceId>/shadow/get/accepted").
 * Messages on the AWS topic contain the entire shadow, including metadata and
 * they can become too large for the modem to handle.
 * Messages on the topic below are published by nRF Connect for Cloud and
 * contain only a part of the original message so it can be received by the
 * device.
 */
#define NCT_ACCEPTED_TOPIC "%s/shadow/get/accepted"
#define NCT_REJECTED_TOPIC AWS "%s/shadow/get/rejected"
#define NCT_UPDATE_DELTA_TOPIC AWS "%s/shadow/update/delta"
#define NCT_UPDATE_TOPIC AWS "%s/shadow/update"
#define NCT_SHADOW_GET AWS "%s/shadow/get"

/* Buffers to hold stage and tenant strings. */
static char stage[NRF_CLOUD_STAGE_ID_MAX_LEN];
static char tenant[NRF_CLOUD_TENANT_ID_MAX_LEN];

/* Null-terminated MQTT client ID */
static char *client_id_buf;

/* Buffers for keeping the topics for nrf_cloud */
static char *accepted_topic;
static char *rejected_topic;
static char *update_delta_topic;
static char *update_topic;
static char *shadow_get_topic;

static bool mqtt_client_initialized;
static bool persistent_session;

#define NCT_RX_LIST 0
#define NCT_TX_LIST 1

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
	struct sockaddr_storage broker;
	struct mqtt_utf8 dc_tx_endp;
	struct mqtt_utf8 dc_rx_endp;
	struct mqtt_utf8 dc_m_endp;
	struct mqtt_utf8 dc_bulk_endp;
	uint16_t message_id;
	uint8_t rx_buf[CONFIG_NRF_CLOUD_MQTT_MESSAGE_BUFFER_LEN];
	uint8_t tx_buf[CONFIG_NRF_CLOUD_MQTT_MESSAGE_BUFFER_LEN];
	uint8_t payload_buf[CONFIG_NRF_CLOUD_MQTT_PAYLOAD_BUFFER_LEN + 1];
} nct;

#define CC_RX_LIST_CNT 3
static struct mqtt_topic nct_cc_rx_list[CC_RX_LIST_CNT];
#define CC_TX_LIST_CNT 2
static struct mqtt_topic nct_cc_tx_list[CC_TX_LIST_CNT];

static uint32_t const nct_cc_rx_opcode_map[] = {
	NCT_CC_OPCODE_UPDATE_REQ,
	NCT_CC_OPCODE_UPDATE_REJECT_RSP,
	NCT_CC_OPCODE_UPDATE_ACCEPT_RSP
};

/* Internal routine to reset data endpoint information. */
static void dc_endpoint_reset(void)
{
	nct.dc_rx_endp.utf8 = NULL;
	nct.dc_rx_endp.size = 0;

	nct.dc_tx_endp.utf8 = NULL;
	nct.dc_tx_endp.size = 0;

	nct.dc_m_endp.utf8 = NULL;
	nct.dc_m_endp.size = 0;

	nct.dc_bulk_endp.utf8 = NULL;
	nct.dc_bulk_endp.size = 0;
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
 * Casting away const for rx, tx, and m seems to be OK because the
 * nct_dc_endpoint_set() caller gets the buffers from
 * json_decode_and_alloc(), which uses nrf_cloud_malloc() to call
 * k_malloc().
 */
static void dc_endpoint_free(void)
{
	if (nct.dc_rx_endp.utf8 != NULL) {
		nrf_cloud_free((void *)nct.dc_rx_endp.utf8);
	}
	if (nct.dc_tx_endp.utf8 != NULL) {
		nrf_cloud_free((void *)nct.dc_tx_endp.utf8);
	}
	if (nct.dc_m_endp.utf8 != NULL) {
		nrf_cloud_free((void *)nct.dc_m_endp.utf8);
	}
	if (nct.dc_bulk_endp.utf8 != NULL) {
		nrf_cloud_free((void *)nct.dc_bulk_endp.utf8);
	}

	dc_endpoint_reset();
#if defined(CONFIG_NRF_CLOUD_FOTA)
	nrf_cloud_fota_endpoint_clear();
#endif
}

static uint32_t dc_send(const struct nct_dc_data *dc_data, uint8_t qos)
{
	if (dc_data == NULL) {
		return -EINVAL;
	}

	struct mqtt_publish_param publish = {
		.message_id = 0,
		.message.topic.qos = qos,
		.message.topic.topic.size = nct.dc_tx_endp.size,
		.message.topic.topic.utf8 = nct.dc_tx_endp.utf8,
	};

	/* Populate payload. */
	if ((dc_data->data.len != 0) && (dc_data->data.ptr != NULL)) {
		publish.message.payload.data = (uint8_t *)dc_data->data.ptr;
		publish.message.payload.len = dc_data->data.len;
	}

	if (qos != MQTT_QOS_0_AT_MOST_ONCE) {
		publish.message_id = get_message_id(dc_data->message_id);
	}

	return mqtt_publish(&nct.client, &publish);
}

static int bulk_send(const struct nct_dc_data *dc_data, enum mqtt_qos qos)
{
	if (dc_data == NULL) {
		LOG_DBG("Passed in structure cannot be NULL");
		return -EINVAL;
	}

	if (qos != MQTT_QOS_0_AT_MOST_ONCE && qos != MQTT_QOS_1_AT_LEAST_ONCE) {
		LOG_DBG("Unsupported MQTT QoS level");
		return -EINVAL;
	}

	struct mqtt_publish_param publish = {
		.message.topic.qos = qos,
		.message.topic.topic.size = nct.dc_bulk_endp.size,
		.message.topic.topic.utf8 = nct.dc_bulk_endp.utf8,
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

	return mqtt_publish(&nct.client, &publish);
}

static bool strings_compare(const char *s1, const char *s2, uint32_t s1_len,
			    uint32_t s2_len)
{
	return (strncmp(s1, s2, MIN(s1_len, s2_len))) ? false : true;
}

/* Verify if the topic is a control channel topic or not. */
static bool control_channel_topic_match(uint32_t list_id,
					const struct mqtt_topic *topic,
					enum nct_cc_opcode *opcode)
{
	struct mqtt_topic *topic_list;
	uint32_t list_size;

	if (list_id == NCT_RX_LIST) {
		topic_list = (struct mqtt_topic *)nct_cc_rx_list;
		list_size = ARRAY_SIZE(nct_cc_rx_list);
	} else if (list_id == NCT_TX_LIST) {
		topic_list = (struct mqtt_topic *)nct_cc_tx_list;
		list_size = ARRAY_SIZE(nct_cc_tx_list);
	} else {
		return false;
	}

	for (uint32_t index = 0; index < list_size; index++) {
		if (strings_compare(
			    topic->topic.utf8, topic_list[index].topic.utf8,
			    topic->topic.size, topic_list[index].topic.size)) {
			*opcode = nct_cc_rx_opcode_map[index];
			return true;
		}
	}
	return false;
}

/* Function to set/generate the MQTT client ID */
static int nct_client_id_set(const char * const client_id)
{
	int ret;
	size_t len;

	if (IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME)) {
		if (client_id) {
			len = strlen(client_id);
		} else {
			return -EINVAL;
		}
	} else {
		if (client_id) {
			LOG_WRN("Not configured to for runtime client ID, ignoring");
		}
		len = nrf_cloud_configured_client_id_length_get();
	}

	if (!len) {
		LOG_WRN("Could not determine size of client ID");
		return -ENOMSG;
	}

	if (client_id_buf) {
		nrf_cloud_free(client_id_buf);
		client_id_buf = NULL;
	}

	/* Add one for NULL terminator */
	++len;

	client_id_buf = nrf_cloud_calloc(len, 1);
	if (!client_id_buf) {
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME)) {
		strncpy(client_id_buf, client_id, len);
	} else {
		ret = nrf_cloud_configured_client_id_get(client_id_buf, len);
		if (ret) {
			LOG_ERR("Could not obtain configured client ID, error: %d", ret);
			return ret;
		}
	}

	LOG_DBG("client_id = %s", log_strdup(client_id_buf));

	return 0;
}

int nct_client_id_get(char *id, size_t id_len)
{
	if (!client_id_buf) {
		return -ENODEV;
	} else if (!id || !id_len) {
		return -EINVAL;
	}

	size_t len = strlen(client_id_buf);

	if (id_len <= len) {
		return -EMSGSIZE;
	}

	strncpy(id, client_id_buf, id_len);
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

static int allocate_and_format_topic(char **topic_buf, const char * const topic_template)
{
	int ret;
	size_t topic_sz;
	const size_t client_sz = strlen(client_id_buf);

	topic_sz = client_sz + strlen(topic_template) - 1;

	*topic_buf = nrf_cloud_calloc(topic_sz, 1);
	if (!*topic_buf) {
		return -ENOMEM;
	}
	ret = snprintk(*topic_buf, topic_sz,
		       topic_template, client_id_buf);
	if (ret <= 0 || ret >= topic_sz) {
		nrf_cloud_free(*topic_buf);
		return -EIO;
	}

	return 0;
}

static void nct_reset_topics(void)
{
	if (accepted_topic) {
		nrf_cloud_free(accepted_topic);
		accepted_topic = NULL;
	}
	if (rejected_topic) {
		nrf_cloud_free(rejected_topic);
		rejected_topic = NULL;
	}
	if (update_delta_topic) {
		nrf_cloud_free(update_delta_topic);
		update_delta_topic = NULL;
	}
	if (update_topic) {
		nrf_cloud_free(update_topic);
		update_topic = NULL;
	}
	if (shadow_get_topic) {
		nrf_cloud_free(shadow_get_topic);
		shadow_get_topic = NULL;
	}

	memset(nct_cc_rx_list, 0, sizeof(nct_cc_rx_list[0]) * CC_RX_LIST_CNT);
	memset(nct_cc_tx_list, 0, sizeof(nct_cc_tx_list[0]) * CC_TX_LIST_CNT);
}

static void nct_topic_lists_populate(void)
{
	/* Add RX topics */
	nct_cc_rx_list[0].qos = MQTT_QOS_1_AT_LEAST_ONCE;
	nct_cc_rx_list[0].topic.utf8 = accepted_topic;
	nct_cc_rx_list[0].topic.size = strlen(accepted_topic);

	nct_cc_rx_list[1].qos = MQTT_QOS_1_AT_LEAST_ONCE;
	nct_cc_rx_list[1].topic.utf8 = rejected_topic;
	nct_cc_rx_list[1].topic.size = strlen(rejected_topic);

	nct_cc_rx_list[2].qos = MQTT_QOS_1_AT_LEAST_ONCE;
	nct_cc_rx_list[2].topic.utf8 = update_delta_topic;
	nct_cc_rx_list[2].topic.size = strlen(update_delta_topic);

	/* Add TX topics */
	nct_cc_tx_list[0].qos = MQTT_QOS_1_AT_LEAST_ONCE;
	nct_cc_tx_list[0].topic.utf8 = shadow_get_topic;
	nct_cc_tx_list[0].topic.size = strlen(shadow_get_topic);

	nct_cc_tx_list[1].qos = MQTT_QOS_1_AT_LEAST_ONCE;
	nct_cc_tx_list[1].topic.utf8 = update_topic;
	nct_cc_tx_list[1].topic.size = strlen(update_topic);
}

static int nct_topics_populate(void)
{
	if (!client_id_buf) {
		return -ENODEV;
	}

	int ret;

	nct_reset_topics();

	ret = allocate_and_format_topic(&accepted_topic, NCT_ACCEPTED_TOPIC);
	if (ret) {
		goto err_cleanup;
	}
	ret = allocate_and_format_topic(&rejected_topic, NCT_REJECTED_TOPIC);
	if (ret) {
		goto err_cleanup;
	}
	ret = allocate_and_format_topic(&update_delta_topic, NCT_UPDATE_DELTA_TOPIC);
	if (ret) {
		goto err_cleanup;
	}
	ret = allocate_and_format_topic(&update_topic, NCT_UPDATE_TOPIC);
	if (ret) {
		goto err_cleanup;
	}
	ret = allocate_and_format_topic(&shadow_get_topic, NCT_SHADOW_GET);
	if (ret) {
		goto err_cleanup;
	}

	LOG_DBG("accepted_topic: %s", log_strdup(accepted_topic));
	LOG_DBG("rejected_topic: %s", log_strdup(rejected_topic));
	LOG_DBG("update_delta_topic: %s", log_strdup(update_delta_topic));
	LOG_DBG("update_topic: %s", log_strdup(update_topic));
	LOG_DBG("shadow_get_topic: %s", log_strdup(shadow_get_topic));

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
	static sec_tag_t sec_tag_list[] = { CONFIG_NRF_CLOUD_SEC_TAG };

	nct.tls_config.peer_verify = 2;
	nct.tls_config.cipher_count = 0;
	nct.tls_config.cipher_list = NULL;
	nct.tls_config.sec_tag_count = ARRAY_SIZE(sec_tag_list);
	nct.tls_config.sec_tag_list = sec_tag_list;
	nct.tls_config.hostname = NRF_CLOUD_HOSTNAME;

#if defined(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES)
#if defined(CONFIG_NRF_MODEM_LIB)
	{
		int err;

		/* Delete certificates */
		nrf_sec_tag_t sec_tag = CONFIG_NRF_CLOUD_SEC_TAG;

		for (enum modem_key_mgmt_cred_type type = 0; type < 5;
		     type++) {
			err = modem_key_mgmt_delete(sec_tag, type);
			LOG_DBG("modem_key_mgmt_delete(%u, %d) => result = %d",
				sec_tag, type, err);
		}

		/* Provision CA Certificate. */
		err = modem_key_mgmt_write(CONFIG_NRF_CLOUD_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					   ca_certificate,
					   strlen(ca_certificate));
		if (err) {
			LOG_ERR("ca_certificate err: %d", err);
			return err;
		}

		/* Provision Private Certificate. */
		err = modem_key_mgmt_write(
			CONFIG_NRF_CLOUD_SEC_TAG,
			MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
			private_key,
			strlen(private_key));
		if (err) {
			LOG_ERR("private_key err: %d", err);
			return err;
		}

		/* Provision Public Certificate. */
		err = modem_key_mgmt_write(
			CONFIG_NRF_CLOUD_SEC_TAG,
			MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
			device_certificate,
			strlen(device_certificate));
		if (err) {
			LOG_ERR("device_certificate err: %d",
				err);
			return err;
		}
	}
#else
	{
		int err;

		err = tls_credential_add(CONFIG_NRF_CLOUD_SEC_TAG,
					 TLS_CREDENTIAL_CA_CERTIFICATE,
					 ca_certificate,
					 sizeof(ca_certificate));
		if (err < 0) {
			LOG_ERR("Failed to register ca certificate: %d", err);
			return err;
		}
		err = tls_credential_add(CONFIG_NRF_CLOUD_SEC_TAG,
					 TLS_CREDENTIAL_PRIVATE_KEY,
					 private_key,
					 sizeof(private_key));
		if (err < 0) {
			LOG_ERR("Failed to register private key: %d", err);
			return err;
		}
		err = tls_credential_add(
			CONFIG_NRF_CLOUD_SEC_TAG,
			TLS_CREDENTIAL_SERVER_CERTIFICATE,
			device_certificate,
			sizeof(device_certificate));
		if (err < 0) {
			LOG_ERR("Failed to register public certificate: %d",
				err);
			return err;
		}
	}
#endif /* defined(CONFIG_NRF_MODEM_LIB) */
#endif /* defined(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES) */

	return 0;
}

static int nct_settings_set(const char *key, size_t len_rd,
			    settings_read_cb read_cb, void *cb_arg)
{
	if (!key) {
		return -EINVAL;
	}

	int read_val;

	LOG_DBG("Settings key: %s, size: %d", log_strdup(key), len_rd);

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
	case NRF_CLOUD_FOTA_EVT_ERASE_DONE: {
		LOG_DBG("NRF_CLOUD_FOTA_EVT_ERASE_DONE");
		break;
	}
	case NRF_CLOUD_FOTA_EVT_DL_PROGRESS: {
		LOG_DBG("NRF_CLOUD_FOTA_EVT_DL_PROGRESS");
		break;
	}
	default: {
		break;
	}
	}
}
#endif

/* Connect to MQTT broker. */
int nct_mqtt_connect(void)
{
	int err;
	if (!mqtt_client_initialized) {

		mqtt_client_init(&nct.client);

		nct.client.broker = (struct sockaddr *)&nct.broker;
		nct.client.evt_cb = nct_mqtt_evt_handler;
		nct.client.client_id.utf8 = (uint8_t *)client_id_buf;
		nct.client.client_id.size = strlen(client_id_buf);
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

	err = mqtt_connect(&nct.client);
	if (err != 0) {
		LOG_DBG("mqtt_connect failed %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_NRF_CLOUD_SEND_NONBLOCKING)) {
		err = fcntl(nct_socket_get(), F_SETFL, O_NONBLOCK);
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

		err = setsockopt(nct_socket_get(), SOL_SOCKET, SO_SNDTIMEO,
				 &timeout, sizeof(timeout));
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

		evt.param.flag = (p->session_present_flag != 0) &&
				 persistent_session;

		if (persistent_session && (p->session_present_flag == 0)) {
			/* Session not present, clear saved state */
			nct_save_session_state(0);
		}

		evt.type = NCT_EVT_CONNECTED;
		event_notify = true;
		break;
	}
	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *p = &_mqtt_evt->param.publish;

		LOG_DBG("MQTT_EVT_PUBLISH: id = %d len = %d",
			p->message_id,
			p->message.payload.len);

		int err = publish_get_payload(mqtt_client,
					      p->message.payload.len);

		if (err < 0) {
			LOG_ERR("publish_get_payload: failed %d", err);
			nrf_cloud_disconnect();
			event_notify = false;
			break;
		}

		/* If the data arrives on one of the subscribed control channel
		 * topic. Then we notify the same.
		 */
		if (control_channel_topic_match(NCT_RX_LIST, &p->message.topic,
						&cc.opcode)) {
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

int nct_init(const char * const client_id)
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
	err = nrf_cloud_fota_init(nrf_cloud_fota_cb_handler);
	if (err < 0) {
		return err;
	} else if (err && persistent_session) {
		/* After a completed FOTA, use clean session */
		nct_save_session_state(0);
	}
#endif
	err = nct_client_id_set(client_id);
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

	if (client_id_buf) {
		nrf_cloud_free(client_id_buf);
		client_id_buf = NULL;
	}

	memset(&nct, 0, sizeof(nct));
	mqtt_client_initialized = false;
}

#if defined(CONFIG_NRF_CLOUD_STATIC_IPV4)
int nct_connect(void)
{
	int err;

	struct sockaddr_in *broker = ((struct sockaddr_in *)&nct.broker);

	inet_pton(AF_INET, CONFIG_NRF_CLOUD_STATIC_IPV4_ADDR,
		  &broker->sin_addr);
	broker->sin_family = AF_INET;
	broker->sin_port = htons(NRF_CLOUD_PORT);

	LOG_DBG("IPv4 Address %s", CONFIG_NRF_CLOUD_STATIC_IPV4_ADDR);
	err = nct_mqtt_connect();

	return err;
}
#else
int nct_connect(void)
{
	int err;
	struct addrinfo *result;
	struct addrinfo *addr;
	struct addrinfo hints = {
		.ai_family = NRF_CLOUD_AF_FAMILY,
		.ai_socktype = SOCK_STREAM
	};

	err = getaddrinfo(NRF_CLOUD_HOSTNAME, NULL, &hints, &result);
	if (err) {
		LOG_DBG("getaddrinfo failed %d", err);
		return -ECHILD;
	}

	addr = result;
	err = -ECHILD;

	/* Look for address of the broker. */
	while (addr != NULL) {
		/* IPv4 Address. */
		if ((addr->ai_addrlen == sizeof(struct sockaddr_in)) &&
		    (NRF_CLOUD_AF_FAMILY == AF_INET)) {
			char addr_str[INET_ADDRSTRLEN];
			struct sockaddr_in *broker =
				((struct sockaddr_in *)&nct.broker);

			broker->sin_addr.s_addr =
				((struct sockaddr_in *)addr->ai_addr)
					->sin_addr.s_addr;
			broker->sin_family = AF_INET;
			broker->sin_port = htons(NRF_CLOUD_PORT);

			inet_ntop(AF_INET,
				 &broker->sin_addr.s_addr,
				 addr_str,
				 sizeof(addr_str));

			LOG_DBG("IPv4 address: %s", log_strdup(addr_str));

			err = nct_mqtt_connect();
			break;
		} else if ((addr->ai_addrlen == sizeof(struct sockaddr_in6)) &&
			   (NRF_CLOUD_AF_FAMILY == AF_INET6)) {
			/* IPv6 Address. */
			struct sockaddr_in6 *broker =
				((struct sockaddr_in6 *)&nct.broker);

			memcpy(broker->sin6_addr.s6_addr,
			       ((struct sockaddr_in6 *)addr->ai_addr)
				       ->sin6_addr.s6_addr,
			       sizeof(struct in6_addr));
			broker->sin6_family = AF_INET6;
			broker->sin6_port = htons(NRF_CLOUD_PORT);

			LOG_DBG("IPv6 Address");
			err = nct_mqtt_connect();
			break;
		} else {
			LOG_DBG("ai_addrlen = %u should be %u or %u",
				(unsigned int)addr->ai_addrlen,
				(unsigned int)sizeof(struct sockaddr_in),
				(unsigned int)sizeof(struct sockaddr_in6));
		}

		addr = addr->ai_next;
	}

	/* Free the address. */
	freeaddrinfo(result);

	return err;
}
#endif /* defined(CONFIG_NRF_CLOUD_STATIC_IPV4) */

int nct_cc_connect(void)
{
	LOG_DBG("nct_cc_connect");

	const struct mqtt_subscription_list subscription_list = {
		.list = (struct mqtt_topic *)&nct_cc_rx_list,
		.list_count = ARRAY_SIZE(nct_cc_rx_list),
		.message_id = NCT_MSG_ID_CC_SUB
	};

	return mqtt_subscribe(&nct.client, &subscription_list);
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

	LOG_DBG("mqtt_publish: id = %d opcode = %d len = %d", publish.message_id,
		cc_data->opcode, cc_data->data.len);

	int err = mqtt_publish(&nct.client, &publish);

	if (err) {
		LOG_ERR("mqtt_publish failed %d", err);
	}

	return err;
}

int nct_cc_disconnect(void)
{
	LOG_DBG("nct_cc_disconnect");

	static const struct mqtt_subscription_list subscription_list = {
		.list = (struct mqtt_topic *)nct_cc_rx_list,
		.list_count = ARRAY_SIZE(nct_cc_rx_list),
		.message_id = NCT_MSG_ID_CC_UNSUB
	};

	return mqtt_unsubscribe(&nct.client, &subscription_list);
}

void nct_dc_endpoint_set(const struct nrf_cloud_data *tx_endp,
			 const struct nrf_cloud_data *rx_endp,
			 const struct nrf_cloud_data *bulk_endp,
			 const struct nrf_cloud_data *m_endp)
{
	LOG_DBG("nct_dc_endpoint_set");

	/* In case the endpoint was previous set, free and reset
	 * before copying new one.
	 */
	dc_endpoint_free();

	nct.dc_tx_endp.utf8 = (const uint8_t *)tx_endp->ptr;
	nct.dc_tx_endp.size = tx_endp->len;

	nct.dc_rx_endp.utf8 = (const uint8_t *)rx_endp->ptr;
	nct.dc_rx_endp.size = rx_endp->len;

	nct.dc_bulk_endp.utf8 = (const uint8_t *)bulk_endp->ptr;
	nct.dc_bulk_endp.size = bulk_endp->len;

	if (m_endp != NULL) {
		nct.dc_m_endp.utf8 = (const uint8_t *)m_endp->ptr;
		nct.dc_m_endp.size = m_endp->len;
#if defined(CONFIG_NRF_CLOUD_FOTA)
		(void)nrf_cloud_fota_endpoint_set_and_report(&nct.client,
			client_id_buf, &nct.dc_m_endp);
		if (persistent_session) {
			/* Check for updates since FOTA topics are
			 * already subscribed to.
			 */
			(void)nrf_cloud_fota_update_check();
		}
#endif
	}
}

void nct_dc_endpoint_get(struct nrf_cloud_data *const tx_endp,
			 struct nrf_cloud_data *const rx_endp,
			 struct nrf_cloud_data *const bulk_endp,
			 struct nrf_cloud_data *const m_endp)
{
	LOG_DBG("nct_dc_endpoint_get");

	tx_endp->ptr = nct.dc_tx_endp.utf8;
	tx_endp->len = nct.dc_tx_endp.size;

	rx_endp->ptr = nct.dc_rx_endp.utf8;
	rx_endp->len = nct.dc_rx_endp.size;

	if (bulk_endp != NULL) {
		bulk_endp->ptr = nct.dc_bulk_endp.utf8;
		bulk_endp->len = nct.dc_bulk_endp.size;
	}

	if (m_endp != NULL) {
		m_endp->ptr = nct.dc_m_endp.utf8;
		m_endp->len = nct.dc_m_endp.size;
	}
}

int nct_dc_connect(void)
{
	LOG_DBG("nct_dc_connect");

	struct mqtt_topic subscribe_topic = {
		.topic = {
			.utf8 = nct.dc_rx_endp.utf8,
			.size = nct.dc_rx_endp.size
		},
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = NCT_MSG_ID_DC_SUB
	};

	return mqtt_subscribe(&nct.client, &subscription_list);
}

int nct_dc_send(const struct nct_dc_data *dc_data)
{
	return dc_send(dc_data, MQTT_QOS_1_AT_LEAST_ONCE);
}

int nct_dc_stream(const struct nct_dc_data *dc_data)
{
	return dc_send(dc_data, MQTT_QOS_0_AT_MOST_ONCE);
}

int nct_dc_bulk_send(const struct nct_dc_data *dc_data, enum mqtt_qos qos)
{
	return bulk_send(dc_data, qos);
}

int nct_dc_disconnect(void)
{
	int ret;

	LOG_DBG("nct_dc_disconnect");

	const struct mqtt_subscription_list subscription_list = {
		.list = (struct mqtt_topic *)&nct.dc_rx_endp,
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
	LOG_DBG("nct_disconnect");

	dc_endpoint_free();
	return mqtt_disconnect(&nct.client);
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
	} else if (nct.client.unacked_ping) {
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
