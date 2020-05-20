/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <net/mqtt.h>
#include <net/socket.h>
#include "slm_util.h"
#include "slm_at_mqtt.h"

LOG_MODULE_REGISTER(mqtt, CONFIG_SLM_LOG_LEVEL);

#define MQTT_MAX_TOPIC_LEN	128
#define MQTT_MAX_URL_LEN	128
#define MQTT_MAX_CID_LEN	64
#define MQTT_MESSAGE_BUFFER_LEN	NET_IPV4_MTU

#define INVALID_FDS -1

/**@brief MQTT connect operations. */
enum slm_mqttcon_operation {
	AT_MQTTCON_DISCONNECT,
	AT_MQTTCON_CONNECT
};

/**@brief MQTT subscribe operations. */
enum slm_mqttsub_operation {
	AT_MQTTSUB_UNSUB,
	AT_MQTTSUB_SUB
};

/**@brief List of supported AT commands. */
enum slm_mqtt_at_cmd_type {
	AT_MQTT_CONNECT,
	AT_MQTT_PUBLISH,
	AT_MQTT_SUBSCRIBE,
	AT_MQTT_UNSUBSCRIBE,
	AT_MQTT_MAX
};

/** forward declaration of cmd handlers **/
static int handle_at_mqtt_connect(enum at_cmd_type cmd_type);
static int handle_at_mqtt_publish(enum at_cmd_type cmd_type);
static int handle_at_mqtt_subscribe(enum at_cmd_type cmd_type);
static int handle_at_mqtt_unsubscribe(enum at_cmd_type cmd_type);

/**@brief SLM AT Command list type. */
static slm_at_cmd_list_t m_mqtt_at_list[AT_MQTT_MAX] = {
	{AT_MQTT_CONNECT, "AT#XMQTTCON", handle_at_mqtt_connect},
	{AT_MQTT_PUBLISH, "AT#XMQTTPUB", handle_at_mqtt_publish},
	{AT_MQTT_SUBSCRIBE, "AT#XMQTTSUB", handle_at_mqtt_subscribe},
	{AT_MQTT_UNSUBSCRIBE, "AT#XMQTTUNSUB", handle_at_mqtt_unsubscribe},
};

static struct slm_mqtt_ctx {
	bool connected;
	bool sec_transport;
	u8_t cid[MQTT_MAX_CID_LEN + 1];
	char url[MQTT_MAX_URL_LEN + 1];
	u32_t port;
	u32_t sec_tag;
} ctx;

/* global functions defined in different files */
void rsp_send(const u8_t *str, size_t len);

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_AT_CMD_RESPONSE_MAX_LEN];

#define THREAD_STACK_SIZE	KB(2)
#define THREAD_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO

static K_THREAD_STACK_DEFINE(mqtt_thread_stack, THREAD_STACK_SIZE);

/* Buffers for MQTT client. */
static u8_t rx_buffer[MQTT_MESSAGE_BUFFER_LEN];
static u8_t tx_buffer[MQTT_MESSAGE_BUFFER_LEN];
static u8_t payload_buf[MQTT_MESSAGE_BUFFER_LEN];

/* The mqtt client struct */
static struct mqtt_client client;

/* MQTT Broker details. */
static struct sockaddr_storage broker;

/* Connected flag */
static K_SEM_DEFINE(mqtt_connected, 0, 1);

/* File descriptor */
static struct pollfd fds = {
	.fd = INVALID_FDS,
};

/**@brief Function to read the published payload.
 */
static int publish_get_payload(struct mqtt_client *c, size_t length)
{
	if (length > sizeof(payload_buf)) {
		return -EMSGSIZE;
	}

	return mqtt_readall_publish_payload(c, payload_buf, length);
}

/**@brief Function to handle received publish event.
 */
