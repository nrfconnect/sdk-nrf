/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <net/azure_iot_hub.h>
#include <hw_id.h>

#include "cloud/cloud_wrapper.h"

#define MODULE azure_iot_hub_integration

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CLOUD_INTEGRATION_LOG_LEVEL);

#if !defined(CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM)
#define CLIENT_ID_LEN 15
#else
#define CLIENT_ID_LEN (sizeof(CONFIG_CLOUD_CLIENT_ID) - 1)
#endif

#define PROP_BAG_CONTENT_TYPE_KEY "%24.ct"
#define PROP_BAG_CONTENT_TYPE_VALUE "application%2Fjson"
#define PROP_BAG_CONTENT_ENCODING_KEY "%24.ce"
#define PROP_BAG_CONTENT_ENCODING_VALUE "utf-8"

#define PROP_BAG_BATCH_KEY "batch"
#define PROP_BAG_NEIGHBOR_CELLS_KEY "ncellmeas"

#define PROP_BAG_AGPS_KEY "agps"
#define PROP_BAG_AGPS_GET_VALUE "get"
#define PROP_BAG_AGPS_RESPONSE_VALUE "result"

#define PROP_BAG_PGPS_KEY "pgps"
#define PROP_BAG_PGPS_GET_VALUE "get"
#define PROP_BAG_PGPS_RESPONSE_VALUE "result"

#define REQUEST_DEVICE_TWIN_STRING ""

static struct azure_iot_hub_property prop_bag_message[] = {
	{
		.key.ptr = PROP_BAG_CONTENT_TYPE_KEY,
		.key.size = sizeof(PROP_BAG_CONTENT_TYPE_KEY) - 1,
		.value.ptr = PROP_BAG_CONTENT_TYPE_VALUE,
		.value.size = sizeof(PROP_BAG_CONTENT_TYPE_VALUE) - 1,
	},
	{
		.key.ptr = PROP_BAG_CONTENT_ENCODING_KEY,
		.key.size = sizeof(PROP_BAG_CONTENT_ENCODING_KEY) - 1,
		.value.ptr = PROP_BAG_CONTENT_ENCODING_VALUE,
		.value.size = sizeof(PROP_BAG_CONTENT_ENCODING_VALUE) - 1,
	},
};
static struct azure_iot_hub_property prop_bag_batch[] = {
	{
		.key.ptr = PROP_BAG_BATCH_KEY,
		.key.size = sizeof(PROP_BAG_BATCH_KEY) - 1,
		.value.ptr = NULL,
	},
	{
		.key.ptr = PROP_BAG_CONTENT_TYPE_KEY,
		.key.size = sizeof(PROP_BAG_CONTENT_TYPE_KEY) - 1,
		.value.ptr = PROP_BAG_CONTENT_TYPE_VALUE,
		.value.size = sizeof(PROP_BAG_CONTENT_TYPE_VALUE) - 1,
	},
	{
		.key.ptr = PROP_BAG_CONTENT_ENCODING_KEY,
		.key.size = sizeof(PROP_BAG_CONTENT_ENCODING_KEY) - 1,
		.value.ptr = PROP_BAG_CONTENT_ENCODING_VALUE,
		.value.size = sizeof(PROP_BAG_CONTENT_ENCODING_VALUE) - 1,
	},
};
static struct azure_iot_hub_property prop_bag_agps[] = {
	{
		.key.ptr = PROP_BAG_AGPS_KEY,
		.key.size = sizeof(PROP_BAG_AGPS_KEY) - 1,
		.value.ptr = PROP_BAG_AGPS_GET_VALUE,
		.value.size = sizeof(PROP_BAG_AGPS_GET_VALUE) - 1,
	},
	{
		.key.ptr = PROP_BAG_CONTENT_TYPE_KEY,
		.key.size = sizeof(PROP_BAG_CONTENT_TYPE_KEY) - 1,
		.value.ptr = PROP_BAG_CONTENT_TYPE_VALUE,
		.value.size = sizeof(PROP_BAG_CONTENT_TYPE_VALUE) - 1,
	},
	{
		.key.ptr = PROP_BAG_CONTENT_ENCODING_KEY,
		.key.size = sizeof(PROP_BAG_CONTENT_ENCODING_KEY) - 1,
		.value.ptr = PROP_BAG_CONTENT_ENCODING_VALUE,
		.value.size = sizeof(PROP_BAG_CONTENT_ENCODING_VALUE) - 1,
	},
};
static struct azure_iot_hub_property prop_bag_pgps[] = {
	{
		.key.ptr = PROP_BAG_PGPS_KEY,
		.key.size = sizeof(PROP_BAG_PGPS_KEY) - 1,
		.value.ptr = PROP_BAG_PGPS_GET_VALUE,
		.value.size = sizeof(PROP_BAG_PGPS_GET_VALUE) - 1,
	},
	{
		.key.ptr = PROP_BAG_CONTENT_TYPE_KEY,
		.key.size = sizeof(PROP_BAG_CONTENT_TYPE_KEY) - 1,
		.value.ptr = PROP_BAG_CONTENT_TYPE_VALUE,
		.value.size = sizeof(PROP_BAG_CONTENT_TYPE_VALUE) - 1,
	},
	{
		.key.ptr = PROP_BAG_CONTENT_ENCODING_KEY,
		.key.size = sizeof(PROP_BAG_CONTENT_ENCODING_KEY) - 1,
		.value.ptr = PROP_BAG_CONTENT_ENCODING_VALUE,
		.value.size = sizeof(PROP_BAG_CONTENT_ENCODING_VALUE) - 1,
	},
};
static struct azure_iot_hub_property prop_bag_ncellmeas[] = {
	{
		.key.ptr = PROP_BAG_NEIGHBOR_CELLS_KEY,
		.key.size = sizeof(PROP_BAG_NEIGHBOR_CELLS_KEY) - 1,
		.value.ptr = NULL,
	},
	{
		.key.ptr = PROP_BAG_CONTENT_TYPE_KEY,
		.key.size = sizeof(PROP_BAG_CONTENT_TYPE_KEY) - 1,
		.value.ptr = PROP_BAG_CONTENT_TYPE_VALUE,
		.value.size = sizeof(PROP_BAG_CONTENT_TYPE_VALUE) - 1,
	},
	{
		.key.ptr = PROP_BAG_CONTENT_ENCODING_KEY,
		.key.size = sizeof(PROP_BAG_CONTENT_ENCODING_KEY) - 1,
		.value.ptr = PROP_BAG_CONTENT_ENCODING_VALUE,
		.value.size = sizeof(PROP_BAG_CONTENT_ENCODING_VALUE) - 1,
	},
};

