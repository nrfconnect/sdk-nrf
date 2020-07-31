/*
 * Copyright (client) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <net/mqtt.h>
#include <net/socket.h>
#include <stdio.h>
#include <net/azure_iot_hub.h>
#include <settings/settings.h>

#include "azure_iot_hub_dps.h"

#if defined(CONFIG_AZURE_FOTA)
#include <net/azure_fota.h>
#endif

#if defined(CONFIG_BOARD_QEMU_X86)
#include "certificates.h"
#endif

#include <logging/log.h>

LOG_MODULE_REGISTER(azure_iot_hub, CONFIG_AZURE_IOT_HUB_LOG_LEVEL);

#define USER_NAME_STATIC	CONFIG_AZURE_IOT_HUB_HOSTNAME "/"	\
				CONFIG_AZURE_IOT_HUB_DEVICE_ID		\
				"/?api-version=2018-06-30"
/* User name when connecting to Azure IoT Hub is on the form
 *	<IoT hub hostname>/<device ID>/?api-version=2018-06-30
 */
#define USER_NAME_TEMPLATE	"%s/%s/?api-version=2018-06-30"

#define TOPIC_DEVICEBOUND	"devices/%s/messages/devicebound/#"
#define TOPIC_TWIN_DESIRED	"$iothub/twin/PATCH/properties/desired/#"
#define TOPIC_TWIN_RES		"$iothub/twin/res/#"
#define TOPIC_TWIN_REPORT	"$iothub/twin/PATCH/properties/reported/?$rid=%d"
#define TOPIC_EVENTS		"devices/%s/messages/events/"
#define TOPIC_TWIN_REQUEST	"$iothub/twin/GET/?$rid=%d"
#define TOPIC_DIRECT_METHODS	"$iothub/methods/POST/#"
#define TOPIC_DIRECT_METHOD_RES	"$iothub/methods/res/%d/?$rid=%d"

#define DPS_USER_NAME		CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE \
				"/registrations/%s/api-version=2019-03-31"

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DEVICE_ID_APP)
static char device_id_buf[CONFIG_AZURE_IOT_HUB_DEVICE_ID_MAX_LEN + 1];
#else
static char device_id_buf[CONFIG_AZURE_IOT_HUB_DEVICE_ID_MAX_LEN + 1] =
	CONFIG_AZURE_IOT_HUB_DEVICE_ID;
#endif

static struct azure_iot_hub_config conn_config = {
	.device_id = device_id_buf,
#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DEVICE_ID_APP)
	.device_id_len = 0,
#else
	.device_id_len = sizeof(device_id_buf) - 1,
#endif
};

static azure_iot_hub_evt_handler_t evt_handler;

/* If DPS is used, the IoT hub hostname is obtained through that service,
 * otherwise it has to be set compile time using CONFIG_AZURE_IOT_HUB_HOSTNAME.
 * The maximal size is length of hub name + device ID length + lengths of
 * ".azure-devices-provisioning.net/" and "/?api-version=2018-06-30"
 */
#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
#define USER_NAME_BUF_LEN	(CONFIG_AZURE_IOT_HUB_HOSTNAME_MAX_LEN + \
				CONFIG_AZURE_IOT_HUB_DEVICE_ID_MAX_LEN + \
				sizeof(".azure-devices-provisioning.net/") + \
				sizeof("/?api-version=2018-06-30"))
static char user_name_buf[USER_NAME_BUF_LEN];
static struct mqtt_utf8 user_name = {
	.utf8 = user_name_buf,
};
#else
static struct mqtt_utf8 user_name = {
	.utf8 = USER_NAME_STATIC,
	.size = sizeof(USER_NAME_STATIC) - 1,
};
#endif /* IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS) */

enum connection_state {
	STATE_IDLE,
	STATE_INIT,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_DISCONNECTING,
};

static enum connection_state connection_state = STATE_IDLE;
static char rx_buffer[CONFIG_AZURE_IOT_HUB_MQTT_RX_TX_BUFFER_LEN];
static char tx_buffer[CONFIG_AZURE_IOT_HUB_MQTT_RX_TX_BUFFER_LEN];
static char payload_buf[CONFIG_AZURE_IOT_HUB_MQTT_PAYLOAD_BUFFER_LEN];

static bool is_initialized;
static struct mqtt_client client;
static struct sockaddr_storage broker;
static K_SEM_DEFINE(connection_poll_sem, 0, 1);
static K_SEM_DEFINE(connected, 0, 1);
static atomic_t dps_disconnecting;

/* Build time asserts */

/* Device ID source must be one of three for the library to work:
 *	- Kconfigured IoT hub device ID: CONFIG_AZURE_IOT_HUB_DEVICE_ID
 *	- Runtime app-provided device ID
 */
BUILD_ASSERT((sizeof(CONFIG_AZURE_IOT_HUB_DEVICE_ID) - 1 > 0) ||
	     IS_ENABLED(CONFIG_AZURE_IOT_HUB_DEVICE_ID_APP),
	     "Device ID must be set by Kconfig or application");

/* Static function signatures */
static int connect_client(struct azure_iot_hub_config *cfg);

