/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <net/aws_iot.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/logging/log.h>

#include "aws_iot_integration.h"
#include "codec.h"

LOG_MODULE_REGISTER(aws_iot_integration, CONFIG_AWS_IOT_INTEGRATION_LOG_LEVEL);

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

/* String used to clear the desired section of the AWS IoT shadow document */
#define SHADOW_DESIRED_CLEAR_STRING "{\"state\":{\"desired\":null}}"

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;

/* Forward declarations */
static void aws_iot_event_handler(const struct aws_iot_evt *const evt);
static void connect_work_fn(struct k_work *work);

/* Delayable work used to schedule connection attempts to AWS IoT Core */
static K_WORK_DELAYABLE_DEFINE(connect_work, connect_work_fn);

/* Local handler used to pass events to the caller. */
static aws_iot_integration_evt_handler_t handler;

/* Structure that keeps track of current state of attributes. */
static struct payload payload;

static bool is_connected_to_aws_iot;
static bool is_connected_to_network;

/* Declare workqueue. This workqueue is used to offload calls to the AWS IoT library.
 * These calls can block, especially aws_iot_connect() and aws_iot_send().
 */
static struct k_work_q queue;
K_THREAD_STACK_DEFINE(stack_area, CONFIG_AWS_IOT_INTEGRATION_WORKQUEUE_STACK_SIZE);

/* Static functions */

static void notify_error(int err)
{
	struct aws_iot_integration_cb_data data = {
		.error = err,
	};

	handler(&data);
}

static void connect(void)
{
	int err;
	static bool init = false;

	if (!is_connected_to_network) {
		LOG_DBG("Not connected to network.... aborting connection attempt to AWS IoT");
		return;
	}

	if (!init) {
		err = aws_iot_init(aws_iot_event_handler);
		if (err) {
			LOG_ERR("aws_iot_init, error: %d", err);
			notify_error(err);
			return;
		}

		init = true;
	}

	err = aws_iot_connect(NULL);
	if (err == -EAGAIN) {
		LOG_INF("Connection attempt timed out, "
			"Next connection retry in %d seconds",
			CONFIG_AWS_IOT_INTEGRATION_RECONNECT_INTERVAL_SECONDS);

		(void)k_work_reschedule_for_queue(
				&queue, &connect_work,
				K_SECONDS(CONFIG_AWS_IOT_INTEGRATION_RECONNECT_INTERVAL_SECONDS));
	} else if (err) {
		LOG_ERR("aws_iot_connect, error: %d", err);
		notify_error(err);
		return;
	}
}

static int disconnect(void)
{
	int err = aws_iot_disconnect();

	if (err) {
		LOG_ERR("aws_iot_disconnect, error: %d", err);
		return err;
	}

	return 0;
}

static int shadow_update_desired_state_clear(void)
{
	int err;
	char message[CONFIG_AWS_IOT_INTEGRATION_MESSAGE_SIZE_MAX] = { 0 };
	struct aws_iot_data tx_data = {
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		/* Address to $aws/things/<thing-name>/shadow/update */
		.topic.type = AWS_IOT_SHADOW_TOPIC_UPDATE,
		.ptr = SHADOW_DESIRED_CLEAR_STRING,
		.len = strlen(SHADOW_DESIRED_CLEAR_STRING)
	};

	err = codec_json_encode_update_message(message, sizeof(message), &payload);
	if (err) {
		LOG_ERR("codec_json_encode, error: %d", err);
		return err;
	}

	LOG_DBG("Clearing the desired shadow state");
	LOG_DBG("Publishing message: %s to $aws/things/%s/shadow/update",
		tx_data.ptr, CONFIG_AWS_IOT_CLIENT_ID_STATIC);

	err = aws_iot_send(&tx_data);
	if (err) {
		LOG_ERR("aws_iot_send, error: %d", err);
		return err;
	}

	return 0;
}

static int shadow_update(void)
{
	int err;
	char message[CONFIG_AWS_IOT_INTEGRATION_MESSAGE_SIZE_MAX] = { 0 };
	struct aws_iot_data tx_data = {
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		/* Address to $aws/things/<thing-name>/shadow/update */
		.topic.type = AWS_IOT_SHADOW_TOPIC_UPDATE,
	};

	err = codec_json_encode_update_message(message, sizeof(message), &payload);
	if (err) {
		LOG_ERR("codec_json_encode, error: %d", err);
		return err;
	}

	tx_data.ptr = message,
	tx_data.len = strlen(message);

	LOG_DBG("Publishing message: %s to $aws/things/%s/shadow/update",
		message, CONFIG_AWS_IOT_CLIENT_ID_STATIC);

	err = aws_iot_send(&tx_data);
	if (err) {
		LOG_ERR("aws_iot_send, error: %d", err);
		return err;
	}

	return 0;
}

static int shadow_clear_delta_and_update_reported(void)
{
	int err;

	if (!is_connected_to_aws_iot) {
		return -ENOTCONN;
	}

	err = shadow_update_desired_state_clear();
	if (err) {
		LOG_ERR("shadow_update_desired_state_clear, error: %d", err);
		return err;
	}

	err = shadow_update();
	if (err) {
		LOG_ERR("shadow_update, error: %d", err);
		return err;
	}

	return 0;
}

