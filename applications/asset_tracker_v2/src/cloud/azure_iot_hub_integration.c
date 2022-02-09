#include <zephyr.h>
#include <net/azure_iot_hub.h>
#include <nrf_modem_at.h>

#include "cloud/cloud_wrapper.h"

#define MODULE azure_iot_hub_integration

#include <logging/log.h>
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

static struct azure_iot_hub_prop_bag prop_bag_message[] = {
	{
		.key = PROP_BAG_CONTENT_TYPE_KEY,
		.value = PROP_BAG_CONTENT_TYPE_VALUE,
	},
	{
		.key = PROP_BAG_CONTENT_ENCODING_KEY,
		.value = PROP_BAG_CONTENT_ENCODING_VALUE,
	},
};
static struct azure_iot_hub_prop_bag prop_bag_batch[] = {
	{
		.key = PROP_BAG_BATCH_KEY,
		.value = NULL,
	},
	{
		.key = PROP_BAG_CONTENT_TYPE_KEY,
		.value = PROP_BAG_CONTENT_TYPE_VALUE,
	},
	{
		.key = PROP_BAG_CONTENT_ENCODING_KEY,
		.value = PROP_BAG_CONTENT_ENCODING_VALUE,
	},
};
static struct azure_iot_hub_prop_bag prop_bag_agps[] = {
	{
		.key = PROP_BAG_AGPS_KEY,
		.value = PROP_BAG_AGPS_GET_VALUE,
	},
	{
		.key = PROP_BAG_CONTENT_TYPE_KEY,
		.value = PROP_BAG_CONTENT_TYPE_VALUE,
	},
	{
		.key = PROP_BAG_CONTENT_ENCODING_KEY,
		.value = PROP_BAG_CONTENT_ENCODING_VALUE,
	},
};
static struct azure_iot_hub_prop_bag prop_bag_pgps[] = {
	{
		.key = PROP_BAG_PGPS_KEY,
		.value = PROP_BAG_PGPS_GET_VALUE,
	},
	{
		.key = PROP_BAG_CONTENT_TYPE_KEY,
		.value = PROP_BAG_CONTENT_TYPE_VALUE,
	},
	{
		.key = PROP_BAG_CONTENT_ENCODING_KEY,
		.value = PROP_BAG_CONTENT_ENCODING_VALUE,
	},
};
static struct azure_iot_hub_prop_bag prop_bag_ncellmeas[] = {
	{
		.key = PROP_BAG_NEIGHBOR_CELLS_KEY,
		.value = NULL,
	},
	{
		.key = PROP_BAG_CONTENT_TYPE_KEY,
		.value = PROP_BAG_CONTENT_TYPE_VALUE,
	},
	{
		.key = PROP_BAG_CONTENT_ENCODING_KEY,
		.value = PROP_BAG_CONTENT_ENCODING_VALUE,
	},
};

