/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <net/socket.h>
#include <stdio.h>
#include <dfu/mcuboot.h>
#include <math.h>
#include <event_manager.h>

#include "cloud_wrapper.h"
#include "cloud/cloud_codec/cloud_codec.h"

#define MODULE cloud_module

#include "modules_common.h"
#include "events/cloud_module_event.h"
#include "events/app_module_event.h"
#include "events/data_module_event.h"
#include "events/util_module_event.h"
#include "events/modem_module_event.h"
#include "events/gps_module_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CLOUD_MODULE_LOG_LEVEL);

BUILD_ASSERT(CONFIG_CLOUD_CONNECT_RETRIES < 14,
	    "Cloud connect retries too large");

struct cloud_msg_data {
	union {
		struct app_module_event app;
		struct data_module_event data;
		struct modem_module_event modem;
		struct cloud_module_event cloud;
		struct util_module_event util;
		struct gps_module_event gps;
	} module;
};

/* Cloud module super states. */
static enum state_type {
	STATE_LTE_DISCONNECTED,
	STATE_LTE_CONNECTED,
	STATE_SHUTDOWN
} state;

/* Cloud module sub states. */
static enum sub_state_type {
	SUB_STATE_CLOUD_DISCONNECTED,
	SUB_STATE_CLOUD_CONNECTED
} sub_state;

static struct k_delayed_work connect_check_work;

struct cloud_backoff_delay_lookup {
	int delay;
};

/* Lookup table for backoff reconnection to cloud. Binary scaling. */
static struct cloud_backoff_delay_lookup backoff_delay[] = {
	{ 32 }, { 64 }, { 128 }, { 256 }, { 512 },
	{ 2048 }, { 4096 }, { 8192 }, { 16384 }, { 32768 },
	{ 65536 }, { 131072 }, { 262144 }, { 524288 }, { 1048576 }
};

/* Variable that keeps track of how many times a reconnection to cloud
 * has been tried without success.
 */
static int connect_retries;

/* Local copy of the device configuration. */
static struct cloud_data_cfg copy_cfg;
const k_tid_t cloud_module_thread;

/* Cloud module message queue. */
#define CLOUD_QUEUE_ENTRY_COUNT		10
#define CLOUD_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_cloud, sizeof(struct cloud_msg_data),
	      CLOUD_QUEUE_ENTRY_COUNT, CLOUD_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
	.name = "cloud",
	.msg_q = &msgq_cloud,
};

/* Forward declarations. */
static void connect_check_work_fn(struct k_work *work);
static void send_config_received(void);

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state) {
	case STATE_LTE_DISCONNECTED:
		return "STATE_LTE_DISCONNECTED";
	case STATE_LTE_CONNECTED:
		return "STATE_LTE_CONNECTED";
	case STATE_SHUTDOWN:
		return "STATE_SHUTDOWN";
	default:
		return "Unknown";
	}
}

static char *sub_state2str(enum sub_state_type new_state)
{
	switch (new_state) {
	case SUB_STATE_CLOUD_DISCONNECTED:
		return "SUB_STATE_CLOUD_DISCONNECTED";
	case SUB_STATE_CLOUD_CONNECTED:
		return "SUB_STATE_CLOUD_CONNECTED";
	default:
		return "Unknown";
	}
}

static void state_set(enum state_type new_state)
{
	if (new_state == state) {
		LOG_DBG("State: %s", state2str(state));
		return;
	}

	LOG_DBG("State transition %s --> %s",
		state2str(state),
		state2str(new_state));

	state = new_state;
}

static void sub_state_set(enum sub_state_type new_state)
{
	if (new_state == sub_state) {
		LOG_DBG("Sub state: %s", sub_state2str(sub_state));
		return;
	}

	LOG_DBG("Sub state transition %s --> %s",
		sub_state2str(sub_state),
		sub_state2str(new_state));

	sub_state = new_state;
}

