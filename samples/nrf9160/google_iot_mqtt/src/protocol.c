/* Protocol implementation. */
/*
 * Copyright (c) 2018-2019 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>

LOG_MODULE_DECLARE(net_google_iot_mqtt, LOG_LEVEL_DBG);
#include "protocol.h"
#include <stdio.h>
#include <zephyr.h>
#include <string.h>
#include <drivers/entropy.h>

#include <net/mqtt.h>

#include <modem/modem_jwt.h>

static uint8_t client_id[] = CONFIG_CLOUD_CLIENT_ID;
static uint8_t client_username[] = "none";
static uint8_t pub_topic[] = CONFIG_CLOUD_PUBLISH_TOPIC;

static struct mqtt_publish_param pub_data;

static uint8_t token[CONFIG_MODEM_JWT_MAX_LEN];

static bool connected;
static uint64_t next_alive;

/* The mqtt client struct */
static struct mqtt_client client_ctx;

/* MQTT Broker details. */
static struct sockaddr_storage broker;

/* Buffers for MQTT client. */
static uint8_t rx_buffer[1024];
static uint8_t tx_buffer[1024];

static sec_tag_t m_sec_tags[] = {
	CONFIG_GOOGLE_CA_CERT_1_SEC_TAG,
	CONFIG_GOOGLE_CA_CERT_2_SEC_TAG
};

void mqtt_subscribe_config(struct mqtt_client *const client)
{
#ifdef CONFIG_CLOUD_SUBSCRIBE_CONFIG
	/* subscribe to config information */
	struct mqtt_topic subs_topic = {
		.topic = {
			.utf8 = CONFIG_CLOUD_SUBSCRIBE_CONFIG,
			.size = strlen(CONFIG_CLOUD_SUBSCRIBE_CONFIG)
		},
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};
	const struct mqtt_subscription_list subs_list = {
		.list = &subs_topic,
		.list_count = 1U,
		.message_id = 1U
	};
	int err;

	err = mqtt_subscribe(client, &subs_list);
	if (err) {
		LOG_ERR("Failed to subscribe to %s item, error %d",
			subs_topic.topic.utf8, err);
	}
#endif
}

void mqtt_evt_handler(struct mqtt_client *const client,
		      const struct mqtt_evt *evt)
{
	switch (evt->type) {
	case MQTT_EVT_SUBACK:
		LOG_INF("SUBACK packet id: %u", evt->param.suback.message_id);
		break;

	case MQTT_EVT_UNSUBACK:
		LOG_INF("UNSUBACK packet id: %u",
				evt->param.suback.message_id);
		break;

	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT connect failed %d", evt->result);
			break;
		}

		connected = true;
		LOG_INF("MQTT client connected!");

		mqtt_subscribe_config(client);

		break;

	case MQTT_EVT_DISCONNECT:
		LOG_INF("MQTT client disconnected %d", evt->result);

		connected = false;

		break;

#ifdef CONFIG_CLOUD_SUBSCRIBE_CONFIG
	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *pub = &evt->param.publish;
		uint8_t d[33];
		int len = pub->message.payload.len;
		int bytes_read;

		LOG_INF("MQTT publish received %d, %d bytes",
			evt->result, len);
		LOG_INF("   id: %d, qos: %d",
			pub->message_id,
			pub->message.topic.qos);
		LOG_INF("   item: %s",
			log_strdup(pub->message.topic.topic.utf8));

		/* assuming the config message is textual */
		while (len) {
			bytes_read = mqtt_read_publish_payload(
				client, d,
				len >= 32 ? 32 : len);
			if (bytes_read < 0 && bytes_read != -EAGAIN) {
				LOG_ERR("failure to read payload");
				break;
			}

			d[bytes_read] = '\0';
			LOG_INF("   payload: %s", log_strdup(d));
			len -= bytes_read;
		}

		/* for MQTT_QOS_0_AT_MOST_ONCE no acknowledgment needed */
		if (pub->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			struct mqtt_puback_param puback = {
				.message_id = pub->message_id
			};

			mqtt_publish_qos1_ack(client, &puback);
		}
		break;
	}
#endif

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBACK error %d", evt->result);
			break;
		}

		/* increment message id for when we send next message */
		pub_data.message_id += 1U;
		LOG_INF("PUBACK packet id: %u",
				evt->param.puback.message_id);
		break;

	default:
		LOG_INF("MQTT event received %d", evt->type);
		break;
	}
}

static int wait_for_input(int timeout)
{
	int res;
	struct zsock_pollfd fds[1] = {
		[0] = {.fd = client_ctx.transport.tls.sock,
		      .events = ZSOCK_POLLIN,
		      .revents = 0},
	};

	res = zsock_poll(fds, 1, timeout);
	if (res < 0) {
		LOG_ERR("poll read event error");
		return -errno;
	}

	return res;
}

#define ALIVE_TIME	(60 * MSEC_PER_SEC)

static struct mqtt_utf8 password = {
	.utf8 = token
};

static struct mqtt_utf8 username = {
	.utf8 = client_username,
	.size = sizeof(client_username)
};

