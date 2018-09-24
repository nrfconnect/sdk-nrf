/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "nrf_cloud_transport.h"
#include "nrf_cloud_mem.h"

#include <zephyr.h>
#include <net/mqtt_socket.h>
#include <net/socket.h>
#include "nrf_inbuilt_key.h"

#include <misc/util.h>

#if defined(CONFIG_NRF_CLOUD_LOG)
#if !defined(LOG_LEVEL)
	#define LOG_LEVEL CONFIG_NRF_CLOUD_LOG_LEVEL
#elif LOG_LEVEL < CONFIG_NRF_CLOUD_LOG_LEVEL
	#undef LOG_LEVEL
	#define LOG_LEVEL CONFIG_NRF_CLOUD_LOG_LEVEL
#endif
#endif /* defined(CONFIG_NRF_CLOUD_LOG) */

#include <logging/log.h>

LOG_MODULE_REGISTER(nrf_cloud_transport);

#if defined(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES)
#include CONFIG_NRF_CLOUD_CERTIFICATES_FILE
#endif /* defined(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES) */

#if !defined(NRF_CLOUD_CLIENT_ID)
#define NRF_CLOUD_CLIENT_ID "my-client-id"
#endif

#define NRF_CLOUD_HOSTNAME CONFIG_NRF_CLOUD_HOST_NAME
#define NRF_CLOUD_PORT CONFIG_NRF_CLOUD_PORT

#define AWS "$aws/things/"

#define NCT_SHADOW_BASE_TOPIC  AWS NRF_CLOUD_CLIENT_ID "/shadow"
#define NCT_ACCEPTED_TOPIC  AWS NRF_CLOUD_CLIENT_ID "/shadow/get/accepted"
#define NCT_REJECTED_TOPIC  AWS NRF_CLOUD_CLIENT_ID "/shadow/get/rejected"
#define NCT_UPDATE_DELTA_TOPIC  AWS NRF_CLOUD_CLIENT_ID "/shadow/update/delta"
#define NCT_UPDATE_TOPIC  AWS NRF_CLOUD_CLIENT_ID "/shadow/update"
#define NCT_SHADOW_GET  AWS NRF_CLOUD_CLIENT_ID "/shadow/get"

#define NCT_CC_SUBSCRIBE_ID 1234
#define NCT_DC_SUBSCRIBE_ID 8765

#define NCT_RX_LIST 0
#define NCT_TX_LIST 1

/* Forward declaration of the event handler registered with MQTT. */
static void nct_mqtt_evt_handler(struct mqtt_client *client,
				 const struct mqtt_evt *evt);

/* nrf_cloud transport instance. */
static struct nct {
	struct mqtt_sec_config tls_config;
	struct mqtt_client client;
	struct sockaddr_in broker;
	struct mqtt_utf8 dc_tx_endp;
	struct mqtt_utf8 dc_rx_endp;
	u32_t message_id;
} nct;

static const struct mqtt_topic nct_cc_rx_list[] = {
	{
		.topic = {
			.utf8 = NCT_ACCEPTED_TOPIC,
			.size = (sizeof(NCT_ACCEPTED_TOPIC) - 1)
		},
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	},
	{
		.topic = {
			.utf8 = NCT_REJECTED_TOPIC,
			.size = (sizeof(NCT_REJECTED_TOPIC) - 1)
		},
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	},
	{
		.topic = {
			.utf8 = NCT_UPDATE_DELTA_TOPIC,
			.size = (sizeof(NCT_UPDATE_DELTA_TOPIC) - 1)
		},
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	}
};

static const struct mqtt_topic nct_cc_tx_list[] = {
	{
		.topic = {
			.utf8 = NCT_SHADOW_GET,
			.size = (sizeof(NCT_SHADOW_GET) - 1)
		},
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	},
	{
		.topic = {
			.utf8 = NCT_UPDATE_TOPIC,
			.size = (sizeof(NCT_UPDATE_TOPIC) - 1)
		},
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	}
};

static u32_t const nct_cc_rx_opcode_map[] = {
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
}

/* Get the next unused message id. */
static u32_t dc_get_next_message_id(void)
{
	nct.message_id++;

	if ((u16_t)nct.message_id == 0) {
		nct.message_id++;
	}

	return nct.message_id;
}