static int handle_mqtt_publish_evt(struct mqtt_client *const c,
					const struct mqtt_evt *evt)
{
	int ret;

	ret = publish_get_payload(c,
				evt->param.publish.message.payload.len);
	if (ret < 0) {
		return ret;
	}

	if (slm_util_hex_check(payload_buf,
				evt->param.publish.message.payload.len)) {
		int size = evt->param.publish.message.payload.len * 2;
		char data_hex[size];

		ret = slm_util_htoa((const u8_t *)&payload_buf,
				evt->param.publish.message.payload.len,
				data_hex, size);
		if (ret < 0) {
			return ret;
		}
		sprintf(rsp_buf, "#XMQTTMSG: %d,%d,%d\r\n",
			DATATYPE_HEXADECIMAL,
			evt->param.publish.message.topic.topic.size,
			ret);
		rsp_send(rsp_buf, strlen(rsp_buf));
		rsp_send(evt->param.publish.message.topic.topic.utf8,
			evt->param.publish.message.topic.topic.size);
		rsp_send("\r\n", 2);
		rsp_send(data_hex, ret);
		rsp_send("\r\n", 2);
	} else {
		sprintf(rsp_buf, "#XMQTTMSG: %d,%d,%d\r\n",
			DATATYPE_PLAINTEXT,
			evt->param.publish.message.topic.topic.size,
			evt->param.publish.message.payload.len);
		rsp_send(rsp_buf, strlen(rsp_buf));
		rsp_send(evt->param.publish.message.topic.topic.utf8,
			evt->param.publish.message.topic.topic.size);
		rsp_send("\r\n", 2);
		rsp_send(payload_buf,
			evt->param.publish.message.payload.len);
		rsp_send("\r\n", 2);
	}

	return 0;
}

/**@brief MQTT client event handler
 */
void mqtt_evt_handler(struct mqtt_client *const c,
		      const struct mqtt_evt *evt)
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
			LOG_DBG("PUBACK packet id: %u\n",
					evt->param.puback.message_id);
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
				LOG_ERR("mqtt_publish_qos2_release: Fail! %d",
					ret);
			} else {
				LOG_DBG("Release, id %u",
					evt->param.pubrec.message_id);
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
				LOG_ERR("mqtt_publish_qos2_complete Failed:%d",
					ret);
			} else {
				LOG_DBG("Complete, id %u",
					evt->param.pubrel.message_id);
			}
		}
		break;

	case MQTT_EVT_PUBCOMP:
		if (evt->result == 0) {
			LOG_DBG("PUBCOMP packet id %u",
				evt->param.pubcomp.message_id);
		}
		break;

	case MQTT_EVT_SUBACK:
		if (evt->result == 0) {
			LOG_DBG("SUBACK packet id: %u\n",
					evt->param.suback.message_id);
		}
		break;

	default:
		LOG_DBG("default: %d\n", evt->type);
		break;
	}

	sprintf(rsp_buf, "#XMQTTEVT: %d,%d\r\n",
		evt->type, ret);
	rsp_send(rsp_buf, strlen(rsp_buf));
}

static void mqtt_thread_fn(void *arg1, void *arg2, void *arg3)
{
	int err;

	while (1) {
		/* Don't go any further until MQTT is connected */
		k_sem_take(&mqtt_connected, K_FOREVER);
		while (ctx.connected) {
			err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
			if (err < 0) {
				LOG_ERR("ERROR: poll %d", errno);
				break;
			}
			err = mqtt_live(&client);
			if ((err != 0) && (err != -EAGAIN)) {
				LOG_ERR("ERROR: mqtt_live %d", err);
				break;
			}
			if ((fds.revents & POLLIN) == POLLIN) {
				err = mqtt_input(&client);
				if (err != 0) {
					LOG_ERR("ERROR: mqtt_input %d", err);
					mqtt_abort(&client);
					break;
				}
			}
			if ((fds.revents & POLLERR) == POLLERR) {
				LOG_ERR("POLLERR");
				mqtt_abort(&client);
				break;
			}
			if ((fds.revents & POLLNVAL) == POLLNVAL) {
				LOG_ERR("POLLNVAL");
				mqtt_abort(&client);
				break;
			}
		}
	}
}

/**@brief Resolves the configured hostname and
 * initializes the MQTT broker structure
 */
