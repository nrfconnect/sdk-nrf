#include <zephyr.h>
#include <net/azure_iot_hub.h>
#include <modem/at_cmd.h>

#include "cloud/cloud_wrapper.h"

#define MODULE azure_iot_hub_integration

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CLOUD_INTEGRATION_LOG_LEVEL);

#if !defined(CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM)
#define CLIENT_ID_LEN 15
#else
#define CLIENT_ID_LEN (sizeof(CONFIG_CLOUD_CLIENT_ID) - 1)
#endif

#define PROP_BAG_COUNT 1
#define PROP_BAG_BATCH "batch"

#define REQUEST_DEVICE_TWIN_STRING ""

static struct azure_iot_hub_prop_bag prop_bag_batch[PROP_BAG_COUNT] = {
		[0].key = PROP_BAG_BATCH,
		[0].value = NULL
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
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DATA_RECEIVED;
		cloud_wrap_evt.data.buf = evt->data.msg.ptr;
		cloud_wrap_evt.data.len = evt->data.msg.len;
		notify = true;
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
	char imei_buf[20];

	/* Retrieve device IMEI from modem. */
	err = at_cmd_write("AT+CGSN", imei_buf, sizeof(imei_buf), NULL);
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
	LOG_DBG(" DPS endpoint: %s", CONFIG_AZURE_IOT_HUB_DPS_HOSTNAME);
	LOG_DBG(" ID scope:     %s", CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE);
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

int cloud_wrap_state_get(void)
{
	int err;
	struct azure_iot_hub_data msg = {
		.ptr = REQUEST_DEVICE_TWIN_STRING,
		.len = sizeof(REQUEST_DEVICE_TWIN_STRING) - 1,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REPORTED
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("azure_iot_hub_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_state_send(char *buf, size_t len)
{
	int err;
	struct azure_iot_hub_data msg = {
		.ptr = buf,
		.len = len,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REPORTED,
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("azure_iot_hub_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_data_send(char *buf, size_t len)
{
	int err;
	struct azure_iot_hub_data msg = {
		.ptr = buf,
		.len = len,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REPORTED,
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("azure_iot_hub_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_batch_send(char *buf, size_t len)
{
	int err;
	struct azure_iot_hub_data msg = {
		.ptr = buf,
		.len = len,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
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

int cloud_wrap_ui_send(char *buf, size_t len)
{
	int err;
	struct azure_iot_hub_data msg = {
		.ptr = buf,
		.len = len,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT
	};

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("azure_iot_hub_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_neighbor_cells_send(char *buf, size_t len)
{
	/* Not supported */
	return -ENOTSUP;
}