/* Free memory allocated for the data endpoint and reset the endpoint. */
static void dc_endpoint_free(void)
{
	if (nct.dc_rx_endp.utf8 != NULL) {
		nrf_cloud_free(nct.dc_rx_endp.utf8);
	}
	if (nct.dc_tx_endp.utf8 != NULL) {
		nrf_cloud_free(nct.dc_tx_endp.utf8);
	}
	dc_endpoint_reset();
}

static u32_t dc_send(const struct nct_dc_data *dc_data, u8_t qos)
{
	if (dc_data == NULL) {
		return -EINVAL;
	}

	struct mqtt_publish_param publish = {
		.message.topic.qos = qos,
		.message.topic.topic.size = nct.dc_tx_endp.size,
		.message.topic.topic.utf8 = nct.dc_tx_endp.utf8,
	};

	/* Populate payload. */
	if ((dc_data->data.len != 0) && (dc_data->data.ptr != NULL)) {
		publish.message.payload.data = (u8_t *)dc_data->data.ptr;
		publish.message.payload.len = dc_data->data.len;
	}

	if (dc_data->id != 0) {
		publish.message_id = dc_data->id;
	} else {
		publish.message_id = dc_get_next_message_id();
	}

	return mqtt_publish(&nct.client, &publish);
}

static bool strings_compare(const char *s1, const char *s2,
			    u32_t s1_len, u32_t s2_len)
{
	return (strncmp(s1, s2, min(s1_len, s2_len))) ? false : true;
}

/* Verify if the topic is a control channel topic or not. */
static bool control_channel_topic_match(u32_t list_id,
					const struct mqtt_topic *topic,
					enum nct_cc_opcode *opcode)
{
	struct mqtt_topic *topic_list;
	u32_t list_size;

	if (list_id == NCT_RX_LIST) {
		topic_list = (struct mqtt_topic *)nct_cc_rx_list;
		list_size = ARRAY_SIZE(nct_cc_rx_list);
	} else if (list_id == NCT_TX_LIST) {
		topic_list = (struct mqtt_topic *)nct_cc_tx_list;
		list_size = ARRAY_SIZE(nct_cc_tx_list);
	} else {
		return false;
	}

	for (u32_t index = 0; index < list_size; index++) {
		if (!strings_compare(
			    topic->topic.utf8,
			    topic_list[index].topic.utf8,
			    topic->topic.size,
			    topic_list[index].topic.size)) {
			*opcode = nct_cc_rx_opcode_map[index];
			return true;
		}
	}
	return false;
}

/* Provisions root CA certificate using nrf_inbuilt_key API */
static int nct_provision(void)
{
	static sec_tag_t sec_tag_list[] = {CONFIG_NRF_CLOUD_SEC_TAG};

	nct.tls_config.peer_verify = 2;
	nct.tls_config.cipher_count = 0;
	nct.tls_config.cipher_list = NULL;
	nct.tls_config.sec_tag_count = ARRAY_SIZE(sec_tag_list);
	nct.tls_config.seg_tag_list = sec_tag_list;
	nct.tls_config.hostname = NRF_CLOUD_HOSTNAME;

	if (IS_ENABLED(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES)) {

		int err;

		/* Delete certificates */
		nrf_sec_tag_t sec_tag = CONFIG_NRF_CLOUD_SEC_TAG;

		for (nrf_key_mgnt_cred_type_t type = 0; type < 5; type++) {
			err = nrf_inbuilt_key_delete(sec_tag, type);
			LOG_DBG("nrf_inbuilt_key_delete(%lu, %d) => result=%d",
				sec_tag, type, err);
		}

		/* Provision CA Certificate. */
		err = nrf_inbuilt_key_write(CONFIG_NRF_CLOUD_SEC_TAG,
					NRF_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					NRF_CLOUD_CA_CERTIFICATE,
					sizeof(NRF_CLOUD_CA_CERTIFICATE));
		if (err) {
			LOG_DBG("NRF_CLOUD_CA_CERTIFICATE error!");
			return err;
		}

		/* Provision Private Certificate. */
		err = nrf_inbuilt_key_write(
			CONFIG_NRF_CLOUD_SEC_TAG,
			NRF_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
			NRF_CLOUD_CLIENT_PRIVATE_KEY,
			sizeof(NRF_CLOUD_CLIENT_PRIVATE_KEY));
		if (err) {
			LOG_DBG("NRF_CLOUD_CLIENT_PRIVATE_KEY error! ");
			return err;
		}

		/* Provision Public Certificate. */
		err = nrf_inbuilt_key_write(
			CONFIG_NRF_CLOUD_SEC_TAG,
			NRF_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
			NRF_CLOUD_CLIENT_PUBLIC_CERTIFICATE,
			sizeof(NRF_CLOUD_CLIENT_PUBLIC_CERTIFICATE));
		if (err) {
			LOG_DBG("NRF_CLOUD_CLIENT_PUBLIC_CERTIFICATE error!");
			return err;
		}
	}
	return 0;
}