/* Static functions */
static void azure_iot_hub_notify_event(struct azure_iot_hub_evt *evt)
{
	if (evt_handler) {
		evt_handler(evt);
	}
}

static int publish_get_payload(struct mqtt_client *const client, size_t length)
{
	if (length > sizeof(payload_buf)) {
		LOG_ERR("Incoming MQTT message too large for payload buffer");
		return -EMSGSIZE;
	}

	return mqtt_readall_publish_payload(client, payload_buf, length);
}

static int topic_subscribe(void)
{
	int err;
	ssize_t len;
	char topic_devicebound[(sizeof(TOPIC_DEVICEBOUND) - 2) +
		CONFIG_AZURE_IOT_HUB_DEVICE_ID_MAX_LEN];
	struct mqtt_topic sub_topics[] = {
		{
			.topic.utf8 = topic_devicebound,
		},
		{
			.topic.size = sizeof(TOPIC_TWIN_DESIRED) - 1,
			.topic.utf8 = TOPIC_TWIN_DESIRED,
		},
		{
			.topic.size = sizeof(TOPIC_TWIN_RES) - 1,
			.topic.utf8 = TOPIC_TWIN_RES,
		},
		{
			.topic.size = sizeof(TOPIC_DIRECT_METHODS) - 1,
			.topic.utf8 = TOPIC_DIRECT_METHODS,
		},
	};
	const struct mqtt_subscription_list sub_list = {
		.list = sub_topics,
		.list_count = ARRAY_SIZE(sub_topics),
		.message_id = k_uptime_get_32(),
	};

	len = snprintk(topic_devicebound, sizeof(topic_devicebound),
		       TOPIC_DEVICEBOUND, conn_config.device_id);
	if ((len < 0) || (len > sizeof(topic_devicebound))) {
		LOG_ERR("Failed to create device bound topic");
		return -ENOMEM;
	}

	sub_topics[0].topic.size = len;

	for (size_t i = 0; i < sub_list.list_count; i++) {
		LOG_DBG("Subscribing to: %s",
			log_strdup(sub_list.list[i].topic.utf8));
	}

	err = mqtt_subscribe(&client, &sub_list);
	if (err) {
		LOG_ERR("Failed to subscribe to topic list, error: %d", err);
	}

	LOG_DBG("Successfully subscribed to default topics");

	return err;
}

static bool is_direct_method(const char *topic, size_t topic_len)
{
	/* Compare topic with the direct method topic prefix
	 * `$iothub/methods/POST/`.
	 */
	return strncmp(TOPIC_DIRECT_METHODS, topic,
		       MIN(sizeof(TOPIC_DIRECT_METHODS) - 2, topic_len)) == 0;
}

static bool is_device_twin_update(const char *topic, size_t topic_len)
{
	/* Compare topic with the device twin desired prefix
	 * `$iothub/twin/PATCH/properties/desired/#`.
	 */
	return strncmp(TOPIC_TWIN_DESIRED, topic,
		       MIN(sizeof(TOPIC_TWIN_DESIRED) - 2, topic_len)) == 0;
}

static bool is_device_twin_result(const char *topic, size_t topic_len)
{
	/* Compare topic with the device twin report result prefix
	 * `$iothub/twin/res/#`.
	 */
	return strncmp(TOPIC_TWIN_RES, topic,
		       MIN(sizeof(TOPIC_TWIN_RES) - 2, topic_len)) == 0;
}

/* Get report result's status code. Returns 0 on success. */
static int get_device_twin_result(char *topic, size_t topic_len,
				  struct azure_iot_hub_result *result)
{
	char *max_ptr = topic + topic_len - 1;
	char *ptr;

	/* Topic format:
	 *   $iothub/twin/res/{status}/?$rid={request id}&$version={version}
	 *
	 * Place pointer at start of status (integer)
	 */
	ptr = topic + sizeof("$iothub/twin/res/") - 1;

	/* Get the status as a string */
	ptr = strtok(ptr, "/");
	if ((ptr == NULL) || (strlen(ptr) > 3)) {
		LOG_ERR("Invalid status");
		return -1;
	}

	result->status = atoi(ptr);

	/* Move pointer to start of request ID */
	ptr = ptr + strlen(ptr) + sizeof("/?$rid=") - 1;
	if (ptr > max_ptr) {
		LOG_ERR("Could not find request ID");
		return -1;
	}

	/* Get the request ID as a string */
	ptr = strtok(ptr, "&");
	if ((ptr == NULL) || (ptr > max_ptr)) {
		LOG_ERR("Invalid request ID");
		return -1;
	}

	result->rid = atoi(ptr);

	LOG_DBG("Device twin received, request ID %d, status: %d",
		result->rid, result->status);

	return 0;
}

/* Note: This function alters the content of the topic string through
 * the use of strtok().
 */
