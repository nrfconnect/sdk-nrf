/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#if defined(CONFIG_POSIX_API)
#include <posix/poll.h>
#else
#include <net/socket.h>
#endif
#include <net/cloud.h>
#include <net/nrf_cloud.h>
#include <net/mqtt.h>
#include "nrf_cloud_codec.h"
#include "nrf_cloud_fsm.h"
#include "nrf_cloud_transport.h"
#include "nrf_cloud_fota.h"
#include "nrf_cloud_mem.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(nrf_cloud, CONFIG_NRF_CLOUD_LOG_LEVEL);

/* Identifier for user association message sent to the cloud.
 * A unique, unsigned 16 bit magic number. Can't be zero.
 */
#define CC_UA_DATA_ID 9547

/* Flag to indicate if a disconnect has been requested. */
static atomic_t disconnect_requested;

/* Flag to indicate if a transport disconnect event has been received. */
static atomic_t transport_disconnected;

/* Flag to indicate if uninit/cleanup is in progress. */
static atomic_t uninit_in_progress;
static K_SEM_DEFINE(uninit_disconnect, 0, 1);

/* Handler registered by the application with the module to receive
 * asynchronous events.
 */
static nrf_cloud_event_handler_t app_event_handler;

/* Maintains the state with respect to the cloud. */
static volatile enum nfsm_state current_state = STATE_IDLE;
static K_MUTEX_DEFINE(state_mutex);

#if IS_ENABLED(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD)
static K_SEM_DEFINE(connection_poll_sem, 0, 1);
static atomic_t connection_poll_active;
static int start_connection_poll();
#endif

enum nfsm_state nfsm_get_current_state(void)
{
	return current_state;
}

void nfsm_set_current_state_and_notify(enum nfsm_state state,
				       const struct nrf_cloud_evt *evt)
{
	bool discon_evt = (evt != NULL) &&
			  (evt->type == NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED);

	k_mutex_lock(&state_mutex, K_FOREVER);

	if (!atomic_get(&uninit_in_progress)) {
		LOG_DBG("state: %d", state);
		current_state = state;
	}

	k_mutex_unlock(&state_mutex);

	if (discon_evt) {
		atomic_set(&transport_disconnected, 1);
	}

	if ((app_event_handler != NULL) && (evt != NULL)) {
		app_event_handler(evt);
	}

	if (discon_evt && atomic_get(&uninit_in_progress)) {
		/* User has been notified of disconnect, continue with un-init */
		k_sem_give(&uninit_disconnect);
	}
}

bool nfsm_get_disconnect_requested(void)
{
	return (bool)atomic_get(&disconnect_requested);
}

