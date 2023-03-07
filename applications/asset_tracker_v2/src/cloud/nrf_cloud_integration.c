/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <net/nrf_cloud.h>
#include <zephyr/net/mqtt.h>
#include <stdio.h>
#include <hw_id.h>

#include "cJSON.h"
#include "json_helpers.h"
#include "json_protocol_names.h"
#include "cloud/cloud_wrapper.h"

#if defined(CONFIG_NRF_MODEM_LIB)
#include <modem/nrf_modem_lib.h>
#endif /* CONFIG_NRF_MODEM_LIB */

#define MODULE nrf_cloud_integration

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CLOUD_INTEGRATION_LOG_LEVEL);

#define REQUEST_DEVICE_STATE_STRING ""

/* String used to filter out responses to
 * cellular position requests (neighbor cell measurements).
 */
#define CELL_POS_FILTER_STRING "CELL_POS"

#define IMEI_LEN 15
#define CLOUD_CLIENT_ID_IMEI_PREFIX_LEN (sizeof(CONFIG_CLOUD_CLIENT_ID_IMEI_PREFIX) - 1)

#if !defined(CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM)
#define NRF_CLOUD_CLIENT_ID_LEN (IMEI_LEN + CLOUD_CLIENT_ID_IMEI_PREFIX_LEN)
static char client_id_buf[NRF_CLOUD_CLIENT_ID_LEN + 1] = CONFIG_CLOUD_CLIENT_ID_IMEI_PREFIX;
#else
#define NRF_CLOUD_CLIENT_ID_LEN (sizeof(CONFIG_CLOUD_CLIENT_ID) - 1)
static char client_id_buf[NRF_CLOUD_CLIENT_ID_LEN + 1];
#endif

static struct k_work_delayable user_associating_work;

static cloud_wrap_evt_handler_t wrapper_evt_handler;

#if defined(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
/* Full modem FOTA requires external flash to hold the full modem image.
 * Below is the external flash device present on the nRF9160 DK version
 * 0.14.0 and higher.
 */
static struct dfu_target_fmfu_fdev ext_flash_dev = {
	.size = 0,
	.offset = 0,
	.dev = DEVICE_DT_GET_ONE(jedec_spi_nor)
};

static int fmfu_and_modem_init(const struct device *dev)
{
	enum nrf_cloud_fota_type fota_type = NRF_CLOUD_FOTA_TYPE__INVALID;
	int ret;

	ARG_UNUSED(dev);

	/* Check if a full modem FOTA job is pending */
	ret = nrf_cloud_fota_pending_job_type_get(&fota_type);

	/* Process pending full modem FOTA job before initializing the modem library */
	if ((ret == 0) && (fota_type == NRF_CLOUD_FOTA_MODEM_FULL)) {
		/* The flash device must be set before a full modem FOTA update can be applied */
		ret = nrf_cloud_fota_fmfu_dev_set(&ext_flash_dev);
		if (ret < 0) {
			LOG_ERR("Failed to set flash device for full modem FOTA, error: %d",
				ret);
		} else {
			ret = nrf_cloud_fota_pending_job_validate(NULL);
			if ((ret < 0) && (ret != -ENODEV)) {
				LOG_ERR("Error validating full modem FOTA job: %d", ret);
			}
		}
	}

	/* Ignore the result, it will be checked later */
	(void)nrf_modem_lib_init(NORMAL_MODE);

	return 0;
}

SYS_INIT(fmfu_and_modem_init, APPLICATION, 0);
#endif /* CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE */

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
		.application = nrf_cloud_fota_is_type_enabled(NRF_CLOUD_FOTA_APPLICATION),
		.bootloader = nrf_cloud_fota_is_type_enabled(NRF_CLOUD_FOTA_BOOTLOADER),
		.modem = nrf_cloud_fota_is_type_enabled(NRF_CLOUD_FOTA_MODEM_DELTA),
		.modem_full = nrf_cloud_fota_is_type_enabled(NRF_CLOUD_FOTA_MODEM_FULL)
	};
	struct nrf_cloud_svc_info_ui ui_info = {
		.gnss = true,
#if defined(CONFIG_BOARD_THINGY91_NRF9160_NS)
		.humidity = true,
		.air_pressure = true,
		.air_quality = true,
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
		LOG_ERR("NRF_CLOUD_EVT_ERROR: %d", evt->status);
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_ERROR;
		notify = true;
		break;
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		LOG_DBG("NRF_CLOUD_EVT_SENSOR_DATA_ACK");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DATA_ACK;
		cloud_wrap_evt.message_id = *(uint16_t *)evt->data.ptr;
		notify = true;
		break;
	case NRF_CLOUD_EVT_PINGRESP:
		LOG_DBG("NRF_CLOUD_EVT_PINGRESP");
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_PING_ACK;
		notify = true;
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
	case NRF_CLOUD_EVT_RX_DATA_GENERAL:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA_GENERAL");
		break;
	case NRF_CLOUD_EVT_RX_DATA_SHADOW:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA_SHADOW");
		/* Configuration data is contained in the shadow events */
		cloud_wrap_evt.type = CLOUD_WRAP_EVT_DATA_RECEIVED;
		cloud_wrap_evt.data.buf = (char *)evt->data.ptr;
		cloud_wrap_evt.data.len = evt->data.len;
		notify = true;
		break;
	case NRF_CLOUD_EVT_RX_DATA_LOCATION:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA_LOCATION");
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
		.application_version = CONFIG_ASSET_TRACKER_V2_APP_VERSION,
#if defined(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
		.fmfu_dev_inf = &ext_flash_dev
#endif
	};

