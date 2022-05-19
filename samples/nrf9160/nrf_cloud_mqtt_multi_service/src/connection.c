/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <modem/lte_lc.h>
#include <zephyr/net/socket.h>
#include <net/nrf_cloud.h>
#include <date_time.h>
#include <zephyr/logging/log.h>
#include <cJSON.h>
#include "fota_support.h"

#include "location_tracking.h"

#include "connection.h"

LOG_MODULE_REGISTER(connection, CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL);

/* Flow control event identifiers */

/* LTE either is connected or isn't */
#define LTE_CONNECTED			(1 << 1)

/* Do not confuse these with the similarly-named features of the generic cloud API.
 * CLOUD_CONNECTED is fired when we first connect to the nRF Cloud.
 * CLOUD_READY is fired when the connection is fully associated and ready to send device messages.
 * CLOUD_ASSOCIATION_REQUEST is a special state only used when first associating a device with
 *				an nRF Cloud user account.
 * CLOUD_DISCONNECTED is fired when disconnection is detected or requested, and will trigger
 *				a total reset of the nRF cloud connection, and the event flag state.
 */
#define CLOUD_CONNECTED			(1 << 1)
#define CLOUD_READY			(1 << 2)
#define CLOUD_ASSOCIATION_REQUEST	(1 << 3)
#define CLOUD_DISCONNECTED		(1 << 4)

/* Time either is or is not known. This is only fired once, and is never cleared. */
#define DATE_TIME_KNOWN			(1 << 1)

/* Flow control event objects for waiting for key events. */
static K_EVENT_DEFINE(lte_connection_events);
static K_EVENT_DEFINE(cloud_connection_events);
static K_EVENT_DEFINE(datetime_connection_events);

/* Message Queue for enqueing outgoing messages during offline periods. */
K_MSGQ_DEFINE(device_message_queue, sizeof(char *), CONFIG_MAX_OUTGOING_MESSAGES, sizeof(char *));

/* Tracks the number of consecutive message-send failures. A total count greater than
 * CONFIG_MAX_CONSECUTIVE_SEND_FAILURES will trigger a connection reset and cooldown.
 * Resets on every successful device message send.
 */
static int send_failure_count;

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
	k_event_post(&cloud_connection_events, DATE_TIME_KNOWN);
}

