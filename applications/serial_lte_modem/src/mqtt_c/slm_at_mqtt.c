/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/rand32.h>
#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_native_tls.h"
#include "slm_at_mqtt.h"

LOG_MODULE_REGISTER(slm_mqtt, CONFIG_SLM_LOG_LEVEL);

#define MQTT_MAX_TOPIC_LEN	128
#define MQTT_MAX_CID_LEN	64

/**@brief MQTT client operations. */
enum slm_mqttcon_operation {
	MQTTC_DISCONNECT,
	MQTTC_CONNECT,
	MQTTC_CONNECT6,
};

/**@brief MQTT subscribe operations. */
enum slm_mqttsub_operation {
	AT_MQTTSUB_UNSUB,
	AT_MQTTSUB_SUB
};

static struct slm_mqtt_ctx {
	int family; /* Socket address family */
	bool connected;
	struct mqtt_utf8 client_id;
	struct mqtt_utf8 username;
	struct mqtt_utf8 password;
	sec_tag_t sec_tag;
	union {
		struct sockaddr_in  broker;
		struct sockaddr_in6 broker6;
	};
} ctx;

static char mqtt_broker_url[SLM_MAX_URL + 1];
static uint16_t mqtt_broker_port;
static char mqtt_clientid[MQTT_MAX_CID_LEN + 1];
static char mqtt_username[SLM_MAX_USERNAME + 1];
static char mqtt_password[SLM_MAX_PASSWORD + 1];

static struct mqtt_publish_param pub_param;
static uint8_t pub_topic[MQTT_MAX_TOPIC_LEN];

#define THREAD_STACK_SIZE	KB(2)
#define THREAD_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO

static struct k_thread mqtt_thread;
static K_THREAD_STACK_DEFINE(mqtt_thread_stack, THREAD_STACK_SIZE);

/* Buffers for MQTT client. */
static uint8_t rx_buffer[CONFIG_SLM_MQTTC_MESSAGE_BUFFER_LEN];
static uint8_t tx_buffer[CONFIG_SLM_MQTTC_MESSAGE_BUFFER_LEN];

/* The mqtt client struct */
static struct mqtt_client client;

/**@brief Function to handle received publish event.
 */
static int handle_mqtt_publish_evt(struct mqtt_client *const c, const struct mqtt_evt *evt)
{
	int size_read = 0;
	int ret;

	rsp_send("\r\n#XMQTTMSG: %d,%d\r\n",
		evt->param.publish.message.topic.topic.size,
		evt->param.publish.message.payload.len);
	data_send(evt->param.publish.message.topic.topic.utf8,
		evt->param.publish.message.topic.topic.size);
	data_send("\r\n", 2);
	do {
		ret = mqtt_read_publish_payload_blocking(c, slm_data_buf, sizeof(slm_data_buf));
		if (ret > 0) {
			data_send(slm_data_buf, ret);
			size_read += ret;
		}
	} while (ret >= 0 && size_read < evt->param.publish.message.payload.len);
	data_send("\r\n", 2);

	/* Send QoS1 acknowledgment */
	if (evt->param.publish.message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
		const struct mqtt_puback_param ack = {
			.message_id = evt->param.publish.message_id
		};

		mqtt_publish_qos1_ack(&client, &ack);
	}

	return 0;
}

/**@brief MQTT client event handler
 */