static void broker_init(void)
{
	int err;
	char addr_str[INET6_ADDRSTRLEN];
	struct addrinfo *result;
	struct addrinfo *addr;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM
	};

	err = getaddrinfo(ctx.url, NULL, &hints, &result);
	if (err) {
		LOG_ERR("ERROR: getaddrinfo failed %d", err);

		return;
	}

	addr = result;
	err = -ENOENT;

	while (addr != NULL) {
		/* IPv4 Address. */
		if (addr->ai_addrlen == sizeof(struct sockaddr_in)) {
			struct sockaddr_in *broker4 =
				((struct sockaddr_in *)&broker);

			broker4->sin_addr.s_addr =
				((struct sockaddr_in *)addr->ai_addr)
				->sin_addr.s_addr;
			broker4->sin_family = AF_INET;
			broker4->sin_port = htons(ctx.port);

			inet_ntop(AF_INET, &broker4->sin_addr, addr_str,
				  sizeof(addr_str));
			LOG_INF("IPv4 Address %s\n", addr_str);
			break;
		} else if (addr->ai_addrlen == sizeof(struct sockaddr_in6)) {
			/* IPv6 Address. */
			struct sockaddr_in6 *broker6 =
				((struct sockaddr_in6 *)&broker);

			memcpy(broker6->sin6_addr.s6_addr,
				((struct sockaddr_in6 *)addr->ai_addr)
				->sin6_addr.s6_addr,
				sizeof(struct in6_addr));
			broker6->sin6_family = AF_INET6;
			broker6->sin6_port = htons(ctx.port);

			inet_ntop(AF_INET6, &broker6->sin6_addr, addr_str,
				  sizeof(addr_str));
			LOG_INF("IPv6 Address %s\n", addr_str);
			break;
		} else {
			LOG_ERR("error: ai_addrlen = %u should be %u or %u\n",
				(unsigned int)addr->ai_addrlen,
				(unsigned int)sizeof(struct sockaddr_in),
				(unsigned int)sizeof(struct sockaddr_in6));
		}

		addr = addr->ai_next;
	}

	/* Free the address. */
	freeaddrinfo(result);
}

/**@brief Initialize the MQTT client structure
 */
static void client_init(void)
{
	/* Init MQTT client */
	mqtt_client_init(&client);

	/* Init MQTT broker */
	broker_init();

	/* MQTT client configuration */
	client.broker = &broker;
	client.evt_cb = mqtt_evt_handler;
	client.client_id.utf8 = ctx.cid;
	client.client_id.size = strlen(ctx.cid);
	client.password = NULL;
	client.user_name = NULL;
	client.protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client.rx_buf = rx_buffer;
	client.rx_buf_size = sizeof(rx_buffer);
	client.tx_buf = tx_buffer;
	client.tx_buf_size = sizeof(tx_buffer);

	/* MQTT transport configuration */
	if (ctx.sec_transport == true) {
		struct mqtt_sec_config *tls_config;

		tls_config = &(client.transport).tls.config;
		tls_config->peer_verify = TLS_PEER_VERIFY_REQUIRED;
		tls_config->cipher_list = NULL;
		tls_config->cipher_count = 0;
		tls_config->sec_tag_count = 1;
		tls_config->sec_tag_list = (int *)&ctx.sec_tag;
		tls_config->hostname = ctx.url;
		client.transport.type = MQTT_TRANSPORT_SECURE;
	} else {
		client.transport.type = MQTT_TRANSPORT_NON_SECURE;
	}
}

/**@brief Initialize the file descriptor structure used by poll.
 */
static int fds_init(struct mqtt_client *c)
{
	if (c->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds.fd = c->transport.tcp.sock;
	} else {
#if defined(CONFIG_MQTT_LIB_TLS)
		fds.fd = c->transport.tls.sock;
#else
		return -ENOTSUP;
#endif
	}

	fds.events = POLLIN;

	return 0;
}

static int do_mqtt_connect(void)
{
	int err = -EINVAL;

	if (ctx.connected) {
		return -EINPROGRESS;
	}

	client_init();

	err = mqtt_connect(&client);
	if (err != 0) {
		LOG_ERR("ERROR: mqtt_connect %d", err);
		return err;
	}

	err = fds_init(&client);
	if (err != 0) {
		LOG_ERR("ERROR: fds_init %d", err);
		return err;
	}

	/** start polling now for CONNACK */
	ctx.connected = true;
	k_sem_give(&mqtt_connected);

	return 0;
}

static int do_mqtt_disconnect(void)
{
	int err;

	err = mqtt_disconnect(&client);
	if (err) {
		LOG_ERR("ERROR: mqtt_disconnect %d", err);
	}
	slm_at_mqtt_uninit();

	return err;
}

static int do_mqtt_publish(u16_t qos, u16_t retain,
				u8_t *topic, size_t topic_len,
				u8_t *msg, size_t msg_len)
{
	int err = -EINVAL;
	struct mqtt_publish_param param;

	if (qos <= MQTT_QOS_2_EXACTLY_ONCE) {
		param.message.topic.qos = (u8_t)qos;
	} else {
		return err;
	}

	if (retain <= 1) {
		param.retain_flag = (u8_t)retain;
	} else {
		return err;
	}
	param.message.topic.topic.utf8 = topic;
	param.message.topic.topic.size = topic_len;
	param.message.payload.data = msg;
	param.message.payload.len = msg_len;
	param.message_id = sys_rand32_get();
	param.dup_flag = 0;

	return mqtt_publish(&client, &param);
}