/* Handlers */
static bool event_handler(const struct event_header *eh)
{
	struct cloud_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_app_module_event(eh)) {
		struct app_module_event *evt = cast_app_module_event(eh);

		msg.module.app = *evt;
		enqueue_msg = true;
	}

	if (is_data_module_event(eh)) {
		struct data_module_event *evt = cast_data_module_event(eh);

		msg.module.data = *evt;
		enqueue_msg = true;
	}

	if (is_modem_module_event(eh)) {
		struct modem_module_event *evt = cast_modem_module_event(eh);

		msg.module.modem = *evt;
		enqueue_msg = true;
	}

	if (is_cloud_module_event(eh)) {
		struct cloud_module_event *evt = cast_cloud_module_event(eh);

		msg.module.cloud = *evt;
		enqueue_msg = true;
	}

	if (is_util_module_event(eh)) {
		struct util_module_event *evt = cast_util_module_event(eh);

		msg.module.util = *evt;
		enqueue_msg = true;
	}

	if (is_gps_module_event(eh)) {
		struct gps_module_event *evt = cast_gps_module_event(eh);

		msg.module.gps = *evt;
		enqueue_msg = true;
	}

	if (enqueue_msg) {
		int err = module_enqueue_msg(&self, &msg);

		if (err) {
			LOG_ERR("Message could not be enqueued");
			SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
		}
	}

	return false;
}

static void cloud_wrap_event_handler(const struct cloud_wrap_event *const evt)
{
	switch (evt->type) {
	case CLOUD_WRAP_EVT_CONNECTING: {
		LOG_DBG("CLOUD_WRAP_EVT_CONNECTING");
		SEND_EVENT(cloud, CLOUD_EVT_CONNECTING);
		break;
	}
	case CLOUD_WRAP_EVT_CONNECTED: {
		LOG_DBG("CLOUD_WRAP_EVT_CONNECTED");
		SEND_EVENT(cloud, CLOUD_EVT_CONNECTED);
		break;
	}
	case CLOUD_WRAP_EVT_DISCONNECTED: {
		LOG_DBG("CLOUD_WRAP_EVT_DISCONNECTED");
		SEND_EVENT(cloud, CLOUD_EVT_DISCONNECTED);
		break;
	}
	case CLOUD_WRAP_EVT_DATA_RECEIVED:
		LOG_DBG("CLOUD_WRAP_EVT_DATA_RECEIVED");

		int err;

		/* Use the config copy when populating the config variable
		 * before it is sent to the Data module. This way we avoid
		 * sending uninitialized variables to the Data module.
		 */

		err = cloud_codec_decode_config(evt->data.buf, &copy_cfg);
		if (err == 0) {
			LOG_DBG("Device configuration encoded");
			send_config_received();
			break;
		} else if (err == -ENODATA) {
			LOG_WRN("Device configuration empty!");
			SEND_EVENT(cloud, CLOUD_EVT_CONFIG_EMPTY);
		} else {
			LOG_ERR("Decoding of device configuration, error: %d",
				err);
			SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
			break;
		}

#if defined(CONFIG_AGPS)
		err = gps_process_agps_data(evt->data.buf, evt->data.len);
		if (err) {
			LOG_WRN("Unable to process agps data, error: %d", err);
		}
#endif
		break;
	case CLOUD_WRAP_EVT_FOTA_DONE: {
		LOG_DBG("CLOUD_WRAP_EVT_FOTA_DONE");
		SEND_EVENT(cloud, CLOUD_EVT_FOTA_DONE);
		break;
	}
	case CLOUD_WRAP_EVT_FOTA_START: {
		LOG_DBG("CLOUD_WRAP_EVT_FOTA_START");
		break;
	}
	case CLOUD_WRAP_EVT_FOTA_ERASE_PENDING:
		LOG_DBG("CLOUD_WRAP_EVT_FOTA_ERASE_PENDING");
		break;
	case CLOUD_WRAP_EVT_FOTA_ERASE_DONE:
		LOG_DBG("CLOUD_WRAP_EVT_FOTA_ERASE_DONE");
		break;
	case CLOUD_WRAP_EVT_FOTA_ERROR:
		LOG_DBG("CLOUD_WRAP_EVT_FOTA_ERROR");
		break;
	case CLOUD_WRAP_EVT_ERROR: {
		LOG_DBG("CLOUD_WRAP_EVT_ERROR");
		SEND_ERROR(cloud, CLOUD_EVT_ERROR, evt->err);
		break;
	}
	default:
		break;

	}
}

