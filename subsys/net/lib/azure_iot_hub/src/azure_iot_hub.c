/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/mqtt.h>
#include <zephyr/settings/settings.h>
#include <stdio.h>

#include <net/azure_iot_hub_dps.h>
#include <net/azure_iot_hub.h>

#include <azure/az_core.h>
#include <azure/az_iot.h>

#include <net/mqtt_helper.h>

#if defined(CONFIG_AZURE_FOTA)
#include <net/azure_fota.h>
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(azure_iot_hub, CONFIG_AZURE_IOT_HUB_LOG_LEVEL);

/* Define a custom AZ_HUB_STATIC macro that exposes internal variables when unit testing. */
#if defined(CONFIG_UNITY)
#define AZ_HUB_STATIC
#else
#define AZ_HUB_STATIC static
#endif

static azure_iot_hub_evt_handler_t evt_handler;

AZ_HUB_STATIC enum iot_hub_state {
	/* The library is uninitialized. */
	STATE_UNINIT,
	/* The library is initialized, no connection established. */
	STATE_DISCONNECTED,
	/* Connecting to Azure IoT Hub. */
	STATE_CONNECTING,
	/* Connected to Azure IoT Hub on TLS level. */
	STATE_TRANSPORT_CONNECTED,
	/* Connected to Azure IoT Hub on MQTT level. */
	STATE_CONNECTED,
	/* Disconnecting from Azure IoT Hub. */
	STATE_DISCONNECTING,

	STATE_COUNT,
} iot_hub_state = STATE_UNINIT;

static K_SEM_DEFINE(connected, 0, 1);
#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
static K_SEM_DEFINE(dps_sem, 0, 1);
#endif

/* Azure SDK for Embedded C variables. */
static az_iot_hub_client iot_hub_client;

/* Static functions */
static void azure_iot_hub_notify_event(struct azure_iot_hub_evt *evt)
{
	if (evt_handler) {
		evt_handler(evt);
	}
}

static const char *state_name_get(enum iot_hub_state state)
{
	switch (state) {
	case STATE_UNINIT: return "STATE_UNINIT";
	case STATE_DISCONNECTED: return "STATE_DISCONNECTED";
	case STATE_CONNECTING: return "STATE_CONNECTING";
	case STATE_TRANSPORT_CONNECTED: return "STATE_TRANSPORT_CONNECTED";
	case STATE_CONNECTED: return "STATE_CONNECTED";
	case STATE_DISCONNECTING: return "STATE_DISCONNECTING";
	default: return "STATE_UNKNOWN";
	}
}

AZ_HUB_STATIC void iot_hub_state_set(enum iot_hub_state new_state)
{
	bool notify_error = false;

	if (iot_hub_state == new_state) {
		LOG_DBG("Skipping transition to the same state (%s)",
			state_name_get(iot_hub_state));
		return;
	}

	/* Check for legal state transitions. */
	switch (iot_hub_state) {
	case STATE_UNINIT:
		if (new_state != STATE_DISCONNECTED) {
			notify_error = true;
		}
		break;
	case STATE_DISCONNECTED:
		if (new_state != STATE_CONNECTING) {
			notify_error = true;
		}
		break;
	case STATE_CONNECTING:
		if (new_state != STATE_CONNECTED &&
		    new_state != STATE_DISCONNECTED) {
			notify_error = true;
		}
		break;
	case STATE_TRANSPORT_CONNECTED:
		if (new_state != STATE_CONNECTING) {
			notify_error = true;
		}
		break;
	case STATE_CONNECTED:
		if (new_state != STATE_DISCONNECTING &&
		    new_state != STATE_DISCONNECTED) {
			notify_error = true;
		}
		break;
	case STATE_DISCONNECTING:
		if ((new_state != STATE_DISCONNECTED) &&
		    (new_state != STATE_TRANSPORT_CONNECTED)) {
			notify_error = true;
		}
		break;
	default:
		LOG_ERR("New state is unknown");
		notify_error = true;
		break;
	}

	if (notify_error) {
		struct azure_iot_hub_evt evt = {
			.type = AZURE_IOT_HUB_EVT_ERROR,
			.data.err = -EINVAL
		};

		LOG_ERR("Invalid connection state transition, %s --> %s",
			state_name_get(iot_hub_state),
			state_name_get(new_state));

		azure_iot_hub_notify_event(&evt);
		return;
	}

	LOG_DBG("State transition: %s --> %s",
		state_name_get(iot_hub_state),
		state_name_get(new_state));

	iot_hub_state = new_state;
}