static bool direct_method_process(char *topic, size_t topic_len,
				  const char *payload, size_t payload_len)
{
	char *max_ptr = topic + topic_len - 1;
	char *ptr;
	char rid_buf[8];
	struct azure_iot_hub_evt evt = {
		.type = AZURE_IOT_HUB_EVT_DIRECT_METHOD,
		.topic.str = topic,
		.topic.len = topic_len,
		.data.method.payload = payload,
		.data.method.payload_len = payload_len,
	};

	/* Topic format: $iothub/methods/POST/{method name}/?$rid={req ID} */
	/* Place pointer at start of method name */
	ptr = topic + sizeof("$iothub/methods/POST/") - 1;

	/* Null-terminate the method name */
	ptr = strtok(ptr, "/");
	if ((ptr == NULL) || ((ptr + strlen(ptr)) > max_ptr)) {
		LOG_ERR("Invalid method name");
		return false;
	}

	evt.data.method.name = ptr;

	LOG_DBG("Direct method name: %s", log_strdup(evt.data.method.name));

	/* Move pointer to start of request ID, {req ID} in topic */
	ptr = ptr + strlen(ptr) + sizeof("/?$rid=") - 1;
	if ((ptr > max_ptr) || ((max_ptr - ptr) > sizeof(rid_buf))) {
		LOG_ERR("Invalid request ID");
		return false;
	}

	/* Get request ID */
	memcpy(rid_buf, ptr, max_ptr - ptr + 1);
	rid_buf[max_ptr - ptr + 1] = '\0';
	evt.data.method.rid = atoi(rid_buf);

	LOG_DBG("Direct method request ID: %d", evt.data.method.rid);

	azure_iot_hub_notify_event(&evt);

	return true;
}

static void device_twin_request(void)
{
	int err;
	struct azure_iot_hub_data msg = {
		.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REQUEST,
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("Failed to send device twin request, error: %d", err);
		return;
	}

	LOG_DBG("Device twin requested");
}

static void device_twin_result_process(char *topic, size_t topic_len,
				       char *payload, size_t payload_len)
{
	int err;
	struct azure_iot_hub_evt evt = {
		.topic.str = topic,
		.topic.len = topic_len,
		.data.msg.ptr = payload,
		.data.msg.len = payload_len,
	};

	err = get_device_twin_result(topic, topic_len, &evt.data.result);
	if (err) {
		LOG_ERR("Failed to process report result");
		return;
	}

	/* Status codes
	 *	200: Response to request for device twin from
	 *	     the device.
	 *	204: Successful update of the device twin's
	 *	     "reported" object.
	 *	400: Bad request, malformed JSON?
	 *	429: Too many requests, check IoT hub throttle
	 *	     settings.
	 *	5xx: Server errors
	 */
	switch (evt.data.result.status) {
	case 200:
#if IS_ENABLED(CONFIG_AZURE_FOTA)
		err = azure_fota_msg_process(payload_buf, payload_len);
		if (err < 0) {
			LOG_ERR("Failed to process FOTA msg");
			return;
		} else if (err == 1) {
			LOG_DBG("FOTA message handled");
			return;
		}
#endif /* IS_ENABLED(CONFIG_AZURE_FOTA) */
		evt.type = AZURE_IOT_HUB_EVT_TWIN_RECEIVED;
		break;
	case 204:
		evt.type = AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS;
		break;
	case 400:
		LOG_DBG("Bad twin request, malformed JSON?");
		evt.type = AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL;
		break;
	case 429:
		LOG_DBG("Too many requests");
		evt.type = AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL;
		break;
	default:
		evt.type = AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL;
		break;
	}

	azure_iot_hub_notify_event(&evt);
}

/* Returns true if event should not be propagated from the calling function */
static bool on_suback(void)
{
#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
	if (dps_reg_in_progress()) {
		int err = dps_send_reg_request();

		if (err) {
			LOG_ERR("DPS registration not sent, error: %d",
				err);
		} else {
			LOG_DBG("DPS registration request sent");
		}

		return true;
	}
#endif /* IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS) */

	if (IS_ENABLED(CONFIG_AZURE_IOT_HUB_AUTO_DEVICE_TWIN_REQUEST) ||
	    IS_ENABLED(CONFIG_AZURE_FOTA)) {
		device_twin_request();
	}

	return false;
}

static void on_publish(struct mqtt_client *const client,
			   const struct mqtt_evt *mqtt_evt)
{
	int err;
	const struct mqtt_publish_param *p = &mqtt_evt->param.publish;
	char *topic = (char *)p->message.topic.topic.utf8;
	size_t topic_len = p->message.topic.topic.size;
	size_t payload_len = p->message.payload.len;
	struct azure_iot_hub_evt evt = {
		.type = AZURE_IOT_HUB_EVT_DATA_RECEIVED,
		.data.msg.ptr = payload_buf,
		.data.msg.len = payload_len,
		.topic.str = (char *)topic,
		.topic.len = topic_len,
	};

	LOG_DBG("MQTT_EVT_PUBLISH: id = %d, len = %d ",
		p->message_id, payload_len);

	err = publish_get_payload(client, payload_len);
	if (err) {
		LOG_ERR("publish_get_payload, error: %d", err);
		return;
	}

	if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
		const struct mqtt_puback_param ack = {
			.message_id = p->message_id
		};

		err = mqtt_publish_qos1_ack(client, &ack);
		if (err) {
			LOG_WRN("Failed to send MQTT ACK, error: %d",
				err);
		}
	}

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
	if (dps_reg_in_progress()) {
		if (dps_process_message(&evt)) {
			/* The message has been processed */
			return;
		}
	}
