/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/net/socket.h>
#include <net/nrf_cloud.h>
#include <date_time.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>

#include "cloud_connection.h"

#include "fota_support.h"
#include "location_tracking.h"
#include "led_control.h"

LOG_MODULE_REGISTER(cloud_connection, CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL);

/* Internal state */

/* Pendable events that threads can wait for. */
#define NETWORK_READY			BIT(0)
#define CLOUD_READY			BIT(1)
#define CLOUD_DISCONNECTED		BIT(2)
#define DATE_TIME_KNOWN			BIT(3)
static K_EVENT_DEFINE(cloud_events);

/* Atomic status flag tracking whether an initial association is in progress. */
atomic_t initial_association;

/* Helper functions for pending on pendable events. */
bool await_network_ready(k_timeout_t timeout)
{
	return k_event_wait(&cloud_events, NETWORK_READY, false, timeout) != 0;
}

bool await_cloud_ready(k_timeout_t timeout)
{
	return k_event_wait(&cloud_events, CLOUD_READY, false, timeout) != 0;
}

bool await_cloud_disconnected(k_timeout_t timeout)
{
	return k_event_wait(&cloud_events, CLOUD_DISCONNECTED, false, timeout) != 0;
}

bool await_date_time_known(k_timeout_t timeout)
{
	return k_event_wait(&cloud_events, DATE_TIME_KNOWN, false, timeout) != 0;
}

/* Delayable work item for handling cloud readiness timeout.
 * The work item is scheduled at a delay any time connection starts and is cancelled when the
 * connection to nRF Cloud becomes ready to use (signalled by NRF_CLOUD_EVT_READY).
 *
 * If the work item executes, that means the nRF Cloud connection did not become ready for use
 * within the delay, and thus the connection should be reset (and then try again later).
 */
static void ready_timeout_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	LOG_INF("nRF Cloud connection did not become ready in time, disconnecting and retrying...");
	disconnect_cloud();
}

static K_WORK_DELAYABLE_DEFINE(ready_timeout_work, ready_timeout_work_fn);

static void start_readiness_timeout(void)
{
	LOG_DBG("Starting cloud connection readiness timeout for %d seconds",
		CONFIG_CLOUD_READY_TIMEOUT_SECONDS);
	k_work_reschedule(&ready_timeout_work, K_SECONDS(CONFIG_CLOUD_READY_TIMEOUT_SECONDS));
}

static void clear_readiness_timeout(void)
{
	LOG_DBG("Stopping cloud connection readiness timeout");
	k_work_cancel_delayable(&ready_timeout_work);
}

/* A callback that the application may register in order to handle custom device messages.
 * This is really a convenience callback to help keep this sample clean and modular. You could
 * implement device message handling directly in the cloud_event_handler if desired.
 */
static dev_msg_handler_cb_t general_dev_msg_handler;
void register_general_dev_msg_handler(dev_msg_handler_cb_t handler_cb)
{
	general_dev_msg_handler = handler_cb;
}

/* This function causes the cloud to disconnect, and updates internal state accordingly.
 *
 * It is also triggerd by cloud disconnection, to update internal state.
 *
 * In this latter case, an unnecessary "Disconnecting from nRF Cloud" and
 * "Already disconnected from nRF Cloud" will be printed.
 *
 * This is done to keep the sample simple, though the log messages may be a bit confusing.
 */
void disconnect_cloud(void)
{
	/* Clear the readiness timeout in case it was running. */
	clear_readiness_timeout();

	/* Clear the Ready event, no longer accurate. */
	k_event_clear(&cloud_events, CLOUD_READY);

	/* Clear the initial association flag, no longer accurate. */
	atomic_set(&initial_association, false);

	/* Disconnect from nRF Cloud -- Blocking call
	 * Will no-op and return -EACCES if already disconnected.
	 */
	LOG_INF("Disconnecting from nRF Cloud");
	int err = nrf_cloud_disconnect();

	/* nrf_cloud_uninit returns -EACCES if we are not currently in a connected state. */
	if (err == -EACCES) {
		LOG_DBG("Already disconnected from nRF Cloud");
	} else if (err) {
		LOG_ERR("Cannot disconnect from nRF Cloud, error: %d. Continuing anyways", err);
	} else {
		LOG_INF("Successfully disconnected from nRF Cloud");
	}

	/* Fire the disconnected event. */
	k_event_post(&cloud_events, CLOUD_DISCONNECTED);
}