/* Connect to MQTT broker. */
int nct_mqtt_connect(void)
{
	mqtt_client_init(&nct.client);

	/* nct.broker.sin_len = sizeof(struct sockaddr_in); */
	nct.broker.sin_family = AF_INET;
	nct.broker.sin_port = htons(NRF_CLOUD_PORT);

	nct.client.broker = (struct sockaddr *)&nct.broker;
	nct.client.evt_cb = nct_mqtt_evt_handler;
	nct.client.client_id.utf8 = (u8_t *)NRF_CLOUD_CLIENT_ID;
	nct.client.client_id.size = strlen(NRF_CLOUD_CLIENT_ID);
	nct.client.protocol_version = MQTT_VERSION_3_1_1;
	nct.client.password = NULL;
	nct.client.user_name = NULL;
	nct.client.transport.type = MQTT_TRANSPORT_SECURE;

	struct mqtt_sec_config *tls_config = &nct.client.transport.tls.config;

	memcpy(tls_config, &nct.tls_config, sizeof(struct mqtt_sec_config));

	return mqtt_connect(&nct.client);
}

/* Handle MQTT events. */
static void nct_mqtt_evt_handler(struct mqtt_client *const mqtt_client,
				 const struct mqtt_evt *_mqtt_evt)
{
	struct nct_evt evt = {
		.status = _mqtt_evt->result
	};
	struct nct_cc_data cc;
	bool event_notify = false;

	switch (_mqtt_evt->type) {
	case MQTT_EVT_CONNACK: {
		LOG_DBG("MQTT_EVT_CONNACK");

		evt.type = NCT_EVT_CONNECTED;
		event_notify = true;
		break;
	}
	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *p = &_mqtt_evt->param.publish;

		LOG_DBG("MQTT_EVT_PUBLISH: id=%d len=%d ",
			p->message_id,
			p->message.payload.len);

		/* If the data arrives on one of the subscribed control channel
		 * topic. Then we notify the same.
		 */
		if (control_channel_topic_match(
			NCT_RX_LIST, &p->message.topic, &cc.opcode)) {

			cc.id = p->message_id;
			cc.data.ptr = p->message.payload.data;
			cc.data.len = p->message.payload.len;

			evt.type = NCT_EVT_CC_RX_DATA;
			evt.param.cc = &cc;
			event_notify = true;
		} else {
			/* Try to match it with one of the data topics. */
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
		LOG_DBG("MQTT_EVT_SUBACK: id=%d result=%d",
			_mqtt_evt->param.suback.message_id,
			_mqtt_evt->result);

		if (_mqtt_evt->param.suback.message_id == NCT_CC_SUBSCRIBE_ID) {
			evt.type = NCT_EVT_CC_CONNECTED;
			event_notify = true;
		}
		if (_mqtt_evt->param.suback.message_id == NCT_DC_SUBSCRIBE_ID) {
			evt.type = NCT_EVT_DC_CONNECTED;
			event_notify = true;
		}
		break;
	}
	case MQTT_EVT_UNSUBACK: {
		LOG_DBG("MQTT_EVT_UNSUBACK");

		if (_mqtt_evt->param.suback.message_id == NCT_CC_SUBSCRIBE_ID) {
			evt.type = NCT_EVT_CC_DISCONNECTED;
			event_notify = true;
		}
		break;
	}
	case MQTT_EVT_PUBACK: {
		LOG_DBG("MQTT_EVT_PUBACK: id=%d result=%d",
			_mqtt_evt->param.puback.message_id,
			_mqtt_evt->result);

		evt.type = NCT_EVT_CC_TX_DATA_CNF;
		evt.param.data_id = _mqtt_evt->param.puback.message_id;
		event_notify = true;
		break;
	}
	case MQTT_EVT_DISCONNECT: {
		LOG_DBG("MQTT_EVT_DISCONNECT: result=%d", _mqtt_evt->result);

		evt.type = NCT_EVT_DISCONNECTED;
		event_notify = true;
		break;
	}
	default:
		break;
	}

	if (event_notify) {
		nct_input(&evt);
	}
}