#endif

	/* Check if it's a direct method invocation */
	if (is_direct_method(topic, topic_len)) {
		if (direct_method_process((char *)topic, topic_len,
						payload_buf, payload_len)) {
			LOG_DBG("Direct method processed");
			return;
		}

		LOG_WRN("Unhandled direct method invocation");
		return;
	}

	/* Check if message is a desired device twin change */
	if (is_device_twin_update(topic, topic_len)) {
#if IS_ENABLED(CONFIG_AZURE_FOTA)
		err = azure_fota_msg_process(payload_buf, payload_len);
		if (err < 0) {
			LOG_ERR("Failed to process FOTA message");
			return;
		} else if (err == 1) {
			LOG_DBG("Device twin update handled (FOTA)");
			return;
		}
#endif /* IS_ENABLED(CONFIG_AZURE_FOTA) */
		evt.type = AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED;

		azure_iot_hub_notify_event(&evt);
	} else if (is_device_twin_result(topic, topic_len)) {
		LOG_DBG("Device twin data received");

		device_twin_result_process(topic, topic_len, payload_buf,
					   payload_len);
	} else {
		azure_iot_hub_notify_event(&evt);
	}
}

static void mqtt_evt_handler(struct mqtt_client *const client,
			     const struct mqtt_evt *mqtt_evt)
{
	int err;
	struct azure_iot_hub_evt evt = { 0 };

	switch (mqtt_evt->type) {
	case MQTT_EVT_CONNACK:
		if (mqtt_evt->param.connack.return_code !=
		    MQTT_CONNECTION_ACCEPTED) {
			LOG_ERR("Connection was rejected with return code %d",
				mqtt_evt->param.connack.return_code);
			LOG_WRN("Is the device certificate valid?");
				return;
		}

		connection_state = STATE_CONNECTED;

		LOG_DBG("MQTT client connected");

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
		if (dps_reg_in_progress()) {
			err = dps_subscribe();
			evt.type = AZURE_IOT_HUB_EVT_DPS_REGISTERING;
		} else
#endif
		{
			err = topic_subscribe();
			evt.type = AZURE_IOT_HUB_EVT_CONNECTED;
		}

		if (err) {
			LOG_ERR("Failed to subscribe to topics, err: %d", err);
		}

		azure_iot_hub_notify_event(&evt);
		break;
	case MQTT_EVT_DISCONNECT:
		LOG_DBG("MQTT_EVT_DISCONNECT: result = %d", mqtt_evt->result);

		connection_state = STATE_INIT;
		evt.type = AZURE_IOT_HUB_EVT_DISCONNECTED;

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
		if (dps_reg_in_progress()) {
			if (dps_process_message(&evt)) {
				/* The message has been processed */
				break;
			}
		}
#endif
		if (atomic_get(&dps_disconnecting) == 1) {
			break;
		}

		azure_iot_hub_notify_event(&evt);
		break;
	case MQTT_EVT_PUBLISH:
		on_publish(client, mqtt_evt);
		break;
	case MQTT_EVT_PUBACK:
		LOG_DBG("MQTT_EVT_PUBACK: id = %d result = %d",
			mqtt_evt->param.puback.message_id,
			mqtt_evt->result);

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
		if (dps_reg_in_progress()) {
			break;
		}
#endif

		evt.type = AZURE_IOT_HUB_EVT_PUBACK;
		evt.data.message_id = mqtt_evt->param.puback.message_id;

		azure_iot_hub_notify_event(&evt);
		break;
	case MQTT_EVT_SUBACK:
		LOG_DBG("MQTT_EVT_SUBACK: id = %d result = %d",
			mqtt_evt->param.suback.message_id,
			mqtt_evt->result);

		if (on_suback()) {
			/* Event DPS-related, and should not be propagated */
			break;
		}

		evt.type = AZURE_IOT_HUB_EVT_READY;
		azure_iot_hub_notify_event(&evt);
		break;
	default:
		break;
	}
}

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
static struct mqtt_utf8 *user_name_get(void)
{
	ssize_t len;

	len = snprintk(user_name_buf, sizeof(user_name_buf), USER_NAME_TEMPLATE,
		       dps_hostname_get(), conn_config.device_id);
	if ((len < 0) || (len > sizeof(user_name_buf))) {
		LOG_ERR("Failed to create user name");
		return NULL;
	}
	user_name.size = len;
	return &user_name;
}
#endif