void mqtt_evt_handler(struct mqtt_client *const c, const struct mqtt_evt *evt)
{
	int ret;

	ret = evt->result;
	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			ctx.connected = false;
		}
		break;

	case MQTT_EVT_DISCONNECT:
		ctx.connected = false;
		break;

	case MQTT_EVT_PUBLISH:
		ret = handle_mqtt_publish_evt(c, evt);
		break;

	case MQTT_EVT_PUBACK:
		if (evt->result == 0) {
			LOG_DBG("PUBACK packet id: %u", evt->param.puback.message_id);
		}
		break;

	case MQTT_EVT_PUBREC:
		if (evt->result != 0) {
			break;
		}
		LOG_DBG("PUBREC packet id: %u", evt->param.pubrec.message_id);
		{
			struct mqtt_pubrel_param param = {
				.message_id = evt->param.pubrel.message_id
			};
			ret = mqtt_publish_qos2_release(&client, &param);
			if (ret) {
				LOG_ERR("mqtt_publish_qos2_release: Fail! %d", ret);
			} else {
				LOG_DBG("Release, id %u", evt->param.pubrec.message_id);
			}
		}
		break;

	case MQTT_EVT_PUBREL:
		if (evt->result != 0) {
			break;
		}
		LOG_DBG("PUBREL packet id %u", evt->param.pubrel.message_id);
		{
			struct mqtt_pubcomp_param param = {
				.message_id = evt->param.pubrel.message_id
			};
			ret = mqtt_publish_qos2_complete(&client, &param);
			if (ret) {
				LOG_ERR("mqtt_publish_qos2_complete Failed:%d", ret);
			} else {
				LOG_DBG("Complete, id %u", evt->param.pubrel.message_id);
			}
		}
		break;

	case MQTT_EVT_PUBCOMP:
		if (evt->result == 0) {
			LOG_DBG("PUBCOMP packet id %u", evt->param.pubcomp.message_id);
		}
		break;

	case MQTT_EVT_SUBACK:
		if (evt->result == 0) {
			LOG_DBG("SUBACK packet id: %u", evt->param.suback.message_id);
		}
		break;

	case MQTT_EVT_UNSUBACK:
		if (evt->result == 0) {
			LOG_DBG("UNSUBACK packet id: %u", evt->param.unsuback.message_id);
		}
		break;

	case MQTT_EVT_PINGRESP:
		if (evt->result == 0) {
			LOG_DBG("PINGRESP packet");
		}
		break;

	default:
		LOG_DBG("default: %d", evt->type);
		break;
	}

	rsp_send("\r\n#XMQTTEVT: %d,%d\r\n", evt->type, ret);
}

static void mqtt_thread_fn(void *arg1, void *arg2, void *arg3)
{
	int err = 0;
	struct pollfd fds;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	fds.fd = client.transport.tcp.sock;
#if defined(CONFIG_MQTT_LIB_TLS)
	if (client.transport.type == MQTT_TRANSPORT_SECURE) {
		fds.fd = client.transport.tls.sock;
	}
#endif
	fds.events = POLLIN;
	while (true) {
		if (!ctx.connected) {
			LOG_WRN("MQTT disconnected");
			err = 0;
			break;
		}
		err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
		if (err < 0) {
			LOG_ERR("ERROR: poll %d", errno);
			break;
		}

		if ((fds.revents & POLLIN) == POLLIN) {
			err = mqtt_input(&client);
			if (err != 0) {
				LOG_ERR("ERROR: mqtt_input %d", err);
				break;
			}
			/* MQTT v3.1.1: If a Client does not receive a PINGRESP Packet within a
			 *  reasonable amount of time after it has sent a PINGREQ, it SHOULD close
			 *  the Network Connection to the Server.
			 */
			if (client.unacked_ping > 1) {
				LOG_ERR("ERROR: mqtt_ping nack %d", client.unacked_ping);
				err = -ENETRESET;
				break;
			}
		}

		/* MQTT v3.1.1: Note that a Server is permitted to disconnect a Client that it
		 * determines to be inactive or non-responsive at any time, regardless of the
		 * Keep Alive value provided by that Client.
		 */
		if ((fds.revents & POLLERR) == POLLERR) {
			LOG_ERR("POLLERR");
			err = -EIO;
			break;
		}
		if ((fds.revents & POLLHUP) == POLLHUP) {
			LOG_ERR("POLLHUP");
			err = -ECONNRESET;
			break;
		}
		if ((fds.revents & POLLNVAL) == POLLNVAL) {
			LOG_ERR("POLLNVAL");
			err = -ENOTCONN;
			break;
		}

		/* poll timeout or revent, send KEEPALIVE */
		err = mqtt_live(&client);
		if (err != 0 && err != -EAGAIN) {
			LOG_ERR("ERROR: mqtt_live %d", err);
			break;
		}
	}

	if (ctx.connected && err != 0) {
		LOG_ERR("Abort MQTT connection (error %d)", err);
		(void)mqtt_abort(&client);
	}

	memset(&ctx, 0, sizeof(ctx));
	ctx.connected = false;
	ctx.sec_tag = INVALID_SEC_TAG;
	client.broker = NULL;

	LOG_INF("MQTT thread terminated");
}

