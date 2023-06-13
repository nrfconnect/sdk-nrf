/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/settings/settings.h>

#include <net/azure_iot_hub.h>
#include <net/azure_iot_hub_dps.h>
#include <net/mqtt_helper.h>

#include <azure/az_core.h>
#include <azure/az_iot.h>

#include "azure_iot_hub_dps_private.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(azure_iot_hub_dps, CONFIG_AZURE_IOT_HUB_LOG_LEVEL);

/* Define a custom AZ_DPS_STATIC macro that exposes internal variables when unit testing. */
#if defined(CONFIG_UNITY)
#define AZ_DPS_STATIC
#else
#define AZ_DPS_STATIC static
#endif

AZ_DPS_STATIC enum dps_state dps_state = DPS_STATE_UNINIT;
AZ_DPS_STATIC struct dps_reg_ctx dps_reg_ctx;
static char operation_id_buf[CONFIG_AZURE_IOT_HUB_DPS_OPERATION_ID_BUFFER_SIZE];
static char assigned_hub_buf[CONFIG_AZURE_IOT_HUB_DPS_HOSTNAME_MAX_LEN];
static char assigned_device_id_buf[CONFIG_AZURE_IOT_HUB_DPS_DEVICE_ID_MAX_LEN];

/* Forward declarations */
static void timeout_work_stop(void);
static int dps_on_settings_loaded(void);
static int dps_settings_handler(const char *key, size_t len,
				settings_read_cb read_cb, void *cb_arg);

static const char *state_name_get(enum dps_state state)
{
	switch (state) {
	case DPS_STATE_UNINIT: return "DPS_STATE_UNINIT";
	case DPS_STATE_DISCONNECTED: return "DPS_STATE_DISCONNECTED";
	case DPS_STATE_CONNECTING: return "DPS_STATE_CONNECTING";
	case DPS_STATE_TRANSPORT_CONNECTED: return "DPS_STATE_TRANSPORT_CONNECTED";
	case DPS_STATE_CONNECTED: return "DPS_STATE_CONNECTED";
	case DPS_STATE_DISCONNECTING: return "DPS_STATE_DISCONNECTING";
	default: return "DPS_STATE_UNKNOWN";
	}
}

static bool dps_state_verify(enum dps_state state)
{
	return (dps_state == state);
}

static void dps_state_set(enum dps_state new_state)
{
	bool notify_error = false;

	if (dps_state == new_state) {
		return;
	}

	/* Check for legal state transitions. */
	switch (dps_state) {
	case DPS_STATE_UNINIT:
		if (new_state != DPS_STATE_DISCONNECTED) {
			notify_error = true;
		}
		break;
	case DPS_STATE_DISCONNECTED:
		if (new_state != DPS_STATE_CONNECTING) {
			notify_error = true;
		}
		break;
	case DPS_STATE_CONNECTING:
		if (new_state != DPS_STATE_CONNECTED &&
		    new_state != DPS_STATE_DISCONNECTING &&
		    new_state != DPS_STATE_DISCONNECTED) {
			notify_error = true;
		}
		break;
	case DPS_STATE_TRANSPORT_CONNECTED:
		if (new_state != DPS_STATE_CONNECTING) {
			notify_error = true;
		}
		break;
	case DPS_STATE_CONNECTED:
		if (new_state != DPS_STATE_DISCONNECTING &&
		    new_state != DPS_STATE_DISCONNECTED) {
			notify_error = true;
		}
		break;
	case DPS_STATE_DISCONNECTING:
		if ((new_state != DPS_STATE_DISCONNECTED) &&
		    (new_state != DPS_STATE_TRANSPORT_CONNECTED)) {
			notify_error = true;
		}
		break;
	default:
		LOG_ERR("New DPS state is unknown");
		notify_error = true;
		break;
	}

	if (notify_error) {
		LOG_ERR("Invalid connection state transition, %s --> %s",
			state_name_get(dps_state),
			state_name_get(new_state));
		return;
	}

	LOG_DBG("State transition: %s --> %s",
		state_name_get(dps_state),
		state_name_get(new_state));

	dps_state = new_state;
}

SETTINGS_STATIC_HANDLER_DEFINE(azure_iot_hub_dps, DPS_SETTINGS_KEY, NULL,
			       dps_settings_handler, dps_on_settings_loaded,
			       NULL);