#if defined(CONFIG_AZURE_IOT_HUB_STATIC_IPV4)
static int broker_init(bool dps)
{
	struct sockaddr_in *broker4 =
		((struct sockaddr_in *)&broker);

	inet_pton(AF_INET, CONFIG_AZURE_IOT_HUB_STATIC_IPV4_ADDR,
		  &broker->sin_addr);
	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(CONFIG_AZURE_IOT_HUB_PORT);

	LOG_DBG("IPv4 Address %s",
		log_strdup(CONFIG_AZURE_IOT_HUB_STATIC_IPV4_ADDR));

	return 0;
}
#else
static int broker_init(bool dps)
{
	int err;
	struct addrinfo *result;
	struct addrinfo *addr;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM
	};

	if (dps) {
#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
		err = getaddrinfo(CONFIG_AZURE_IOT_HUB_DPS_HOSTNAME,
				  NULL, &hints, &result);
		if (err) {
			LOG_ERR("getaddrinfo, error %d", err);
			return -err;
		}
#else
		LOG_ERR("DPS is not enabled");

		return -ENOTSUP;
#endif /* CONFIG_AZURE_IOT_HUB_DPS */
	} else {
		if (IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)) {
			err = getaddrinfo(dps_hostname_get(),
					  NULL, &hints, &result);
		} else {
			err = getaddrinfo(CONFIG_AZURE_IOT_HUB_HOSTNAME,
					  NULL, &hints, &result);
		}
	}

	if (err) {
		LOG_ERR("getaddrinfo, error %d", err);
		return -err;
	}

	addr = result;

	while (addr != NULL) {
		if (addr->ai_addrlen == sizeof(struct sockaddr_in)) {
			struct sockaddr_in *broker4 =
				((struct sockaddr_in *)&broker);
			char ipv4_addr[NET_IPV4_ADDR_LEN];

			broker4->sin_addr.s_addr =
				((struct sockaddr_in *)addr->ai_addr)
				->sin_addr.s_addr;
			broker4->sin_family = AF_INET;
			broker4->sin_port = htons(CONFIG_AZURE_IOT_HUB_PORT);

			inet_ntop(AF_INET, &broker4->sin_addr.s_addr, ipv4_addr,
				  sizeof(ipv4_addr));
			LOG_DBG("IPv4 Address found %s", log_strdup(ipv4_addr));
			break;
		}

		LOG_DBG("ai_addrlen = %u should be %u or %u",
			(unsigned int)addr->ai_addrlen,
			(unsigned int)sizeof(struct sockaddr_in),
			(unsigned int)sizeof(struct sockaddr_in6));

		addr = addr->ai_next;
		break;
	}

	freeaddrinfo(result);

	return err;
}
#endif /* !defined(CONFIG_AZURE_IOT_HUB_STATIC_IPV4) */

#if !defined(CONFIG_BSD_LIBRARY)
static int certificates_provision(void)
{
	static bool certs_added;
	int err;

	if (!IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) || certs_added) {
		return 0;
	}

	err = tls_credential_add(CONFIG_AZURE_IOT_HUB_SEC_TAG,
				 TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_certificate,
				 sizeof(ca_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register CA certificate: %d",
			err);
		return err;
	}

	err = tls_credential_add(CONFIG_AZURE_IOT_HUB_SEC_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 private_key,
				 sizeof(private_key));
	if (err < 0) {
		LOG_ERR("Failed to register private key: %d", err);
		return err;
	}

	err = tls_credential_add(CONFIG_AZURE_IOT_HUB_SEC_TAG,
				 TLS_CREDENTIAL_SERVER_CERTIFICATE,
				 device_certificate,
				 sizeof(device_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register public certificate: %d",
			err);
		return err;
	}

	certs_added = true;

	return 0;
}
#endif /* !defined(CONFIG_BSD_LIBRARY) */

static int client_broker_init(struct mqtt_client *const client, bool dps)
{
	int err;
	static sec_tag_t sec_tag_list[] = { CONFIG_AZURE_IOT_HUB_SEC_TAG };
	struct mqtt_sec_config *tls_cfg = &(client->transport).tls.config;

	mqtt_client_init(client);

	err = broker_init(dps);
	if (err) {
		return err;
	}

	/* The following parameters differ between DPS and IoT hub:
	 *	client->user_name
	 *	tls_cfg->hostname
	 */

	client->broker			= &broker;
	client->evt_cb			= mqtt_evt_handler;
	client->client_id.utf8		= conn_config.device_id;
	client->client_id.size		= strlen(conn_config.device_id);
	client->password		= NULL;
	client->protocol_version	= MQTT_VERSION_3_1_1;
	client->rx_buf			= rx_buffer;
	client->rx_buf_size		= sizeof(rx_buffer);
	client->tx_buf			= tx_buffer;
	client->tx_buf_size		= sizeof(tx_buffer);
	client->transport.type		= MQTT_TRANSPORT_SECURE;

	tls_cfg->peer_verify		= TLS_PEER_VERIFY_REQUIRED;
	tls_cfg->cipher_count		= 0;
	tls_cfg->cipher_list		= NULL; /* Use default */
	tls_cfg->sec_tag_count		= ARRAY_SIZE(sec_tag_list);
	tls_cfg->sec_tag_list		= sec_tag_list;

#if defined(CONFIG_BSD_LIBRARY)
	tls_cfg->session_cache		=
		IS_ENABLED(CONFIG_AZURE_IOT_HUB_TLS_SESSION_CACHING) ?
			TLS_SESSION_CACHE_ENABLED : TLS_SESSION_CACHE_DISABLED;
#else
	/* TLS session caching is not supported by the Zephyr network stack */
	tls_cfg->session_cache = TLS_SESSION_CACHE_DISABLED;

	err = certificates_provision();
	if (err) {
		LOG_ERR("Could not provision certificates, error: %d", err);
		return err;
	}
#endif /* !defined(CONFIG_BSD_LIBRARY) */

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
	if (dps_get_reg_state() == DPS_STATE_REGISTERING) {
		ssize_t len;
		static char dps_user_name_buf[100];
		static struct mqtt_utf8 dps_user_name = {
			.utf8 = dps_user_name_buf,
		};

		len = snprintk(dps_user_name_buf,
			       sizeof(dps_user_name_buf),
			       DPS_USER_NAME, conn_config.device_id);
		if ((len < 0) || len > sizeof(dps_user_name_buf)) {
			LOG_ERR("Failed to set DPS user name");
			return -EFAULT;
		}

		client->user_name = &dps_user_name;
		client->user_name->size = len;
		tls_cfg->hostname = CONFIG_AZURE_IOT_HUB_DPS_HOSTNAME;

		LOG_DBG("Using DPS configuration for MQTT connection");
	} else {
		struct mqtt_utf8 *user_name_ptr = user_name_get();

		if (user_name_ptr == NULL) {
			LOG_ERR("No user name set, aborting");
			return -EFAULT;
		}

		tls_cfg->hostname = dps_hostname_get();
		client->user_name = user_name_get();
	}
#else /* IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS) */
	tls_cfg->hostname = CONFIG_AZURE_IOT_HUB_HOSTNAME;
	client->user_name = &user_name;
#endif/* !IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS) */

	return 0;
}