/**@brief Resolves the configured hostname and
 * initializes the MQTT broker structure
 */
static int broker_init(void)
{
	int err;
	struct sockaddr sa = {
		.sa_family = AF_UNSPEC
	};

	err = util_resolve_host(0, mqtt_broker_url, mqtt_broker_port, ctx.family,
		Z_LOG_OBJECT_PTR(slm_mqtt), &sa);
	if (err) {
		return -EAGAIN;
	}
	if (sa.sa_family == AF_INET) {
		ctx.broker = *(struct sockaddr_in *)&sa;
	} else {
		ctx.broker6 = *(struct sockaddr_in6 *)&sa;
	}

	return 0;
}

/**@brief Initialize the MQTT client structure
 */
static void client_init(void)
{
	/* Init MQTT client */
	mqtt_client_init(&client);

	/* MQTT client configuration */
	if (ctx.family == AF_INET) {
		client.broker = &ctx.broker;
	} else {
		client.broker = &ctx.broker6;
	}
	client.evt_cb = mqtt_evt_handler;
	client.client_id.utf8 = mqtt_clientid;
	client.client_id.size = strlen(mqtt_clientid);
	client.password = NULL;
	if (ctx.username.size > 0) {
		client.user_name = &ctx.username;
		if (ctx.password.size > 0) {
			client.password = &ctx.password;
		}
	} else {
		client.user_name = NULL;
		/* ignore password if no user_name */
	}

	/* MQTT buffers configuration */
	client.rx_buf = rx_buffer;
	client.rx_buf_size = sizeof(rx_buffer);
	client.tx_buf = tx_buffer;
	client.tx_buf_size = sizeof(tx_buffer);

#if defined(CONFIG_MQTT_LIB_TLS)
	/* MQTT transport configuration */
	if (ctx.sec_tag != INVALID_SEC_TAG) {
		struct mqtt_sec_config *tls_config;

		tls_config = &(client.transport).tls.config;
		tls_config->peer_verify   = TLS_PEER_VERIFY_REQUIRED;
		tls_config->cipher_list   = NULL;
		tls_config->cipher_count  = 0;
		tls_config->sec_tag_count = 1;
		tls_config->sec_tag_list  = (int *)&ctx.sec_tag;
		tls_config->hostname      = mqtt_broker_url;
		client.transport.type     = MQTT_TRANSPORT_SECURE;
	} else {
		client.transport.type     = MQTT_TRANSPORT_NON_SECURE;
	}
#else
	client.transport.type = MQTT_TRANSPORT_NON_SECURE;
#endif
}

static int do_mqtt_connect(void)
{
	int err;

	if (ctx.connected) {
		return -EISCONN;
	}

	/* Init MQTT broker */
	err = broker_init();
	if (err) {
		return err;
	}

	/* Connect to MQTT broker */
	client_init();
	err = mqtt_connect(&client);
	if (err != 0) {
		LOG_ERR("ERROR: mqtt_connect %d", err);
		return err;
	}

	ctx.connected = true;
	k_thread_create(&mqtt_thread, mqtt_thread_stack,
			K_THREAD_STACK_SIZEOF(mqtt_thread_stack),
			mqtt_thread_fn, NULL, NULL, NULL,
			THREAD_PRIORITY, K_USER, K_NO_WAIT);

	return 0;
}

static int do_mqtt_disconnect(void)
{
	int err;

	if (!ctx.connected) {
		return -ENOTCONN;
	}

	err = mqtt_disconnect(&client);
	if (err) {
		LOG_ERR("ERROR: mqtt_disconnect %d", err);
		return err;
	}

	if (k_thread_join(&mqtt_thread, K_SECONDS(CONFIG_MQTT_KEEPALIVE)) != 0) {
		LOG_WRN("Wait for thread terminate failed");
	}

	return err;
}

static int do_mqtt_publish(uint8_t *msg, size_t msg_len)
{
	pub_param.message.payload.data = msg;
	pub_param.message.payload.len  = msg_len;

	return mqtt_publish(&client, &pub_param);
}