bool await_date_time_known(k_timeout_t timeout)
{
	return k_event_wait(&cloud_connection_events, DATE_TIME_KNOWN, false, timeout) != 0;
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
 * @brief Notify that a cloud association request has been made.
 */
static void notify_cloud_requested_association(void)
{
	k_event_post(&cloud_connection_events, CLOUD_ASSOCIATION_REQUEST);
}

/**
 * @brief Check whether a user association request has been received from nRF Cloud.
 *
 * If true, we have received an association request, and we must restart the nRF Cloud connection
 * after association succeeds.
 *
 * This flag is reset by the reconnection attempt.
 *
 * @return bool - Whether we have been requested to associate with an nRF Cloud user.
 */
static bool cloud_has_requested_association(void)
{
	return k_event_wait(&cloud_connection_events, CLOUD_ASSOCIATION_REQUEST,
			    false, K_NO_WAIT) != 0;
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
 * @brief Handler for events from nRF Cloud Lib.
 *
 * @param nrf_cloud_evt Passed in event.
 */
static void cloud_event_handler(const struct nrf_cloud_evt *nrf_cloud_evt)
{
	switch (nrf_cloud_evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTED");
		/* Notify that we have connected to the nRF Cloud. */
		notify_cloud_connected();
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTING");
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST");
		/* This event indicates that the user must associate the device with their
		 * nRF Cloud account in the nRF Cloud portal.
		 */
		LOG_INF("Please add this device to your cloud account in the nRF Cloud portal.");

		/* Notify that we have been requested to associate with a user account.
		 * This will cause the next NRF_CLOUD_EVT_USER_ASSOCIATED event to
		 * disconnect and reconnect the device to nRF Cloud, which is required
		 * when devices are first associated with an nRF Cloud account.
		 */
		notify_cloud_requested_association();
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATED");
		/* Indicates successful association with an nRF Cloud account.
		 * Will be fired every time the device connects.
		 * If an association request has been previously received from nRF Cloud,
		 * this means this is the first association of the device, and we must disconnect
		 * and reconnect to ensure proper function of the nRF Cloud connection.
		 */

		if (cloud_has_requested_association()) {
			/* We rely on the connection loop to reconnect automatically afterwards. */
			LOG_INF("Device successfully associated with cloud!");
			disconnect_cloud();
		}
		break;
	case NRF_CLOUD_EVT_READY:
		LOG_DBG("NRF_CLOUD_EVT_READY");
		/* Notify that nRF Cloud is ready for communications from us. */
		notify_cloud_ready();
		break;
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		LOG_DBG("NRF_CLOUD_EVT_SENSOR_DATA_ACK");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED");
		/* Notify that we have lost contact with nRF Cloud. */
		disconnect_cloud();
		break;
	case NRF_CLOUD_EVT_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_ERROR: %d", nrf_cloud_evt->status);
		break;
	case NRF_CLOUD_EVT_RX_DATA:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA");
		LOG_DBG("%d bytes received from cloud", nrf_cloud_evt->data.len);

		/* The Location library sends assistance data requests on our behalf, but cannot
		 * intercept the responses. We must, therefore, check all received MQTT payloads for
		 * assistance data ourselves.
		 */
		location_assistance_data_handler(nrf_cloud_evt->data.ptr, nrf_cloud_evt->data.len);
		break;
	case NRF_CLOUD_EVT_FOTA_START:
		LOG_DBG("NRF_CLOUD_EVT_FOTA_START");
		break;
	case NRF_CLOUD_EVT_FOTA_DONE: {
		enum nrf_cloud_fota_type fota_type = NRF_CLOUD_FOTA_TYPE__INVALID;

		if (nrf_cloud_evt->data.ptr) {
			fota_type = *((enum nrf_cloud_fota_type *) nrf_cloud_evt->data.ptr);
		}

		LOG_DBG("NRF_CLOUD_EVT_FOTA_DONE, FOTA type: %s",
			fota_type == NRF_CLOUD_FOTA_APPLICATION	  ?		"Application"	:
			fota_type == NRF_CLOUD_FOTA_MODEM_DELTA	  ?		"Modem (delta)"	:
			fota_type == NRF_CLOUD_FOTA_BOOTLOADER	  ?		"Bootloader"	:
										"Invalid");

		/* Notify fota_support of the completed download. */
		on_fota_downloaded();
		break;
	}
	case NRF_CLOUD_EVT_FOTA_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_FOTA_ERROR");
		break;
	default:
		LOG_DBG("Unknown event type: %d", nrf_cloud_evt->type);
		break;
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
		if (IS_ENABLED(CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL_DBG)) {
			char log_buf[60];
			ssize_t len;

			len = snprintf(log_buf, sizeof(log_buf),
				"LTE_EVENT: eDRX parameter update: eDRX: %f, PTW: %f",
				evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
			if (len > 0) {
				LOG_DBG("%s", log_strdup(log_buf));
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
		.application = fota_capable(),
		.bootloader = fota_capable() && boot_fota_capable(),
		.modem = fota_capable()
	};
	struct nrf_cloud_svc_info_ui ui_info = {
		.gps = location_tracking_enabled(),
		.temperature = IS_ENABLED(CONFIG_TEMP_TRACKING),
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
		LOG_ERR("Failed to update device shadow, error: %d", err);
	}
}

int consume_device_message(void)
{
	char *msg;
	int ret;

	LOG_DBG("Consuming an enqueued device message");

	/* Wait until a message is available to send. */
	ret = k_msgq_get(&device_message_queue, &msg, K_FOREVER);
	if (ret) {
		LOG_ERR("Failed to retrieve item from outgoing message queue, error: %d", ret);
		return -ret;
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
	LOG_DBG("Attempting to transmit enqueued device message");
	struct nrf_cloud_tx_data mqtt_msg = {
		.data.ptr = msg,
		.data.len = strlen(msg),
		.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
	};

	ret = nrf_cloud_send(&mqtt_msg);
	if (ret) {
		LOG_ERR("Transmission of enqueued device message failed, nrf_cloud_send "
			"gave error: %d. The message will be re-enqueued and tried again "
			"later.", ret);

		/* Re-enqueue the message for later retry (creating a new copy of it in heap). */
		send_device_message(msg);

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
		LOG_DBG("Enqueued device message consumed successfully");

		/* Reset the failure counter, since we succeeded. */
		send_failure_count = 0;
	}

	/* Clean up. */
	k_free(msg);

	return ret;
}

int send_device_message(const char *const msg)
{
	/* Copy message data onto the heap (will be freed by the message consumer when done). */

	char *msg_buf = k_malloc(strlen(msg) + 1);

	if (!msg_buf) {
		LOG_ERR("Could not alloc memory for new device message");
		return -ENOMEM;
	}
	strcpy(msg_buf, msg);
	LOG_DBG("Enqueued message: %s", log_strdup(msg_buf));

	/* Attempt to append data onto message queue. */
	if (k_msgq_put(&device_message_queue, &msg_buf, K_NO_WAIT)) {
		LOG_ERR("Device message rejected, outgoing message queue is full");
		k_free(msg_buf);
		return -ENOMEM;
	}

	return 0;
}

int send_device_message_cJSON(cJSON *msg_obj)
{
	int ret = 0;
	char *msg = NULL;

	if (!msg_obj) {
		LOG_ERR("Cannot send NULL device message object");
		return -EINVAL;
	}

	/* Convert message object to a string. */
	msg = cJSON_PrintUnformatted(msg_obj);
	if (msg == NULL) {
		LOG_ERR("Failed to convert cJSON device message object to string");
		return -ENOMEM;
	}

	/* Send the string as a device message. */
	ret = send_device_message(msg);

	/* Clean up */
	k_free(msg);
	return ret;
}

/**
 * @brief Reset connection to nRF Cloud, and reset connection status event state.
 * For internal use only. Externally, disconnect_cloud() may be used instead.
 */
static void reset_cloud(void)
{
	int err;

	/* Wait for a few seconds to help residual events settle. */
	LOG_INF("Resetting nRF Cloud transport");
	k_sleep(K_SECONDS(20));

	/* Disconnect from nRF Cloud and uninit the cloud library. */
	err = nrf_cloud_uninit();

	/* nrf_cloud_uninit returns -EBUSY if reset is blocked by a FOTA job. */
	if (err == -EBUSY) {
		LOG_ERR("Could not reset nRF Cloud transport due to ongoing FOTA job. "
			"Continuing without resetting");
	} else if (err) {
		LOG_ERR("Could not reset nRF Cloud transport, error %d. "
			"Continuing without resetting", err);
	} else {
		LOG_INF("nRF Cloud transport has been reset");
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

	LOG_INF("Connecting to nRF Cloud");

	/* Initialize nrf_cloud library. */
	struct nrf_cloud_init_param params = {
		.event_handler = cloud_event_handler
	};
	err = nrf_cloud_init(&params);
	if (err) {
		LOG_ERR("Cloud lib could not be initialized, error: %d", err);
		return err;
	}

	/* Begin attempting to connect persistently. */
	while (true) {
		LOG_INF("Next connection retry in %d seconds",
			CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS);

		err = nrf_cloud_connect(NULL);
		if (err) {
			LOG_ERR("cloud_connect, error: %d", err);
		}

		/* Wait for cloud connection success. If succeessful, break out of the loop. */
		if (await_cloud_connected(
			K_SECONDS(CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS))) {
			break;
		}
	}

	/* Wait for cloud to become ready, resetting if we time out or are disconnected. */
	if (!await_cloud_ready(K_SECONDS(CONFIG_CLOUD_READY_TIMEOUT_SECONDS), true)) {
		LOG_INF("nRF Cloud failed to become ready. Resetting connection.");
		reset_cloud();
		return -ETIMEDOUT;
	}

	LOG_INF("Connected to nRF Cloud");
	return 0;
}

/**
 * @brief Set up modem and start trying to connect to LTE.
 *
 * @return int - 0 on success, otherwise a negative error code.
 */
static int setup_lte(void)
{
	int err;

	LOG_INF("Setting up LTE");

	/* Register to be notified when the modem has figured out the current time. */
	date_time_register_handler(date_time_evt_handler);

	/* Perform Configuration */
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already configured and LTE connected. */
		return 0;
	}

	if (IS_ENABLED(CONFIG_POWER_SAVING_MODE_ENABLE)) {
		/* Requesting PSM before connecting allows the modem to inform
		 * the network about our wish for certain PSM configuration
		 * already in the connection procedure instead of in a separate
		 * request after the connection is in place, which may be
		 * rejected in some networks.
		 */
		err = lte_lc_psm_req(true);
		if (err) {
			LOG_ERR("Failed to set PSM parameters, error: %d", err);
			return err;
		}

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

void manage_connection(void)
{
	/* Enable the modem and start trying to connect to LTE.
	 * This is done once only, since the modem handles connection persistence then after.
	 *
	 * (Once we request connection, the modem will automatically try to reconnect whenever
	 *  connection is lost).
	 */

	LOG_INF("Connecting to LTE network. This may take several minutes...");
	if (setup_lte()) {
		LOG_ERR("LTE initialization failed. Continuing anyway. This may fail.");
	}

	while (true) {
		/* Wait for LTE to become connected (or re-connected if connection was lost). */
		LOG_INF("Waiting for connection to LTE network...");

		(void)await_lte_connection(K_FOREVER);
		LOG_INF("Connected to LTE network");

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