/* Static module functions. */
static void send_data_ack(void *ptr, size_t len, bool sent)
{
	struct cloud_module_event *cloud_module_event =
			new_cloud_module_event();

	if (len < 0) {
		LOG_WRN("Data to be ACKen has zero length");
		return;
	}

	cloud_module_event->type = CLOUD_EVT_DATA_ACK;
	cloud_module_event->data.ack.ptr = ptr;
	cloud_module_event->data.ack.sent = sent;
	cloud_module_event->data.ack.len = len;

	EVENT_SUBMIT(cloud_module_event);
}

static void send_config_received(void)
{
	struct cloud_module_event *cloud_module_event =
			new_cloud_module_event();

	cloud_module_event->type = CLOUD_EVT_CONFIG_RECEIVED;
	cloud_module_event->data.config = copy_cfg;

	EVENT_SUBMIT(cloud_module_event);
}

static void data_send(struct data_module_event *evt)
{
	int err;

	err = cloud_wrap_data_send(evt->data.buffer.buf, evt->data.buffer.len);
	if (err) {
		LOG_ERR("cloud_wrap_data_send, err: %d", err);
		send_data_ack(evt->data.buffer.buf,
			      evt->data.buffer.len,
			      false);
		return;
	}

	LOG_DBG("Data sent, data pointer: %p", evt->data.buffer.buf);

	send_data_ack(evt->data.buffer.buf, evt->data.buffer.len, true);
}

static void config_send(struct data_module_event *evt)
{
	int err;

	err = cloud_wrap_state_send(evt->data.buffer.buf, evt->data.buffer.len);
	if (err) {
		LOG_ERR("cloud_wrap_state_send, err: %d", err);
		send_data_ack(evt->data.buffer.buf,
			      evt->data.buffer.len,
			      false);
		return;
	}

	LOG_DBG("Configuration sent, data pointer: %p", evt->data.buffer.buf);

	send_data_ack(evt->data.buffer.buf, evt->data.buffer.len, true);
}

static void config_get(void)
{
	int err;

	err = cloud_wrap_state_get();
	if (err) {
		LOG_ERR("cloud_wrap_state_get, err: %d", err);
	} else {
		LOG_DBG("Device configuration requested");
	}
}

static void batch_data_send(struct data_module_event *evt)
{
	int err;

	err = cloud_wrap_batch_send(evt->data.buffer.buf, evt->data.buffer.len);
	if (err) {
		LOG_ERR("cloud_wrap_batch_send, err: %d", err);
		send_data_ack(evt->data.buffer.buf,
			      evt->data.buffer.len,
			      false);
		return;
	}

	LOG_DBG("Batch sent, data pointer: %p", evt->data.buffer.buf);

	send_data_ack(evt->data.buffer.buf, evt->data.buffer.len, true);
}

static void ui_data_send(struct data_module_event *evt)
{
	int err;

	err = cloud_wrap_ui_send(evt->data.buffer.buf, evt->data.buffer.len);
	if (err) {
		LOG_ERR("cloud_wrap_ui_send, err: %d", err);
		send_data_ack(evt->data.buffer.buf,
			      evt->data.buffer.len,
			      false);
		return;
	}

	LOG_DBG("UI sent, data pointer: %p", evt->data.buffer.buf);

	send_data_ack(evt->data.buffer.buf, evt->data.buffer.len, true);
}