#if !defined(CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM)
	char hw_id_buf[HW_ID_LEN];

	err = hw_id_get(hw_id_buf, ARRAY_SIZE(hw_id_buf));

	if (err) {
		LOG_ERR("Failed to retrieve device ID");
		return err;
	}

	strncat(client_id_buf, hw_id_buf, IMEI_LEN);
#else
	snprintf(client_id_buf, sizeof(client_id_buf), "%s", CONFIG_CLOUD_CLIENT_ID);
#endif

	config.client_id = client_id_buf;

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

	err = nrf_cloud_connect();
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

int cloud_wrap_state_send(char *buf, size_t len, bool ack, uint32_t id)
{
	int err;
	struct nrf_cloud_tx_data msg = {
		.data.ptr = buf,
		.data.len = len,
		.id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_STATE,
	};

	err = nrf_cloud_send(&msg);
	if (err) {
		LOG_ERR("nrf_cloud_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_batch_send(char *buf, size_t len, bool ack, uint32_t id)
{
	int err;
	struct nrf_cloud_tx_data msg = {
		.data.ptr = buf,
		.data.len = len,
		.id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_BULK,
	};

	err = nrf_cloud_send(&msg);
	if (err) {
		LOG_ERR("nrf_cloud_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_ui_send(char *buf, size_t len, bool ack, uint32_t id,
		       const struct lwm2m_obj_path path_list[])
{
	ARG_UNUSED(path_list);

	int err;
	struct nrf_cloud_tx_data msg = {
		.data.ptr = buf,
		.data.len = len,
		.id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
	};

	err = nrf_cloud_send(&msg);
	if (err) {
		LOG_ERR("nrf_cloud_send, error: %d", err);
		return err;
	}

	return 0;
}

int cloud_wrap_cloud_location_send(char *buf, size_t len, bool ack, uint32_t id)
{
	int err;
	struct nrf_cloud_tx_data msg = {
		.data.ptr = buf,
		.data.len = len,
		.id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
	};

	err = nrf_cloud_send(&msg);
	if (err) {
		LOG_ERR("nrf_cloud_send, error: %d", err);
		return err;
	}

	return 0;
}

#if defined(CONFIG_LOCATION_METHOD_WIFI)
int cloud_wrap_wifi_access_points_send(char *buf, size_t len, bool ack, uint32_t id)
{
	int err;
	struct nrf_cloud_tx_data msg = {
		.data.ptr = buf,
		.data.len = len,
		.id = id,
		.qos = ack ? MQTT_QOS_1_AT_LEAST_ONCE : MQTT_QOS_0_AT_MOST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
	};

	err = nrf_cloud_send(&msg);
	if (err) {
		LOG_ERR("nrf_cloud_send, error: %d", err);
		return err;
	}

	return 0;
}
#endif

int cloud_wrap_state_get(bool ack, uint32_t id)
{
	/* Not supported, the nRF Cloud library automatically requests the cloud-side state upon
	 * an established connection.
	 */
	return -ENOTSUP;
}

int cloud_wrap_data_send(char *buf, size_t len, bool ack, uint32_t id,
			 const struct lwm2m_obj_path path_list[])
{
	ARG_UNUSED(path_list);
	/* Not supported, all data is sent to the bulk topic. */
	return -ENOTSUP;
}

int cloud_wrap_agps_request_send(char *buf, size_t len, bool ack, uint32_t id)
{
	/* Not supported, A-GPS is requested internally via the nRF Cloud A-GPS library. */
	return -ENOTSUP;
}

int cloud_wrap_pgps_request_send(char *buf, size_t len, bool ack, uint32_t id)
{
	/* Not supported, P-GPS is requested internally via the nRF Cloud P-GPS library. */
	return -ENOTSUP;
}

int cloud_wrap_memfault_data_send(char *buf, size_t len, bool ack, uint32_t id)
{
	/* Not supported */
	return -ENOTSUP;
}
