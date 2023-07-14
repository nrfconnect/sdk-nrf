/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/net/socket.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_coap.h>
#include <date_time.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>

#include "connection.h"

#include "nrf_cloud_codec_internal.h"
#include "handle_fota.h"
#include "location_tracking.h"
#include "led_control.h"

LOG_MODULE_REGISTER(connection, CONFIG_COAP_MULTI_SERVICE_LOG_LEVEL);

/* Flow control event identifiers */

/* LTE either is connected or isn't */
#define LTE_CONNECTED			(1 << 1)

/* CLOUD_CONNECTED is fired when we first connect to the nRF Cloud.
 * CLOUD_READY is fired when the connection is fully associated and ready to send device messages.
 * CLOUD_DISCONNECTED is fired when disconnection is detected or requested, and will trigger
 *				a total reset of the nRF cloud connection, and the event flag state.
 */
#define CLOUD_CONNECTED			(1 << 1)
#define CLOUD_READY			(1 << 2)
#define CLOUD_DISCONNECTED		(1 << 3)

/* Time either is or is not known. This is only fired once, and is never cleared. */
#define DATE_TIME_KNOWN			(1 << 1)

/* Flow control event objects for waiting for key events. */
static K_EVENT_DEFINE(lte_connection_events);
static K_EVENT_DEFINE(cloud_connection_events);
static K_EVENT_DEFINE(datetime_connection_events);

/* Message Queue for enqueing outgoing messages during offline periods. */
K_MSGQ_DEFINE(device_message_queue,
	      sizeof(struct nrf_cloud_sensor_data *),
	      CONFIG_MAX_OUTGOING_MESSAGES,
	      sizeof(struct nrf_cloud_sensor_data *));

/* Tracks the number of consecutive message-send failures. A total count greater than
 * CONFIG_MAX_CONSECUTIVE_SEND_FAILURES will trigger a connection reset and cooldown.
 * Resets on every successful device message send.
 */
static int send_failure_count;

static dev_msg_handler_cb_t general_dev_msg_handler;

/* Modem info struct used for modem FW version and cell info used for single-cell requests */
#if defined(CONFIG_MODEM_INFO)
static struct modem_param_info mdm_param;
#endif

static void free_queued_dev_msg_message(struct nrf_cloud_sensor_data *msg);

/**
 * @brief Notify that LTE connection has been established.
 */
static void notify_lte_connected(void)
{
	k_event_post(&lte_connection_events, LTE_CONNECTED);
}

/**
 * @brief Reset the LTE connection event flag.
 */
static void clear_lte_connected(void)
{
	k_event_set(&lte_connection_events, 0);
}

bool await_lte_connection(k_timeout_t timeout)
{
	LOG_DBG("Awaiting LTE Connection");
	return k_event_wait_all(&lte_connection_events, LTE_CONNECTED, false, timeout) != 0;
}

/**
 * @brief Notify that the current date and time have become known.
 */
static void notify_date_time_known(void)
{
	k_event_post(&datetime_connection_events, DATE_TIME_KNOWN);
}

bool await_date_time_known(k_timeout_t timeout)
{
	return k_event_wait(&datetime_connection_events, DATE_TIME_KNOWN, false, timeout) != 0;
}

/**
 * @brief Notify that connection to nRF Cloud has been established.
 */
static void notify_cloud_connected(void)
{
	k_event_post(&cloud_connection_events, CLOUD_CONNECTED);
}

/**
 * @brief Notify that cloud connection is ready.
 */
static void notify_cloud_ready(void)
{
	k_event_post(&cloud_connection_events, CLOUD_READY);
}

/**
 * @brief Clear nRF Cloud connection events.
 */
static void clear_cloud_connection_events(void)
{
	k_event_set(&cloud_connection_events, 0);
}

/**
 * @brief Await a connection to nRF Cloud (ignoring LTE connection state and cloud readiness).
 *
 * @param timeout - The time to wait before timing out.
 * @return true if occurred.
 * @return false if timed out.
 */
static bool await_cloud_connected(k_timeout_t timeout)
{
	LOG_DBG("Awaiting Cloud Connection");
	return k_event_wait(&cloud_connection_events, CLOUD_CONNECTED, false, timeout) != 0;
}

/**
 * @brief Wait for nRF Cloud readiness.
 *
 * @param timeout - The time to wait before timing out.
 * @param timeout_on_disconnection - Should cloud disconnection events count as a timeout?
 * @return true if occurred.
 * @return false if timed out.
 */