static char client_id_buf[CLIENT_ID_LEN + 1];
static struct azure_iot_hub_config config = {
	.use_dps = IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS),
	.device_id.ptr = client_id_buf,
	.device_id.size = sizeof(client_id_buf) - 1,
};

static cloud_wrap_evt_handler_t wrapper_evt_handler;

static void cloud_wrapper_notify_event(const struct cloud_wrap_event *evt)
{
	if ((wrapper_evt_handler != NULL) && (evt != NULL)) {
		wrapper_evt_handler(evt);
	} else {
		LOG_ERR("Library event handler not registered, or empty event");
	}
}

/*
 * @brief Function that handles incoming data from the Azure IoT Hub.
 *	  This function notifies the cloud module with the appropriate event based on the
 *	  contents of the included topic and property bags.
 *
 * @param[in] event  Pointer to Azure IoT Hub event.
 */
static void incoming_message_handle(struct azure_iot_hub_evt *event)
{
	struct cloud_wrap_event cloud_wrap_evt = {
		.data.buf = event->data.msg.payload.ptr,
		.data.len = event->data.msg.payload.size,
		.type = CLOUD_WRAP_EVT_DATA_RECEIVED
	};

	/* If the incoming message is not received on the devicebound topic or if no property bags
	 * are included, the application is notified at once with the generic message type,
	 * CLOUD_WRAP_EVT_DATA_RECEIVED.
	 */
	if ((event->topic.type != AZURE_IOT_HUB_TOPIC_DEVICEBOUND) ||
	    (event->topic.property_count == 0)) {
		goto notify;
	}

	/* Iterate over property bags included in incoming devicebound topic to filter
	 * on A-GPS and P-GPS related key and value pairs.
	 */
	for (int i = 0; i < event->topic.property_count; i++) {
		if ((event->topic.properties[i].key.ptr == NULL) ||
		    (event->topic.properties[i].value.ptr == NULL)) {
			LOG_DBG("Key or value is NULL");
			continue;
		}

		/* Property bags are null terminated strings. */
		if (!strncmp(event->topic.properties[i].key.ptr, PROP_BAG_AGPS_KEY,
			     event->topic.properties[i].key.size) &&
		    !strncmp(event->topic.properties[i].value.ptr, PROP_BAG_AGPS_RESPONSE_VALUE,
			     event->topic.properties[i].value.size)) {
			cloud_wrap_evt.type = CLOUD_WRAP_EVT_AGPS_DATA_RECEIVED;
		} else if (!strncmp(event->topic.properties[i].key.ptr, PROP_BAG_PGPS_KEY,
				    event->topic.properties[i].key.size) &&
			   !strncmp(event->topic.properties[i].value.ptr,
				    PROP_BAG_PGPS_RESPONSE_VALUE,
				    event->topic.properties[i].value.size)) {
			cloud_wrap_evt.type = CLOUD_WRAP_EVT_PGPS_DATA_RECEIVED;
		}
	}

notify:
	cloud_wrapper_notify_event(&cloud_wrap_evt);
}

