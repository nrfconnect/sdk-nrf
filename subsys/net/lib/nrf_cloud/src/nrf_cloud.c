/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <net/socket.h>
#include <net/cloud.h>
#include <net/nrf_cloud.h>
#include "nrf_cloud_codec.h"
#include "nrf_cloud_fsm.h"
#include "nrf_cloud_transport.h"
#include "nrf_cloud_mem.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(nrf_cloud, CONFIG_NRF_CLOUD_LOG_LEVEL);

/* Validates if the API was requested in the right state. */
#define NOT_VALID_STATE(EXPECTED) ((EXPECTED) < current_state)

/* Identifier for user association message sent to the cloud.
 * A unique, unsigned 16 bit magic number. Can't be zero.
 */
#define CC_UA_DATA_ID 9547

/* Maintains the state with respect to the cloud. */
static volatile enum nfsm_state current_state = STATE_IDLE;

/* Flag to indicate if a disconnect has been requested. */
static atomic_t disconnect_requested;

/* Handler registered by the application with the module to receive
 * asynchronous events.
 */
static nrf_cloud_event_handler_t app_event_handler;

static K_MUTEX_DEFINE(state_mutex);

enum nfsm_state nfsm_get_current_state(void)
{
	return current_state;
}

void nfsm_set_current_state_and_notify(enum nfsm_state state,
				       const struct nrf_cloud_evt *evt)
{
	k_mutex_lock(&state_mutex, K_FOREVER);
	LOG_DBG("state: %d", state);

	current_state = state;
	if ((app_event_handler != NULL) && (evt != NULL)) {
		app_event_handler(evt);
	}
	k_mutex_unlock(&state_mutex);
}