static int connect_client(struct azure_iot_hub_config *cfg)
{
	int err;
	bool use_dps =
		IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS) &&
		(dps_get_reg_state() != DPS_STATE_REGISTERED);
	struct azure_iot_hub_evt evt = {
		.type = AZURE_IOT_HUB_EVT_CONNECTING,
	};

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
	if (use_dps) {
		evt.type = AZURE_IOT_HUB_EVT_DPS_CONNECTING;

		err = dps_start();
		if (err == -EALREADY) {
			use_dps = false;
			LOG_INF("The device is already registered to IoT hub");
		} else if (err == -EFAULT) {
			LOG_ERR("Failed to start DPS");
			return err;
		}
	}
#endif /* IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS) */

	err = client_broker_init(&client, use_dps);
	if (err) {
		LOG_ERR("client_broker_init, error: %d", err);
		return err;
	}

	connection_state = STATE_CONNECTING;

	/* Notify _CONNECTING event, either to IoT hub or DPS */
	azure_iot_hub_notify_event(&evt);

	err = mqtt_connect(&client);
	if (err) {
		LOG_ERR("mqtt_connect, error: %d", err);
		return err;
	}

	/* Set the current socket and start reading from it in polling thread */
	conn_config.socket = client.transport.tls.sock;

	k_sem_give(&connection_poll_sem);

	return 0;
}

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
/* Callback handler for DPS events */
static void dps_handler(enum dps_reg_state state)
{
	int err;
	struct azure_iot_hub_evt evt;

	/* Whenever this handler is called, it's expected to cause a
	 * DPS disconnection. This state variable is set before any event
	 * handlers are called in case the user chooses to manually disconnect
	 * in one of those.
	 */
	atomic_set(&dps_disconnecting, 1);

	switch (state) {
	case DPS_STATE_REGISTERED:
		evt.type = AZURE_IOT_HUB_EVT_DPS_DONE;
		azure_iot_hub_notify_event(&evt);
		break;
	case DPS_STATE_FAILED:
		LOG_ERR("The DPS registration was not successful");
		evt.type = AZURE_IOT_HUB_EVT_DPS_FAILED;
		azure_iot_hub_notify_event(&evt);
		break;
	default:
		LOG_WRN("Unhandled DPS state: %d", state);
		break;
	}

	LOG_DBG("Disconnecting from DPS server");

	err = azure_iot_hub_disconnect();
	if (err) {
		LOG_ERR("Failed to disconnect from DPS, error: %d", err);

		if (state == DPS_STATE_REGISTERED) {
			LOG_WRN("Trying to connect to Azure IoT Hub anyway");
		} else {
			atomic_set(&dps_disconnecting, 0);
			return;
		}
	}

	atomic_set(&dps_disconnecting, 0);

	if (state != DPS_STATE_REGISTERED) {
		/* We have no IoT hub to connect to */
		return;
	}

	LOG_DBG("Connecting to assigned IoT hub (%s)",
		log_strdup(dps_hostname_get()));

	err = connect_client(&conn_config);
	if (err) {
		LOG_ERR("Failed connection to IoT hub, err: %d", err);
	}
}
#endif

#if IS_ENABLED(CONFIG_AZURE_FOTA)

static void fota_report_send(struct azure_fota_event *evt)
{
	int err;
	struct azure_iot_hub_data msg = {
		.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REPORTED,
		.ptr = evt->report,
		.len = evt->report ? strlen(evt->report) : 0,
	};

	if (evt->report == NULL) {
		LOG_WRN("No report available to send");
		return;
	}

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("Failed to send FOTA report, error: %d", err);
		return;
	}

	LOG_DBG("FOTA report sent");
}