static int do_mqtt_subscribe(u16_t op,
				u8_t *topic_buf,
				size_t topic_len,
				u16_t qos)
{
	int err = -EINVAL;
	struct mqtt_topic subscribe_topic;

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = sys_rand32_get()
	};

	if (qos <= MQTT_QOS_2_EXACTLY_ONCE) {
		subscribe_topic.qos = (u8_t)qos;
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

/**@brief handle AT#XMQTTCON commands
 *  AT#XMQTTCON=<op>[,<cid>,<url>,<port>[,<sec_tag>]]
 *  AT#XMQTTCON?
 *  AT#XMQTTCON=?
 */
static int handle_at_mqtt_connect(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;

	u16_t op;
	size_t url_sz = MQTT_MAX_URL_LEN;
	size_t cid_sz = MQTT_MAX_CID_LEN;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&at_param_list) <= 1) {
			return -EINVAL;
		}
		err = at_params_short_get(&at_param_list, 1, &op);
		if (err < 0) {
			return err;
		}
		if (op == AT_MQTTCON_CONNECT) {
			if (at_params_valid_count_get(&at_param_list) <= 4) {
				return -EINVAL;
			}
			if (ctx.connected) {
				return -EISCONN;
			}

			memset(&ctx, 0, sizeof(ctx));

			err = at_params_string_get(&at_param_list, 2,
							ctx.cid, &cid_sz);
			if (err < 0) {
				return err;
			}
			ctx.cid[cid_sz] = '\0';
			err = at_params_string_get(&at_param_list, 3,
							ctx.url, &url_sz);
			if (err < 0) {
				return err;
			}
			ctx.url[url_sz] = '\0';
			err = at_params_int_get(&at_param_list, 4, &ctx.port);
			if (err < 0) {
				return err;
			}
			if (at_params_valid_count_get(&at_param_list) == 6) {
				err = at_params_int_get(&at_param_list, 5,
							&ctx.sec_tag);
				if (err < 0) {
					return err;
				}
				ctx.sec_transport = true;
			}
			err = do_mqtt_connect();
			break;
		} else if (op == AT_MQTTCON_DISCONNECT) {
			if (!ctx.connected) {
				return -ENOTCONN;
			}
			LOG_DBG("Disconnect from broker");
			err = do_mqtt_disconnect();
			if (err) {
				LOG_ERR("Fail to disconnect. Error: %d", err);
			}
		} break;

	case AT_CMD_TYPE_READ_COMMAND:
		if (ctx.sec_transport) {
			sprintf(rsp_buf,
				"#XMQTTCON: %d,\"%s\",\"%s\",%d,%d\r\n",
				ctx.connected, ctx.cid, ctx.url, ctx.port,
				ctx.sec_tag);
		} else {
			sprintf(rsp_buf, "#XMQTTCON: %d,\"%s\",\"%s\",%d\r\n",
				ctx.connected, ctx.cid, ctx.url, ctx.port);
		}
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "#XMQTTCON: (0, 1), <cid>, <url>, <port>,"
					" <sec_tag>\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XMQTTPUB commands
 *  AT#XMQTTPUB=<topic>,<datatype>,<msg>,<qos>,<retain>
 *  AT#XMQTTPUB? READ command not supported
 *  AT#XMQTTPUB=?
 */