enum iot_hub_state iot_hub_state_get(void)
{
	return iot_hub_state;
}

static bool iot_hub_state_verify(enum iot_hub_state state)
{
	return (iot_hub_state_get() == state);
}

static int topic_subscribe(void)
{
	int err;
	struct mqtt_topic sub_topics[] = {
		{
			.topic.utf8 = AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC,
			.topic.size = sizeof(AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC) - 1,
		},
		{
			.topic.size = sizeof(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC) - 1,
			.topic.utf8 = AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC,
		},
		{
			.topic.size = sizeof(AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC) - 1,
			.topic.utf8 = AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC,
		},
		{
			.topic.size = sizeof(AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC) - 1,
			.topic.utf8 = AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC,
		},
	};
	struct mqtt_subscription_list sub_list = {
		.list = sub_topics,
		.list_count = ARRAY_SIZE(sub_topics),
		.message_id = k_uptime_get_32(),
	};

	err = mqtt_helper_subscribe(&sub_list);
	if (err) {
		LOG_ERR("Failed to subscribe to topic list, error: %d", err);
		return err;
	}

	LOG_DBG("Successfully subscribed to default topics");

	return 0;
}


static void device_twin_result_process(az_iot_hub_client_twin_response *twin_msg,
				       struct azure_iot_hub_buf topic,
				       struct azure_iot_hub_buf payload)
{
	struct azure_iot_hub_evt evt = {
		.topic.name.ptr = topic.ptr,
		.topic.name.size = topic.size,
		.data.msg.payload = payload,
	};

	LOG_DBG("Request ID: %.*s", az_span_size(twin_msg->request_id),
				    az_span_ptr(twin_msg->request_id));

	switch (twin_msg->status) {
	case AZ_IOT_STATUS_OK: {
#if IS_ENABLED(CONFIG_AZURE_FOTA)
		int err = azure_fota_msg_process(payload.ptr, payload.size);

		if (err < 0) {
			LOG_ERR("Failed to process FOTA msg");
		} else if (err == 1) {
			LOG_DBG("FOTA message handled");
		}

		/* Forward the device twin to the application. */
#endif /* IS_ENABLED(CONFIG_AZURE_FOTA) */
		evt.type = AZURE_IOT_HUB_EVT_TWIN_RECEIVED;
		break;
	}
	case AZ_IOT_STATUS_ACCEPTED:
	case AZ_IOT_STATUS_NO_CONTENT:
		evt.type = AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS;
		evt.data.result.request_id.ptr = az_span_ptr(twin_msg->request_id);
		evt.data.result.request_id.size = az_span_size(twin_msg->request_id);
		evt.data.result.status = twin_msg->status;
		evt.data.result.payload = payload;
		break;
	case AZ_IOT_STATUS_BAD_REQUEST:
	case AZ_IOT_STATUS_UNAUTHORIZED:
	case AZ_IOT_STATUS_FORBIDDEN:
	case AZ_IOT_STATUS_NOT_FOUND:
	case AZ_IOT_STATUS_NOT_ALLOWED:
	case AZ_IOT_STATUS_NOT_CONFLICT:
	case AZ_IOT_STATUS_PRECONDITION_FAILED:
	case AZ_IOT_STATUS_REQUEST_TOO_LARGE:
	case AZ_IOT_STATUS_UNSUPPORTED_TYPE:
	case AZ_IOT_STATUS_THROTTLED:
	case AZ_IOT_STATUS_CLIENT_CLOSED:
	case AZ_IOT_STATUS_SERVER_ERROR:
	case AZ_IOT_STATUS_BAD_GATEWAY:
	case AZ_IOT_STATUS_SERVICE_UNAVAILABLE:
	case AZ_IOT_STATUS_TIMEOUT:
		LOG_DBG("Failed twin request, status code: %d", twin_msg->status);
		evt.type = AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL;
		break;
	default:
		LOG_ERR("Unrecognized status code: %d", twin_msg->status);
		evt.type = AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL;
		break;
	}

	azure_iot_hub_notify_event(&evt);
}

static void device_twin_request(void)
{
	int err;
	struct azure_iot_hub_msg msg = {
		.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REQUEST,
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("Failed to send device twin request, error: %d", err);
		return;
	}

	LOG_DBG("Device twin requested");
}

