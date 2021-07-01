#include <zephyr.h>
#include <net/nrf_cloud.h>
#include <net/mqtt.h>

#include "cJSON.h"
#include "json_helpers.h"
#include "json_protocol_names.h"
#include "cloud/cloud_wrapper.h"

#define MODULE nrf_cloud_integration

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CLOUD_INTEGRATION_LOG_LEVEL);

#define NRF_CLOUD_SERVICE_INFO "{"								\
					"\"state\":{"						\
						"\"reported\":{"				\
							"\"device\":{"				\
								"\"serviceInfo\":{"		\
									"\"ui\":["		\
										"\"GPS\","	\
										"\"HUMID\","	\
										"\"RSRP\","	\
										"\"BUTTON\","	\
										"\"TEMP\""	\
									"],"			\
									"\"fota_v2\":["		\
										"\"APP\","	\
										"\"MODEM\","	\
										"\"BOOT\""	\
									"]"			\
								"}"				\
							"}"					\
						"}"						\
					"}"							\
				"}"

#define REQUEST_DEVICE_STATE_STRING ""

static cloud_wrap_evt_handler_t wrapper_evt_handler;

static void cloud_wrapper_notify_event(const struct cloud_wrap_event *evt)
{
	if ((wrapper_evt_handler != NULL) && (evt != NULL)) {
		wrapper_evt_handler(evt);
	} else {
		LOG_ERR("Library event handler not registered, or empty event");
	}
}

static int send_service_info(void)
{
	int err;
	struct nrf_cloud_tx_data msg = {
		.data.ptr = NRF_CLOUD_SERVICE_INFO,
		.data.len = sizeof(NRF_CLOUD_SERVICE_INFO) - 1,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_STATE,
	};

	err = nrf_cloud_send(&msg);
	if (err) {
		LOG_ERR("nrf_cloud_send, error: %d", err);
		return err;
	}

	LOG_DBG("nRF Cloud service info sent: %s", NRF_CLOUD_SERVICE_INFO);

	return 0;
}

static void nrf_cloud_event_handler(const struct nrf_cloud_evt *evt)
{
	struct cloud_wrap_event cloud_wrap_evt = { 0 };
	bool notify = false;

	switch (evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTING");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTED");
		break;
	case NRF_CLOUD_EVT_READY:
		LOG_DBG("NRF_CLOUD_EVT_READY");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_CONNECTED;
		notify = true;

		int err = send_service_info();

		if (err) {
			LOG_ERR("Failed to send nRF Cloud service information");
			cloud_wrap_evt.type = CLOUD_WRAP_EVT_ERROR;
		}
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DISCONNECTED;
		notify = true;
		break;
	case NRF_CLOUD_EVT_ERROR:
		LOG_ERR("NRF_CLOUD_EVT_ERROR");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_ERROR;
		notify = true;
		break;
	case NRF_CLOUD_EVT_SENSOR_ATTACHED:
		LOG_DBG("NRF_CLOUD_EVT_SENSOR_ATTACHED");
		break;
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		LOG_DBG("NRF_CLOUD_EVT_SENSOR_DATA_ACK");
		break;
	case NRF_CLOUD_EVT_FOTA_DONE:
		LOG_DBG("NRF_CLOUD_EVT_FOTA_DONE");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_FOTA_DONE;
		notify = true;
		break;
	case NRF_CLOUD_EVT_RX_DATA:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DATA_RECEIVED;
		cloud_wrap_evt.data.buf = (char *)evt->data.ptr;
		cloud_wrap_evt.data.len = evt->data.len;
		notify = true;
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		LOG_WRN("NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST");
		LOG_WRN("Add the device to nRF Cloud and wait for it to reconnect");
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATED");
		break;
	default:
		LOG_ERR("Unknown nRF Cloud event type: %d", evt->type);
		break;
	}

	if (notify) {
		cloud_wrapper_notify_event(&cloud_wrap_evt);
	}
}

int cloud_wrap_init(cloud_wrap_evt_handler_t event_handler)
{
	int err;
	struct nrf_cloud_init_param config = {
		.event_handler = nrf_cloud_event_handler,
	};

	err = nrf_cloud_init(&config);
	if (err) {
		LOG_ERR("nrf_cloud_init, error: %d", err);
		return err;
	}

	LOG_DBG("********************************************");
	LOG_DBG(" The Asset Tracker v2 has started");
	LOG_DBG(" Version:     %s", CONFIG_ASSET_TRACKER_V2_APP_VERSION);
	LOG_DBG(" Cloud:       %s", "nRF Cloud");
	LOG_DBG(" Endpoint:    %s", CONFIG_NRF_CLOUD_HOST_NAME);
	LOG_DBG("********************************************");

	wrapper_evt_handler = event_handler;

	return 0;
}