static int do_mqtt_subscribe(uint16_t op,
				uint8_t *topic_buf,
				size_t topic_len,
				uint16_t qos)
{
	int err = -EINVAL;
	struct mqtt_topic subscribe_topic;
	static uint16_t sub_message_id;

	sub_message_id++;
	if (sub_message_id == UINT16_MAX) {
		sub_message_id = 1;
	}

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = sub_message_id
	};

	if (qos <= MQTT_QOS_2_EXACTLY_ONCE) {
		subscribe_topic.qos = (uint8_t)qos;
	} else {
		return err;
	}
	subscribe_topic.topic.utf8 = topic_buf;
	subscribe_topic.topic.size = topic_len;

	if (op == 1) {
		err = mqtt_subscribe(&client, &subscription_list);
	} else if (op == 0) {
		err = mqtt_unsubscribe(&client, &subscription_list);
	}

	return err;
}

/* Handles AT#XMQTTCON commands. */
int handle_at_mqtt_connect(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t op;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = at_params_unsigned_short_get(&slm_at_param_list, 1, &op);
		if (err) {
			return err;
		}
		if (op == MQTTC_CONNECT || op == MQTTC_CONNECT6)  {
			size_t clientid_sz = sizeof(mqtt_clientid);
			size_t username_sz = sizeof(mqtt_username);
			size_t password_sz = sizeof(mqtt_password);
			size_t url_sz = sizeof(mqtt_broker_url);

			err = util_string_get(&slm_at_param_list, 2, mqtt_clientid, &clientid_sz);
			if (err) {
				return err;
			}
			err = util_string_get(&slm_at_param_list, 3, mqtt_username, &username_sz);
			if (err) {
				return err;
			} else {
				ctx.username.utf8 = mqtt_username;
				ctx.username.size = strlen(mqtt_username);
			}
			err = util_string_get(&slm_at_param_list, 4, mqtt_password, &password_sz);
			if (err) {
				return err;
			} else {
				ctx.password.utf8 = mqtt_password;
				ctx.password.size = strlen(mqtt_password);
			}
			err = util_string_get(&slm_at_param_list, 5, mqtt_broker_url, &url_sz);
			if (err) {
				return err;
			}
			err = at_params_unsigned_short_get(
				&slm_at_param_list, 6, &mqtt_broker_port);
			if (err) {
				return err;
			}
			ctx.sec_tag = INVALID_SEC_TAG;
			if (at_params_valid_count_get(&slm_at_param_list) > 7) {
				err = at_params_unsigned_int_get(
					&slm_at_param_list, 7, &ctx.sec_tag);
				if (err) {
					return err;
				}
			}
			ctx.family = (op == MQTTC_CONNECT) ? AF_INET : AF_INET6;
			err = do_mqtt_connect();
		} else if (op == MQTTC_DISCONNECT) {
			err = do_mqtt_disconnect();
		} else {
			err = -EINVAL;
		}
		break;

	case AT_CMD_TYPE_READ_COMMAND:
		if (ctx.connected) {
			if (ctx.sec_tag != INVALID_SEC_TAG) {
				rsp_send("\r\n#XMQTTCON: %d,\"%s\",\"%s\",%d,%d\r\n",
					 ctx.connected, mqtt_clientid, mqtt_broker_url,
					 mqtt_broker_port, ctx.sec_tag);
			} else {
				rsp_send("\r\n#XMQTTCON: %d,\"%s\",\"%s\",%d\r\n",
					 ctx.connected, mqtt_clientid, mqtt_broker_url,
					 mqtt_broker_port);
			}
		} else {
			rsp_send("\r\n#XMQTTCON: %d\r\n", ctx.connected);
		}
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XMQTTCON: (%d,%d,%d),<client_id>,<username>,"
			 "<password>,<url>,<port>,<sec_tag>\r\n",
			 MQTTC_DISCONNECT, MQTTC_CONNECT, MQTTC_CONNECT6);
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

static int mqtt_datamode_callback(uint8_t op, const uint8_t *data, int len, uint8_t flags)
{
	int ret = 0;

	if (op == DATAMODE_SEND) {
		if ((flags & SLM_DATAMODE_FLAGS_MORE_DATA) != 0) {
			LOG_ERR("Datamode buffer overflow");
			(void)exit_datamode_handler(-EOVERFLOW);
			return -EOVERFLOW;
		}
		ret = do_mqtt_publish((uint8_t *)data, len);
		LOG_INF("datamode send: %d", ret);
	} else if (op == DATAMODE_EXIT) {
		LOG_DBG("MQTT datamode exit");
	}

	return ret;
}