/* Returns true if the direct method was processed and no error occurred. */
static bool direct_method_process(az_iot_hub_client_method_request *method_request,
				  struct mqtt_helper_buf topic,
				  struct mqtt_helper_buf payload)
{
	struct azure_iot_hub_evt evt = {
		.type = AZURE_IOT_HUB_EVT_DIRECT_METHOD,
		.topic.name.ptr = topic.ptr,
		.topic.name.size = topic.size,
		.data.method.payload.ptr = payload.ptr,
		.data.method.payload.size = payload.size,
	};

	if (az_span_size(method_request->name) == 0) {
		LOG_WRN("No direct method name, event will not be notified");
		return false;
	}

	if (az_span_size(method_request->request_id) == 0) {
		LOG_WRN("No request ID, direct method processing aborted");
		return false;
	}

	evt.data.method.name.ptr = az_span_ptr(method_request->name);
	evt.data.method.name.size = az_span_size(method_request->name);

	evt.data.method.request_id.ptr = az_span_ptr(method_request->request_id);
	evt.data.method.request_id.size = az_span_size(method_request->request_id);

	LOG_DBG("Direct method name: %.*s", evt.data.method.name.size, evt.data.method.name.ptr);
	LOG_DBG("Direct method request ID: %.*s",
		evt.data.method.request_id.size,
		evt.data.method.request_id.ptr);

	azure_iot_hub_notify_event(&evt);

	return true;
}

AZ_HUB_STATIC void on_publish(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload)
{
	struct azure_iot_hub_property properties[CONFIG_AZURE_IOT_HUB_MSG_PROPERTY_RECV_MAX_COUNT];
	struct azure_iot_hub_evt evt = {
		.type = AZURE_IOT_HUB_EVT_DATA_RECEIVED,
		.data.msg.payload.ptr = payload.ptr,
		.data.msg.payload.size = payload.size,
		.topic.name.ptr = topic.ptr,
		.topic.name.size = topic.size,
		.topic.properties = properties,
	};
	az_span topic_span = az_span_create(topic.ptr, topic.size);
	az_iot_hub_client_c2d_request c2d_msg;
	az_iot_hub_client_method_request method_request;
	az_iot_hub_client_twin_response twin_msg;

	/* The topic needs to be parsed with multiple functions in order
	 * to determine the type of incoming message.
	 */
	if (az_result_succeeded(
	    az_iot_hub_client_c2d_parse_received_topic(&iot_hub_client, topic_span, &c2d_msg))) {
		/* The message was received on the device-bound (C2D) topic. */
		evt.topic.type = AZURE_IOT_HUB_TOPIC_DEVICEBOUND;
		LOG_DBG("Message type: Device-bound");
	} else if (az_result_succeeded(
		   az_iot_hub_client_methods_parse_received_topic(&iot_hub_client,
								  topic_span,
								  &method_request))) {
		/* The message was received on the device method topic. */
		evt.topic.type = AZURE_IOT_HUB_TOPIC_DIRECT_METHOD;
		LOG_DBG("Message type: Direct method");
	} else if (az_result_succeeded(
		   az_iot_hub_client_twin_parse_received_topic(&iot_hub_client,
							       topic_span,
							       &twin_msg))) {
		/* Message was received on the device twin topic.
		 * Now we need to determine what type of device twin message it is.
		 */
		switch (twin_msg.response_type) {
		case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_GET:
			evt.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REQUEST_RESULT;
			LOG_DBG("Message type: AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_GET");
			break;
		case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
			evt.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_DESIRED;
			LOG_DBG("Message type: "
				"AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES");
			break;
		case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
			evt.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REQUEST_RESULT;
			LOG_DBG("Message type: "
				"AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES");
			break;
		case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_REQUEST_ERROR:
			evt.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REQUEST_RESULT;
			LOG_DBG("Message type: "
				"AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_REQUEST_ERROR");
			break;
		default:
			LOG_ERR("Unrecognized twin message type: %d. Treated as C2D message",
				twin_msg.response_type);
			evt.topic.type = AZURE_IOT_HUB_TOPIC_DEVICEBOUND;
			break;
		}
	} else {
		LOG_ERR("Unrecognized topic: %.*s", topic.size, topic.ptr);
		evt.topic.type = AZURE_IOT_HUB_TOPIC_UNKNOWN;
		return;
	}

	switch (evt.topic.type) {
	case AZURE_IOT_HUB_TOPIC_DEVICEBOUND: {
		az_span name;
		az_span value;
		size_t property_idx = 0;

		evt.topic.type = AZURE_IOT_HUB_TOPIC_DEVICEBOUND;

		while (az_iot_message_properties_next(&c2d_msg.properties, &name, &value) ==
		       AZ_OK) {
			if (property_idx >= ARRAY_SIZE(properties)) {
				LOG_WRN("Not enough space to store copy all properties");
				break;
			}

			properties[property_idx].key.ptr = az_span_ptr(name);
			properties[property_idx].key.size = az_span_size(name);
			properties[property_idx].value.ptr = az_span_ptr(value);
			properties[property_idx].value.size = az_span_size(value);

			property_idx++;
		}

		if (property_idx > 0) {
			evt.topic.property_count = property_idx;

			LOG_DBG("Property bag count: %d", evt.topic.property_count);
		}

		break;
	}
	case AZURE_IOT_HUB_TOPIC_DIRECT_METHOD:
		if (direct_method_process(&method_request,  topic, payload)) {
			LOG_DBG("Direct method processed");
			return;
		}

		LOG_WRN("Unhandled direct method invocation");
		return;
	case AZURE_IOT_HUB_TOPIC_TWIN_DESIRED: {
#if IS_ENABLED(CONFIG_AZURE_FOTA)
		int err = azure_fota_msg_process(payload.ptr, payload.size);

		if (err < 0) {
			LOG_ERR("Failed to process FOTA message");
		} else if (err == 1) {
			LOG_DBG("Device twin update handled (FOTA)");
		}

		/* Forward the device twin to the application. */
#endif /* IS_ENABLED(CONFIG_AZURE_FOTA) */
		evt.type = AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED;

		azure_iot_hub_notify_event(&evt);
	};
		return;
	case AZURE_IOT_HUB_TOPIC_TWIN_REQUEST_RESULT:
		LOG_DBG("Device twin data received");
		device_twin_result_process(&twin_msg, evt.topic.name, evt.data.msg.payload);
		return;
	default:
		/* Should not be reachable */
		LOG_ERR("Topic parsing failed");
	}

	azure_iot_hub_notify_event(&evt);
}