/**@brief Callback when settings_load() is called. */
static int dps_settings_handler(const char *key, size_t len,
				settings_read_cb read_cb, void *cb_arg)
{
	int err;
	static uint32_t hostname_len;
	static uint32_t device_id_len;

	if (strcmp(key, DPS_SETTINGS_HOSTNAME_LEN_KEY) == 0) {
		err = read_cb(cb_arg, &hostname_len, sizeof(hostname_len));
		if (err < 0) {
			LOG_ERR("Failed to read hostname length, error: %d", err);
			return err;
		}

		LOG_DBG("Azure IoT Hub hostname length found: %d", hostname_len);

		return 0;
	}

	if (strcmp(key, DPS_SETTINGS_HOSTNAME_KEY) == 0) {
		err = read_cb(cb_arg, assigned_hub_buf,
			      MIN(sizeof(assigned_hub_buf), hostname_len));
		if (err < 0) {
			LOG_ERR("Failed to read hostname, error: %d", err);
			return err;
		}

		dps_reg_ctx.status = AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNED;
		dps_reg_ctx.assigned_hub = az_span_create(assigned_hub_buf,
						MIN(sizeof(assigned_hub_buf), hostname_len));

		LOG_DBG("Azure IoT Hub hostname found: %.*s",
			az_span_size(dps_reg_ctx.assigned_hub),
			(char *)az_span_ptr(dps_reg_ctx.assigned_hub));
	}

	if (strcmp(key, DPS_SETTINGS_DEVICE_ID_LEN_KEY) == 0) {
		err = read_cb(cb_arg, &device_id_len, sizeof(device_id_len));
		if (err < 0) {
			LOG_ERR("Failed to read device ID length, error: %d", err);
			return err;
		}

		LOG_DBG("Assigned device ID length found: %d", device_id_len);

		return 0;
	}

	if (strcmp(key, DPS_SETTINGS_DEVICE_ID_KEY) == 0) {
		err = read_cb(cb_arg, assigned_device_id_buf,
			      MIN(sizeof(assigned_device_id_buf), device_id_len));
		if (err < 0) {
			LOG_ERR("Failed to read device ID, error: %d", err);
			return err;
		}

		dps_reg_ctx.status = AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNED;
		dps_reg_ctx.assigned_device_id =
			az_span_create(assigned_device_id_buf,
				       MIN(sizeof(assigned_device_id_buf), device_id_len));

		LOG_DBG("Assigned device ID found: %.*s",
			az_span_size(dps_reg_ctx.assigned_device_id),
			(char *)az_span_ptr(dps_reg_ctx.assigned_device_id));
	}

	return 0;
}

AZ_DPS_STATIC int dps_save_hostname(char *hostname_ptr, size_t hostname_len)
{
	int err;
	size_t buf_hostname_len = MIN(sizeof(assigned_hub_buf) - 1, hostname_len);

	memcpy(assigned_hub_buf, hostname_ptr,  buf_hostname_len);
	assigned_hub_buf[buf_hostname_len] = '\0';

	dps_reg_ctx.assigned_hub = az_span_create(assigned_hub_buf, buf_hostname_len);

	err = settings_save_one(DPS_SETTINGS_KEY "/" DPS_SETTINGS_HOSTNAME_LEN_KEY,
				&buf_hostname_len, sizeof(buf_hostname_len));
	if (err) {
		return err;
	}

	err = settings_save_one(DPS_SETTINGS_KEY "/" DPS_SETTINGS_HOSTNAME_KEY,
				assigned_hub_buf, buf_hostname_len);
	if (err) {
		return err;
	}

	LOG_DBG("Assigned IoT Hub updated (size: %d): %.*s",
		az_span_size(dps_reg_ctx.assigned_hub),
		az_span_size(dps_reg_ctx.assigned_hub),
		(char *)az_span_ptr(dps_reg_ctx.assigned_hub));

	return 0;
}