static int decode_and_notify_attribute_change(char *message, size_t len)
{
	int err;

	err = codec_json_decode_delta_message(message, len, &payload);
	if (err) {
		LOG_ERR("codec_json_decode_delta_message, error: %d", err);
		return err;
	}

	LOG_ERR("New onoff state received: %d", payload.state.reported.node.onoff.onoff);
	LOG_ERR("New level control state received: %d", payload.state.reported.node.onoff.level_control);

	/* Notify handler of the attribute change. */
	struct aws_iot_integration_cb_data data = {
		.attribute_id = ATTRIBUTE_ID_ONOFF,
		.value = payload.state.reported.node.onoff.onoff
	};

	if (!handler(&data)) {
		LOG_ERR("Handler returned false, setting attribute failed");
		return -EFAULT;
	}

	data.attribute_id = ATTRIBUTE_ID_LEVEL_CONTROL;
	data.value = payload.state.reported.node.onoff.level_control;

	if (!handler(&data)) {
		LOG_ERR("Handler returned false, setting attribute failed");
		return -EFAULT;
	}

	/* Attribute successfully set, update the device shadow with the applied value. */
	err = shadow_update();
	if (err) {
		LOG_ERR("shadow_update, error: %d", err);
		return err;
	}

	return 0;
}

static void on_data_received(const struct aws_iot_evt *const evt)
{
	LOG_INF("Received message: \"%.*s\" on topic: \"%.*s\"", evt->data.msg.len,
								 evt->data.msg.ptr,
								 evt->data.msg.topic.len,
								 evt->data.msg.topic.str);

	switch (evt->data.msg.topic.type_received) {
		case AWS_IOT_SHADOW_TOPIC_UPDATE_DELTA:
			LOG_DBG("Message on update/delta topic received");

			int err = decode_and_notify_attribute_change(evt->data.msg.ptr,
								     evt->data.msg.len);

			if (err) {
				LOG_ERR("decode_and_notify_attribute_change, error: %d", err);
				notify_error(err);
				return;
			}

			break;
		default:
			/* Don't care */
			break;
	}
}

static void aws_iot_event_handler(const struct aws_iot_evt *const evt)
{
	int err;

	switch (evt->type) {
	case AWS_IOT_EVT_CONNECTING:
		LOG_INF("AWS_IOT_EVT_CONNECTING");
		break;
	case AWS_IOT_EVT_CONNECTED:
		LOG_INF("AWS_IOT_EVT_CONNECTED");
		k_work_cancel_delayable(&connect_work);

		is_connected_to_aws_iot = true;

		/* We want to make local changes to the clusters have precedence.
		 * Therefore we need to delete the desired section once whe connect to avoid
		 * desired attributes set in the shadow to take effect.
		 */
		err = shadow_clear_delta_and_update_reported();
		if (err) {
			LOG_ERR("shadow_clear_delta_and_update_reported, error: %d", err);
			notify_error(err);
			return;
		}

		break;
	case AWS_IOT_EVT_DISCONNECTED:
		LOG_INF("AWS_IOT_EVT_DISCONNECTED");
		k_work_reschedule_for_queue(&queue, &connect_work, K_SECONDS(5));
		is_connected_to_aws_iot = false;
		break;
	case AWS_IOT_EVT_DATA_RECEIVED:
		LOG_INF("AWS_IOT_EVT_DATA_RECEIVED");
		on_data_received(evt);
		break;
	default:
		/* Don't care */
		break;
	}
}

static void l4_handler(struct net_mgmt_event_callback *cb,
			 uint32_t mgmt_event,
			 struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_ERR("Network connected");
		is_connected_to_network = true;
		k_work_reschedule_for_queue(&queue, &connect_work, K_SECONDS(5));
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_ERR("Network disconnected");
		is_connected_to_network = false;
		(void)disconnect();
		break;
	default:
		/* Don't care */
		break;
	}
}

static void connect_work_fn(struct k_work *work)
{
	connect();
}

static int net_mgmt_subscribe(void)
{
	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_cb, l4_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	k_work_queue_init(&queue);
	k_work_queue_start(&queue, stack_area,
			   K_THREAD_STACK_SIZEOF(stack_area),
			   K_LOWEST_APPLICATION_THREAD_PRIO,
			   NULL);

	return 0;
}

/* Public functions */

int aws_iot_integration_register_callback(aws_iot_integration_evt_handler_t callback)
{
	if (callback == NULL) {
		return -EINVAL;
	}

	handler = callback;
	return 0;
}

void aws_iot_integration_attribute_set(uint32_t id, uint32_t value)
{
	int err;
	bool attribute_change = false;

	if ((id == ATTRIBUTE_ID_ONOFF) &&
	    (payload.state.reported.node.onoff.onoff != value)) {

		payload.state.reported.node.onoff.onoff = value;
		attribute_change = true;
	}

	if ((id == ATTRIBUTE_ID_LEVEL_CONTROL) &&
	    (payload.state.reported.node.onoff.level_control != value)) {

		payload.state.reported.node.onoff.level_control = value;
		attribute_change = true;
	}

	if (!attribute_change) {
		return;
	}

	LOG_DBG("Attribute changed, updating shadow");

	/* Try updating the changed attribute's value in the shadow,
	 * if not connected this will fail, but the library will sync with the shadow as
	 * soon as the connection is established.
	*/
	err = shadow_clear_delta_and_update_reported();
	if (err && err != -ENOTCONN) {
		LOG_ERR("shadow_clear_delta_and_update_reported, error: %d", err);
		notify_error(err);
		return;
	}

	return;
}

SYS_INIT(net_mgmt_subscribe, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