static void connect_cloud(void)
{
	int err;
	int backoff_sec = backoff_delay[connect_retries].delay;

	LOG_DBG("Connecting to cloud");

	if (connect_retries > CONFIG_CLOUD_CONNECT_RETRIES) {
		LOG_WRN("Too many failed cloud connection attempts");
		SEND_ERROR(cloud, CLOUD_EVT_ERROR, -ENETUNREACH);
		return;
	}

	/* The cloud will return error if cloud_wrap_connect() is called while
	 * the socket is polled on in the internal cloud thread or the
	 * cloud backend is the wrong state. We cannot treat this as an error as
	 * it is rather common that cloud_connect can be called under these
	 * conditions.
	 */
	err = cloud_wrap_connect();
	if (err) {
		LOG_ERR("cloud_connect failed, error: %d", err);
	}

	connect_retries++;

	LOG_WRN("Cloud connection establishment in progress");
	LOG_WRN("New connection attempt in %d seconds if not successful",
		backoff_sec);

	/* Start timer to check connection status after backoff */
	k_delayed_work_submit(&connect_check_work, K_SECONDS(backoff_sec));
}

/* If this work is executed, it means that the connection attempt was not
 * successful before the backoff timer expired. A timeout message is then
 * added to the message queue to signal the timeout.
 */
static void connect_check_work_fn(struct k_work *work)
{
	LOG_DBG("Cloud connection timeout occurred");

	SEND_EVENT(cloud, CLOUD_EVT_CONNECTION_TIMEOUT);
}

static int setup(void)
{
	int err;

	err = cloud_wrap_init(cloud_wrap_event_handler);
	if (err) {
		LOG_ERR("cloud_wrap_init, error: %d", err);
		return err;
	}

	/* After a successful initializaton, tell the bootloader that the
	 * current image is confirmed to be working.
	 */
	boot_write_img_confirmed();

	return 0;
}

/* Message handler for STATE_LTE_CONNECTED. */
static void on_state_lte_connected(struct cloud_msg_data *msg)
{
	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_DISCONNECTED)) {
		state_set(STATE_LTE_DISCONNECTED);
		sub_state_set(SUB_STATE_CLOUD_DISCONNECTED);

		connect_retries = 0;

		k_delayed_work_cancel(&connect_check_work);

		return;
	}

#if defined(CONFIG_AGPS) && defined(CONFIG_AGPS_SRC_SUPL)
	if (IS_EVENT(msg, gps, GPS_EVT_AGPS_NEEDED)) {
		int err;

		err = gps_agps_request(msg->module.gps.data.agps_request,
				       GPS_SOCKET_NOT_PROVIDED);
		if (err) {
			LOG_WRN("Failed to request A-GPS data, error: %d", err);
		}
	}
#endif /* CONFIG_AGPS && CONFIG_AGPS_SRC_SUPL */
}

/* Message handler for STATE_LTE_DISCONNECTED. */
static void on_state_lte_disconnected(struct cloud_msg_data *msg)
{
	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_CONNECTED)) {
		state_set(STATE_LTE_CONNECTED);

		/* LTE is now connected, cloud connection can be attempted */
		connect_cloud();
	}
}