/**
 * @brief Attempt to connect to nRF Cloud and update internal state accordingly.
 *
 * @retval true if successful
 * @retval false if connection failed
 */
static bool connect_cloud(void)
{
	LOG_INF("Connecting to nRF Cloud");

	/* Clear the disconnected flag, no longer accurate. */
	k_event_clear(&cloud_events, CLOUD_DISCONNECTED);

	/* Connect to nRF Cloud -- Non-blocking. State updates are handled in callbacks. */
	int err = nrf_cloud_connect();

	/* If we were already connected, treat as a successful connection, but do nothing. */
	if (err == NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED) {
		LOG_WRN("Already connected to nRF Cloud");
		return true;
	}

	/* If the connection attempt fails, report and exit. */
	if (err != 0) {
		LOG_ERR("Could not connect to nRF Cloud, error: %d", err);
		return false;
	}

	/* If the connect attempt succeeded, start the readiness timeout. */
	LOG_INF("Connected to nRF Cloud");
	start_readiness_timeout();
	return true;
}

/**
 * @brief Update internal state in response to achieving readiness.
 */
static void cloud_ready(void)
{
	/* Clear the readiness timeout, since we have become ready. */
	clear_readiness_timeout();

	/* Notify that the nRF Cloud connection is ready for use. */
	k_event_post(&cloud_events, CLOUD_READY);
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
	struct nrf_cloud_device_status device_status = {
		.modem = NULL,
		.svc = &service_info
	};

	LOG_DBG("Updating shadow");

	err = nrf_cloud_shadow_device_status_update(&device_status);
	if (err) {
		LOG_ERR("Failed to update device shadow, error: %d", err);
	}
}