AZ_HUB_STATIC void on_connack(enum mqtt_conn_return_code return_code)
{
	int err;
	struct azure_iot_hub_evt evt = {
		.type = AZURE_IOT_HUB_EVT_CONNECTED,
	};

	if (return_code != MQTT_CONNECTION_ACCEPTED) {
		LOG_ERR("Connection was rejected with return code %d",
			return_code);
		LOG_WRN("Is the device certificate valid?");
		evt.data.err = return_code;
		evt.type = AZURE_IOT_HUB_EVT_CONNECTION_FAILED;
		iot_hub_state_set(STATE_DISCONNECTED);
		azure_iot_hub_notify_event(&evt);
		return;
	}

	iot_hub_state_set(STATE_CONNECTED);

	LOG_DBG("MQTT mqtt_client connected");

	err = topic_subscribe();
	if (err) {
		LOG_ERR("Failed to subscribe to topics, err: %d", err);
	}

	azure_iot_hub_notify_event(&evt);
}

static void on_disconnect(int result)
{
	struct azure_iot_hub_evt evt = {
		.type = AZURE_IOT_HUB_EVT_DISCONNECTED,
	};

	LOG_WRN("DISCONNECT, result: %d", result);
	iot_hub_state_set(STATE_DISCONNECTED);

	azure_iot_hub_notify_event(&evt);
}

AZ_HUB_STATIC void on_puback(uint16_t message_id, int result)
{
	struct azure_iot_hub_evt evt = {
		.type = AZURE_IOT_HUB_EVT_PUBACK,
		.data.message_id = message_id,
	};

	azure_iot_hub_notify_event(&evt);
}

AZ_HUB_STATIC void on_suback(uint16_t message_id, int result)
{
	struct azure_iot_hub_evt evt = {
		.type = AZURE_IOT_HUB_EVT_READY,
	};

	azure_iot_hub_notify_event(&evt);

	if (IS_ENABLED(CONFIG_AZURE_IOT_HUB_AUTO_DEVICE_TWIN_REQUEST) ||
	    IS_ENABLED(CONFIG_AZURE_FOTA)) {
		device_twin_request();
	}
}

static void on_pingresp(void)
{
	struct azure_iot_hub_evt evt = {
		.type = AZURE_IOT_HUB_EVT_PINGRESP,
	};

	azure_iot_hub_notify_event(&evt);
}