int nrf_cloud_init(const struct nrf_cloud_init_param *param)
{
	int err;

	if (current_state != STATE_IDLE ||
	    atomic_get(&uninit_in_progress)) {
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
	err = nrf_cloud_codec_init();
	if (err) {
		return err;
	}

	/* Initialize the transport. */
	err = nct_init(param->client_id);
	if (err) {
		return err;
	}

	app_event_handler = param->event_handler;

	nfsm_set_current_state_and_notify(STATE_INITIALIZED, NULL);

	return 0;
}

int nrf_cloud_uninit(void)
{
	int err = 0;
	enum nfsm_state prev_state;

#if defined(CONFIG_NRF_CLOUD_FOTA)
	err = nrf_cloud_fota_uninit();
	if (err == -EBUSY) {
		LOG_WRN("Cannot uninitialize while a FOTA job is active");
		return -EBUSY;
	}
#endif

	atomic_set(&uninit_in_progress, 1);

	k_mutex_lock(&state_mutex, K_FOREVER);
	/* Save state and set to idle to prevent other API calls */
	prev_state = current_state;
	current_state = STATE_IDLE;
	k_mutex_unlock(&state_mutex);

	if (prev_state >= STATE_CONNECTED) {
		LOG_DBG("Disconnecting from nRF Cloud");

		atomic_set(&disconnect_requested, 1);
		k_sem_reset(&uninit_disconnect);
		(void)nct_disconnect();

		err = k_sem_take(&uninit_disconnect, K_SECONDS(30));
		if (err == -EAGAIN) {
			LOG_WRN("Did not receive expected disconnect event during cloud unint");
			err = -EISCONN;
		}
	}

	LOG_DBG("Cleaning up nRF Cloud resources");
	app_event_handler = NULL;
	nct_uninit();

	atomic_set(&uninit_in_progress, 0);
	return err;
}

static int connect_error_translate(const int err)
{
	switch (err) {
	case 0:
		return NRF_CLOUD_CONNECT_RES_SUCCESS;
	case -ECHILD:
		return NRF_CLOUD_CONNECT_RES_ERR_NETWORK;
	case -EACCES:
		return NRF_CLOUD_CONNECT_RES_ERR_NOT_INITD;
	case -ENOEXEC:
		return NRF_CLOUD_CONNECT_RES_ERR_BACKEND;
	case -EINVAL:
		return NRF_CLOUD_CONNECT_RES_ERR_PRV_KEY;
	case -EOPNOTSUPP:
		return NRF_CLOUD_CONNECT_RES_ERR_CERT;
	case -ECONNREFUSED:
		return NRF_CLOUD_CONNECT_RES_ERR_CERT_MISC;
	case -ETIMEDOUT:
		return NRF_CLOUD_CONNECT_RES_ERR_TIMEOUT_NO_DATA;
	case -ENOMEM:
		return NRF_CLOUD_CONNECT_RES_ERR_NO_MEM;
	case -EINPROGRESS:
		return NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED;
	default:
		LOG_ERR("nRF cloud connect failed %d", err);
		return NRF_CLOUD_CONNECT_RES_ERR_MISC;
	}
}

static int connect_to_cloud(void)
{
	atomic_set(&disconnect_requested, 0);
	return nct_connect();
}

int nrf_cloud_connect(const struct nrf_cloud_connect_param *param)
{
	int err;

	if (current_state == STATE_IDLE) {
		return NRF_CLOUD_CONNECT_RES_ERR_NOT_INITD;
	} else if (current_state != STATE_INITIALIZED) {
		return NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED;
	}

#if IS_ENABLED(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD)
	err = start_connection_poll();
#else
	err = connect_to_cloud();
	if (!err) {
		atomic_set(&transport_disconnected,0);
	}
#endif
	return connect_error_translate(err);
}

int nrf_cloud_disconnect(void)
{
	if (current_state < STATE_CONNECTED) {
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
	};

	if (current_state != STATE_DC_CONNECTED) {
		return -EACCES;
	}

	if (param == NULL) {
		return -EINVAL;
	}

	if (IS_VALID_USER_TAG(param->tag)) {
		sensor_data.message_id = param->tag;
	} else {
		sensor_data.message_id = NCT_MSG_ID_USE_NEXT_INCREMENT;
	}

	err = nrf_cloud_encode_shadow_data(param, &sensor_data.data);
	if (err) {
		return err;
	}

	err = nct_cc_send(&sensor_data);
	nrf_cloud_free((void *)sensor_data.data.ptr);

	return err;
}

int nrf_cloud_shadow_device_status_update(const struct nrf_cloud_device_status *const dev_status)
{
	int err = 0;
	struct nrf_cloud_tx_data tx_data = {
		.topic_type = NRF_CLOUD_TOPIC_STATE,
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	if (current_state != STATE_DC_CONNECTED) {
		return -EACCES;
	}

	err = nrf_cloud_device_status_encode(dev_status, &tx_data.data, true);
	if (err) {
		return err;
	}

	err = nrf_cloud_send(&tx_data);

	nrf_cloud_device_status_free(&tx_data.data);

	return err;
}

int nrf_cloud_sensor_data_send(const struct nrf_cloud_sensor_data *param)
{
	int err;
	struct nct_dc_data sensor_data;

	if (current_state != STATE_DC_CONNECTED) {
		return -EACCES;
	}

	if (param == NULL) {
		return -EINVAL;
	}

	err = nrf_cloud_encode_sensor_data(param, &sensor_data.data);
	if (err) {
		return err;
	}

	if (IS_VALID_USER_TAG(param->tag)) {
		sensor_data.message_id = param->tag;
	} else {
		sensor_data.message_id = NCT_MSG_ID_USE_NEXT_INCREMENT;
	}

	err = nct_dc_send(&sensor_data);
	nrf_cloud_free((void *)sensor_data.data.ptr);

	return err;
}

int nrf_cloud_sensor_data_stream(const struct nrf_cloud_sensor_data *param)
{
	int err;
	struct nct_dc_data sensor_data;

	if (current_state != STATE_DC_CONNECTED) {
		return -EACCES;
	}

	if (param == NULL) {
		return -EINVAL;
	}

	err = nrf_cloud_encode_sensor_data(param, &sensor_data.data);
	if (err) {
		return err;
	}

	err = nct_dc_stream(&sensor_data);
	nrf_cloud_free((void *)sensor_data.data.ptr);

	return err;
}

int nrf_cloud_send(const struct nrf_cloud_tx_data *msg)
{
	int err;

	if (current_state != STATE_DC_CONNECTED) {
		return -EACCES;
	}

	switch (msg->topic_type) {
	case NRF_CLOUD_TOPIC_STATE: {
		const struct nct_cc_data shadow_data = {
			.opcode = NCT_CC_OPCODE_UPDATE_REQ,
			.data.ptr = msg->data.ptr,
			.data.len = msg->data.len,
			.message_id = NCT_MSG_ID_USE_NEXT_INCREMENT
		};

		err = nct_cc_send(&shadow_data);
		if (err) {
			LOG_ERR("nct_cc_send failed, error: %d\n", err);
			return err;
		}

		break;
	}
	case NRF_CLOUD_TOPIC_MESSAGE: {
		const struct nct_dc_data buf = {
			.data.ptr = msg->data.ptr,
			.data.len = msg->data.len,
			.message_id = NCT_MSG_ID_USE_NEXT_INCREMENT
		};

		if (msg->qos == MQTT_QOS_0_AT_MOST_ONCE) {
			err = nct_dc_stream(&buf);
		} else if (msg->qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			err = nct_dc_send(&buf);
		} else {
			err = -EINVAL;
			LOG_ERR("Unsupported QoS setting");
			return err;
		}

		break;
	}
	case NRF_CLOUD_TOPIC_BULK: {
		const struct nct_dc_data buf = {
			.data.ptr = msg->data.ptr,
			.data.len = msg->data.len,
			.message_id = NCT_MSG_ID_USE_NEXT_INCREMENT
		};

		err = nct_dc_bulk_send(&buf, msg->qos);
		if (err) {
			LOG_ERR("nct_dc_bulk_send failed, error: %d", err);
			return err;
		}

		break;
	}
	default:
		LOG_ERR("Unknown topic type");
		return -ENODATA;
	}

	return 0;
}

int nrf_cloud_tenant_id_get(char *id_buf, size_t id_len)
{
	return nct_tenant_id_get(id_buf, id_len);
}

int nct_input(const struct nct_evt *evt)
{
	return nfsm_handle_incoming_event(evt, current_state);
}

void nct_apply_update(const struct nrf_cloud_evt * const evt)
{
	app_event_handler(evt);
}

int nrf_cloud_process(void)
{
	return nct_process();
}

#if IS_ENABLED(CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD)
static int start_connection_poll()
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

	return 0;
}

void nrf_cloud_run(void)
{
	int ret;
	struct pollfd fds[1];
	struct nrf_cloud_evt evt;

start:
	k_sem_take(&connection_poll_sem, K_FOREVER);
	atomic_set(&connection_poll_active, 1);

	evt.type = NRF_CLOUD_EVT_TRANSPORT_CONNECTING;
	evt.status = NRF_CLOUD_CONNECT_RES_SUCCESS;
	nfsm_set_current_state_and_notify(nfsm_get_current_state(), &evt);

	ret = connect_to_cloud();
	ret = connect_error_translate(ret);

	if (ret != NRF_CLOUD_CONNECT_RES_SUCCESS) {
		evt.type = NRF_CLOUD_EVT_TRANSPORT_CONNECTING;
		evt.status = ret;
		nfsm_set_current_state_and_notify(nfsm_get_current_state(), &evt);
		goto reset;
	} else {
		LOG_DBG("Cloud connection request sent");
	}

	fds[0].fd = nct_socket_get();
	fds[0].events = POLLIN;

	/* Only disconnect events will occur below */
	evt.type = NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED;
	atomic_set(&transport_disconnected, 0);

	while (true) {
		ret = poll(fds, ARRAY_SIZE(fds), nct_keepalive_time_left());

		/* If poll returns 0 the timeout has expired. */
		if (ret == 0) {
			ret = nrf_cloud_process();
			if ((ret < 0) && (ret != -EAGAIN)) {
				LOG_DBG("Disconnecting; nrf_cloud_process returned an error: %d",
					ret);
				evt.status = NRF_CLOUD_DISCONNECT_CLOSED_BY_REMOTE;
				break;
			}
			continue;
		}

		if ((fds[0].revents & POLLIN) == POLLIN) {
			ret = nrf_cloud_process();
			if ((ret < 0) && (ret != -EAGAIN)) {
				LOG_DBG("Disconnecting; nrf_cloud_process returned an error: %d",
					ret);
				evt.status = NRF_CLOUD_DISCONNECT_CLOSED_BY_REMOTE;
				break;
			}

			if (atomic_get(&transport_disconnected) == 1) {
				LOG_DBG("The cloud socket is already closed");
				break;
			}

			continue;
		}

		if (ret < 0) {
			LOG_ERR("poll() returned an error: %d", ret);
			evt.status = NRF_CLOUD_DISCONNECT_MISC;
			break;
		}

		if ((fds[0].revents & POLLNVAL) == POLLNVAL) {
			LOG_DBG("Socket error: POLLNVAL");
			if (nfsm_get_disconnect_requested()) {
				LOG_DBG("The cloud socket was disconnected by request");
			} else {
				LOG_DBG("The cloud socket was unexpectedly closed");
			}

			evt.status = NRF_CLOUD_DISCONNECT_INVALID_REQUEST;
			break;
		}

		if ((fds[0].revents & POLLHUP) == POLLHUP) {
			LOG_DBG("Socket error: POLLHUP");
			LOG_DBG("Connection was closed by the cloud");
			evt.status = NRF_CLOUD_DISCONNECT_CLOSED_BY_REMOTE;
			break;
		}

		if ((fds[0].revents & POLLERR) == POLLERR) {
			LOG_DBG("Socket error: POLLERR");
			LOG_DBG("Cloud connection was unexpectedly closed");
			evt.status = NRF_CLOUD_DISCONNECT_MISC;
			break;
		}
	}

	/* Send the event if the transport has not already been disconnected */
	if (atomic_get(&transport_disconnected) == 0) {
		nfsm_set_current_state_and_notify(STATE_INITIALIZED, &evt);
		nrf_cloud_disconnect();
	}

reset:
	atomic_set(&connection_poll_active, 0);
	k_sem_take(&connection_poll_sem, K_NO_WAIT);
	goto start;
}

#ifdef CONFIG_BOARD_QEMU_X86
#define POLL_THREAD_STACK_SIZE 4096
#else
#define POLL_THREAD_STACK_SIZE 3072
#endif
K_THREAD_DEFINE(connection_poll_thread, POLL_THREAD_STACK_SIZE,
		nrf_cloud_run, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
#endif

#if defined(CONFIG_CLOUD_API)
/* Cloud API specific wrappers. */
static struct cloud_backend *nrf_cloud_backend;

static enum cloud_disconnect_reason
api_disconnect_status_translate(const enum nrf_cloud_disconnect_status status)
{
	switch (status) {
	case NRF_CLOUD_DISCONNECT_USER_REQUEST:
		return CLOUD_DISCONNECT_USER_REQUEST;
	case NRF_CLOUD_DISCONNECT_CLOSED_BY_REMOTE:
		return CLOUD_DISCONNECT_CLOSED_BY_REMOTE;
	case NRF_CLOUD_DISCONNECT_INVALID_REQUEST:
		return CLOUD_DISCONNECT_INVALID_REQUEST;
	case NRF_CLOUD_DISCONNECT_MISC:
	default:
		return CLOUD_DISCONNECT_MISC;
	}
}

static int api_connect_error_translate(const int err)
{
	switch (err) {
	case NRF_CLOUD_CONNECT_RES_SUCCESS:
		return CLOUD_CONNECT_RES_SUCCESS;
	case NRF_CLOUD_CONNECT_RES_ERR_NETWORK:
		return CLOUD_CONNECT_RES_ERR_NETWORK;
	case NRF_CLOUD_CONNECT_RES_ERR_NOT_INITD:
		return CLOUD_CONNECT_RES_ERR_NOT_INITD;
	case NRF_CLOUD_CONNECT_RES_ERR_BACKEND:
		return CLOUD_CONNECT_RES_ERR_BACKEND;
	case NRF_CLOUD_CONNECT_RES_ERR_PRV_KEY:
		return CLOUD_CONNECT_RES_ERR_PRV_KEY;
	case NRF_CLOUD_CONNECT_RES_ERR_CERT:
		return CLOUD_CONNECT_RES_ERR_CERT;
	case NRF_CLOUD_CONNECT_RES_ERR_CERT_MISC:
		return CLOUD_CONNECT_RES_ERR_CERT_MISC;
	case NRF_CLOUD_CONNECT_RES_ERR_TIMEOUT_NO_DATA:
		return CLOUD_CONNECT_RES_ERR_TIMEOUT_NO_DATA;
	case NRF_CLOUD_CONNECT_RES_ERR_NO_MEM:
		return CLOUD_CONNECT_RES_ERR_NO_MEM;
	case NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED:
		return CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED;
	default:
		LOG_ERR("nRF cloud connect failed %d", err);
		return CLOUD_CONNECT_RES_ERR_MISC;
	}
}

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
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTING");

		evt.type = CLOUD_EVT_CONNECTING;
		evt.data.err =
			api_connect_error_translate(nrf_cloud_evt->status);
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
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		/* Not implemented; the cloud API does not support a
		 * tag/message ID when sending data.
		 */
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED");

		atomic_set(&transport_disconnected, 1);
		evt.data.err =
			api_disconnect_status_translate(nrf_cloud_evt->status);
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
		evt.data.msg.endpoint.type = CLOUD_EP_MSG;
		evt.data.msg.endpoint.str =
			(char *)nrf_cloud_evt->topic.ptr;
		evt.data.msg.endpoint.len = nrf_cloud_evt->topic.len;

		cloud_notify_event(nrf_cloud_backend, &evt, config->user_data);
		break;
	case NRF_CLOUD_EVT_FOTA_DONE:
		LOG_DBG("NRF_CLOUD_EVT_FOTA_DONE");

		evt.type = CLOUD_EVT_FOTA_DONE;
		evt.data.msg.buf = (char *)nrf_cloud_evt->data.ptr;
		evt.data.msg.len = nrf_cloud_evt->data.len;

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
	struct nrf_cloud_init_param params = {
		.event_handler = api_event_handler
	};

#if defined(CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME)
	if (!backend->config->id || !backend->config->id_len) {
		LOG_ERR("ID has not been set in the backend config");
		return -EINVAL;
	} else if (backend->config->id_len > NRF_CLOUD_CLIENT_ID_MAX_LEN) {
		LOG_ERR("ID must not exceed %d characters",
			NRF_CLOUD_CLIENT_ID_MAX_LEN);
		return -EBADR;
	}

	params.client_id = backend->config->id;
#endif

	backend->config->handler = handler;
	nrf_cloud_backend = (struct cloud_backend *)backend;

	return nrf_cloud_init(&params);
}

static int api_uninit(const struct cloud_backend *const backend)
{
	ARG_UNUSED(backend);
	return nrf_cloud_uninit();
}

static int api_connect(const struct cloud_backend *const backend)
{
	int err;

	err = nrf_cloud_connect(NULL);
	if (!err) {
		backend->config->socket = nct_socket_get();
	}

	return api_connect_error_translate(err);
}

static int api_disconnect(const struct cloud_backend *const backend)
{
	ARG_UNUSED(backend);
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
	case CLOUD_EP_MSG: {
		struct nrf_cloud_tx_data buf = {
			.data.ptr = msg->buf,
			.data.len = msg->len,
			.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
		};

		if (msg->qos == CLOUD_QOS_AT_MOST_ONCE) {
			buf.qos = MQTT_QOS_0_AT_MOST_ONCE;
		} else if (msg->qos == CLOUD_QOS_AT_LEAST_ONCE) {
			buf.qos = MQTT_QOS_1_AT_LEAST_ONCE;
		} else {
			err = -EINVAL;
			LOG_ERR("Unsupported QoS setting");
			return err;
		}

		err = nrf_cloud_send(&buf);
		if (err) {
			LOG_ERR("nrf_cloud_send failed, error: %d", err);
			return err;
		}

		break;
	}
	case CLOUD_EP_STATE: {
		struct nrf_cloud_tx_data shadow_data = {
			.data.ptr = msg->buf,
			.data.len = msg->len,
			.topic_type = NRF_CLOUD_TOPIC_STATE,
		};

		err = nrf_cloud_send(&shadow_data);
		if (err) {
			LOG_ERR("nrf_cloud_send failed, error: %d", err);
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
	return nrf_cloud_process();
}

static int api_keepalive_time_left(const struct cloud_backend *const backend)
{
	return nct_keepalive_time_left();
}

static int api_input(const struct cloud_backend *const backend)
{
	return nrf_cloud_process();
}

static int api_user_data_set(const struct cloud_backend *const backend,
			 void *user_data)
{
	backend->config->user_data = user_data;
	return 0;
}

static int api_id_get(const struct cloud_backend *const backend,
		      char *id, size_t id_len)
{
	return nrf_cloud_client_id_get(id, id_len);
}

static const struct cloud_api nrf_cloud_api = {
	.init = api_init,
	.uninit = api_uninit,
	.connect = api_connect,
	.disconnect = api_disconnect,
	.send = api_send,
	.ping = api_ping,
	.keepalive_time_left = api_keepalive_time_left,
	.input = api_input,
	.user_data_set = api_user_data_set,
	.id_get = api_id_get
};

CLOUD_BACKEND_DEFINE(NRF_CLOUD, nrf_cloud_api);

#endif /* CONFIG_CLOUD_API */