static void fota_evt_handler(struct azure_fota_event *fota_evt)
{
	struct azure_iot_hub_evt evt = { 0 };

	if (fota_evt == NULL) {
		return;
	}

	switch (fota_evt->type) {
	case AZURE_FOTA_EVT_REPORT:
		LOG_DBG("AZURE_FOTA_EVT_REPORT");
		fota_report_send(fota_evt);
		break;
	case AZURE_FOTA_EVT_START:
		LOG_DBG("AZURE_FOTA_EVT_START");
		evt.type = AZURE_IOT_HUB_EVT_FOTA_START;
		fota_report_send(fota_evt);
		azure_iot_hub_notify_event(&evt);
		break;
	case AZURE_FOTA_EVT_DONE:
		LOG_DBG("AZURE_FOTA_EVT_DONE");
		evt.type = AZURE_IOT_HUB_EVT_FOTA_DONE;
		fota_report_send(fota_evt);
		azure_iot_hub_notify_event(&evt);
		break;
	case AZURE_FOTA_EVT_ERASE_PENDING:
		LOG_DBG("AZURE_FOTA_EVT_ERASE_PENDING");
		evt.type = AZURE_IOT_HUB_EVT_FOTA_ERASE_PENDING;
		azure_iot_hub_notify_event(&evt);
		break;
	case AZURE_FOTA_EVT_ERASE_DONE:
		LOG_DBG("AZURE_FOTA_EVT_ERASE_DONE");
		evt.type = AZURE_IOT_HUB_EVT_FOTA_ERASE_DONE;
		azure_iot_hub_notify_event(&evt);
		break;
	case AZURE_FOTA_EVT_ERROR:
		LOG_ERR("AZURE_FOTA_EVT_ERROR");
		fota_report_send(fota_evt);
		evt.type = AZURE_IOT_HUB_EVT_FOTA_ERROR;
		break;
	default:
		LOG_ERR("Unhandled FOTA event, type: %d", fota_evt->type);
		break;
	}
}
#endif /* CONFIG_AZURE_FOTA */

/* Public interface */

int azure_iot_hub_ping(void)
{
	return mqtt_live(&client);
}

uint32_t azure_iot_hub_keepalive_time_left(void)
{
	return mqtt_keepalive_time_left(&client);
}

int azure_iot_hub_input(void)
{
	return mqtt_input(&client);
}

int azure_iot_hub_send(const struct azure_iot_hub_data *const tx_data)
{
	ssize_t len;
	static char topic[100];
	struct mqtt_publish_param param = {
		.message.payload.data = tx_data->ptr,
		.message.payload.len = tx_data->len,
		.message.topic.qos = tx_data->qos,
		.message_id = tx_data->message_id,
		.dup_flag = tx_data->dup_flag,
		.retain_flag = tx_data->retain_flag,
	};

	if (connection_state != STATE_CONNECTED) {
		LOG_ERR("Azure IoT Hub is not connected");
		return -EACCES;
	}

	switch (tx_data->topic.type) {
	case AZURE_IOT_HUB_TOPIC_EVENT:
		len = snprintk(topic, sizeof(topic),
			       TOPIC_EVENTS, conn_config.device_id);
		if ((len < 0) || (len > sizeof(topic))) {
			LOG_ERR("Failed to create event topic");
			return -ENOMEM;
		}
		break;
	case AZURE_IOT_HUB_TOPIC_TWIN_REPORTED:
		len = snprintk(topic, sizeof(topic),
				TOPIC_TWIN_REPORT, k_uptime_get_32());
		if ((len < 0) || (len > sizeof(topic))) {
			LOG_ERR("Failed to create twin report topic");
			return -ENOMEM;
		}
		break;
	case AZURE_IOT_HUB_TOPIC_TWIN_REQUEST:
		len = snprintk(topic, sizeof(topic),
			       TOPIC_TWIN_REQUEST, k_uptime_get_32());
		if ((len < 0) || (len > sizeof(topic))) {
			LOG_ERR("Failed to create device twin request");
			return -ENOMEM;
		}
		break;
	default:
		LOG_ERR("No topic specified");
		return -ENOMSG;
	}

	param.message.topic.topic.size = len;
	param.message.topic.topic.utf8 = topic;

	LOG_DBG("Publishing to topic: %s",
		log_strdup(param.message.topic.topic.utf8));

	return mqtt_publish(&client, &param);
}

int azure_iot_hub_disconnect(void)
{
	int err;

	if (connection_state != STATE_CONNECTED) {
		LOG_ERR("Azure IoT Hub is not connected");
		return -EACCES;
	}

	connection_state = STATE_DISCONNECTING;

	err = mqtt_disconnect(&client);
	if (err) {
		LOG_ERR("Failed to disconnect MQTT client, error: %d", err);
		return err;
	}

	connection_state = STATE_INIT;

	return 0;
}

int azure_iot_hub_connect(void)
{
	if (connection_state != STATE_INIT) {
		LOG_ERR("Azure IoT Hub is not initialized");
		return -EACCES;
	}

	return connect_client(&conn_config);
}