/* External event handlers */

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
			LOG_INF("Network connectivity lost!");

			/* Clear the network ready flag */
			k_event_clear(&cloud_events, NETWORK_READY);

			/* Disconnect from cloud as well. */
			disconnect_cloud();
		} else {
			LOG_INF("Network connectivity gained!");

			/* Set the network ready flag */
			k_event_post(&cloud_events, NETWORK_READY);
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

/* Handler for date_time library events, used to keep track of whether the current time is known */
static void date_time_evt_handler(const struct date_time_evt *date_time_evt)
{
	if (date_time_is_valid()) {
		k_event_post(&cloud_events, DATE_TIME_KNOWN);
	} else {
		k_event_clear(&cloud_events, DATE_TIME_KNOWN);
	}
}

/* Handler for events from nRF Cloud Lib. */
static void cloud_event_handler(const struct nrf_cloud_evt *nrf_cloud_evt)
{
	switch (nrf_cloud_evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTED");
		/* There isn't much to do here since what we really care about is association! */
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTING");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR: %d", nrf_cloud_evt->status);
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST");
		/* This event indicates that the user must associate the device with their
		 * nRF Cloud account in the nRF Cloud portal.
		 *
		 * The device must then disconnect and reconnect to nRF Cloud after association
		 * succeeds.
		 */
		LOG_INF("Please add this device to your cloud account in the nRF Cloud portal.");

		/* Store the fact that this is an initial association.
		 *
		 * This will cause the next NRF_CLOUD_EVT_USER_ASSOCIATED event to
		 * disconnect and reconnect the device to nRF Cloud, which is required
		 * when devices are first associated with an nRF Cloud account.
		 */
		atomic_set(&initial_association, true);
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATED");
		/* Indicates successful association with an nRF Cloud account.
		 * Fired every time the device connects (unless the device is not associated).
		 *
		 * If this is an initial association, the device must disconnect and
		 * and reconnect before using nRF Cloud.
		 */

		if (atomic_get(&initial_association)) {
			/* Disconnect as is required.
			 * The connection loop will handle reconnection afterwards.
			 */
			LOG_INF("Device successfully associated with cloud! Reconnecting");
			disconnect_cloud();
		}
		break;
	case NRF_CLOUD_EVT_READY:
		LOG_DBG("NRF_CLOUD_EVT_READY");

		/* Handle achievement of readiness */
		cloud_ready();

		/* Update the device shadow */
		update_shadow();

		break;
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		LOG_DBG("NRF_CLOUD_EVT_SENSOR_DATA_ACK");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED");

		/* The nRF Cloud library itself has disconnected for some reason.
		 * Execute a manual disconnect so that the event flags are updated.
		 * The internal nrf_cloud_disconnect call will no-op.
		 */
		disconnect_cloud();

		break;
	case NRF_CLOUD_EVT_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_ERROR: %d", nrf_cloud_evt->status);
		break;
	case NRF_CLOUD_EVT_RX_DATA_GENERAL:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA_GENERAL");
		LOG_DBG("%d bytes received from cloud", nrf_cloud_evt->data.len);

		/* Pass the device message along to the application, if it is listening */
		if (general_dev_msg_handler) {
			/* To keep the sample simple, we invoke the callback directly.
			 * If you want to do complex operations in this callback without blocking
			 * receipt of data from nRF Cloud, you should set up a work queue and pass
			 * messages to it either here, or from inside the callback.
			 */
			general_dev_msg_handler(&nrf_cloud_evt->data);
		}

		break;
	case NRF_CLOUD_EVT_RX_DATA_SHADOW:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA_SHADOW");
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
			fota_type == NRF_CLOUD_FOTA_MODEM_FULL	  ?		"Modem (full)"	:
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

	return 0;
}


/**
 * @brief Set up the nRF Cloud library
 *
 * Call this before setup_network so that any pending FOTA job is handled first.
 * This avoids calling setup_network pointlessly right before a FOTA-initiated reboot.
 *
 * @return int - 0 on success, otherwise negative error code.
 */
static int setup_cloud(void)
{
	/* Initialize nrf_cloud library. */
	struct nrf_cloud_init_param params = {
		.event_handler = cloud_event_handler,
		.fmfu_dev_inf = get_full_modem_fota_fdev(),
		.application_version = CONFIG_APP_VERSION
	};

	int err = nrf_cloud_init(&params);

	if (err) {
		LOG_ERR("nRF Cloud library could not be initialized, error: %d", err);
		return err;
	}

	return 0;
}

/**
 * @brief Set up network and start trying to connect.
 *
 * @return int - 0 on success, otherwise a negative error code.
 */
static int setup_network(void)
{
	int err;

	/* Perform Configuration */
	if (IS_ENABLED(CONFIG_POWER_SAVING_MODE_ENABLE)) {
		/* Requesting PSM before connecting allows the modem to inform
		 * the network about our wish for certain PSM configuration
		 * already in the connection procedure instead of in a separate
		 * request after the connection is in place, which may be
		 * rejected in some networks.
		 */
		LOG_INF("Requesting PSM mode");

		err = lte_lc_psm_req(true);
		if (err) {
			LOG_ERR("Failed to set PSM parameters, error: %d", err);
			return err;
		} else {
			LOG_INF("PSM mode requested");
		}
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

void cloud_connection_thread_fn(void)
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

	/* Set up network and start trying to connect.
	 * This is done once only, since the network implementation should handle network
	 * persistence then after. (Once we request connection, it will automatically try to
	 * reconnect whenever connection is lost).
	 */
	LOG_INF("Setting up network...");
	if (setup_network()) {
		LOG_ERR("Fatal: Network setup failed");
		long_led_pattern(LED_FAILURE);
		return;
	}

	/* Indefinitely maintain a connection to nRF Cloud whenever the network is reachable. */
	while (true) {
		LOG_INF("Waiting for network ready...");

		if (IS_ENABLED(CONFIG_LED_VERBOSE_INDICATION)) {
			long_led_pattern(LED_WAITING);
		}

		(void)await_network_ready(K_FOREVER);

		LOG_INF("Network is ready");

		/* Attempt to connect to nRF Cloud. */
		if (connect_cloud()) {
			LOG_DBG("Awaiting disconnection from nRF Cloud");

			/* and then wait patiently for a connection problem. */
			(void)await_cloud_disconnected(K_FOREVER);

			LOG_INF("Disconnected from nRF Cloud");
		}

		LOG_INF("Retrying in %d seconds...", CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS);

		/* Wait a bit before trying again. */
		k_sleep(K_SECONDS(CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS));
	}
}
