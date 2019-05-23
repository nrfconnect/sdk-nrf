/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <net/cloud.h>
#include <net/cloud_backend.h>
#include <nrf_cloud.h>
#include "nrf_cloud_codec.h"
#include "nrf_cloud_fsm.h"
#include "nrf_cloud_transport.h"
#include "nrf_cloud_mem.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(nrf_cloud, CONFIG_NRF_CLOUD_LOG_LEVEL);

/* Validates if the API was requested in the right state. */
#define NOT_VALID_STATE(EXPECTED) ((EXPECTED) < m_current_state)

/* Identifier for user association message sent to the cloud.
 * A unique, unsigned 16 bit magic number. Can't be zero.
 */
#define CC_UA_DATA_ID 9547

/* Maintains the state with respect to the cloud. */
static volatile enum nfsm_state m_current_state = STATE_IDLE;

/* Handler registered by the application with the module to receive
 * asynchronous events.
 */
static nrf_cloud_event_handler_t m_event_handler;

enum nfsm_state nfsm_get_current_state(void)
{
	return m_current_state;
}

void nfsm_set_current_state_and_notify(enum nfsm_state state,
				       const struct nrf_cloud_evt *evt)
{
	LOG_DBG("state: %d", state);

	m_current_state = state;
	if ((m_event_handler != NULL) && (evt != NULL)) {
		m_event_handler(evt);
	}
}

int nrf_cloud_init(const struct nrf_cloud_init_param *param)
{
	int err;

	if (m_current_state != STATE_IDLE) {
		return -EACCES;
	}

	if (param->event_handler == NULL) {
		return -EINVAL;
	}

	/* Initialize the state machine. */
	err = nfsm_init();
	if (err) {
		return err;
	}
	/* Initialize the encoder, decoder unit. */
	err = nrf_codec_init();
	if (err) {
		return err;
	}

	/* Initialize the transport. */
	err = nct_init();
	if (err) {
		return err;
	}

	m_event_handler = param->event_handler;
	m_current_state = STATE_INITIALIZED;

	return 0;
}

int nrf_cloud_connect(const struct nrf_cloud_connect_param *param)
{
	if (NOT_VALID_STATE(STATE_INITIALIZED)) {
		return -EACCES;
	}
	return nct_connect();
}

int nrf_cloud_disconnect(void)
{
	if (NOT_VALID_STATE(STATE_CONNECTED)) {
		return -EACCES;
	}
	return nct_disconnect();
}

int nrf_cloud_user_associate(const struct nrf_cloud_ua_param *param)
{
	int err;

	if (param == NULL) {
		return -EINVAL;
	}

	if (NOT_VALID_STATE(STATE_UA_INPUT_WAIT)) {
		return -EACCES;
	}

	struct nct_cc_data ua_msg = {
		.opcode = NCT_CC_OPCODE_UPDATE_REQ,
		.id = CC_UA_DATA_ID
	};

	err = nrf_cloud_encode_ua(param, &ua_msg.data);
	if (err) {
		return err;
	}

	err = nct_cc_send(&ua_msg);
	nrf_cloud_free((void *)ua_msg.data.ptr);

	return err;
}

int nrf_cloud_sensor_attach(const struct nrf_cloud_sa_param *param)
{
	if (NOT_VALID_STATE(STATE_DC_CONNECTED)) {
		return -EACCES;
	}

	const struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_SENSOR_ATTACHED
	};

	nfsm_set_current_state_and_notify(STATE_DC_CONNECTED, &evt);

	return 0;
}

int nrf_cloud_sensor_data_send(const struct nrf_cloud_sensor_data *param)
{
	int err;
	struct nct_dc_data sensor_data;

	if (NOT_VALID_STATE(STATE_DC_CONNECTED)) {
		return -EACCES;
	}

	if (param == NULL) {
		return -EINVAL;
	}

	err = nrf_cloud_encode_sensor_data(param, &sensor_data.data);
	if (err) {
		return err;
	}

	sensor_data.id = param->tag;
	err = nct_dc_send(&sensor_data);
	nrf_cloud_free((void *)sensor_data.data.ptr);

	return err;
}

int nrf_cloud_sensor_data_stream(const struct nrf_cloud_sensor_data *param)
{
	int err;
	struct nct_dc_data sensor_data;

	if (NOT_VALID_STATE(STATE_DC_CONNECTED)) {
		return -EACCES;
	}

	if (param == NULL) {
		return -EINVAL;
	}

	err = nrf_cloud_encode_sensor_data(param, &sensor_data.data);
	if (err) {
		return err;
	}

	sensor_data.id = param->tag;
	err = nct_dc_stream(&sensor_data);
	nrf_cloud_free((void *)sensor_data.data.ptr);

	return err;
}