void mqtt_startup(char *hostname, int port)
{
	int err, cnt;
	char pub_msg[64];
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;
	struct mqtt_client *client = &client_ctx;
	static struct zsock_addrinfo hints;
	struct zsock_addrinfo *haddr;
	int retries = 5;

	while (retries) {
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = 0;
		cnt = 0;
		while ((err = getaddrinfo("mqtt.2030.ltsapis.goog", "8883", &hints,
					  &haddr)) && cnt < 3) {
			LOG_ERR("Unable to get address for broker, retrying");
			cnt++;
		}

		if (err != 0) {
			LOG_ERR("Unable to get address for broker, error %d",
				err);
			return;
		}
		LOG_INF("DNS resolved for mqtt.2030.ltsapis.goog:8883");

		mqtt_client_init(client);

		/* Modem-based JWT calc from nrf/tests/lib/modem_jwt/src/jwt_test.c */
		struct jwt_data jwt = {0};
		jwt.audience = CONFIG_CLOUD_AUDIENCE;
		jwt.exp_delta_s = 86400;  /* 24 hours */
		jwt.sec_tag = CONFIG_CLIENT_SEC_TAG;
		jwt.key = JWT_KEY_TYPE_CLIENT_PRIV; 
		jwt.alg = JWT_ALG_TYPE_ES256;
		err = modem_jwt_generate(&jwt);
		if (err != 0) {
			LOG_ERR("Unable to generate JWT on modem, error %d",
				err);
			return;
		}
		memcpy(token, jwt.jwt_buf, strlen(jwt.jwt_buf));  
		// TODO: Would be nice to pass in directly, rather than copying, but mqtt_utf8 requires constant .utf8 pointer

		broker4->sin_family = AF_INET;
		broker4->sin_port = htons(port);
		net_ipaddr_copy(&broker4->sin_addr,
				&net_sin(haddr->ai_addr)->sin_addr);

		/* MQTT client configuration */
		client->broker = &broker;
		client->evt_cb = mqtt_evt_handler;
		client->client_id.utf8 = client_id;
		client->client_id.size = strlen(client_id);
		client->password = &password;
		password.size = strlen(jwt.jwt_buf);
		client->user_name = &username;
		client->protocol_version = MQTT_VERSION_3_1_1;

		/* MQTT buffers configuration */
		client->rx_buf = rx_buffer;
		client->rx_buf_size = sizeof(rx_buffer);
		client->tx_buf = tx_buffer;
		client->tx_buf_size = sizeof(tx_buffer);

		/* MQTT transport configuration */
		client->transport.type = MQTT_TRANSPORT_SECURE;

		struct mqtt_sec_config *tls_config =
				&client->transport.tls.config;

		tls_config->peer_verify = TLS_PEER_VERIFY_REQUIRED; 
		tls_config->cipher_list = NULL;
		tls_config->sec_tag_list = m_sec_tags;
		tls_config->sec_tag_count = ARRAY_SIZE(m_sec_tags);
		tls_config->hostname = hostname;

#if defined(CONFIG_NRF_MODEM_LIB)
		tls_config->session_cache = IS_ENABLED(CONFIG_MQTT_TLS_SESSION_CACHING) ?
							TLS_SESSION_CACHE_ENABLED :
							TLS_SESSION_CACHE_DISABLED;
#else
		/* TLS session caching is not supported by the Zephyr network stack */
		tls_config->session_cache = TLS_SESSION_CACHE_DISABLED;
#endif

		LOG_INF("Connecting to host: %s", hostname);
		err = mqtt_connect(client);
		if (err != 0) {
			LOG_ERR("could not connect, error %d", err);
			mqtt_disconnect(client);
			retries--;
			k_msleep(ALIVE_TIME);
			continue;
		}

		if (wait_for_input(5 * MSEC_PER_SEC) > 0) {
			mqtt_input(client);
			if (!connected) {
				LOG_ERR("failed to connect to mqtt_broker");
				mqtt_disconnect(client);
				retries--;
				k_msleep(ALIVE_TIME);
				continue;
			} else {
				break;
			}
		} else {
			LOG_ERR("failed to connect to mqtt broker");
			mqtt_disconnect(client);
			retries--;
			k_msleep(ALIVE_TIME);
			continue;
		}
	}

	if (!connected) {
		LOG_ERR("Failed to connect to client, aborting");
		return;
	}

	/* initialize publish structure */
	pub_data.message.topic.topic.utf8 = pub_topic;
	pub_data.message.topic.topic.size = strlen(pub_topic);
	pub_data.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE;
	pub_data.message.payload.data = (uint8_t *)pub_msg;
	pub_data.message_id = 1U;
	pub_data.dup_flag = 0U;
	pub_data.retain_flag = 1U;

	mqtt_live(client);

	next_alive = k_uptime_get() + ALIVE_TIME;

	while (1) {
		LOG_INF("Publishing data");
		sprintf(pub_msg, "%s: %d\n", "payload", pub_data.message_id);
		pub_data.message.payload.len = strlen(pub_msg);
		err = mqtt_publish(client, &pub_data);
		if (err) {
			LOG_ERR("could not publish, error %d", err);
			break;
		}

		/* idle and process messages */
		while (k_uptime_get() < next_alive) {
			LOG_INF("... idling ...");
			if (wait_for_input(5 * MSEC_PER_SEC) > 0) {
				mqtt_input(client);
			}
		}

		mqtt_live(client);
		next_alive += ALIVE_TIME;
	}
}