AZ_DPS_STATIC int dps_save_device_id(char *device_id_ptr, size_t device_id_len)
{
	int err;
	size_t buf_device_id_len = MIN(sizeof(assigned_device_id_buf) - 1, device_id_len);

	memcpy(assigned_device_id_buf, device_id_ptr,  buf_device_id_len);
	assigned_device_id_buf[buf_device_id_len] = '\0';

	dps_reg_ctx.assigned_device_id = az_span_create(assigned_device_id_buf, buf_device_id_len);

	err = settings_save_one(DPS_SETTINGS_KEY "/" DPS_SETTINGS_DEVICE_ID_LEN_KEY,
				&buf_device_id_len, sizeof(buf_device_id_len));
	if (err) {
		return err;
	}

	err = settings_save_one(DPS_SETTINGS_KEY "/" DPS_SETTINGS_DEVICE_ID_KEY,
				assigned_device_id_buf, buf_device_id_len);
	if (err) {
		return err;
	}

	LOG_DBG("Assigned device ID updated (size: %d): %.*s",
		az_span_size(dps_reg_ctx.assigned_device_id),
		az_span_size(dps_reg_ctx.assigned_device_id),
		(char *)az_span_ptr(dps_reg_ctx.assigned_device_id));

	return 0;
}

static int dps_on_settings_loaded(void)
{
	LOG_DBG("Settings fully loaded");

	k_sem_give(&dps_reg_ctx.settings_loaded);

	return 0;
}

static int dps_settings_init(void)
{
	int err;

	err = settings_subsys_init();
	if (err) {
		LOG_ERR("Failed to initialize settings subsystem, error: %d",
			err);
		return err;
	}

	/* This module loads settings for all application modules */
	err = settings_load_subtree(DPS_SETTINGS_KEY);
	if (err) {
		LOG_ERR("Cannot load settings");
		return err;
	}

	return 0;
}

static bool dps_is_assigned(az_iot_provisioning_client_register_response *msg)
{
	if (msg->operation_status != AZ_IOT_PROVISIONING_STATUS_ASSIGNED) {
		LOG_ERR("The device was not assigned to hub");
		LOG_DBG("Registration state: %d", msg->operation_status);
		LOG_DBG("Operation status: %d", msg->status);
		LOG_DBG("Extended error code: %d",
			msg->registration_state.extended_error_code);
		LOG_DBG("Error message: %.*s ",
			az_span_size(msg->registration_state.error_message),
			az_span_ptr(msg->registration_state.error_message));
		LOG_DBG("Error timestamp: %.*s",
			az_span_size(msg->registration_state.error_timestamp),
			az_span_ptr(msg->registration_state.error_timestamp));
		LOG_DBG("Error tracking ID: %.*s",
			az_span_size(msg->registration_state.error_tracking_id),
			az_span_ptr(msg->registration_state.error_tracking_id));

		return false;
	}

	return true;
}

/* To be called when registration is completed, both in case of success and failure.
 *
 * @param msg can be NULL if the failure might not be contained within the message.
 */
AZ_DPS_STATIC void on_reg_completed(az_iot_provisioning_client_register_response *msg)
{
	int err;
	char *hostname_ptr;
	size_t hostname_len;
	char *device_id_ptr;
	size_t device_id_len;

	if (!dps_state_verify(DPS_STATE_DISCONNECTED) &&
	    !dps_state_verify(DPS_STATE_DISCONNECTING)) {
		dps_state_set(DPS_STATE_DISCONNECTING);
	}

	timeout_work_stop();

	err = mqtt_helper_disconnect();
	if (err) {
		LOG_ERR("Failed to disconnect gracefully, error: %d", err);
		dps_state_set(DPS_STATE_DISCONNECTED);
		mqtt_helper_deinit();
	}

	if (msg == NULL) {
		LOG_ERR("There was a failure during DPS registration, process is stopped");

		dps_reg_ctx.status = AZURE_IOT_HUB_DPS_REG_STATUS_FAILED;
		dps_reg_ctx.cb(dps_reg_ctx.status);

		return;
	}

	/* Check if an IoT hub has been assigned and copy the hostname if it was. */
	if (!dps_is_assigned(msg)) {
		LOG_ERR("The registration was not successful, no IoT Hub was assigned");

		dps_reg_ctx.status = AZURE_IOT_HUB_DPS_REG_STATUS_FAILED;
		dps_reg_ctx.cb(dps_reg_ctx.status);

		return;
	}

	hostname_ptr = az_span_ptr(msg->registration_state.assigned_hub_hostname);
	hostname_len = az_span_size(msg->registration_state.assigned_hub_hostname);

	err = dps_save_hostname(hostname_ptr, hostname_len);
	if (err) {
		LOG_ERR("Failed to save hostname to flash, error: %d", err);
	}

	device_id_ptr = az_span_ptr(msg->registration_state.device_id);
	device_id_len = az_span_size(msg->registration_state.device_id);

	err = dps_save_device_id(device_id_ptr, device_id_len);
	if (err) {
		LOG_ERR("Failed to save device ID to flash, error: %d", err);
	}

	LOG_DBG("Device ID %.*s is now registered to %.*s",
		az_span_size(dps_reg_ctx.reg_id),
		(char *)az_span_ptr(dps_reg_ctx.reg_id),
		az_span_size(dps_reg_ctx.assigned_hub),
		(char *)az_span_ptr(dps_reg_ctx.assigned_hub));

	dps_reg_ctx.status = AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNED;
	dps_reg_ctx.cb(dps_reg_ctx.status);
}