static char client_id_buf[CLIENT_ID_LEN + 1];
static struct azure_iot_hub_config config;
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
		.data.buf = event->data.msg.ptr,
		.data.len = event->data.msg.len,
		.type = CLOUD_WRAP_EVT_DATA_RECEIVED
	};

	/* If the incoming message is not received on the devicebound topic or if no property bags
	 * are included, the application is notified at once with the generic message type,
	 * CLOUD_WRAP_EVT_DATA_RECEIVED.
	 */
	if ((event->topic.type != AZURE_IOT_HUB_TOPIC_DEVICEBOUND) ||
	    (event->topic.prop_bag_count == 0)) {
		goto notify;
	}

	/* Iterate over property bags included in incoming devicebound topic to filter
	 * on A-GPS and P-GPS related key and value pairs.
	 */
	for (int i = 0; i < event->topic.prop_bag_count; i++) {
		if (event->topic.prop_bag[i].key == NULL || event->topic.prop_bag[i].key == NULL) {
			LOG_DBG("Key or value is NULL");
			continue;
		}

		/* Property bags are null terminated strings. */
		if (!strcmp(event->topic.prop_bag[i].key, PROP_BAG_AGPS_KEY) &&
		    !strcmp(event->topic.prop_bag[i].value, PROP_BAG_AGPS_RESPONSE_VALUE)) {
			cloud_wrap_evt.type = CLOUD_WRAP_EVT_AGPS_DATA_RECEIVED;
		} else if (!strcmp(event->topic.prop_bag[i].key, PROP_BAG_PGPS_KEY) &&
			   !strcmp(event->topic.prop_bag[i].value, PROP_BAG_PGPS_RESPONSE_VALUE)) {
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
	case AZURE_IOT_HUB_EVT_DPS_CONNECTING:
		LOG_DBG("AZURE_IOT_HUB_EVT_DPS_CONNECTING");
		break;
	case AZURE_IOT_HUB_EVT_DPS_REGISTERING:
		LOG_DBG("AZURE_IOT_HUB_EVT_DPS_REGISTERING");
		break;
	case AZURE_IOT_HUB_EVT_DPS_DONE:
		LOG_DBG("AZURE_IOT_HUB_EVT_DPS_DONE");
		break;
	case AZURE_IOT_HUB_EVT_DPS_FAILED:
		LOG_DBG("AZURE_IOT_HUB_EVT_DPS_FAILED");
		break;
	case AZURE_IOT_HUB_EVT_TWIN_RECEIVED:
		LOG_DBG("AZURE_IOT_HUB_EVT_TWIN_RECEIVED");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DATA_RECEIVED;
		cloud_wrap_evt.data.buf = evt->data.msg.ptr;
		cloud_wrap_evt.data.len = evt->data.msg.len;
		notify = true;
		break;
	case AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED:
		LOG_DBG("AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DATA_RECEIVED;
		cloud_wrap_evt.data.buf = evt->data.msg.ptr;
		cloud_wrap_evt.data.len = evt->data.msg.len;
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
	char imei_buf[20 + sizeof("OK\r\n")];

	/* Retrieve device IMEI from modem. */
	err = nrf_modem_at_cmd(imei_buf, sizeof(imei_buf), "AT+CGSN");
	if (err) {
		LOG_ERR("Not able to retrieve device IMEI from modem");
		return err;
	}

	/* Set null character at the end of the device IMEI. */
	imei_buf[CLIENT_ID_LEN] = 0;

	snprintk(client_id_buf, sizeof(client_id_buf), "%s", imei_buf);

#else
	snprintk(client_id_buf, sizeof(client_id_buf), "%s", CONFIG_CLOUD_CLIENT_ID);
#endif

	/* Fetch IMEI from modem data and set IMEI as cloud connection ID */
	config.device_id = client_id_buf;
	config.device_id_len = strlen(client_id_buf);

	err = azure_iot_hub_init(&config, azure_iot_hub_event_handler);
	if (err) {
		LOG_ERR("azure_iot_hub_init, error: %d", err);
		return err;
	}

	LOG_DBG("********************************************");
	LOG_DBG(" The Asset Tracker v2 has started");
	LOG_DBG(" Version:      %s", CONFIG_ASSET_TRACKER_V2_APP_VERSION);
	LOG_DBG(" Client ID:    %s", log_strdup(client_id_buf));
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

	err = azure_iot_hub_connect();
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
	struct azure_iot_hub_data msg = {
		.ptr = REQUEST_DEVICE_TWIN_STRING,
		.len = sizeof(REQUEST_DEVICE_TWIN_STRING) - 1,
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
	struct azure_iot_hub_data msg = {
		.ptr = buf,
		.len = len,
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

int cloud_wrap_data_send(char *buf, size_t len, bool ack, uint32_t id)
{
	int err;
	struct azure_iot_hub_data msg = {
		.ptr = buf,
		.len = len,
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
	struct azure_iot_hub_data msg = {
		.ptr = buf,
		.len = len,
		.message_id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.topic.prop_bag = prop_bag_batch,
		.topic.prop_bag_count = ARRAY_SIZE(prop_bag_batch)
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("azure_iot_hub_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_ui_send(char *buf, size_t len, bool ack, uint32_t id)
{
	int err;
	struct azure_iot_hub_data msg = {
		.ptr = buf,
		.len = len,
		.message_id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.topic.prop_bag = prop_bag_message,
		.topic.prop_bag_count = ARRAY_SIZE(prop_bag_message)
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("azure_iot_hub_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_neighbor_cells_send(char *buf, size_t len, bool ack, uint32_t id)
{
	int err;
	struct azure_iot_hub_data msg = {
		.ptr = buf,
		.len = len,
		.message_id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.topic.prop_bag = prop_bag_ncellmeas,
		.topic.prop_bag_count = ARRAY_SIZE(prop_bag_ncellmeas)
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
	struct azure_iot_hub_data msg = {
		.ptr = buf,
		.len = len,
		.message_id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.topic.prop_bag = prop_bag_agps,
		.topic.prop_bag_count = ARRAY_SIZE(prop_bag_agps)
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
	struct azure_iot_hub_data msg = {
		.ptr = buf,
		.len = len,
		.message_id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.topic.prop_bag = prop_bag_pgps,
		.topic.prop_bag_count = ARRAY_SIZE(prop_bag_pgps)
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