int azure_iot_hub_init(const struct azure_iot_hub_config *const config,
		       const azure_iot_hub_evt_handler_t event_handler)
{
	int err;

	if (connection_state != STATE_IDLE) {
		LOG_ERR("Azure IoT Hub is already initialized");
		return -EALREADY;
	}

	if (event_handler == NULL) {
		LOG_ERR("Event handler must be provided");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_AZURE_IOT_HUB_DEVICE_ID_APP)) {
		if (config == NULL) {
			LOG_ERR("Configuration must be provided");
			return -EINVAL;
		} else if (config->device_id == NULL) {
			LOG_ERR("Client ID not set in the application");
			return -EINVAL;
		} else if (config->device_id_len >
			   CONFIG_AZURE_IOT_HUB_DEVICE_ID_MAX_LEN) {
			LOG_ERR("Device ID is too long, maximum length is %d",
				CONFIG_AZURE_IOT_HUB_DEVICE_ID_MAX_LEN);
			return -ENOMEM;
		}

		memcpy(conn_config.device_id, config->device_id,
		       config->device_id_len);
		conn_config.device_id_len = config->device_id_len;
	}

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
	struct dps_config cfg = {
		.mqtt_client = &client,
		.handler = dps_handler,
		.reg_id = conn_config.device_id,
		.reg_id_len = conn_config.device_id_len,
	};

	err = dps_init(&cfg);
	if (err) {
		LOG_ERR("Failed to initialize DPS, error: %d", err);
		return err;
	}

	LOG_DBG("DPS initialized");
#endif /* IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS) */

#if IS_ENABLED(CONFIG_AZURE_FOTA)
	err = azure_fota_init(fota_evt_handler);
	if (err) {
		LOG_ERR("Failed to initialize Azure FOTA, error: %d", err);
		return err;
	}

	LOG_DBG("Azure FOTA initialized");
#endif

	evt_handler = event_handler;
	is_initialized = true;
	(void)err;

	connection_state = STATE_INIT;

	return 0;
}

int azure_iot_hub_method_respond(
	struct azure_iot_hub_result *result)
{
	ssize_t len;
	static char topic[100];
	struct mqtt_publish_param param = {
		.message.payload.data = (char *)result->payload,
		.message.payload.len = result->payload_len,
		.message.topic.topic.utf8 = topic,
	};

	if (connection_state != STATE_CONNECTED) {
		LOG_ERR("Azure IoT Hub is not connected");
		return -EACCES;
	}

	len = snprintk(topic, sizeof(topic), TOPIC_DIRECT_METHOD_RES,
			result->status, result->rid);
	if ((len < 0) || (len > sizeof(topic))) {
		LOG_ERR("Failed to create method result topic");
		return -ENOMEM;
	}

	param.message.topic.topic.size = len;

	LOG_DBG("Publishing to topic: %s",
		log_strdup(param.message.topic.topic.utf8));

	return mqtt_publish(&client, &param);
}

static void azure_iot_hub_run(void)
{
	int ret;
	struct pollfd fds[1];
start:
	k_sem_take(&connection_poll_sem, K_FOREVER);

	fds[0].events = POLLIN;
	fds[0].fd = conn_config.socket;

	while (true) {
		ret = poll(fds, ARRAY_SIZE(fds),
			mqtt_keepalive_time_left(&client));
		if (ret == 0) {
			if (mqtt_keepalive_time_left(&client) < 1000) {
				azure_iot_hub_ping();
			}
			continue;
		}

		if ((fds[0].revents & POLLIN) == POLLIN) {
			azure_iot_hub_input();

			/* The connection might have changed during
			 * call to azure_iot_hub_input(), as MQTT callbacks
			 * are called from this context, which again allow
			 * connect()/disconnect() functions to be called.
			 * Switching from DPS to IoT hub is an example of this.
			 * Therefore, the fd to poll must be updated to
			 * account for the event that it has changed.
			 */
			fds[0].fd = conn_config.socket;
			continue;
		}

		if (ret < 0) {
			LOG_ERR("poll() returned an error: %d", -errno);
			goto start;
		}

		if ((fds[0].revents & POLLNVAL) == POLLNVAL) {
			if (connection_state == STATE_DISCONNECTING) {
				/* POLLNVAL is to be expected while
				 * disconnecting, as the socket will be closed
				 * by the MQTT library and become invalid.
				 */
				LOG_DBG("POLLNVAL while disconnecting");
				goto start;
			} else if (connection_state == STATE_INIT) {
				LOG_DBG("POLLNVAL, no active connection");
				goto start;
			} else {
				LOG_DBG("Socket error: POLLNVAL");
				LOG_DBG("The socket was unexpectedly closed");
			}

			goto start;
		}

		if ((fds[0].revents & POLLHUP) == POLLHUP) {
			LOG_DBG("Socket error: POLLHUP");
			LOG_DBG("Connection was unexpectedly closed");
			goto start;
		}

		if ((fds[0].revents & POLLERR) == POLLERR) {
			LOG_DBG("Socket error: POLLERR");
			LOG_DBG("Connection was unexpectedly closed");
			goto start;
		}
	}
}

K_THREAD_DEFINE(connection_poll_thread, CONFIG_AZURE_IOT_HUB_STACK_SIZE,
		azure_iot_hub_run, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