static void on_reg_failed(void)
{
	on_reg_completed(NULL);
}

static void reg_status_request_send(void)
{
	int err;
	struct mqtt_publish_param param = {
		.message.payload.data = NULL,
		.message.payload.len = 0,
		.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		.message_id = k_uptime_get_32(),
	};
	char topic[CONFIG_AZURE_IOT_HUB_DPS_TOPIC_BUFFER_SIZE];
	size_t topic_len;

	dps_reg_ctx.retry_count++;

	if (dps_reg_ctx.retry_count > AZURE_IOT_HUB_DPS_REG_STATUS_UPDATE_MAX_RETRIES) {
		LOG_ERR("DPS retry max count (%d) exceeded, disconnecting",
			AZURE_IOT_HUB_DPS_REG_STATUS_UPDATE_MAX_RETRIES);
		on_reg_failed();

		return;
	}

	err = az_iot_provisioning_client_query_status_get_publish_topic(
		&dps_reg_ctx.dps_client,
		dps_reg_ctx.operation_id,
		topic,
		sizeof(topic),
		&topic_len);
	if (az_result_failed(err)) {
		LOG_ERR("Failed to get status query topic, az error: %d", err);
		on_reg_failed();
		return;
	}

	LOG_DBG("Status query topic: %.*s", topic_len, topic);

	param.message.topic.topic.utf8 = topic;
	param.message.topic.topic.size = topic_len;

	LOG_DBG("Requesting DPS status update");

	err = mqtt_helper_publish(&param);
	if (err) {
		LOG_ERR("Failed to send status request, err: %d", err);
		on_reg_failed();
		return;
	}
}

/* When this delayable work function is called, it means that a timeout has happened, and the
 * registration process has for some reason stalled for CONFIG_AZURE_IOT_HUB_DPS_TIMEOUT_SEC.
 */
static void timeout_work_fn(struct k_work *work)
{
	LOG_ERR("A timeout occurred, the registration process is terminated");
	on_reg_failed();
}

static void timeout_work_restart_timer(void)
{
	LOG_DBG("Restarting timeout: %d sec", CONFIG_AZURE_IOT_HUB_DPS_TIMEOUT_SEC);

	k_work_reschedule(&dps_reg_ctx.timeout_work,
			  K_SECONDS(CONFIG_AZURE_IOT_HUB_DPS_TIMEOUT_SEC));
}

static void timeout_work_stop(void)
{
	LOG_DBG("Stopping timeout timer");

	k_work_cancel_delayable(&dps_reg_ctx.timeout_work);
}