static bool await_cloud_ready(k_timeout_t timeout, bool timeout_on_disconnection)
{
	LOG_DBG("Awaiting Cloud Ready");
	int await_condition = CLOUD_READY;

	if (timeout_on_disconnection) {
		await_condition |= CLOUD_DISCONNECTED;
	}

	return k_event_wait(&cloud_connection_events, await_condition,
			    false, timeout) == CLOUD_READY;
}

/*
 * This is really a convenience callback to help keep this sample clean and modular. You could
 * implement device message handling directly in the cloud_event_handler if desired.
 */
void register_general_dev_msg_handler(dev_msg_handler_cb_t handler_cb)
{
	general_dev_msg_handler = handler_cb;
}

bool await_connection(k_timeout_t timeout)
{
	return await_lte_connection(timeout) && await_cloud_ready(timeout, false);
}

bool cloud_is_connected(void)
{
	return k_event_wait(&cloud_connection_events, CLOUD_CONNECTED, false, K_NO_WAIT) != 0;
}

void disconnect_cloud(void)
{
	k_event_post(&cloud_connection_events, CLOUD_DISCONNECTED);
}

bool await_cloud_disconnection(k_timeout_t timeout)
{
	return k_event_wait(&cloud_connection_events, CLOUD_DISCONNECTED, false, timeout) != 0;
}

bool cloud_is_disconnecting(void)
{
	return k_event_wait(&cloud_connection_events, CLOUD_DISCONNECTED, false, K_NO_WAIT) != 0;
}

/**
 * @brief Handler for date_time events. Used exclusively to detect when we have obtained
 * a valid modem time.
 *
 * @param date_time_evt - The date_time event. Ignored.
 */
static void date_time_evt_handler(const struct date_time_evt *date_time_evt)
{
	if (date_time_is_valid()) {
		notify_date_time_known();
	}
}

/**
 * @brief Handler for LTE events coming from modem.
 *
 * @param evt Events from modem.
 */
