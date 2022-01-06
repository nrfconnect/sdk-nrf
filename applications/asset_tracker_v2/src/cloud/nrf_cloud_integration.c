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

#define REQUEST_DEVICE_STATE_STRING ""

/* String used to filter out responses to
 * cellular position requests (neighbor cell measurements).
 */
#define CELL_POS_FILTER_STRING "CELL_POS"

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
	struct nrf_cloud_svc_info_fota fota_info = {
		.application = true,
		.bootloader = true,
		.modem = true
	};
	struct nrf_cloud_svc_info_ui ui_info = {
		.gps = true,
		.humidity = true,
		.air_pressure = true,
		.rsrp = true,
		.temperature = true,
		.button = true
	};
	struct nrf_cloud_svc_info service_info = {
		.fota = &fota_info,
		.ui = &ui_info
	};
	struct nrf_cloud_device_status device_status = {
		.modem = NULL,
		.svc = &service_info

	};

	err = nrf_cloud_shadow_device_status_update(&device_status);
	if (err) {
		LOG_ERR("nrf_cloud_shadow_device_status_update, error: %d", err);
		return err;
	}

	LOG_DBG("nRF Cloud service info sent");

	return 0;
}

/* Function used to filter out responses to cellular position requests. */
static int cell_pos_response_filter(char *response, size_t len)
{
	ARG_UNUSED(len);

	if (strstr(response, CELL_POS_FILTER_STRING) != NULL) {
		return -EINVAL;
	}

	return 0;
}

static void nrf_cloud_event_handler(const struct nrf_cloud_evt *evt)
{
	struct cloud_wrap_event cloud_wrap_evt = { 0 };
	bool notify = false;
	int err;

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

		err = send_service_info();
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

		/* Filter out responses to cellular position requests. The application does not care
		 * about getting its location back after neighbor cell measurements have been
		 * sent to cloud.
		 */
		err = cell_pos_response_filter((char *)evt->data.ptr, evt->data.len);
		if (err) {
			LOG_DBG("Cellular position response received, aborting");
			return;
		}

		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DATA_RECEIVED;
		cloud_wrap_evt.data.buf = (char *)evt->data.ptr;
		cloud_wrap_evt.data.len = evt->data.len;
		notify = true;
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		LOG_WRN("NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST");
		LOG_WRN("Add the device to nRF Cloud and wait for it to reconnect");

		/* A disconnect/reconnect is required by nRF Cloud after the
		 * NRF_CLOUD_EVT_USER_ASSOCIATED event is received. This event is sent from
		 * nRF Cloud when the device has been added to an nRF Cloud account.
		 * However, it is likely that the device is in PSM when this occurs and not able to
		 * receive this event.
		 *
		 * Due to this, we explicitly disconnect the application
		 * when the NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST event is received and depend on
		 * the reconnection routine in the cloud module to reconnect the application until
		 * the device has been registered to the nRF Cloud account.
		 *
		 * It is expected that the application will disconnect and reconnect to nRF Cloud
		 * several times during device association.
		 */
		err = nrf_cloud_disconnect();
		if (err) {
			LOG_ERR("nrf_cloud_disconnect failed, error: %d", err);

			/* If disconnection from nRF Cloud fails, the cloud module is notified with
			 * an error. The application is expected to perform a reboot in order
			 * to reconnect to nRF Cloud and complete device association.
			 */
			cloud_wrap_evt.type = CLOUD_WRAP_EVT_ERROR;
			notify = true;
		}
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

int cloud_wrap_batch_send(char *buf, size_t len)
{
	int err;
	struct nrf_cloud_tx_data msg = {
		.data.ptr = buf,
		.data.len = len,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_BULK,
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

int cloud_wrap_state_get(void)
{
	/* Not supported, the nRF Cloud library automatically requests the cloud-side state upon
	 * an established connection.
	 */
	return -ENOTSUP;
}

int cloud_wrap_data_send(char *buf, size_t len)
{
	/* Not supported, all data is sent to the bulk topic. */
	return -ENOTSUP;
}

int cloud_wrap_agps_request_send(char *buf, size_t len)
{
	/* Not supported, A-GPS is requested internally via the nRF Cloud A-GPS library. */
	return -ENOTSUP;
}

int cloud_wrap_pgps_request_send(char *buf, size_t len)
{
	/* Not supported, P-GPS is requested internally via the nRF Cloud P-GPS library. */
	return -ENOTSUP;
}

int cloud_wrap_memfault_data_send(char *buf, size_t len)
{
	/* Not supported */
	return -ENOTSUP;
}