static void handle_reg_update(az_span topic_span, az_span payload_span)
{
	int err;
	az_iot_provisioning_client_register_response msg = {0};

	err = az_iot_provisioning_client_parse_received_topic_and_payload(
		&dps_reg_ctx.dps_client,
		topic_span,
		payload_span,
		&msg);
	if (az_result_failed(err)) {
		LOG_WRN("The incoming message was not a registration response message");
		LOG_HEXDUMP_DBG(az_span_ptr(topic_span), az_span_size(topic_span), "Topic: ");
		LOG_HEXDUMP_DBG(az_span_ptr(payload_span), az_span_size(payload_span), "Payload: ");

		/* This should not happen while DPS is the only user of the MQTT helper, so consider
		 * it an unexpected event, report failure and stop DPS process.
		 */

		on_reg_failed();
		return;
	}

	LOG_DBG("DPS registration request response received");
	LOG_DBG("Status code is %d", msg.status);

	/* Check if the request was successful, and if not whether we should continue to poll the
	 * status for it.
	 */
	if (!az_iot_status_succeeded(msg.status)) {
		if (!az_iot_status_retriable(msg.status)) {
			LOG_WRN("The status code %d indicates a non-retriable error", msg.status);

			dps_reg_ctx.status = AZURE_IOT_HUB_DPS_REG_STATUS_FAILED;

			on_reg_completed(&msg);
			return;
		}

		/* The status should be polled again */
	}

	if (az_iot_provisioning_client_operation_complete(msg.operation_status)) {
		on_reg_completed(&msg);
		return;
	}

	/* The registration process is still pending, and a new request should
	 * be sent after waiting the indicated retry time.
	 */

	dps_reg_ctx.status = AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNING;

	if (msg.retry_after_seconds == 0) {
		LOG_DBG("No retry time found, using default value of %d", RETRY_AFTER_SEC_DEFAULT);
		msg.retry_after_seconds = RETRY_AFTER_SEC_DEFAULT;

	} else {
		LOG_DBG("Received retry value: %d", msg.retry_after_seconds);
	}

	if (az_span_size(msg.operation_id) > sizeof(operation_id_buf)) {
		LOG_ERR("Operation ID is too large for the buffer, aborting DPS registration");
		/* This is a critical error and status can't be requested without the ID */

		on_reg_completed(&msg);
		return;
	}

	(void)az_span_copy(dps_reg_ctx.operation_id, msg.operation_id);
	dps_reg_ctx.operation_id = az_span_slice(dps_reg_ctx.operation_id, 0,
						 az_span_size(msg.operation_id));

	LOG_DBG("Operation ID: %.*s",
		az_span_size(dps_reg_ctx.operation_id),
		(char *)az_span_ptr(dps_reg_ctx.operation_id));

	dps_reg_ctx.cb(dps_reg_ctx.status);

	/* Waiting for retry. It is okay to wait here, as this happens in the MQTT helper thread
	 * context, and there is no other activity pending there.
	 */
	k_sleep(K_SECONDS(msg.retry_after_seconds));

	reg_status_request_send();
}

static int dps_id_scope_set(struct azure_iot_hub_buf id_scope)
{
	char *id = id_scope.ptr;
	size_t id_len = id_scope.size;

	if ((id == NULL) || (id_len == 0)) {
		LOG_DBG("No ID scope provided, using ID scope from Kconfig: %s",
			CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE);

		id = CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE;
		id_len = MAX(sizeof(CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE) - 1, 0);
	}

	dps_reg_ctx.id_scope = az_span_create(id, id_len);

	LOG_DBG("Setting DPS ID scope: %.*s",
		az_span_size(dps_reg_ctx.id_scope),
		(char *)az_span_ptr(dps_reg_ctx.id_scope));

	return 0;
}

static int dps_reg_id_set(struct azure_iot_hub_buf reg_id)
{
	char *id = reg_id.ptr;
	size_t id_len = reg_id.size;

	if ((id == NULL) || (id_len == 0)) {
		LOG_DBG("No registration ID provided, using ID from Kconfig: %s",
			CONFIG_AZURE_IOT_HUB_DPS_REG_ID);

		id = CONFIG_AZURE_IOT_HUB_DPS_REG_ID;
		id_len = MAX(sizeof(CONFIG_AZURE_IOT_HUB_DPS_REG_ID) - 1, 0);
	}

	dps_reg_ctx.reg_id = az_span_create(id, id_len);

	LOG_DBG("Setting DPS registration ID: %.*s",
		az_span_size(dps_reg_ctx.reg_id),
		(char *)az_span_ptr(dps_reg_ctx.reg_id));

	return 0;
}

static int topic_subscribe(void)
{
	int err;
	struct mqtt_topic dps_sub_topic = {
		.topic = {
			.utf8 = AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC,
			.size = sizeof(AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC) - 1,
		},
	};
	struct mqtt_subscription_list sub_list = {
		.list = &dps_sub_topic,
		.list_count = 1,
		.message_id = k_uptime_get_32(),
	};

	for (size_t i = 0; i < sub_list.list_count; i++) {
		LOG_DBG("Subscribing to: %.*s", sub_list.list[i].topic.size,
						(char *)sub_list.list[i].topic.utf8);
	}

	err = mqtt_helper_subscribe(&sub_list);
	if (err) {
		LOG_ERR("Failed to subscribe to topic list, error: %d", err);
		return err;
	}

	LOG_DBG("Successfully subscribed to DPS topics");

	return 0;
}