static int handle_at_mqtt_publish(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;

	u16_t qos, retain, datatype;
	u8_t topic[MQTT_MAX_TOPIC_LEN];
	size_t topic_sz = MQTT_MAX_TOPIC_LEN;
	u8_t msg[MQTT_MESSAGE_BUFFER_LEN];
	size_t msg_sz = MQTT_MESSAGE_BUFFER_LEN;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&at_param_list) != 6) {
			return -EINVAL;
		}
		err = at_params_string_get(&at_param_list, 1, topic, &topic_sz);
		if (err < 0) {
			return err;
		}
		topic[topic_sz] = '\0';
		err = at_params_short_get(&at_param_list, 2, &datatype);
		if (err < 0) {
			return err;
		}
		err = at_params_string_get(&at_param_list, 3, msg, &msg_sz);
		if (err < 0) {
			return err;
		}
		msg[msg_sz] = '\0';
		err = at_params_short_get(&at_param_list, 4, &qos);
		if (err < 0) {
			return err;
		}
		err = at_params_short_get(&at_param_list, 5, &retain);
		if (err < 0) {
			return err;
		}
		if (datatype == DATATYPE_HEXADECIMAL) {
			size_t data_len = msg_sz / 2;
			u8_t data_hex[data_len];

			data_len = slm_util_atoh(msg, msg_sz,
						data_hex, data_len);
			if (data_len > 0) {
				err = do_mqtt_publish(qos, retain,
							topic, topic_sz,
							data_hex, data_len);
			}
		} else {
			err = do_mqtt_publish(qos, retain,
						topic, topic_sz,
						msg, msg_sz);
			if (err < 0) {
				return err;
			}
		}
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "#XMQTTPUB: <topic>, (0, 1), <msg>,"
					" (0, 1, 2), (0, 1)\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XMQTTSUB commands
 *  AT#XMQTTSUB=<topic>,<qos>
 *  AT#XMQTTSUB? READ command not supported
 *  AT#XMQTTSUB=?
 */
static int handle_at_mqtt_subscribe(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	u16_t qos;
	char topic[MQTT_MAX_TOPIC_LEN];
	int topic_sz = MQTT_MAX_TOPIC_LEN;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&at_param_list) == 3) {
			err = at_params_string_get(&at_param_list, 1,
							topic, &topic_sz);
			if (err < 0) {
				return err;
			}
			topic[topic_sz] = '\0';
			err = at_params_short_get(&at_param_list, 2, &qos);
			if (err < 0) {
				return err;
			}
			err = do_mqtt_subscribe(AT_MQTTSUB_SUB,
						topic, topic_sz, qos);
		} else {
			return -EINVAL;
		}
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "#XMQTTSUB: <topic>, (0, 1, 2)\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief handle AT#XMQTTUNSUB commands
 *  AT#XMQTTUNSUB=<topic>
 *  AT#XMQTTUNSUB? READ command not supported
 *  AT#XMQTTUNSUB=?
 */
static int handle_at_mqtt_unsubscribe(enum at_cmd_type cmd_type)
{
	int err = -EINVAL;
	char topic[MQTT_MAX_TOPIC_LEN];
	int topic_sz = MQTT_MAX_TOPIC_LEN;

	switch (cmd_type) {
	case AT_CMD_TYPE_SET_COMMAND:
		if (at_params_valid_count_get(&at_param_list) == 2) {
			err = at_params_string_get(&at_param_list, 1,
							topic, &topic_sz);
			if (err < 0) {
				return err;
			}
			topic[topic_sz] = '\0';
			err = do_mqtt_subscribe(AT_MQTTSUB_UNSUB,
						topic, topic_sz, 0);
		} else {
			return -EINVAL;
		}
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "#XMQTTUNSUB: <topic>\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		err = 0;
		break;

	default:
		break;
	}

	return err;
}

/**@brief API to handle MQTT AT commands
 */
int slm_at_mqtt_parse(const char *at_cmd)
{
	int ret = -ENOTSUP;
	enum at_cmd_type type;

	for (int i = 0; i < AT_MQTT_MAX; i++) {
		if (slm_util_cmd_casecmp(at_cmd, m_mqtt_at_list[i].string)) {
			ret = at_parser_params_from_str(at_cmd, NULL,
						&at_param_list);
			if (ret) {
				LOG_ERR("Failed to parse AT command %d", ret);
				return -EINVAL;
			}
			type = at_parser_cmd_type_get(at_cmd);
			ret = m_mqtt_at_list[i].handler(type);
			break;
		}
	}

	return ret;
}

/**@brief API to list MQTT AT commands
 */
void slm_at_mqtt_clac(void)
{
	for (int i = 0; i < AT_MQTT_MAX; i++) {
		sprintf(rsp_buf, "%s\r\n", m_mqtt_at_list[i].string);
		rsp_send(rsp_buf, strlen(rsp_buf));
	}
}

int slm_at_mqtt_init(void)
{
	return 0;
}

int slm_at_mqtt_uninit(void)
{
	client.broker = NULL;
	fds.fd = INVALID_FDS;

	return 0;
}

K_THREAD_DEFINE(mqtt_thread, K_THREAD_STACK_SIZEOF(mqtt_thread_stack),
		mqtt_thread_fn, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, 0);