int cloud_wrap_connect(void)
{
	int err;

	err = nrf_cloud_connect(NULL);
	if (err) {
		LOG_ERR("nrf_cloud_connect, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_disconnect(void)
{
	int err;

	err = nrf_cloud_disconnect();
	if (err) {
		LOG_ERR("nrf_cloud_disconnect, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_state_get(void)
{
	/* Not supported by nRF Cloud */
	return 0;
}

int cloud_wrap_state_send(char *buf, size_t len)
{
	int err;
	struct nrf_cloud_tx_data msg = {
		.data.ptr = buf,
		.data.len = len,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_STATE,
	};

	err = nrf_cloud_send(&msg);
	if (err) {
		LOG_ERR("nrf_cloud_send, error: %d", err);
		return err;
	}

	return 0;
}

/* Decode data received from the data module and send the data to the correct endpoint. Ideally all
 * data should be sent to one endpoint in one message, but due to current restrictions in
 * nRF Cloud they must be sent to different endpoints.
 */
static int decode_and_send(cJSON *object, const char *object_name, bool device_state)
{
	int err;
	char *tx_buffer;
	struct nrf_cloud_tx_data msg = {0};
	cJSON *msg_ref = NULL;

	msg_ref = cJSON_DetachItemFromObject(object, object_name);
	if (msg_ref != NULL) {
		if (device_state) {
			/* When state object is detached from root, we need to add the reported
			 * object back to state object before sending to the shadow.
			 */
			cJSON *state_obj = cJSON_CreateObject();

			if (state_obj == NULL) {
				cJSON_Delete(msg_ref);
				return -ENOMEM;
			}

			json_add_obj(state_obj, OBJECT_STATE, msg_ref);

			tx_buffer = cJSON_Print(state_obj);
			cJSON_Delete(state_obj);
			msg.topic_type = NRF_CLOUD_TOPIC_STATE;

		} else {
			tx_buffer = cJSON_Print(msg_ref);
			cJSON_Delete(msg_ref);
			msg.topic_type = NRF_CLOUD_TOPIC_MESSAGE;
		}

		if (tx_buffer == NULL) {
			LOG_ERR("Failed to allocate memory for JSON string");
			return -ENOMEM;
		}

		msg.data.ptr = tx_buffer;
		msg.data.len = strlen(tx_buffer);
		msg.qos = MQTT_QOS_0_AT_MOST_ONCE;

		err = nrf_cloud_send(&msg);
		k_free(tx_buffer);
		if (err) {
			LOG_ERR("nrf_cloud_send, error: %d", err);
			return err;
		}
	}

	return 0;
}

int cloud_wrap_data_send(char *buf, size_t len)
{
	int err;
	cJSON *root_obj = NULL;

	root_obj = cJSON_Parse(buf);
	if (root_obj == NULL) {
		return -ENOENT;
	}

	err = decode_and_send(root_obj, OBJECT_MSG_TEMP, false);
	if (err) {
		goto clean_exit;
	}

	err = decode_and_send(root_obj, OBJECT_MSG_HUMID, false);
	if (err) {
		goto clean_exit;
	}

	err = decode_and_send(root_obj, OBJECT_MSG_GPS, false);
	if (err) {
		goto clean_exit;
	}

	err = decode_and_send(root_obj, OBJECT_MSG_RSRP, false);
	if (err) {
		goto clean_exit;
	}

	err = decode_and_send(root_obj, OBJECT_STATE, true);
	if (err) {
		goto clean_exit;
	}

clean_exit:
	cJSON_Delete(root_obj);
	return err;
}

int cloud_wrap_batch_send(char *buf, size_t len)
{
	int err;
	struct nrf_cloud_tx_data msg = {
		.data.ptr = buf,
		.data.len = len,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
	};

	err = nrf_cloud_send(&msg);
	if (err) {
		LOG_ERR("nrf_cloud_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_ui_send(char *buf, size_t len)
{
	int err;
	struct nrf_cloud_tx_data msg = {
		.data.ptr = buf,
		.data.len = len,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
	};

	err = nrf_cloud_send(&msg);
	if (err) {
		LOG_ERR("nrf_cloud_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_neighbor_cells_send(char *buf, size_t len)
{
	/* Not supported */
	return -ENOTSUP;
}