AZ_HUB_STATIC void on_error(enum mqtt_helper_error error)
{
	struct azure_iot_hub_evt evt = { 0 };

	switch (error) {
	case MQTT_HELPER_ERROR_MSG_SIZE:
		evt.type = AZURE_IOT_HUB_EVT_ERROR_MSG_SIZE;
		break;
	default:
		LOG_ERR("Unrecognized error type: %d", error);
		return;
	}

	azure_iot_hub_notify_event(&evt);
}

/* The functions expects an already initialized az_iot_message_properties */
AZ_HUB_STATIC int msg_properties_add(az_iot_message_properties * az_properties,
			      struct azure_iot_hub_property *properties,
			      size_t property_count)
{
	int err;

	for (size_t i = 0; i < property_count; i++) {
		az_span name = az_span_create(properties[i].key.ptr, properties[i].key.size);
		az_span value = az_span_create(properties[i].value.ptr, properties[i].value.size);

		err = az_iot_message_properties_append(az_properties, name, value);
		if (az_result_failed(err)) {
			LOG_WRN("az_iot_message_properties_append failed, az err: %d", err);
			return err;
		}
	}

	return 0;
}

#if IS_ENABLED(CONFIG_AZURE_FOTA)

static void fota_report_send(struct azure_fota_event *evt)
{
	int err;
	struct azure_iot_hub_msg msg = {
		.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REPORTED,
		.payload.ptr = evt->report,
		.payload.size = evt->report ? strlen(evt->report) : 0,
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
		azure_iot_hub_notify_event(&evt);
		break;
	default:
		LOG_ERR("Unhandled FOTA event, type: %d", fota_evt->type);
		break;
	}
}
#endif /* CONFIG_AZURE_FOTA */

static int request_id_create_and_get(char *buf, size_t buf_size)
{
	int len = snprintk(buf, sizeof(buf_size), "%d", k_uptime_get_32());

	if ((len < 0) || (len >= buf_size)) {
		LOG_ERR("Failed to create request ID");
		return -ENOMEM;
	}

	LOG_DBG("Request ID not specified, using \"%s\"", buf);

	return 0;
}

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
static void dps_handler(enum azure_iot_hub_dps_reg_status state)
{
	switch (state) {
	case AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED:
		LOG_DBG("AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED");
		break;
	case AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNED:
		LOG_DBG("AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNED");
		k_sem_give(&dps_sem);
		break;
	case AZURE_IOT_HUB_DPS_REG_STATUS_FAILED:
		LOG_DBG("AZURE_IOT_HUB_DPS_REG_STATUS_FAILED");
		/* TODO: In case of failure, _start() will still wait for the sampehore to time out.
		 *	 This can be improved to give feedbck to the app sooner.
		 */
		break;
	case AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNING:
		LOG_DBG("AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNING");
		break;
	default:
		LOG_WRN("Unhandled DPS state: %d", state);
		break;
	}
}
#endif /* IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS) */

/* Public interface */

int azure_iot_hub_init(const azure_iot_hub_evt_handler_t event_handler)
{
	if (!iot_hub_state_verify(STATE_UNINIT) && !iot_hub_state_verify(STATE_DISCONNECTED)) {
		LOG_WRN("Azure IoT Hub cannot be re-initialized in this state: %s",
			state_name_get(iot_hub_state_get()));
		return -EALREADY;
	}

	if (event_handler == NULL) {
		LOG_ERR("Event handler must be provided");
		return -EINVAL;
	}

#if IS_ENABLED(CONFIG_AZURE_FOTA)
	int err = azure_fota_init(fota_evt_handler);

	if (err) {
		LOG_ERR("Failed to initialize Azure FOTA, error: %d", err);
		return err;
	}

	LOG_DBG("Azure FOTA initialized");
#endif

	evt_handler = event_handler;

	iot_hub_state_set(STATE_DISCONNECTED);

	return 0;
}