static int dps_send_reg_request(void)
{
	struct mqtt_publish_param param = {
		.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		.message_id = k_uptime_get_32(),
	};
	char topic_buf[CONFIG_AZURE_IOT_HUB_DPS_TOPIC_BUFFER_SIZE];
	size_t topic_len = sizeof(topic_buf);
	int err;

	err = az_iot_provisioning_client_register_get_publish_topic(
		&dps_reg_ctx.dps_client,
		topic_buf,
		sizeof(topic_buf),
		&topic_len);
	if (az_result_failed(err)) {
		LOG_ERR("Failed to get registration topic, error: %d", err);
		return -EMSGSIZE;
	}

	LOG_DBG("Topic buffer size is %d, actual topic size is: %d", sizeof(topic_buf), topic_len);

	param.message.topic.topic.utf8 = topic_buf;
	param.message.topic.topic.size = topic_len;

	LOG_DBG("Publishing to DPS registration topic: %.*s, no payload",
		param.message.topic.topic.size,
		(char *)param.message.topic.topic.utf8);

	return mqtt_helper_publish(&param);
}

AZ_DPS_STATIC int provisioning_client_init(struct mqtt_helper_conn_params *conn_params)
{
	int err;
	az_span dps_hostname = AZ_SPAN_LITERAL_FROM_STR(CONFIG_AZURE_IOT_HUB_DPS_HOSTNAME);

	dps_reg_ctx.operation_id = az_span_create(operation_id_buf, sizeof(operation_id_buf));
	dps_reg_ctx.assigned_hub = az_span_create(assigned_hub_buf, sizeof(assigned_hub_buf));
	dps_reg_ctx.assigned_device_id = az_span_create(assigned_device_id_buf,
							sizeof(assigned_device_id_buf));
	dps_reg_ctx.retry_count = 0;

	/* Reset the Azure SDK client */
	memset(&dps_reg_ctx.dps_client, 0, sizeof(dps_reg_ctx.dps_client));

	err = az_iot_provisioning_client_init(
		&dps_reg_ctx.dps_client,
		dps_hostname,
		dps_reg_ctx.id_scope,
		dps_reg_ctx.reg_id,
		NULL);
	if (az_result_failed(err)) {
		LOG_ERR("Failed to initialize DPS client, error: 0x%08x", err);
		return -EFAULT;
	}

	err = az_iot_provisioning_client_get_client_id(
		&dps_reg_ctx.dps_client,
		conn_params->device_id.ptr,
		conn_params->device_id.size,
		&conn_params->device_id.size);
	if (az_result_failed(err)) {
		LOG_ERR("Failed to get client ID, error: 0x%08x", err);
		return -EFAULT;
	}

	LOG_DBG("Client ID (size: %d): %.*s", conn_params->device_id.size,
		conn_params->device_id.size, conn_params->device_id.ptr);

	err = az_iot_provisioning_client_get_user_name(
		&dps_reg_ctx.dps_client,
		conn_params->user_name.ptr,
		conn_params->user_name.size,
		&conn_params->user_name.size);
	if (az_result_failed(err)) {
		LOG_ERR("Failed to get user name, error: 0x%08x", err);
		return -EFAULT;
	}

	LOG_DBG("User name (size: %d): %.*s", conn_params->user_name.size,
		conn_params->user_name.size, conn_params->user_name.ptr);

	return 0;
}

AZ_DPS_STATIC void on_publish(struct mqtt_helper_buf topic, struct mqtt_helper_buf payload)
{
	az_span topic_span = az_span_create(topic.ptr, topic.size);
	az_span payload_span = az_span_create(payload.ptr, payload.size);

	timeout_work_restart_timer();
	handle_reg_update(topic_span, payload_span);
}

static void on_connack(enum mqtt_conn_return_code return_code, bool session_present)
{
	ARG_UNUSED(session_present);

	int err;

	if (return_code != MQTT_CONNECTION_ACCEPTED) {
		LOG_ERR("Connection was rejected with return code %d", return_code);
		LOG_WRN("Is the device certificate valid?");
		on_reg_failed();
		return;
	}

	dps_state_set(DPS_STATE_CONNECTED);

	LOG_DBG("MQTT connected");

	err = topic_subscribe();
	if (err) {
		LOG_ERR("Failed to subscribe to topics, err: %d", err);
		on_reg_failed();
		return;
	}

	timeout_work_restart_timer();
}

static void on_disconnect(int result)
{
	ARG_UNUSED(result);
	dps_state_set(DPS_STATE_DISCONNECTED);
	mqtt_helper_deinit();
}

static void on_puback(uint16_t message_id, int result)
{
	timeout_work_restart_timer();
}