static void azure_iot_hub_event_handler(struct azure_iot_hub_evt *const evt)
{
	struct cloud_wrap_event cloud_wrap_evt = { 0 };
	bool notify = false;

	switch (evt->type) {
	case AZURE_IOT_HUB_EVT_CONNECTING:
		LOG_DBG("AZURE_IOT_HUB_EVT_CONNECTING");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_CONNECTING;
		notify = true;
		break;
	case AZURE_IOT_HUB_EVT_CONNECTED:
		LOG_DBG("AZURE_IOT_HUB_EVT_CONNECTED");
		break;
	case AZURE_IOT_HUB_EVT_CONNECTION_FAILED:
		LOG_DBG("AZURE_IOT_HUB_EVT_CONNECTION_FAILED");
		break;
	case AZURE_IOT_HUB_EVT_DISCONNECTED:
		LOG_DBG("AZURE_IOT_HUB_EVT_DISCONNECTED");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DISCONNECTED;
		notify = true;
		break;
	case AZURE_IOT_HUB_EVT_READY:
		LOG_DBG("AZURE_IOT_HUB_EVT_READY");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_CONNECTED;
		notify = true;
		break;
	case AZURE_IOT_HUB_EVT_DATA_RECEIVED:
		LOG_DBG("AZURE_IOT_HUB_EVT_DATA_RECEIVED");
		incoming_message_handle((struct azure_iot_hub_evt *)evt);
		break;
	case AZURE_IOT_HUB_EVT_TWIN_RECEIVED:
		LOG_DBG("AZURE_IOT_HUB_EVT_TWIN_RECEIVED");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DATA_RECEIVED;
		cloud_wrap_evt.data.buf = evt->data.msg.payload.ptr;
		cloud_wrap_evt.data.len = evt->data.msg.payload.size;
		notify = true;
		break;
	case AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED:
		LOG_DBG("AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DATA_RECEIVED;
		cloud_wrap_evt.data.buf = evt->data.msg.payload.ptr;
		cloud_wrap_evt.data.len = evt->data.msg.payload.size;
		notify = true;
		break;
	case AZURE_IOT_HUB_EVT_DIRECT_METHOD:
		LOG_DBG("AZURE_IOT_HUB_EVT_DIRECT_METHOD");
		break;
	case AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS:
		LOG_DBG("AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS");
		break;
	case AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL:
		LOG_DBG("AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL");
		break;
	case AZURE_IOT_HUB_EVT_PUBACK:
		LOG_DBG("AZURE_IOT_HUB_EVT_PUBACK");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DATA_ACK;
		cloud_wrap_evt.message_id = evt->data.message_id;
		notify = true;
		break;
	case AZURE_IOT_HUB_EVT_PINGRESP:
		LOG_DBG("AZURE_IOT_HUB_EVT_PINGRESP");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_PING_ACK;
		notify = true;
		break;
	case AZURE_IOT_HUB_EVT_ERROR:
		LOG_DBG("AZURE_IOT_HUB_EVT_ERROR");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_ERROR;
		cloud_wrap_evt.err = evt->data.err;
		notify = true;
		break;
	case AZURE_IOT_HUB_EVT_FOTA_START:
		LOG_DBG("AZURE_IOT_HUB_EVT_FOTA_START");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_FOTA_START;
		notify = true;
		break;
	case AZURE_IOT_HUB_EVT_FOTA_DONE:
		LOG_DBG("AZURE_IOT_HUB_EVT_FOTA_DONE");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_FOTA_DONE;
		notify = true;
		break;
	case AZURE_IOT_HUB_EVT_FOTA_ERASE_PENDING:
		LOG_DBG("AZURE_IOT_HUB_EVT_FOTA_ERASE_PENDING");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_FOTA_ERASE_PENDING;
		notify = true;
		break;
	case AZURE_IOT_HUB_EVT_FOTA_ERASE_DONE:
		LOG_DBG("AZURE_IOT_HUB_EVT_FOTA_ERASE_DONE");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_FOTA_ERASE_DONE;
		notify = true;
		break;
	case AZURE_IOT_HUB_EVT_FOTA_ERROR:
		LOG_DBG("AZURE_IOT_HUB_EVT_FOTA_ERROR");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_FOTA_ERROR;
		notify = true;
		break;
	default:
		LOG_ERR("Unknown Azure IoT Hub event type: %d", evt->type);
		break;
	}

	if (notify) {
		cloud_wrapper_notify_event(&cloud_wrap_evt);
	}
}

int cloud_wrap_init(cloud_wrap_evt_handler_t event_handler)
{
	int err;

#if !defined(CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM)
	char hw_id_buf[HW_ID_LEN];

	err = hw_id_get(hw_id_buf, ARRAY_SIZE(hw_id_buf));

	if (err) {
		LOG_ERR("Failed to retrieve device ID");
		return err;
	}

	snprintk(client_id_buf, sizeof(client_id_buf), "%s", hw_id_buf);

#else
	snprintk(client_id_buf, sizeof(client_id_buf), "%s", CONFIG_CLOUD_CLIENT_ID);
#endif

	err = azure_iot_hub_init(azure_iot_hub_event_handler);
	if (err) {
		LOG_ERR("azure_iot_hub_init, error: %d", err);
		return err;
	}

	LOG_DBG("********************************************");
	LOG_DBG(" The Asset Tracker v2 has started");
	LOG_DBG(" Version:      %s", CONFIG_ASSET_TRACKER_V2_APP_VERSION);
	LOG_DBG(" Client ID:    %s", client_id_buf);
	LOG_DBG(" Cloud:        %s", "Azure IoT Hub");
#if defined(CONFIG_AZURE_IOT_HUB_DPS)
	LOG_DBG(" DPS endpoint: %s", CONFIG_AZURE_IOT_HUB_DPS_HOSTNAME);
	LOG_DBG(" ID scope:     %s", CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE);
#endif /* CONFIG_AZURE_IOT_HUB_DPS */
	LOG_DBG("********************************************");

	wrapper_evt_handler = event_handler;

	return 0;
}

int cloud_wrap_connect(void)
{
	int err;

	err = azure_iot_hub_connect(&config);
	if (err) {
		LOG_ERR("azure_iot_hub_connect, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_disconnect(void)
{
	int err;

	err = azure_iot_hub_disconnect();
	if (err) {
		LOG_ERR("azure_iot_hub_disconnect, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_state_get(bool ack, uint32_t id)
{
	int err;
	struct azure_iot_hub_msg msg = {
		.payload.ptr = REQUEST_DEVICE_TWIN_STRING,
		.payload.size = sizeof(REQUEST_DEVICE_TWIN_STRING) - 1,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REPORTED
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("azure_iot_hub_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_state_send(char *buf, size_t len, bool ack, uint32_t id)
{
	int err;
	struct azure_iot_hub_msg msg = {
		.payload.ptr = buf,
		.payload.size = len,
		.message_id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REPORTED,
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("azure_iot_hub_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_data_send(char *buf, size_t len, bool ack, uint32_t id,
			 const struct lwm2m_obj_path path_list[])
{
	ARG_UNUSED(path_list);

	int err;
	struct azure_iot_hub_msg msg = {
		.payload.ptr = buf,
		.payload.size = len,
		.message_id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REPORTED,
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("azure_iot_hub_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_batch_send(char *buf, size_t len, bool ack, uint32_t id)
{
	int err;
	struct azure_iot_hub_msg msg = {
		.payload.ptr = buf,
		.payload.size = len,
		.message_id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.topic.properties = prop_bag_batch,
		.topic.property_count = ARRAY_SIZE(prop_bag_batch)
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("azure_iot_hub_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_ui_send(char *buf, size_t len, bool ack, uint32_t id,
		       const struct lwm2m_obj_path path_list[])
{
	ARG_UNUSED(path_list);

	int err;
	struct azure_iot_hub_msg msg = {
		.payload.ptr = buf,
		.payload.size = len,
		.message_id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.topic.properties = prop_bag_message,
		.topic.property_count = ARRAY_SIZE(prop_bag_message)
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("azure_iot_hub_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_cloud_location_send(char *buf, size_t len, bool ack, uint32_t id)
{
	int err;
	struct azure_iot_hub_msg msg = {
		.payload.ptr = buf,
		.payload.size = len,
		.message_id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.topic.properties = prop_bag_ncellmeas,
		.topic.property_count = ARRAY_SIZE(prop_bag_ncellmeas)
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("azure_iot_hub_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_agps_request_send(char *buf, size_t len, bool ack, uint32_t id)
{
	int err;
	struct azure_iot_hub_msg msg = {
		.payload.ptr = buf,
		.payload.size = len,
		.message_id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.topic.properties = prop_bag_agps,
		.topic.property_count = ARRAY_SIZE(prop_bag_agps)
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("azure_iot_hub_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_pgps_request_send(char *buf, size_t len, bool ack, uint32_t id)
{
	int err;
	struct azure_iot_hub_msg msg = {
		.payload.ptr = buf,
		.payload.size = len,
		.message_id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.topic.properties = prop_bag_pgps,
		.topic.property_count = ARRAY_SIZE(prop_bag_pgps)
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("azure_iot_hub_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_memfault_data_send(char *buf, size_t len, bool ack, uint32_t id)
{
	/* Not supported */
	return -ENOTSUP;
}