int azure_iot_hub_connect(const struct azure_iot_hub_config *config)
{
	int err;
	char user_name_buf[CONFIG_AZURE_IOT_HUB_USER_NAME_BUF_SIZE];
	size_t user_name_len;
	az_span hostname_span;
	az_span device_id_span;
	struct mqtt_helper_conn_params conn_params = {
		.hostname.ptr = config ? config->hostname.ptr : NULL,
		.hostname.size = config ? config->hostname.size : 0,
		.device_id.ptr = config ? config->device_id.ptr : NULL,
		.device_id.size = config ? config->device_id.size : 0,
		.user_name = {
			.ptr = user_name_buf,
		},
	};
	struct mqtt_helper_cfg cfg = {
		.cb = {
			.on_connack = on_connack,
			.on_disconnect = on_disconnect,
			.on_publish = on_publish,
			.on_puback = on_puback,
			.on_suback = on_suback,
			.on_pingresp = on_pingresp,
			.on_error = on_error,
		},
	};
	struct azure_iot_hub_evt evt = {
		evt.type = AZURE_IOT_HUB_EVT_CONNECTING,
	};

	if (iot_hub_state_verify(STATE_CONNECTING)) {
		LOG_WRN("Azure IoT Hub connection establishment in progress");
		return -EINPROGRESS;
	}  else if (iot_hub_state_verify(STATE_CONNECTED)) {
		LOG_WRN("Azure IoT Hub is already connected");
		return -EALREADY;
	} else if (!iot_hub_state_verify(STATE_DISCONNECTED)) {
		LOG_WRN("Azure IoT Hub is not in initialized and disconnected state");
		return -ENOENT;
	}

	/* Use static IP if that is configured */
	if (sizeof(CONFIG_MQTT_HELPER_STATIC_IP_ADDRESS) > 1) {
		LOG_DBG("Using static IP address: %s", CONFIG_MQTT_HELPER_STATIC_IP_ADDRESS);

		conn_params.hostname.ptr = CONFIG_MQTT_HELPER_STATIC_IP_ADDRESS;
		conn_params.hostname.size = sizeof(CONFIG_MQTT_HELPER_STATIC_IP_ADDRESS) - 1;
	} else if ((conn_params.hostname.size == 0) && !IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)) {
		/* Set hostname to Kconfig value if it was not provided and DPS is not enabled. */
		LOG_DBG("No hostname provided, using Kconfig value: %s",
			CONFIG_AZURE_IOT_HUB_HOSTNAME);

		conn_params.hostname.ptr = CONFIG_AZURE_IOT_HUB_HOSTNAME;
		conn_params.hostname.size = sizeof(CONFIG_AZURE_IOT_HUB_HOSTNAME) - 1;
	}

	/* Set device ID to Kconfig value if it was not provided and DPS is not enabled. */
	if ((conn_params.device_id.size == 0) && !IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)) {
		LOG_DBG("No device ID provided, using Kconfig value: %s",
			CONFIG_AZURE_IOT_HUB_DEVICE_ID);

		conn_params.device_id.ptr = CONFIG_AZURE_IOT_HUB_DEVICE_ID;
		conn_params.device_id.size = sizeof(CONFIG_AZURE_IOT_HUB_DEVICE_ID) - 1;
	}

	iot_hub_state_set(STATE_CONNECTING);

	/* Notify the application that the library is currently connecting to the IoT hub. */
	azure_iot_hub_notify_event(&evt);

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
	if (config && config->use_dps) {
		struct azure_iot_hub_dps_config dps_cfg = {
			.handler = dps_handler,
			.reg_id.ptr = conn_params.device_id.ptr,
			.reg_id.size = conn_params.device_id.size,
			.id_scope = {
				.ptr = CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE,
				.size = sizeof(CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE) - 1,
			},
		};
		struct azure_iot_hub_buf hostname = {
			.ptr = conn_params.hostname.ptr,
			.size = conn_params.hostname.size,
		};
		struct azure_iot_hub_buf device_id = {
			.ptr = conn_params.device_id.ptr,
			.size = conn_params.device_id.size,
		};

		LOG_DBG("Starting DPS, timeout is %d seconds",
			CONFIG_AZURE_IOT_HUB_DPS_TIMEOUT_SEC);

		err = azure_iot_hub_dps_init(&dps_cfg);
		if (err) {
			LOG_ERR("azure_iot_hub_dps_init failed, error: %d", err);
			goto exit;
		}

		err = azure_iot_hub_dps_start();
		switch (err) {
		case 0:
			err = k_sem_take(&dps_sem, K_SECONDS(CONFIG_AZURE_IOT_HUB_DPS_TIMEOUT_SEC));
			if (err != 0) {
				LOG_ERR("DPS timed out, connection attempt terminated");
				err = -ETIMEDOUT;
				goto exit;
			}

			break;
		case -EALREADY:
			LOG_DBG("Already assigned to an IoT hub, skipping DPS");
			break;
		default:
			LOG_ERR("azure_iot_hub_dps_start failed, error: %d", err);
			goto exit;
		}

		err = azure_iot_hub_dps_hostname_get(&hostname);
		if (err) {
			LOG_ERR("Failed to get the stored hostname from DPS, error: %d", err);
			err = -EFAULT;
			goto exit;
		}

		conn_params.hostname.ptr = hostname.ptr;
		conn_params.hostname.size = hostname.size;

		LOG_DBG("Using the assigned hub (size: %d): %s",
			conn_params.hostname.size, conn_params.hostname.ptr);

		err = azure_iot_hub_dps_device_id_get(&device_id);
		if (err) {
			LOG_ERR("Failed to get the stored device ID from DPS, error: %d", err);
			err = -EFAULT;
			goto exit;
		}

		conn_params.device_id.ptr = device_id.ptr;
		conn_params.device_id.size = device_id.size;

		LOG_DBG("Using the assigned device ID: %.*s",
			conn_params.device_id.size,
			conn_params.device_id.ptr);
	}