int nct_input(const struct nct_evt *evt)
{
	return nfsm_handle_incoming_event(evt, m_current_state);
}

void nrf_cloud_process(void)
{
	nct_process();
}

#if defined(CONFIG_CLOUD_API)

/* Cloud API specific wrappers. */

static struct cloud_backend *nrf_cloud_backend;

static void event_handler(const struct nrf_cloud_evt *evt)
{
	struct cloud_backend_config *backend_config =
		nrf_cloud_backend->config;
	struct cloud_event cloud_evt = { 0 };

	switch (evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTED");
		cloud_evt.type = CLOUD_EVT_CONNECTED;
		cloud_notify_event(nrf_cloud_backend, &cloud_evt,
				   backend_config->user_data);
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST");
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATED");
		break;
	case NRF_CLOUD_EVT_READY:
		LOG_DBG("NRF_CLOUD_EVT_READY");
		cloud_evt.type = CLOUD_EVT_READY;
		cloud_notify_event(nrf_cloud_backend, &cloud_evt,
				   backend_config->user_data);
		break;
	case NRF_CLOUD_EVT_SENSOR_ATTACHED:
		LOG_DBG("NRF_CLOUD_EVT_SENSOR_ATTACHED");
		break;
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		LOG_DBG("NRF_CLOUD_EVT_SENSOR_DATA_ACK");
		cloud_evt.type = CLOUD_EVT_DATA_SENT;
		cloud_notify_event(nrf_cloud_backend, &cloud_evt,
				   backend_config->user_data);
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED");
		cloud_evt.type = CLOUD_EVT_DISCONNECTED;
		cloud_notify_event(nrf_cloud_backend, &cloud_evt,
				   backend_config->user_data);
		break;
	case NRF_CLOUD_EVT_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_ERROR, status: %d", evt->status);
		cloud_evt.type = CLOUD_EVT_ERROR;
		cloud_notify_event(nrf_cloud_backend, &cloud_evt,
				   backend_config->user_data);
		break;
	case NRF_CLOUD_EVT_RX_DATA:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA");
		cloud_evt.type = CLOUD_EVT_DATA_RECEIVED;
		cloud_notify_event(nrf_cloud_backend, &cloud_evt,
				   backend_config->user_data);
		break;
	default:
		LOG_DBG("Received unknown %d", evt->type);
		break;
	}
}

static int init(const struct cloud_backend *const backend,
		cloud_evt_handler_t handler)
{
	const struct nrf_cloud_init_param params = {
		.event_handler = event_handler
	};

	backend->config->handler = handler;
	nrf_cloud_backend = (struct cloud_backend *)backend;

	return nrf_cloud_init(&params);
}

static int uninit(const struct cloud_backend *const backend)
{
	LOG_INF("uninit() is not implemented");

	return 0;
}

static int connect(const struct cloud_backend *const backend)
{
	int err;

	err = nrf_cloud_connect(NULL);

	backend->config->socket = nct_socket_get();

	return err;
}

static int disconnect(const struct cloud_backend *const backend)
{
	return nrf_cloud_disconnect();
}

static int send(const struct cloud_backend *const backend,
		const struct cloud_msg *const msg)
{
	int err = 0;
	struct nct_dc_data buf = {
		.data.ptr = msg->buf,
		.data.len = msg->len
	};

	if (msg->endpoint.len != 0) {
		/* Unsupported case where topic is not the default. */
		return -ENOTSUP;
	}

	if (msg->endpoint.type == CLOUD_EP_TOPIC_MSG) {
		if (msg->qos == CLOUD_QOS_AT_MOST_ONCE) {
			err = nct_dc_stream(&buf);
		} else if (msg->qos == CLOUD_QOS_AT_LEAST_ONCE) {
			err = nct_dc_send(&buf);
		} else {
			err = -EINVAL;
			LOG_ERR("Unsupported QoS setting.");
		}
	}

	return 0;
}

static int ping(const struct cloud_backend *const backend)
{
	/* TODO: Do only ping, nrf_cloud_process() also checks for input. */
	nrf_cloud_process();

	return 0;
}

static int input(const struct cloud_backend *const backend)
{
	nrf_cloud_process();

	return 0;
}

static int user_data_set(const struct cloud_backend *const backend,
			 void *user_data)
{
	backend->config->user_data = user_data;

	return 0;
}

static const struct cloud_api nrf_cloud_api = {
	.init = init,
	.uninit = uninit,
	.connect = connect,
	.disconnect = disconnect,
	.send = send,
	.ping = ping,
	.input = input,
	.user_data_set = user_data_set
};

CLOUD_BACKEND_DEFINE(NRF_CLOUD, nrf_cloud_api);

#endif /* CONFIG_CLOUD_API */