/* Message handler for SUB_STATE_CLOUD_CONNECTED. */
static void on_sub_state_cloud_connected(struct cloud_msg_data *msg)
{
	if (IS_EVENT(msg, cloud, CLOUD_EVT_DISCONNECTED)) {
		sub_state_set(SUB_STATE_CLOUD_DISCONNECTED);

		k_delayed_work_submit(&connect_check_work, K_NO_WAIT);

		return;
	}

#if defined(CONFIG_AGPS) && defined(CONFIG_AGPS_SRC_NRF_CLOUD)
	if (IS_EVENT(msg, gps, GPS_EVT_AGPS_NEEDED)) {
		int err;

		err = gps_agps_request(msg->module.gps.data.agps_request,
				       GPS_SOCKET_NOT_PROVIDED);
		if (err) {
			LOG_WRN("Failed to request A-GPS data, error: %d", err);
		}

		return;
	}
#endif /* CONFIG_AGPS && CONFIG_AGPS_SRC_NRF_CLOUD */

	if (IS_EVENT(msg, data, DATA_EVT_DATA_SEND)) {
		data_send(&msg->module.data);
	}

	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_SEND)) {
		config_send(&msg->module.data);
	}

	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_GET)) {
		config_get();
	}

	if (IS_EVENT(msg, data, DATA_EVT_DATA_SEND_BATCH)) {
		batch_data_send(&msg->module.data);
	}

	if (IS_EVENT(msg, data, DATA_EVT_UI_DATA_SEND)) {
		ui_data_send(&msg->module.data);
	}
}

/* Message handler for SUB_STATE_CLOUD_DISCONNECTED. */
static void on_sub_state_cloud_disconnected(struct cloud_msg_data *msg)
{
	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONNECTED)) {
		sub_state_set(SUB_STATE_CLOUD_CONNECTED);

		connect_retries = 0;
		k_delayed_work_cancel(&connect_check_work);
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONNECTION_TIMEOUT)) {
		connect_cloud();
	}
}

/* Message handler for all states. */
static void on_all_states(struct cloud_msg_data *msg)
{
	if (IS_EVENT(msg, util, UTIL_EVT_SHUTDOWN_REQUEST)) {
		/* The module doesn't have anything to shut down and can
		 * report back immediately.
		 */
		SEND_EVENT(cloud, CLOUD_EVT_SHUTDOWN_READY);
		state_set(STATE_SHUTDOWN);
	}

	if (is_data_module_event(&msg->module.data.header)) {
		switch (msg->module.data.type) {
		case DATA_EVT_CONFIG_INIT:
			/* Fall through. */
		case DATA_EVT_CONFIG_READY:
			copy_cfg = msg->module.data.data.cfg;
			break;
		default:
			break;
		}
	}
}

static void module_thread_fn(void)
{
	int err;
	struct cloud_msg_data msg;

	self.thread_id = k_current_get();

	module_start(&self);

	state_set(STATE_LTE_DISCONNECTED);
	sub_state_set(SUB_STATE_CLOUD_DISCONNECTED);

	k_delayed_work_init(&connect_check_work, connect_check_work_fn);

	err = setup();
	if (err) {
		LOG_ERR("setup, error %d", err);
		SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
	}

	while (true) {
		module_get_next_msg(&self, &msg);

		switch (state) {
		case STATE_LTE_CONNECTED:
			switch (sub_state) {
			case SUB_STATE_CLOUD_CONNECTED:
				on_sub_state_cloud_connected(&msg);
				break;
			case SUB_STATE_CLOUD_DISCONNECTED:
				on_sub_state_cloud_disconnected(&msg);
				break;
			default:
				LOG_ERR("Unknown Cloud module sub state");
				break;
			}

			on_state_lte_connected(&msg);
			break;
		case STATE_LTE_DISCONNECTED:
			on_state_lte_disconnected(&msg);
			break;
		case STATE_SHUTDOWN:
			/* The shutdown state has no transition. */
			break;
		default:
			LOG_ERR("Unknown Cloud module state.");
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(cloud_module_thread, CONFIG_CLOUD_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, data_module_event);
EVENT_SUBSCRIBE(MODULE, app_module_event);
EVENT_SUBSCRIBE(MODULE, modem_module_event);
EVENT_SUBSCRIBE(MODULE, cloud_module_event);
EVENT_SUBSCRIBE(MODULE, gps_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, util_module_event);