static void on_suback(uint16_t message_id, int result)
{
	int err;

	err = dps_send_reg_request();
	if (err) {
		LOG_ERR("Failed to send DPS registration request, error: %d", err);
		on_reg_failed();
		return;
	}

	dps_reg_ctx.status = AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNING;
	dps_reg_ctx.cb(dps_reg_ctx.status);

	timeout_work_restart_timer();
}

/* Public interface */

int azure_iot_hub_dps_init(struct azure_iot_hub_dps_config *cfg)
{
	int err;

	if ((cfg == NULL) || (cfg->handler == NULL)) {
		return -EINVAL;
	}

	if (!dps_state_verify(DPS_STATE_UNINIT) && !dps_state_verify(DPS_STATE_DISCONNECTED))  {
		LOG_ERR("DPS cannot start in the current state (%s)", state_name_get(dps_state));
	}

	dps_reg_ctx.cb = cfg->handler;

	err = dps_reg_id_set(cfg->reg_id);
	if (err) {
		return err;
	}

	if (az_span_size(dps_reg_ctx.reg_id) == 0) {
		LOG_ERR("Registration ID length is zero, DPS cannot proceed");
		return -EFAULT;
	}

	err = dps_id_scope_set(cfg->id_scope);
	if (err) {
		return err;
	}

	if (az_span_size(dps_reg_ctx.id_scope) == 0) {
		LOG_ERR("ID scope length is zero, DPS cannot proceed");
		return -EFAULT;
	}

	dps_reg_ctx.status = AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED;

	k_sem_init(&dps_reg_ctx.settings_loaded, 0, 1);
	k_work_init_delayable(&dps_reg_ctx.timeout_work, timeout_work_fn);

	err = dps_settings_init();
	if (err) {
		return err;
	}

	dps_state_set(DPS_STATE_DISCONNECTED);

	dps_reg_ctx.is_init = true;

	dps_reg_ctx.cb(AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED);

	return 0;
}

int azure_iot_hub_dps_start(void)
{
	int err;
	char client_id_buf[CONFIG_AZURE_IOT_HUB_DPS_DEVICE_ID_MAX_LEN];
	char user_name_buf[CONFIG_AZURE_IOT_HUB_DPS_USER_NAME_BUFFER_SIZE];
	static bool initial_load_done;
	struct mqtt_helper_conn_params conn_params = {
		.hostname = {
			.ptr = CONFIG_AZURE_IOT_HUB_DPS_HOSTNAME,
			.size = sizeof(CONFIG_AZURE_IOT_HUB_DPS_HOSTNAME) - 1,
		},
		.user_name = {
			.ptr = user_name_buf,
			.size = sizeof(user_name_buf),
		},
		.device_id = {
			.ptr = client_id_buf,
			.size = sizeof(client_id_buf),
		},
	};
	struct mqtt_helper_cfg cfg = {
		.cb = {
			.on_connack = on_connack,
			.on_disconnect = on_disconnect,
			.on_publish = on_publish,
			.on_puback = on_puback,
			.on_suback = on_suback,
		},
	};

	if (!dps_state_verify(DPS_STATE_DISCONNECTED)) {
		LOG_ERR("DPS cannot start in the current state (%s)", state_name_get(dps_state));
		return -EACCES;
	}

	if (dps_reg_ctx.status == AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNING) {
		LOG_ERR("Registration is already ongoing");
		return -EALREADY;
	}

	if (!initial_load_done) {
		err = k_sem_take(&dps_reg_ctx.settings_loaded, K_SECONDS(1));
		if (err) {
			LOG_ERR("Settings were not loaded in time");
			dps_state_set(DPS_STATE_DISCONNECTED);

			return -ETIMEDOUT;
		}

		initial_load_done = true;
	}

	if ((az_span_size(dps_reg_ctx.assigned_hub) > 0) &&
	    (az_span_size(dps_reg_ctx.assigned_device_id) > 0)) {
		LOG_DBG("Device \"%.*s\" is assigned to IoT hub: %.*s",
			az_span_size(dps_reg_ctx.assigned_device_id),
			(char *)az_span_ptr(dps_reg_ctx.assigned_device_id),
			az_span_size(dps_reg_ctx.assigned_hub),
			(char *)az_span_ptr(dps_reg_ctx.assigned_hub));

		LOG_DBG("To re-register, call azure_iot_hub_dps_reset() first");

		dps_reg_ctx.status = AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNED;

		return -EALREADY;
	}

	dps_reg_ctx.status = AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED;

	err = provisioning_client_init(&conn_params);
	if (err) {
		return err;
	}

	err = mqtt_helper_init(&cfg);
	if (err) {
		LOG_ERR("mqtt_helper_init failed, error: %d", err);
		return -EFAULT;
	}

	err = mqtt_helper_connect(&conn_params);
	if (err) {
		LOG_ERR("mqtt_helper_connect failed, error: %d", err);
		return err;
	}

	dps_state_set(DPS_STATE_CONNECTING);

	timeout_work_restart_timer();

	return 0;
}

