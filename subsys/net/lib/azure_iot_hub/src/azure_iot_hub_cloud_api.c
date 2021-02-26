/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/cloud.h>
#include <net/azure_iot_hub.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(azure_iot_hub_cloud, CONFIG_AZURE_IOT_HUB_LOG_LEVEL);

static struct cloud_backend *azure_iot_hub_backend;

static void api_event_handler(struct azure_iot_hub_evt *evt)
{
	struct cloud_backend_config *config = azure_iot_hub_backend->config;
	struct cloud_event cloud_evt = { 0 };

	switch (evt->type) {
	case AZURE_IOT_HUB_EVT_CONNECTING:
		cloud_evt.type = CLOUD_EVT_CONNECTING;
		cloud_notify_event(azure_iot_hub_backend, &cloud_evt,
				   config->user_data);
		break;
	case AZURE_IOT_HUB_EVT_CONNECTED:
		cloud_evt.type = CLOUD_EVT_CONNECTED;
		cloud_notify_event(azure_iot_hub_backend, &cloud_evt,
				   config->user_data);

		break;
	case AZURE_IOT_HUB_EVT_CONNECTION_FAILED:
		cloud_evt.type = CLOUD_EVT_ERROR;
		cloud_evt.data.err = evt->data.err;
		cloud_notify_event(azure_iot_hub_backend, &cloud_evt,
				   config->user_data);
		break;
	case AZURE_IOT_HUB_EVT_DISCONNECTED:
		cloud_evt.type = CLOUD_EVT_DISCONNECTED;
		cloud_notify_event(azure_iot_hub_backend, &cloud_evt,
				   config->user_data);
		break;
	case AZURE_IOT_HUB_EVT_READY:
		cloud_evt.type = CLOUD_EVT_READY;
		cloud_notify_event(azure_iot_hub_backend, &cloud_evt,
				   config->user_data);
		break;
	case AZURE_IOT_HUB_EVT_DATA_RECEIVED:
		cloud_evt.type = CLOUD_EVT_DATA_RECEIVED;
		cloud_evt.data.msg.buf = evt->data.msg.ptr;
		cloud_evt.data.msg.len = evt->data.msg.len;
		cloud_evt.data.msg.endpoint.type = CLOUD_EP_MSG;
		cloud_evt.data.msg.endpoint.str = evt->topic.str;
		cloud_evt.data.msg.endpoint.len = evt->topic.len;
		cloud_notify_event(azure_iot_hub_backend, &cloud_evt,
				   config->user_data);
		break;
	case AZURE_IOT_HUB_EVT_TWIN_RECEIVED:
		cloud_evt.type = CLOUD_EVT_DATA_RECEIVED;
		cloud_evt.data.msg.buf = evt->data.msg.ptr;
		cloud_evt.data.msg.len = evt->data.msg.len;
		cloud_evt.data.msg.endpoint.type = CLOUD_EP_STATE;
		cloud_evt.data.msg.endpoint.str = evt->topic.str;
		cloud_evt.data.msg.endpoint.len = evt->topic.len;
		cloud_notify_event(azure_iot_hub_backend, &cloud_evt,
				   config->user_data);
		break;
	case AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED:
		cloud_evt.type = CLOUD_EVT_DATA_RECEIVED;
		cloud_evt.data.msg.buf = evt->data.msg.ptr;
		cloud_evt.data.msg.len = evt->data.msg.len;
		cloud_evt.data.msg.endpoint.type = CLOUD_EP_STATE;
		cloud_evt.data.msg.endpoint.str = evt->topic.str;
		cloud_evt.data.msg.endpoint.len = evt->topic.len;
		cloud_notify_event(azure_iot_hub_backend, &cloud_evt,
				   config->user_data);
		break;
	case AZURE_IOT_HUB_EVT_FOTA_START:
		cloud_evt.type = CLOUD_EVT_FOTA_START;
		cloud_notify_event(azure_iot_hub_backend, &cloud_evt,
				   config->user_data);
		break;
	case AZURE_IOT_HUB_EVT_FOTA_DONE:
		cloud_evt.type = CLOUD_EVT_FOTA_DONE;
		cloud_notify_event(azure_iot_hub_backend, &cloud_evt,
				   config->user_data);
		break;
	case AZURE_IOT_HUB_EVT_FOTA_ERASE_PENDING:
		cloud_evt.type = CLOUD_EVT_FOTA_ERASE_PENDING;
		cloud_notify_event(azure_iot_hub_backend, &cloud_evt,
				   config->user_data);
		break;
	case AZURE_IOT_HUB_EVT_FOTA_ERASE_DONE:
		cloud_evt.type = CLOUD_EVT_FOTA_ERASE_DONE;
		cloud_notify_event(azure_iot_hub_backend, &cloud_evt,
				   config->user_data);
	case AZURE_IOT_HUB_EVT_FOTA_ERROR:
		cloud_evt.type = CLOUD_EVT_FOTA_ERROR;
		cloud_notify_event(azure_iot_hub_backend, &cloud_evt,
				   config->user_data);
		break;
	case AZURE_IOT_HUB_EVT_ERROR:
		cloud_evt.type = CLOUD_EVT_ERROR;
		cloud_evt.data.err = evt->data.err;
		cloud_notify_event(azure_iot_hub_backend, &cloud_evt,
				   config->user_data);
		break;
	default:
		break;
	}
}