static void lte_event_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		LOG_DBG("LTE_EVENT: Network registration status %d, %s", evt->nw_reg_status,
			evt->nw_reg_status == LTE_LC_NW_REG_NOT_REGISTERED ?	  "Not Registered" :
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?	 "Registered Home" :
			evt->nw_reg_status == LTE_LC_NW_REG_SEARCHING ?		       "Searching" :
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTRATION_DENIED ?
									     "Registration Denied" :
			evt->nw_reg_status == LTE_LC_NW_REG_UNKNOWN ?			 "Unknown" :
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING ?
									      "Registered Roaming" :
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_EMERGENCY ?
									    "Registered Emergency" :
			evt->nw_reg_status == LTE_LC_NW_REG_UICC_FAIL ?		       "UICC Fail" :
											 "Invalid");

		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			/* Clear connected status. */
			clear_lte_connected();

			/* Also reset the nRF Cloud connection if we were currently connected
			 * Failing to do this will result in nrf_cloud_send stalling upon connection
			 * re-establishment.
			 *
			 * We check cloud_is_disconnecting solely to avoid double-printing the
			 * LTE connection lost message. This check has no other effect.
			 */
			if (cloud_is_connected() && !cloud_is_disconnecting()) {
				LOG_INF("LTE connection lost. Disconnecting from nRF Cloud too...");
				disconnect_cloud();
			}
		} else {
			/* Notify we are connected to LTE. */
			notify_lte_connected();
		}

		break;
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_DBG("LTE_EVENT: PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		/* This check is necessary to silence compiler warnings by
		 * sprintf when debug logs are not enabled.
		 */
		if (IS_ENABLED(CONFIG_COAP_MULTI_SERVICE_LOG_LEVEL_DBG)) {
			char log_buf[60];
			ssize_t len;

			len = snprintf(log_buf, sizeof(log_buf),
				"LTE_EVENT: eDRX parameter update: eDRX: %f, PTW: %f",
				evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
			if (len > 0) {
				LOG_DBG("%s", log_buf);
			}
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_DBG("LTE_EVENT: RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_DBG("LTE_EVENT: LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		LOG_DBG("LTE_EVENT: Active LTE mode changed: %s",
			evt->lte_mode == LTE_LC_LTE_MODE_NONE ? "None" :
			evt->lte_mode == LTE_LC_LTE_MODE_LTEM ? "LTE-M" :
			evt->lte_mode == LTE_LC_LTE_MODE_NBIOT ? "NB-IoT" :
			"Unknown");
		break;
	case LTE_LC_EVT_MODEM_EVENT:
		LOG_DBG("LTE_EVENT: Modem domain event, type: %s",
			evt->modem_evt == LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE ?
				"Light search done" :
			evt->modem_evt == LTE_LC_MODEM_EVT_SEARCH_DONE ?
				"Search done" :
			evt->modem_evt == LTE_LC_MODEM_EVT_RESET_LOOP ?
				"Reset loop detected" :
			evt->modem_evt == LTE_LC_MODEM_EVT_BATTERY_LOW ?
				"Low battery" :
			evt->modem_evt == LTE_LC_MODEM_EVT_OVERHEATED ?
				"Modem is overheated" :
				"Unknown");
		break;
	default:
		break;
	}
}

/**
 * @brief Updates the nRF Cloud shadow with information about supported capabilities, current
 *  firmware running, FOTA support, and so on.
 */
static void update_shadow(void)
{
	int err;
	struct nrf_cloud_svc_info_fota fota_info = {
		.application = nrf_cloud_fota_is_type_enabled(NRF_CLOUD_FOTA_APPLICATION),
		.bootloader = nrf_cloud_fota_is_type_enabled(NRF_CLOUD_FOTA_BOOTLOADER),
		.modem = nrf_cloud_fota_is_type_enabled(NRF_CLOUD_FOTA_MODEM_DELTA),
		.modem_full = nrf_cloud_fota_is_type_enabled(NRF_CLOUD_FOTA_MODEM_FULL)
	};
	struct nrf_cloud_svc_info_ui ui_info = {
		.gnss = location_tracking_enabled(),
		.temperature = IS_ENABLED(CONFIG_TEMP_TRACKING),
		.log = IS_ENABLED(CONFIG_NRF_CLOUD_LOG_BACKEND) &&
		       IS_ENABLED(CONFIG_LOG_BACKEND_NRF_CLOUD_OUTPUT_TEXT),
		.dictionary_log = IS_ENABLED(CONFIG_NRF_CLOUD_LOG_BACKEND) &&
				  IS_ENABLED(CONFIG_LOG_BACKEND_NRF_CLOUD_OUTPUT_DICTIONARY)
	};
	struct nrf_cloud_svc_info service_info = {
		.fota = &fota_info,
		.ui = &ui_info
	};
	struct nrf_cloud_modem_info modem_info = {
		.device = NRF_CLOUD_INFO_SET,
		.network = NRF_CLOUD_INFO_SET,
		.sim = IS_ENABLED(CONFIG_MODEM_INFO_ADD_SIM) ? NRF_CLOUD_INFO_SET : 0,
		/* Use the modem info already obtained */
#if defined(CONFIG_MODEM_INFO)
		.mpi = &mdm_param,
#endif
		/* Include the application version */
		.application_version = CONFIG_APP_VERSION
	};
	struct nrf_cloud_device_status device_status = {
		.modem = &modem_info,
		.svc = &service_info
	};

	err = nrf_cloud_coap_shadow_device_status_update(&device_status);
	if (err) {
		LOG_ERR("Failed to update device shadow, error: %d", err);
	}
}

static struct nrf_cloud_sensor_data *allocate_dev_msg_for_queue(struct nrf_cloud_sensor_data *
								msg_to_copy)
{
	struct nrf_cloud_sensor_data *new_msg = k_malloc(sizeof(struct nrf_cloud_sensor_data));

	LOG_INF("type:%d, data_type:%d", msg_to_copy->type, msg_to_copy->data_type);
	if (new_msg == NULL) {
		LOG_ERR("Out of memory error");
	} else if (msg_to_copy) {
		*new_msg = *msg_to_copy;
		if (msg_to_copy->data_type == NRF_CLOUD_DATA_TYPE_BLOCK) {
			uint8_t *new_data = k_malloc(msg_to_copy->data.len);

			if (new_data != NULL) {
				memcpy(new_data, msg_to_copy->data.ptr, msg_to_copy->data.len);
				new_msg->data.ptr = new_data;
			} else {
				LOG_ERR("Out of memory error");
				new_msg->data.ptr = NULL;
				new_msg->data.len = 0;
				k_free(new_msg);
				new_msg = NULL;
			}
		}
	}

	return new_msg;
}

static int enqueue_device_message(struct nrf_cloud_sensor_data *const msg, const bool create_copy)
{
	if (!msg) {
		return -EINVAL;
	}

	struct nrf_cloud_sensor_data *q_msg = msg;

	if (create_copy) {
		/* Allocate a new nrf_cloud_obj structure for the message queue.
		 * Copy the contents of msg_obj, which contains a pointer to the
		 * original message data, into the new structure.
		 */
		q_msg = allocate_dev_msg_for_queue(msg);
		if (!q_msg) {
			return -ENOMEM;
		}
	}

	/* Attempt to append data onto message queue. */
	LOG_DBG("Adding device message to queue");
	if (k_msgq_put(&device_message_queue, &q_msg, K_NO_WAIT)) {
		LOG_ERR("Device message rejected, outgoing message queue is full");
		if (create_copy) {
			free_queued_dev_msg_message(q_msg);
		}
		return -ENOMEM;
	}

	return 0;
}

static void free_queued_dev_msg_message(struct nrf_cloud_sensor_data *msg)
{
	if (msg == NULL) {
		return;
	}
	/* Free the data block attached to the msg */
	if (msg->data_type == NRF_CLOUD_DATA_TYPE_BLOCK) {
		k_free((uint8_t *)msg->data.ptr);
		msg->data.ptr = NULL;
		msg->data.len = 0;
	}
	/* Free the msg itself */
	k_free(msg);
}

int consume_device_message(void)
{
	struct nrf_cloud_sensor_data *queued_msg;
	int ret;
	const char *app_id;

	LOG_DBG("Consuming an enqueued device message");

	/* Wait until a message is available to send. */
	ret = k_msgq_get(&device_message_queue, &queued_msg, K_FOREVER);
	if (ret) {
		LOG_ERR("Failed to retrieve item from outgoing message queue, error: %d", ret);
		return -ret;
	}

	app_id = nrf_cloud_sensor_app_id_lookup(queued_msg->type);
	if (app_id == NULL) {
		return -EINVAL;
	}

	/* Wait until we are able to send it. */
	LOG_DBG("Waiting for valid connection before transmitting device message");
	(void)await_connection(K_FOREVER);

	/* Attempt to send it.
	 *
	 * Note, it is possible (and better) to batch-send device messages when more than one is
	 * queued up. We limit this sample to sending individual messages mainly to keep the sample
	 * simple and accessible. See the Asset Tracker V2 application for an example of batch
	 * message sending.
	 */
	LOG_DBG("Attempting to transmit enqueued device message type:%d, data_type:%d, app_id:%s",
		queued_msg->type, queued_msg->data_type, app_id);

	/* Send message */
	switch (queued_msg->data_type) {
	case NRF_CLOUD_DATA_TYPE_BLOCK:
		if ((queued_msg->type == NRF_CLOUD_SENSOR_GNSS) &&
		    (queued_msg->data.ptr != NULL) &&
		    (queued_msg->data.len == sizeof(struct nrf_cloud_gnss_data))) {
			ret = nrf_cloud_coap_location_send((const struct nrf_cloud_gnss_data *)
							   queued_msg->data.ptr);
		}
		break;
	case NRF_CLOUD_DATA_TYPE_STR:
		/* TODO -- implement public interface for coap string messages */
		break;
	case NRF_CLOUD_DATA_TYPE_DOUBLE:
		ret = nrf_cloud_coap_sensor_send(app_id, queued_msg->double_val, queued_msg->ts_ms);
		break;
	case NRF_CLOUD_DATA_TYPE_INT:
		ret = nrf_cloud_coap_sensor_send(app_id, (double)queued_msg->int_val,
						 queued_msg->ts_ms);
		break;
	}

	if (ret) {
		LOG_ERR("Transmission of enqueued device message failed, error: %d."
			" The message will be re-enqueued and tried again later.", ret);

		/* Re-enqueue the message for later retry.
		 * No need to create a copy since we already copied the
		 * message object struct when it was first enqueued.
		 */
		ret = enqueue_device_message(queued_msg, false);
		if (ret) {
			LOG_ERR("Could not re-enqueue message, discarding.");
			free_queued_dev_msg_message(queued_msg);
		}

		/* Increment the failure counter. */
		send_failure_count += 1;

		/* If we have failed too many times in a row, there is likely a bigger problem,
		 * and we should reset our connection to nRF Cloud, and wait for a few seconds.
		 */
		if (send_failure_count > CONFIG_MAX_CONSECUTIVE_SEND_FAILURES) {
			/* Disconnect. */
			disconnect_cloud();

			/* Wait for a few seconds before trying again. */
			k_sleep(K_SECONDS(CONFIG_CONSECUTIVE_SEND_FAILURE_COOLDOWN_SECONDS));
		}
	} else {
		/* Clean up the message receive from the queue */
		free_queued_dev_msg_message(queued_msg);

		LOG_DBG("Enqueued device message consumed successfully");

		/* Either overwrite the existing pattern with a short success pattern, or just
		 * disable the previously requested pattern, depending on if we are in verbose mode.
		 */
		if (IS_ENABLED(CONFIG_LED_VERBOSE_INDICATION)) {
			short_led_pattern(LED_SUCCESS);
		} else {
			stop_led_pattern();
		}

		/* Reset the failure counter, since we succeeded. */
		send_failure_count = 0;
	}

	return ret;
}

int send_device_message(struct nrf_cloud_sensor_data *const msg)
{
	/* Enqueue the message, creating a copy to be managed by the queue. */
	int ret = enqueue_device_message(msg, true);

	if (ret) {
		LOG_ERR("Cannot add message to queue");
	}

	return ret;
}

/**
 * @brief Set up the nRF Cloud library
 * Must be called before setup_lte, so that pending FOTA jobs can be checked before LTE init.
 * @return int - 0 on success, otherwise negative error code.
 */
static int setup_cloud(void)
{
	int err;

#if defined(CONFIG_NRF_CLOUD_FOTA)
	err = handle_fota_init();
	if (err) {
		LOG_ERR("Error initializing FOTA: %d", err);
		return err;
	}
	err = handle_fota_begin();
	if (err) {
		LOG_ERR("Error initializing FOTA: %d", err);
		return err;
	}
#endif

	err = nrf_cloud_coap_init();
	if (err) {
		LOG_ERR("Failed to initialize CoAP client: %d", err);
		return err;
	}

	return err;
}

/**
 * @brief Close any connection to nRF Cloud, and reset connection status event state.
 * For internal use only. Externally, disconnect_cloud() may be used to trigger a disconnect.
 */
static void reset_cloud(void)
{
	int err;

	/* Wait for a few seconds to help residual events settle. */
	LOG_INF("Disconnecting from nRF Cloud");
	k_sleep(K_SECONDS(20));

	/* Disconnect from nRF Cloud */
	err = nrf_cloud_coap_disconnect();
	if (err) {
		LOG_ERR("Cannot disconnect from nRF Cloud, error: %d. Continuing anyways", err);
	} else {
		LOG_INF("Successfully disconnected from nRF Cloud");
	}

	/* Clear cloud connection event state (reset to initial state). */
	clear_cloud_connection_events();
}

/**
 * @brief Establish a connection to nRF Cloud (presuming we are connected to LTE).
 *
 * @return int - 0 on success, otherwise negative error code.
 */
static int connect_cloud(void)
{
	int err;

	LOG_INF("Connecting to nRF Cloud using CoAP...");

	/* Begin attempting to connect persistently. */
	while (true) {
		LOG_INF("Next connection retry in %d seconds",
			CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS);

		err = nrf_cloud_coap_connect();
		if (err) {
			LOG_ERR("Failed to connect and get authorized: %d", err);
		} else {
			notify_cloud_connected();
		}

		/* Wait for cloud connection success. If succeessful, break out of the loop. */
		if (await_cloud_connected(
			K_SECONDS(CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS))) {
			notify_cloud_ready();
			break;
		}
	}

	LOG_INF("Connected to nRF Cloud using CoAP");

	return err;
}

/**
 * @brief Set up the modem library.
 *
 * @return int - 0 on success, otherwise a negative error code.
 */
static int setup_modem(void)
{
	int ret;

	/*
	 * If there is a pending modem delta firmware update stored, nrf_modem_lib_init will
	 * attempt to install it before initializing the modem library, and return a
	 * positive value to indicate that this occurred. This code can be used to
	 * determine whether the update was successful.
	 */
	ret = nrf_modem_lib_init();

	if (ret < 0) {
		LOG_ERR("Modem library initialization failed, error: %d", ret);
		return ret;
	} else if (ret == NRF_MODEM_DFU_RESULT_OK) {
		LOG_DBG("Modem library initialized after "
			"successful modem firmware update.");
	} else if (ret > 0) {
		LOG_ERR("Modem library initialized after "
			"failed modem firmware update, error: %d", ret);
	} else {
		LOG_DBG("Modem library initialized.");
	}

	/* Register to be notified when the modem has figured out the current time. */
	date_time_register_handler(date_time_evt_handler);

	/* Modem info library is used to obtain the modem FW version
	 * and network info for single-cell requests
	 */
#if defined(CONFIG_MODEM_INFO)
	ret = modem_info_init();
	if (ret) {
		LOG_ERR("Modem info initialization failed, error: %d", ret);
		return ret;
	}

	ret = modem_info_params_init(&mdm_param);
	if (ret) {
		LOG_ERR("Modem info params initialization failed, error: %d", ret);
		return ret;
	}
#endif

	return 0;
}

static void get_modem_info(void)
{
#if defined(CONFIG_MODEM_INFO)
	int ret;

	ret = modem_info_params_get(&mdm_param);
	if (ret) {
		LOG_ERR("Modem info params reading failed, error: %d", ret);
	}

	char mfwv_str[128];

	if (modem_info_string_get(MODEM_INFO_FW_VERSION, mfwv_str, sizeof(mfwv_str)) <= 0) {
		LOG_WRN("Failed to get modem FW version");
	} else {
		LOG_INF("Modem FW version: %s", mfwv_str);
	}
#endif
}

/**
 * @brief Set up LTE and start trying to connect.
 *
 * Must be called AFTER setup_cloud to ensure proper operation of FOTA.
 * @return int - 0 on success, otherwise a negative error code.
 */
static int setup_lte(void)
{
	int err;

	/* Perform Configuration */
	LOG_INF("Requesting PSM mode");

	err = lte_lc_psm_req(true);
	if (err) {
		LOG_ERR("Failed to set PSM parameters, error: %d", err);
		return err;
	} else {
		LOG_INF("PSM mode requested");
	}

	/* Modem events must be enabled before we can receive them. */
	err = lte_lc_modem_events_enable();
	if (err) {
		LOG_ERR("lte_lc_modem_events_enable failed, error: %d", err);
		return err;
	}

	/* Init the modem, and start keeping an active connection.
	 * Note that if connection is lost, the modem will automatically attempt to
	 * re-establish it after this call.
	 */
	LOG_INF("Starting connection to LTE network...");
	err = lte_lc_init_and_connect_async(lte_event_handler);
	if (err) {
		LOG_ERR("Modem could not be configured, error: %d", err);
		return err;
	}

	return 0;
}

void message_queue_thread_fn(void)
{
	/* Continually attempt to consume device messages */
	while (true) {
		(void) consume_device_message();
	}
}

void connection_management_thread_fn(void)
{
	long_led_pattern(LED_WAITING);

	/* Enable the modem */
	LOG_INF("Setting up modem...");
	if (setup_modem()) {
		LOG_ERR("Fatal: Modem setup failed");
		long_led_pattern(LED_FAILURE);
		return;
	}

	/* The nRF Cloud library need only be initialized once, and does not need to be reset
	 * under any circumstances, even error conditions.
	 */
	LOG_INF("Setting up nRF Cloud library...");
	if (setup_cloud()) {
		LOG_ERR("Fatal: nRF Cloud library setup failed");
		long_led_pattern(LED_FAILURE);
		return;
	}

	/* Set up LTE and start trying to connect.
	 * This is done once only, since the modem handles connection persistence then after.
	 *
	 * (Once we request connection, the modem will automatically try to reconnect whenever
	 *  connection is lost).
	 */
	LOG_INF("Setting up LTE...");
	if (setup_lte()) {
		LOG_ERR("Fatal: LTE setup failed");
		long_led_pattern(LED_FAILURE);
		return;
	}

	LOG_INF("Connecting to LTE network. This may take several minutes...");
	while (true) {
		/* Wait for LTE to become connected (or re-connected if connection was lost). */
		LOG_INF("Waiting for connection to LTE network...");

		if (IS_ENABLED(CONFIG_LED_VERBOSE_INDICATION)) {
			long_led_pattern(LED_WAITING);
		}

		(void)await_lte_connection(K_FOREVER);
		LOG_INF("Connected to LTE network");

		get_modem_info();

		/* Attempt to connect to nRF Cloud. */
		if (!connect_cloud()) {
			/* If successful, update the device shadow. */
			update_shadow();

			/* and then wait patiently for a connection problem. */
			(void)await_cloud_disconnection(K_FOREVER);

			LOG_INF("Disconnected from nRF Cloud");
		} else {
			LOG_INF("Failed to connect to nRF Cloud");
		}

		/* Reset cloud connection state before trying again. */
		reset_cloud();

		/* Wait a bit before trying again. */
		k_sleep(K_SECONDS(CONFIG_CLOUD_CONNECTION_REESTABLISH_DELAY_SECONDS));
	}
}