int azure_iot_hub_dps_hostname_get(struct azure_iot_hub_buf *buf)
{
	size_t hostname_len = az_span_size(dps_reg_ctx.assigned_hub);
	char *hostname_ptr = az_span_ptr(dps_reg_ctx.assigned_hub);

	LOG_DBG("hostname_ptr: %p, hostname_len: %d", (void *)hostname_ptr, hostname_len);

	if (buf == NULL) {
		return -EINVAL;
	}

	if ((hostname_len == 0) || (hostname_ptr == NULL)) {
		LOG_DBG("No assigned hub found");
		return -ENOENT;
	}

	buf->ptr = hostname_ptr;
	buf->size = hostname_len;

	return 0;
}

int azure_iot_hub_dps_device_id_get(struct azure_iot_hub_buf *buf)
{
	size_t dev_id_len = az_span_size(dps_reg_ctx.assigned_device_id);
	char *dev_id_ptr = az_span_ptr(dps_reg_ctx.assigned_device_id);

	LOG_DBG("dev_id_ptr: %p, dev_id_len: %d", (void *)dev_id_ptr, dev_id_len);

	if (buf == NULL) {
		return -EINVAL;
	}

	if ((dev_id_len == 0) || (dev_id_ptr == NULL)) {
		LOG_DBG("No assigned device ID found");
		return -ENOENT;
	}

	buf->ptr = dev_id_ptr;
	buf->size = dev_id_len;

	return 0;
}

int azure_iot_hub_dps_hostname_delete(void)
{
	int err;

	err = settings_delete(DPS_SETTINGS_KEY "/" DPS_SETTINGS_HOSTNAME_KEY);
	if (err) {
		LOG_ERR("Failed to delete Azure IoT Hub hostname, error: %d", err);
		return err;
	}

	err = settings_delete(DPS_SETTINGS_KEY "/" DPS_SETTINGS_HOSTNAME_LEN_KEY);
	if (err) {
		LOG_ERR("Failed to delete Azure IoT Hub hostname length, error: %d", err);
		return err;
	}

	dps_reg_ctx.assigned_hub = AZ_SPAN_EMPTY;

	return 0;
}

int azure_iot_hub_dps_device_id_delete(void)
{
	int err;

	err = settings_delete(DPS_SETTINGS_KEY "/" DPS_SETTINGS_DEVICE_ID_KEY);
	if (err) {
		LOG_ERR("Failed to delete assigned device ID, error: %d", err);
		return err;
	}

	err = settings_delete(DPS_SETTINGS_KEY "/" DPS_SETTINGS_DEVICE_ID_LEN_KEY);
	if (err) {
		LOG_ERR("Failed to delete assigned device ID length, error: %d", err);
		return err;
	}

	dps_reg_ctx.assigned_device_id = AZ_SPAN_EMPTY;

	return 0;
}

int azure_iot_hub_dps_reset(void)
{
	int err;
	struct azure_iot_hub_buf empty_buf = {0};

	/* Close active MQTT connection before proceeding with cleanup. */
	if (!dps_state_verify(DPS_STATE_UNINIT) && !dps_state_verify(DPS_STATE_DISCONNECTED)) {
		err = mqtt_helper_disconnect();
		if (err) {
			LOG_ERR("Failed to disconnect MQTT, abandoning connection, error: %d", err);
		} else {
			LOG_WRN("The MQTT connection has been closed");
		}
	}

	dps_state = DPS_STATE_UNINIT;

	memset(&dps_reg_ctx, 0, sizeof(dps_reg_ctx));
	dps_id_scope_set(empty_buf);
	dps_reg_id_set(empty_buf);

	dps_reg_ctx.status = AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED;

	err = azure_iot_hub_dps_hostname_delete();
	if (err) {
		return err;
	};

	return azure_iot_hub_dps_device_id_delete();
}