#endif /* IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS) */

	err = mqtt_helper_init(&cfg);
	if (err) {
		LOG_ERR("mqtt_helper_init failed, error: %d", err);
		err = -EFAULT;
		goto exit;
	}

	hostname_span = az_span_create(conn_params.hostname.ptr, conn_params.hostname.size);
	device_id_span = az_span_create(conn_params.device_id.ptr, conn_params.device_id.size);

	/* Initialize Azure SDK client instance. */
	err = az_iot_hub_client_init(
		&iot_hub_client,
		hostname_span,
		device_id_span,
		NULL);
	if (az_result_failed(err)) {
		LOG_ERR("Failed to initialize IoT Hub mqtt_client, result code: %d", err);
		err = -EFAULT;
		goto exit;
	}

	err = az_iot_hub_client_get_user_name(&iot_hub_client,
					      user_name_buf,
					      sizeof(user_name_buf),
					      &user_name_len);
	if (az_result_failed(err)) {
		LOG_ERR("Failed to get user name, az error: 0x%08x", err);
		err = -EFAULT;
		goto exit;
	}

	conn_params.user_name.size = user_name_len;

	LOG_DBG("User name: %.*s", conn_params.user_name.size, conn_params.user_name.ptr);
	LOG_DBG("User name buffer size is %d, actual user name size is: %d",
		sizeof(user_name_buf), user_name_len);

	err = mqtt_helper_connect(&conn_params);
	if (err) {
		LOG_ERR("mqtt_helper_connect failed, error: %d", err);
		goto exit;
	}

	return 0;

exit:
	iot_hub_state_set(STATE_DISCONNECTED);
	return err;
}

