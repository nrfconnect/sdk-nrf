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

static struct k_work_delayable user_associating_work;

static cloud_wrap_evt_handler_t wrapper_evt_handler;

static void cloud_wrapper_notify_event(const struct cloud_wrap_event *evt)
{
	if ((wrapper_evt_handler != NULL) && (evt != NULL)) {
		wrapper_evt_handler(evt);
	} else {
		LOG_ERR("Library event handler not registered, or empty event");
	}
}

/* Work item that is called if user association fails to succeed within
 * CONFIG_CLOUD_USER_ASSOCIATION_TIMEOUT_SEC.
 */
static void user_association_work_fn(struct k_work *work)
{
	/* Report an irrecoverable error in case user association fails. Will cause a reboot of the
	 * application.
	 */
	struct cloud_wrap_event cloud_wrap_evt = {
		.type = CLOUD_WRAP_EVT_ERROR,
		.err = -ETIMEDOUT
	};

	LOG_ERR("Failed to associate the device within: %d seconds",
		CONFIG_CLOUD_USER_ASSOCIATION_TIMEOUT_SEC);

	cloud_wrapper_notify_event(&cloud_wrap_evt);
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
#if defined(CONFIG_BOARD_THINGY91_NRF9160_NS)
		.humidity = true,
		.air_pressure = true,
		.temperature = true,
#endif
		.rsrp = true,
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
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_CONNECTING;
		notify = true;
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
	case NRF_CLOUD_EVT_FOTA_START:
		LOG_DBG("NRF_CLOUD_EVT_FOTA_START");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_FOTA_START;
		notify = true;
		break;
	case NRF_CLOUD_EVT_FOTA_DONE:
		LOG_DBG("NRF_CLOUD_EVT_FOTA_DONE");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_FOTA_DONE;
		notify = true;
		break;
	case NRF_CLOUD_EVT_FOTA_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_FOTA_ERROR");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_FOTA_ERROR;
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
		LOG_WRN("Add the device to nRF Cloud to complete user association");

		/* Schedule a work item that causes an error in case user association is not
		 * carried out within a configurable amount of time.
		 */
		k_work_schedule(&user_associating_work,
				K_SECONDS(CONFIG_CLOUD_USER_ASSOCIATION_TIMEOUT_SEC));

		cloud_wrap_evt.type = CLOUD_WRAP_EVT_USER_ASSOCIATION_REQUEST;
		notify = true;
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATED");

		/* Disconnect/reconnect the device to apply the appropriate device policies and to
		 * complete the user association process.
		 */
		if (k_work_delayable_is_pending(&user_associating_work)) {
			err = nrf_cloud_disconnect();
			if (err) {
				LOG_ERR("nrf_cloud_disconnect, error: %d", err);

				cloud_wrap_evt.type = CLOUD_WRAP_EVT_ERROR;
				cloud_wrap_evt.err = err;
				cloud_wrapper_notify_event(&cloud_wrap_evt);
				return;
			}

			/* Cancel user association timeout upon completion of user association. */
			k_work_cancel_delayable(&user_associating_work);

			/* Notify the rest of the applicaiton that user association succeeded. */
			cloud_wrap_evt.type = CLOUD_WRAP_EVT_USER_ASSOCIATED;
			notify = true;
		}
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

	k_work_init_delayable(&user_associating_work, user_association_work_fn);

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