int nct_init(void)
{
	int err;

	dc_endpoint_reset();

	err = nct_provision();
	if (err) {
		return err;
	}

	return mqtt_init();
}

int nct_connect(void)
{
	int err;
	struct addrinfo *result;
	struct addrinfo *addr;

	err = getaddrinfo(NRF_CLOUD_HOSTNAME, NULL, NULL, &result);
	if (err) {
		return err;
	}

	addr = result;
	err = -ENOENT;

	/* Look for IPv4 address of the broker. */
	while (addr != NULL) {
		/* IPv4 Address. */
		if (addr->ai_addrlen == 4) {
			nct.broker.sin_addr.s4_addr32[0] =
			    ((struct nrf_sockaddr_in *)result->ai_addr)
				->sin_addr.s_addr;
			err = nct_mqtt_connect();
			break;
		} else if (addr->ai_addrlen == 6) {
			/* TODO: Add support for IPv6 address. */
		}

		addr = addr->ai_next;
		break;
	}

	/* Free the address. */
	freeaddrinfo(result);

	return err;
}

int nct_cc_connect(void)
{
	const struct mqtt_subscription_list subscription_list = {
		.list = (struct mqtt_topic *)&nct_cc_rx_list,
		.list_count = ARRAY_SIZE(nct_cc_rx_list),
		.message_id = NCT_CC_SUBSCRIBE_ID
	};

	return mqtt_subscribe(&nct.client, &subscription_list);
}

int nct_cc_send(const struct nct_cc_data *cc_data)
{
	if (cc_data == NULL) {
		return -EINVAL;
	}

	if (cc_data->opcode >= ARRAY_SIZE(nct_cc_tx_list)) {
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
		publish.message.payload.data = (u8_t *)cc_data->data.ptr,
		publish.message.payload.len = cc_data->data.len;
	}

	publish.message_id = cc_data->id;

	LOG_DBG("mqtt_publish: id=%d opcode=%d len=%d",
		publish.message_id, cc_data->opcode,
		cc_data->data.len);

	return mqtt_publish(&nct.client, &publish);
}

int nct_cc_disconnect(void)
{
	LOG_DBG("nct_cc_disconnect");

	static const struct mqtt_subscription_list subscription_list = {
		.list = (struct mqtt_topic *)nct_cc_rx_list,
		.list_count = ARRAY_SIZE(nct_cc_rx_list),
		.message_id = NCT_CC_SUBSCRIBE_ID
	};

	return mqtt_unsubscribe(&nct.client, &subscription_list);
}

void nct_dc_endpoint_set(const struct nrf_cloud_data *tx_endp,
			 const struct nrf_cloud_data *rx_endp)
{
	LOG_DBG("nct_dc_endpoint_set");

	/* In case the endpoint was previous set, free and reset
	 * before copying new one.
	 */
	dc_endpoint_free();

	nct.dc_tx_endp.utf8 = (u8_t *)tx_endp->ptr;
	nct.dc_tx_endp.size = tx_endp->len;

	nct.dc_rx_endp.utf8 = (u8_t *)rx_endp->ptr;
	nct.dc_rx_endp.size = rx_endp->len;
}

void nct_dc_endpoint_get(struct nrf_cloud_data *const tx_endp,
			 struct nrf_cloud_data *const rx_endp)
{
	LOG_DBG("nct_dc_endpoint_get");

	tx_endp->ptr = nct.dc_tx_endp.utf8;
	tx_endp->len = nct.dc_tx_endp.size;

	rx_endp->ptr = nct.dc_rx_endp.utf8;
	rx_endp->len = nct.dc_rx_endp.size;
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
		.message_id = NCT_DC_SUBSCRIBE_ID
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

int nct_dc_disconnect(void)
{
	LOG_DBG("nct_dc_disconnect");

	const struct mqtt_subscription_list subscription_list = {
		.list = (struct mqtt_topic *)&nct.dc_rx_endp,
		.list_count = 1,
		.message_id = NCT_DC_SUBSCRIBE_ID
	};

	return mqtt_unsubscribe(&nct.client, &subscription_list);
}

int nct_disconnect(void)
{
	LOG_DBG("nct_disconnect");

	dc_endpoint_free();
	return mqtt_disconnect(&nct.client);
}

void nct_process(void)
{
	mqtt_input(&nct.client);
	mqtt_live();
}