static int api_init(const struct cloud_backend *const backend,
		    cloud_evt_handler_t handler)
{
	struct azure_iot_hub_config config = {
		.device_id = backend->config->id,
		.device_id_len = backend->config->id_len
	};

	azure_iot_hub_backend = (struct cloud_backend *)backend;
	azure_iot_hub_backend->config->handler = handler;

	return azure_iot_hub_init(&config, api_event_handler);
}

static int api_connect(const struct cloud_backend *const backend)
{
	int err;

	err = azure_iot_hub_connect();
	if (err >= 0) {
		/* At this point, we can't be sure that the socket will be
		 * used for the IoT hub connection or the DPS connection only.
		 * Therefore, we don't forward the socket, and only support
		 * polling the socket in the backend. Subject to change.
		 */
		backend->config->socket = -1;

		return CLOUD_CONNECT_RES_SUCCESS;
	}

	switch (err) {
	case -ECHILD:
		return CLOUD_CONNECT_RES_ERR_NETWORK;
	case -EACCES:
		return CLOUD_CONNECT_RES_ERR_NOT_INITD;
	case -ENOEXEC:
		return CLOUD_CONNECT_RES_ERR_BACKEND;
	case -EINVAL:
		return CLOUD_CONNECT_RES_ERR_PRV_KEY;
	case -EOPNOTSUPP:
		return CLOUD_CONNECT_RES_ERR_CERT;
	case -ECONNREFUSED:
		return CLOUD_CONNECT_RES_ERR_CERT_MISC;
	case -ETIMEDOUT:
		return CLOUD_CONNECT_RES_ERR_TIMEOUT_NO_DATA;
	case -ENOMEM:
		return CLOUD_CONNECT_RES_ERR_NO_MEM;
	default:
		LOG_DBG("azure_iot_hub_connect failed %d", err);
		return CLOUD_CONNECT_RES_ERR_MISC;
	}
}

static int api_disconnect(const struct cloud_backend *const backend)
{
	return azure_iot_hub_disconnect();
}

static int api_send(const struct cloud_backend *const backend,
		  const struct cloud_msg *const msg)
{
	struct azure_iot_hub_data tx_data = {
		.ptr = msg->buf,
		.len = msg->len,
		.qos = msg->qos,
		.topic.str = msg->endpoint.str,
		.topic.len = msg->endpoint.len,
	};

	switch (msg->endpoint.type) {
	case CLOUD_EP_STATE_GET:
		tx_data.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REQUEST;
		break;
	case CLOUD_EP_STATE:
		tx_data.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REPORTED;
		break;
	case CLOUD_EP_MSG:
		tx_data.topic.type = AZURE_IOT_HUB_TOPIC_EVENT;
		break;
	default:
		if ((tx_data.topic.str == NULL) || (tx_data.topic.len == 0)) {
			LOG_ERR("Unrecognized topic");
			return -EINVAL;
		}

		break;
	}

	return azure_iot_hub_send(&tx_data);
}

static int api_input(const struct cloud_backend *const backend)
{
	return azure_iot_hub_input();
}

static int api_ping(const struct cloud_backend *const backend)
{
	return azure_iot_hub_ping();
}

static int api_keepalive_time_left(const struct cloud_backend *const backend)
{
	return azure_iot_hub_keepalive_time_left();
}

static const struct cloud_api azure_iot_hub_api = {
	.init			= api_init,
	.connect		= api_connect,
	.disconnect		= api_disconnect,
	.send			= api_send,
	.ping			= api_ping,
	.keepalive_time_left	= api_keepalive_time_left,
	.input			= api_input,
};

CLOUD_BACKEND_DEFINE(AZURE_IOT_HUB, azure_iot_hub_api);