int azure_iot_hub_send(const struct azure_iot_hub_msg *const msg)
{
	int err;
	ssize_t topic_len;
	char topic[CONFIG_AZURE_IOT_HUB_TOPIC_MAX_LEN + 1];

	if (msg == NULL) {
		return -EINVAL;
	}

	struct mqtt_publish_param param = {
		.message.payload.data = msg->payload.ptr,
		.message.payload.len = msg->payload.size,
		.message.topic.qos = msg->qos,
		.message_id = msg->message_id,
		.dup_flag = msg->dup_flag,
		.retain_flag = msg->retain_flag,
	};

	if (!iot_hub_state_verify(STATE_CONNECTED)) {
		LOG_WRN("Azure IoT Hub is not connected");
		return -ENOTCONN;
	}

	switch (msg->topic.type) {
	case AZURE_IOT_HUB_TOPIC_EVENT: {
		az_iot_message_properties az_properties;
		size_t property_count = (size_t)msg->topic.property_count;
		char property_buf[CONFIG_AZURE_IOT_HUB_MSG_PROPERTY_BUFFER_SIZE] = {0};

		if (property_count > 0) {
			az_span prop_span = AZ_SPAN_FROM_BUFFER(property_buf);

			LOG_DBG("Message property count: %d", property_count);

			err = az_iot_message_properties_init(&az_properties, prop_span, 0);
			if (az_result_failed(err)) {
				LOG_ERR("Failed to initialize properties, az error: 0x%08x", err);
				return -EFAULT;
			}
			err = msg_properties_add(&az_properties, msg->topic.properties,
						 property_count);
			if (err) {
				LOG_ERR("Failed to add properties, az error: %d", err);

				return -EMSGSIZE;
			}
		}

		err = az_iot_hub_client_telemetry_get_publish_topic(
			&iot_hub_client,
			(property_count > 0) ? &az_properties : NULL,
			topic,
			sizeof(topic),
			&topic_len);
		if (az_result_failed(err)) {
			LOG_ERR("Failed to get telemetry topic, az error: 0x%08x", err);

			if (err == AZ_ERROR_NOT_ENOUGH_SPACE) {
				LOG_DBG("Too small topic buffer");

				return -EMSGSIZE;
			}

			return -EFAULT;
		}

		break;
	}
	case AZURE_IOT_HUB_TOPIC_TWIN_REPORTED: {
		char request_id[20];
		char *request_id_ptr = msg->request_id.ptr;
		size_t request_id_len = msg->request_id.size;
		az_span request_id_span;

		if (msg->request_id.size == 0) {
			err = request_id_create_and_get(request_id, sizeof(request_id));
			if (err) {
				LOG_ERR("Failed to create request ID");
				return -ENOMEM;
			}

			request_id_ptr = request_id;
			request_id_len = strlen(request_id);
		}

		request_id_span = az_span_create(request_id_ptr, request_id_len);

		err = az_iot_hub_client_twin_patch_get_publish_topic(
			&iot_hub_client,
			request_id_span,
			topic,
			sizeof(topic),
			&topic_len);
		if (az_result_failed(err)) {
			LOG_ERR("Failed to get twin patch topic, az error: %d", err);
			return -EFAULT;
		}
		break;
	}
	case AZURE_IOT_HUB_TOPIC_TWIN_REQUEST: {
		char request_id[20];
		char *request_id_ptr = msg->request_id.ptr;
		size_t request_id_len = msg->request_id.size;
		az_span request_id_span;

		if (msg->request_id.size == 0) {
			err = request_id_create_and_get(request_id, sizeof(request_id));
			if (err) {
				LOG_ERR("Failed to create request ID");
				return -ENOMEM;
			}

			request_id_ptr = request_id;
			request_id_len = strlen(request_id);
		}

		request_id_span = az_span_create(request_id_ptr, request_id_len);

		err = az_iot_hub_client_twin_document_get_publish_topic(
			&iot_hub_client,
			request_id_span,
			topic,
			sizeof(topic),
			&topic_len);
		if (az_result_failed(err)) {
			LOG_ERR("Failed to create get twin topic, az error: %d", err);
			return -EFAULT;
		}

		break;
	}
	default:
		LOG_ERR("No topic specified");
		return -ENOMSG;
	}

	param.message.topic.topic.size = topic_len;
	param.message.topic.topic.utf8 = topic;

	return mqtt_helper_publish(&param);
}

int azure_iot_hub_disconnect(void)
{
	int err;

	if (!iot_hub_state_verify(STATE_CONNECTED)) {
		LOG_WRN("Azure IoT Hub is not connected");
		return -ENOTCONN;
	}

	iot_hub_state_set(STATE_DISCONNECTING);

	err = mqtt_helper_disconnect();
	if (err) {
		LOG_ERR("Failed to gracefully disconnect from MQTT broker, error: %d", err);
		iot_hub_state_set(STATE_DISCONNECTED);
		return -ENXIO;
	}

	/* The MQTT library only propagates the MQTT_DISCONNECT event
	 * if the call to mqtt_disconnect() is successful. In that case the
	 * setting of STATE_DISCONNECTED is carried out in the mqtt_evt_handler.
	 */

	return 0;
}

int azure_iot_hub_method_respond(struct azure_iot_hub_result *result)
{
	int err;
	size_t len;
	static char topic[100];

	if (result == NULL) {
		return -EINVAL;
	}

	struct mqtt_publish_param param = {
		.message.payload.data = result->payload.ptr,
		.message.payload.len = result->payload.size,
		.message.topic.topic.utf8 = topic,
	};
	az_span request_id_span = az_span_create(result->request_id.ptr, result->request_id.size);

	if (!iot_hub_state_verify(STATE_CONNECTED)) {
		LOG_WRN("Azure IoT Hub is not connected");
		return -ENOTCONN;
	}

	err = az_iot_hub_client_methods_response_get_publish_topic(
		&iot_hub_client,
		request_id_span,
		result->status,
		topic,
		sizeof(topic),
		&len);
	if (az_result_failed(err)) {
		LOG_ERR("Failed to get method response topic, az error: %d", err);
		return -EFAULT;
	}

	param.message.topic.topic.size = len;

	LOG_DBG("Publishing to topic: %.*s", len, (char *)param.message.topic.topic.utf8);

	err = mqtt_helper_publish(&param);
	if (err) {
		LOG_ERR("mqtt_helper_publish() failed with error: %d", err);
		return -ENXIO;
	}

	return 0;
}