int nrf_cloud_init(const struct nrf_cloud_init_param *param)
{
	int err;

	if (current_state != STATE_IDLE) {
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

	app_event_handler = param->event_handler;
	current_state = STATE_INITIALIZED;

	return 0;
}

int nrf_cloud_connect(const struct nrf_cloud_connect_param *param)
{
	if (NOT_VALID_STATE(STATE_INITIALIZED)) {
		return -EACCES;
	}
	atomic_set(&disconnect_requested, 0);
	return nct_connect();
}

int nrf_cloud_disconnect(void)
{
	if (NOT_VALID_STATE(STATE_DC_CONNECTED) &&
	    NOT_VALID_STATE(STATE_CC_CONNECTED)) {
		return -EACCES;
	}

	atomic_set(&disconnect_requested, 1);
	return nct_disconnect();
}

int nrf_cloud_shadow_update(const struct nrf_cloud_sensor_data *param)
{
	int err;
	struct nct_cc_data sensor_data = {
		.opcode = NCT_CC_OPCODE_UPDATE_REQ,
		.id = param->tag
	};

	if (NOT_VALID_STATE(STATE_DC_CONNECTED)) {
		return -EACCES;
	}

	if (param == NULL) {
		return -EINVAL;
	}

	err = nrf_cloud_encode_shadow_data(param, &sensor_data.data);
	if (err) {
		return err;
	}

	err = nct_cc_send(&sensor_data);
	nrf_cloud_free((void *)sensor_data.data.ptr);

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
	return nfsm_handle_incoming_event(evt, current_state);
}

void nct_apply_update(void)
{
	static const struct nrf_cloud_evt evt = {
		.type = NRF_CLOUD_EVT_FOTA_DONE
	};

	app_event_handler(&evt);
}

void nrf_cloud_process(void)
{
	nct_process();
}

#if defined(CONFIG_CLOUD_API)
/* Cloud API specific wrappers. */

#define POLL_TIMEOUT_MS 500
static struct cloud_backend *nrf_cloud_backend;
static K_SEM_DEFINE(connection_poll_sem, 0, 1);
static atomic_t connection_poll_active;

static void api_event_handler(const struct nrf_cloud_evt *nrf_cloud_evt)
{
	struct cloud_backend_config *config = nrf_cloud_backend->config;
	struct cloud_event evt = { 0 };

	switch (nrf_cloud_evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTED");

		evt.type = CLOUD_EVT_CONNECTED;
		evt.data.persistent_session = (nrf_cloud_evt->status != 0);
		cloud_notify_event(nrf_cloud_backend, &evt, config->user_data);
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST");

		evt.type = CLOUD_EVT_PAIR_REQUEST;

		cloud_notify_event(nrf_cloud_backend, &evt, config->user_data);
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATED");

		evt.type = CLOUD_EVT_PAIR_DONE;

		cloud_notify_event(nrf_cloud_backend, &evt, config->user_data);
		break;
	case NRF_CLOUD_EVT_READY:
		LOG_DBG("NRF_CLOUD_EVT_READY");

		evt.type = CLOUD_EVT_READY;

		cloud_notify_event(nrf_cloud_backend, &evt, config->user_data);
		break;
	case NRF_CLOUD_EVT_SENSOR_ATTACHED:
		LOG_DBG("NRF_CLOUD_EVT_SENSOR_ATTACHED");

		break;
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		LOG_DBG("NRF_CLOUD_EVT_SENSOR_DATA_ACK");

		evt.type = CLOUD_EVT_DATA_SENT;

		cloud_notify_event(nrf_cloud_backend, &evt, config->user_data);
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED");

		evt.data.err = CLOUD_DISCONNECT_MISC;

		if (atomic_get(&disconnect_requested)) {
#if IS_ENABLED(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD)
			if (atomic_get(&connection_poll_active)) {
				/* Poll thread will send disconnect event */
				break;
			}
#endif
			evt.data.err = CLOUD_DISCONNECT_USER_REQUEST;
		}

		evt.type = CLOUD_EVT_DISCONNECTED;

		cloud_notify_event(nrf_cloud_backend, &evt, config->user_data);
		break;
	case NRF_CLOUD_EVT_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_ERROR: %d", nrf_cloud_evt->status);

		evt.type = CLOUD_EVT_ERROR;

		cloud_notify_event(nrf_cloud_backend, &evt, config->user_data);
		break;
	case NRF_CLOUD_EVT_RX_DATA:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA");

		evt.type = CLOUD_EVT_DATA_RECEIVED;
		evt.data.msg.buf = (char *)nrf_cloud_evt->data.ptr;
		evt.data.msg.len = nrf_cloud_evt->data.len;
		evt.data.msg.endpoint.type = CLOUD_EP_TOPIC_MSG;
		evt.data.msg.endpoint.str =
			(char *)nrf_cloud_evt->topic.ptr;
		evt.data.msg.endpoint.len = nrf_cloud_evt->topic.len;

		cloud_notify_event(nrf_cloud_backend, &evt, config->user_data);
		break;
	case NRF_CLOUD_EVT_FOTA_DONE:
		LOG_DBG("NRF_CLOUD_EVT_FOTA_DONE");

		evt.type = CLOUD_EVT_FOTA_DONE;

		cloud_notify_event(nrf_cloud_backend, &evt, config->user_data);
		break;
	default:
		LOG_DBG("Unknown event type: %d", nrf_cloud_evt->type);
		break;
	}
}

static int api_init(const struct cloud_backend *const backend,
		cloud_evt_handler_t handler)
{
	const struct nrf_cloud_init_param params = {
		.event_handler = api_event_handler
	};

	backend->config->handler = handler;
	nrf_cloud_backend = (struct cloud_backend *)backend;

	return nrf_cloud_init(&params);
}

static int api_uninit(const struct cloud_backend *const backend)
{
	LOG_INF("uninit() is not implemented");

	return 0;
}

static int translate_connect_error(const int err)
{
	switch (err) {
	case 0:
		return CLOUD_CONNECT_RES_SUCCESS;
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
	case -EINPROGRESS:
		return CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED;
	default:
		LOG_ERR("nRF cloud connect failed %d", err);
		return CLOUD_CONNECT_RES_ERR_MISC;
	}
}

static int start_connection_poll(const struct cloud_backend *const backend)
{
	if (current_state == STATE_IDLE) {
		return -EACCES;
	}

	if (atomic_get(&connection_poll_active)) {
		LOG_DBG("Connection poll in progress");
		return -EINPROGRESS;
	}

	atomic_set(&disconnect_requested, 0);
	k_sem_give(&connection_poll_sem);

	return CLOUD_CONNECT_RES_SUCCESS;
}

static int api_connect(const struct cloud_backend *const backend)
{
	int err;

	if (IS_ENABLED(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD)) {
		err = start_connection_poll(backend);
	} else {
		err = nrf_cloud_connect(NULL);
		if (!err) {
			backend->config->socket = nct_socket_get();
		}
	}

	return translate_connect_error(err);
}

static int api_disconnect(const struct cloud_backend *const backend)
{
	return nrf_cloud_disconnect();
}

static int api_send(const struct cloud_backend *const backend,
		const struct cloud_msg *const msg)
{
	int err = 0;

	if (msg->endpoint.len != 0) {
		/* Unsupported case where topic is not the default. */
		return -ENOTSUP;
	}

	switch (msg->endpoint.type) {
	case CLOUD_EP_TOPIC_MSG: {
		const struct nct_dc_data buf = {
			.data.ptr = msg->buf,
			.data.len = msg->len
		};

		if (msg->qos == CLOUD_QOS_AT_MOST_ONCE) {
			err = nct_dc_stream(&buf);
		} else if (msg->qos == CLOUD_QOS_AT_LEAST_ONCE) {
			err = nct_dc_send(&buf);
		} else {
			err = -EINVAL;
			LOG_ERR("Unsupported QoS setting.");
			return err;
		}
		break;
	}
	case CLOUD_EP_TOPIC_STATE: {
		struct nct_cc_data shadow_data = {
			.opcode = NCT_CC_OPCODE_UPDATE_REQ,
			.data.ptr = msg->buf,
			.data.len = msg->len
		};

		err = nct_cc_send(&shadow_data);
		if (err) {
			LOG_ERR("nct_cc_send failed, error: %d\n", err);
			return err;
		}
		break;
	}
	default:
		LOG_DBG("Unknown cloud endpoint type: %d", msg->endpoint.type);
		break;
	}

	if (err) {
		return err;
	}

	return 0;
}

static int api_ping(const struct cloud_backend *const backend)
{
	/* TODO: Do only ping, nrf_cloud_process() also checks for input. */
	nrf_cloud_process();

	return 0;
}

static int api_keepalive_time_left(const struct cloud_backend *const backend)
{
	return nct_keepalive_time_left();
}

static int api_input(const struct cloud_backend *const backend)
{
	nrf_cloud_process();

	return 0;
}

static int api_user_data_set(const struct cloud_backend *const backend,
			 void *user_data)
{
	backend->config->user_data = user_data;

	return 0;
}

void nrf_cloud_run(void)
{
	int ret;
	struct pollfd fds[1];
	struct cloud_event cloud_evt = {
		.type = CLOUD_EVT_DISCONNECTED,
		.data = { .err = CLOUD_DISCONNECT_MISC}
	};

start:
	k_sem_take(&connection_poll_sem, K_FOREVER);
	atomic_set(&connection_poll_active, 1);

	cloud_evt.data.err = CLOUD_CONNECT_RES_SUCCESS;
	cloud_evt.type = CLOUD_EVT_CONNECTING;
	cloud_notify_event(nrf_cloud_backend, &cloud_evt, NULL);

	ret = nrf_cloud_connect(NULL);
	ret = translate_connect_error(ret);

	if (ret != CLOUD_CONNECT_RES_SUCCESS) {
		cloud_evt.data.err = ret;
		cloud_evt.type = CLOUD_EVT_CONNECTING;
		cloud_notify_event(nrf_cloud_backend, &cloud_evt, NULL);
		goto reset;
	} else {
		LOG_DBG("Cloud connection request sent.");
	}

	fds[0].fd = nct_socket_get();
	fds[0].events = POLLIN;

	/* Only disconnect events will occur below */
	cloud_evt.type = CLOUD_EVT_DISCONNECTED;

	while (true) {
		ret = poll(fds, ARRAY_SIZE(fds), POLL_TIMEOUT_MS);

		if (ret == 0) {
			if (cloud_keepalive_time_left(nrf_cloud_backend) <
			    POLL_TIMEOUT_MS) {
				api_ping(nrf_cloud_backend);
			}
			continue;
		}

		if ((fds[0].revents & POLLIN) == POLLIN) {
			api_input(nrf_cloud_backend);
			continue;
		}

		if (ret < 0) {
			LOG_ERR("poll() returned an error: %d", ret);
			cloud_evt.data.err = CLOUD_DISCONNECT_MISC;
			break;
		}

		if (atomic_get(&disconnect_requested)) {
			atomic_set(&disconnect_requested, 0);
			LOG_DBG("Expected disconnect event.");
			cloud_evt.data.err = CLOUD_DISCONNECT_USER_REQUEST;
			cloud_notify_event(nrf_cloud_backend, &cloud_evt, NULL);
			goto reset;
		}

		if ((fds[0].revents & POLLNVAL) == POLLNVAL) {
			LOG_DBG("Socket error: POLLNVAL");
			LOG_DBG("The cloud socket was unexpectedly closed.");
			cloud_evt.data.err = CLOUD_DISCONNECT_INVALID_REQUEST;
			break;
		}

		if ((fds[0].revents & POLLHUP) == POLLHUP) {
			LOG_DBG("Socket error: POLLHUP");
			LOG_DBG("Connection was closed by the cloud.");
			cloud_evt.data.err = CLOUD_DISCONNECT_CLOSED_BY_REMOTE;
			break;
		}

		if ((fds[0].revents & POLLERR) == POLLERR) {
			LOG_DBG("Socket error: POLLERR");
			LOG_DBG("Cloud connection was unexpectedly closed.");
			cloud_evt.data.err = CLOUD_DISCONNECT_MISC;
			break;
		}
	}

	cloud_notify_event(nrf_cloud_backend, &cloud_evt, NULL);
	nrf_cloud_disconnect();

reset:
	atomic_set(&connection_poll_active, 0);
	k_sem_take(&connection_poll_sem, K_NO_WAIT);
	goto start;
}

#if IS_ENABLED(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD)
#ifdef CONFIG_BOARD_QEMU_X86
#define POLL_THREAD_STACK_SIZE 4096
#else
#define POLL_THREAD_STACK_SIZE 2560
#endif
K_THREAD_DEFINE(connection_poll_thread, POLL_THREAD_STACK_SIZE,
		nrf_cloud_run, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
#endif

static const struct cloud_api nrf_cloud_api = {
	.init = api_init,
	.uninit = api_uninit,
	.connect = api_connect,
	.disconnect = api_disconnect,
	.send = api_send,
	.ping = api_ping,
	.keepalive_time_left = api_keepalive_time_left,
	.input = api_input,
	.user_data_set = api_user_data_set
};

CLOUD_BACKEND_DEFINE(NRF_CLOUD, nrf_cloud_api);

#endif /* CONFIG_CLOUD_API */