/* Handles AT#XMQTTPUB commands. */
int handle_at_mqtt_publish(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;

	uint16_t qos = MQTT_QOS_0_AT_MOST_ONCE;
	uint16_t retain = 0;
	size_t topic_sz = MQTT_MAX_TOPIC_LEN;
	uint8_t pub_msg[SLM_MAX_PAYLOAD_SIZE];
	size_t msg_sz = sizeof(pub_msg);
	uint16_t param_count = at_params_valid_count_get(&slm_at_param_list);

	if (!ctx.connected) {
		return -ENOTCONN;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		err = util_string_get(&slm_at_param_list, 1, pub_topic, &topic_sz);
		if (err) {
			return err;
		}
		pub_msg[0] = '\0';
		if (param_count > 2) {
			err = util_string_get(&slm_at_param_list, 2, pub_msg, &msg_sz);
			if (err) {
				return err;
			}
		}
		if (param_count > 3) {
			err = at_params_unsigned_short_get(&slm_at_param_list, 3, &qos);
			if (err) {
				return err;
			}
		}
		if (param_count > 4) {
			err = at_params_unsigned_short_get(&slm_at_param_list, 4, &retain);
			if (err) {
				return err;
			}
		}

		/* common publish parameters*/
		if (qos <= MQTT_QOS_2_EXACTLY_ONCE) {
			pub_param.message.topic.qos = (uint8_t)qos;
		} else {
			return -EINVAL;
		}
		if (retain <= 1) {
			pub_param.retain_flag = (uint8_t)retain;
		} else {
			return -EINVAL;
		}
		pub_param.message.topic.topic.utf8 = pub_topic;
		pub_param.message.topic.topic.size = topic_sz;
		pub_param.dup_flag = 0;
		pub_param.message_id++;
		if (pub_param.message_id == UINT16_MAX) {
			pub_param.message_id = 1;
		}
		if (strlen(pub_msg) == 0) {
			/* Publish payload in data mode */
			err = enter_datamode(mqtt_datamode_callback);
		} else {
			err = do_mqtt_publish(pub_msg, msg_sz);
		}
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XMQTTPUB: <topic>,<msg>,(0,1,2),(0,1)\r\n");
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/* Handles AT#XMQTTSUB commands. */
int handle_at_mqtt_subscribe(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	uint16_t qos;
	char topic[MQTT_MAX_TOPIC_LEN];
	int topic_sz = MQTT_MAX_TOPIC_LEN;

	if (!ctx.connected) {
		return -ENOTCONN;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&slm_at_param_list) == 3) {
			err = util_string_get(&slm_at_param_list, 1, topic, &topic_sz);
			if (err < 0) {
				return err;
			}
			err = at_params_unsigned_short_get(&slm_at_param_list, 2, &qos);
			if (err < 0) {
				return err;
			}
			err = do_mqtt_subscribe(AT_MQTTSUB_SUB, topic, topic_sz, qos);
		} else {
			return -EINVAL;
		}
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XMQTTSUB: <topic>,(0,1,2)\r\n");
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/* Handles AT#XMQTTUNSUB commands. */
int handle_at_mqtt_unsubscribe(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char topic[MQTT_MAX_TOPIC_LEN];
	int topic_sz = MQTT_MAX_TOPIC_LEN;

	if (!ctx.connected) {
		return -ENOTCONN;
	}

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&slm_at_param_list) == 2) {
			err = util_string_get(&slm_at_param_list, 1,
							topic, &topic_sz);
			if (err < 0) {
				return err;
			}
			err = do_mqtt_subscribe(AT_MQTTSUB_UNSUB,
						topic, topic_sz, 0);
		} else {
			return -EINVAL;
		}
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		rsp_send("\r\n#XMQTTUNSUB: <topic>\r\n");
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

int slm_at_mqtt_init(void)
{
	pub_param.message_id = 0;
	memset(&ctx, 0, sizeof(ctx));
	ctx.sec_tag = INVALID_SEC_TAG;

	return 0;
}

int slm_at_mqtt_uninit(void)
{
	client.broker = NULL;

	return 0;
}
