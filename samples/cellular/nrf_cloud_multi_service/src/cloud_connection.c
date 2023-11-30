/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <date_time.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_log.h>
#if defined(CONFIG_NRF_CLOUD_COAP)
#include <net/nrf_cloud_coap.h>
#include "fota_support_coap.h"
#endif

#include "cloud_connection.h"
#include "provisioning_support.h"
#include "fota_support.h"
#include "location_tracking.h"
#include "led_control.h"

LOG_MODULE_REGISTER(cloud_connection, CONFIG_MULTI_SERVICE_LOG_LEVEL);

/* Internal state */

/* Pendable events that threads can wait for. */
#define NETWORK_READY			BIT(0)
#define CLOUD_CONNECTED			BIT(1)
#define CLOUD_READY			BIT(2)
#define CLOUD_DISCONNECTED		BIT(3)
#define DATE_TIME_KNOWN			BIT(4)
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

/* Wait for a connection result, and return true if connection was successful within the timeout,
 * otherwise return false.
 */
static bool await_connection_result(k_timeout_t timeout)
{
	/* After a connection attempt, either CLOUD_CONNECTED or CLOUD_DISCONNECTED will be
	 * raised, depending on whether the connection succeeded.
	 */
	uint32_t events = CLOUD_CONNECTED | CLOUD_DISCONNECTED;

	return (k_event_wait(&cloud_events, events, false, timeout) & CLOUD_CONNECTED) != 0;
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


/* Start the readiness timeout if readiness is not already achieved. */
static void start_readiness_timeout(void)
{
	/* It doesn't make sense to start the readiness timeout if we're already ready. */
	if (!k_event_test(&cloud_events, CLOUD_READY)) {
		return;
	}

	LOG_DBG("Starting cloud connection readiness timeout for %d seconds",
		CONFIG_CLOUD_READY_TIMEOUT_SECONDS);

	k_work_reschedule(&ready_timeout_work, K_SECONDS(CONFIG_CLOUD_READY_TIMEOUT_SECONDS));
}

static void clear_readiness_timeout(void)
{
	LOG_DBG("Stopping cloud connection readiness timeout");
	k_work_cancel_delayable(&ready_timeout_work);
}

/**
 * @brief Update internal state in response to achieving connection.
 */
static void cloud_connected(void)
{
	LOG_INF("Connected to nRF Cloud");

	/* Notify that the nRF Cloud connection is established. */
	k_event_post(&cloud_events, CLOUD_CONNECTED);
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

	/* Clear the Ready and Connected events, no longer accurate. */
	k_event_clear(&cloud_events, CLOUD_READY | CLOUD_CONNECTED);

	/* Clear the initial association flag, no longer accurate. */
	atomic_set(&initial_association, false);

	/* Disconnect from nRF Cloud -- Blocking call
	 * Will no-op and return -EACCES if already disconnected.
	 */
	LOG_INF("Disconnecting from nRF Cloud");
	int err;

#if defined(CONFIG_NRF_CLOUD_MQTT)
	err = nrf_cloud_disconnect();
#elif defined(CONFIG_NRF_CLOUD_COAP)
	err = nrf_cloud_coap_disconnect();
#endif
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
 * Blocks until connection attempt either succeeds or fails.
 *
 * @retval true if successful
 * @retval false if connection failed
 */
static bool connect_cloud(void)
{
	LOG_INF("Connecting to nRF Cloud");

	/* Clear the disconnected flag, no longer accurate. */
	k_event_clear(&cloud_events, CLOUD_DISCONNECTED);

	int err;

#if defined(CONFIG_NRF_CLOUD_MQTT)
	/* Connect to nRF Cloud -- Non-blocking. State updates are handled in callbacks. */
	err = nrf_cloud_connect();
#elif defined(CONFIG_NRF_CLOUD_COAP)
	/* Connect via CoAP -- blocking. */
	err = nrf_cloud_coap_connect(CONFIG_APP_VERSION);

	/* Cloud is immediately ready and connected since nrf_cloud_coap_connect is blocking. */
	if (!err) {
		cloud_connected();
		cloud_ready();
		return true;
	}
#endif

	/* If we were already connected, treat as a successful connection, but do nothing. */
	if (err == NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED) {
		LOG_WRN("Already connected to nRF Cloud");
		return true;
	}

	/* If the connection attempt fails immediately, report and exit. */
	if (err != 0) {
		LOG_ERR("Could not connect to nRF Cloud, error: %d", err);
		return false;
	}

	/* Wait for the connection to either complete or fail. */
	if (!await_connection_result(K_FOREVER)) {
		LOG_ERR("Could not connect to nRF Cloud");
		return false;
	}

	/* If connection succeeded and we aren't already ready, start the readiness timeout.
	 * (Readiness check is performed by start_readiness_timeout).
	 */
	start_readiness_timeout();
	return true;
}

/* External event handlers */

/* Handler for L4/connectivity events
 * This allows the cloud module to react to network gain and loss.
 * The conn_mgr subsystem is responsible for seeking / maintaining network connectivity and
 * firing these events.
 */
static struct net_mgmt_event_callback l4_callback;
static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t event, struct net_if *iface)
{
	if (event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connectivity gained!");

		k_sleep(K_MSEC(1000));

		/* Set the network ready flag */
		k_event_post(&cloud_events, NETWORK_READY);

		/* If LTE-event-driven date_time updates are disabled, manually trigger a date_time
		 * timestamp refresh.
		 *
		 * Note: The CONFIG_DATE_TIME_AUTO_UPDATE setting controls specifically whether
		 * LTE-event-driven date_time updates are enabled. The date_time library will still
		 * periodically refresh its timestamp if CONFIG_DATE_TIME_AUTO_UPDATE is disabled,
		 * but this refresh is infrequent, so we are manually requesting a refresh
		 * whenever internet access becomes available so that we get a timestamp
		 * immediately.
		 */
		if (!IS_ENABLED(CONFIG_DATE_TIME_AUTO_UPDATE)) {
			date_time_update_async(NULL);
		}

	} else if (event == NET_EVENT_L4_DISCONNECTED) {
		LOG_INF("Network connectivity lost!");

		/* Clear the network ready flag */
		k_event_clear(&cloud_events, NETWORK_READY);

		/* Disconnect from cloud as well. */
		disconnect_cloud();
	}
}

/* Handler for date_time library events, used to keep track of whether the current time is known */
static void date_time_event_handler(const struct date_time_evt *date_time_evt)
{
	if (date_time_is_valid()) {
		k_event_post(&cloud_events, DATE_TIME_KNOWN);
	} else {
		k_event_clear(&cloud_events, DATE_TIME_KNOWN);
	}
}

#if defined(CONFIG_NRF_CLOUD_MQTT)
/* Handler for nRF Cloud shadow events */
static void handle_shadow_event(struct nrf_cloud_obj_shadow_data *const shadow)
{
	if (!shadow) {
		return;
	}

	int err;

	if (shadow->type == NRF_CLOUD_OBJ_SHADOW_TYPE_DELTA) {
		LOG_DBG("Shadow: Delta - version: %d, timestamp: %lld",
			shadow->delta->ver,
			shadow->delta->ts);

		/* Always accept since this sample, by default,
		 * doesn't have any application specific shadow handling
		 */
		err = nrf_cloud_obj_shadow_delta_response_encode(&shadow->delta->state, true);
		if (err) {
			LOG_ERR("Failed to encode shadow response: %d", err);
			return;
		}

		err = nrf_cloud_obj_shadow_update(&shadow->delta->state);
		if (err) {
			LOG_ERR("Failed to send shadow response, error: %d", err);
		}
	} else if (shadow->type == NRF_CLOUD_OBJ_SHADOW_TYPE_ACCEPTED) {
		LOG_DBG("Shadow: Accepted");
	}
}

/* Handler for events from nRF Cloud Lib. */
static void cloud_event_handler(const struct nrf_cloud_evt *nrf_cloud_evt)
{
	switch (nrf_cloud_evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTED");

		/* Handle connection success. */
		cloud_connected();
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTING");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR: %d", nrf_cloud_evt->status);

		/* Disconnect from cloud immediately rather than wait for retry timeout. */
		disconnect_cloud();

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

		/* The nRF Cloud library will automatically update the
		 * device's shadow based on the build configuration.
		 * See config NRF_CLOUD_SEND_SHADOW_INFO for details.
		 */

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
	case NRF_CLOUD_EVT_RX_DATA_SHADOW: {
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA_SHADOW");
		handle_shadow_event(nrf_cloud_evt->shadow);
		break;
	}
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
#endif /* CONFIG_NRF_CLOUD_MQTT */

/**
 * @brief Set up for the nRF Cloud connection (without connecting)
 *
 * Sets up required event hooks and initializes the nrf_cloud library.
 *
 * @return int - 0 on success, otherwise negative error code.
 */
static int setup_cloud(void)
{
	int err;

	/* Register to be notified of network availability changes.
	 *
	 * If the chosen connectivity layer becomes ready instantaneously, it is possible that
	 * L4_CONNECTED will be fired before reaching this function, in which case we will miss
	 * the notification.
	 *
	 * If that is a serious concern, use SYS_INIT with priority 0 (less than
	 * CONFIG_NET_CONNECTION_MANAGER_PRIORITY) to register this hook before conn_mgr
	 * initializes.
	 *
	 * In reality, connectivity layers such as LTE take some time to go online, so registering
	 * the hook here is fine.
	 */
	net_mgmt_init_event_callback(
		&l4_callback, l4_event_handler, NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED
	);
	net_mgmt_add_event_callback(&l4_callback);

	/* Register to be notified when the modem has figured out the current time. */
	date_time_register_handler(date_time_event_handler);

#if defined(CONFIG_NRF_CLOUD_MQTT)
	/* Initialize nrf_cloud library. */
	struct nrf_cloud_init_param params = {
		.event_handler = cloud_event_handler,
		.fmfu_dev_inf = get_full_modem_fota_fdev(),
		.application_version = CONFIG_APP_VERSION
	};

	err = nrf_cloud_init(&params);
	if (err) {
		LOG_ERR("nRF Cloud library could not be initialized, error: %d", err);
		return err;
	}

#elif defined(CONFIG_NRF_CLOUD_COAP)
#if defined(CONFIG_COAP_FOTA)
	err = coap_fota_init();
	if (err) {
		LOG_ERR("Error initializing FOTA: %d", err);
		return err;
	}
	err = coap_fota_begin();
	if (err) {
		LOG_ERR("Error starting FOTA: %d", err);
		return err;
	}
#endif /* CONFIG_COAP_FOTA */
	err = nrf_cloud_coap_init();
	if (err) {
		LOG_ERR("Failed to initialize CoAP client: %d", err);
		return err;
	}
#endif /* CONFIG_NRF_CLOUD_COAP */

	return 0;
}

/* Check whether nRF Cloud credentials are installed. If not, sleep forever. */
static void check_credentials(void)
{
	int status = nrf_cloud_credentials_configured_check();

	if (status == -ENOTSUP) {
		LOG_WRN("nRF Cloud credentials are not installed. Please install and reboot.");
		k_sleep(K_FOREVER);
	} else if (status) {
		LOG_ERR("Error while checking for credentials: %d. Proceeding anyway.", status);
	}
}

void cloud_connection_thread_fn(void)
{
	long_led_pattern(LED_WAITING);

	LOG_INF("Enabling connectivity...");
	conn_mgr_all_if_connect(true);

	LOG_INF("Setting up nRF Cloud library...");
	if (setup_cloud()) {
		LOG_ERR("Fatal: nRF Cloud library setup failed");
		long_led_pattern(LED_FAILURE);
		return;
	}

	/* Check for credentials, if the feature is enabled. */
	if (IS_ENABLED(CONFIG_NRF_CLOUD_CHECK_CREDENTIALS)) {
		check_credentials();
	}

	/* Indefinitely maintain a connection to nRF Cloud whenever the network is reachable. */
	while (true) {
		LOG_INF("Waiting for network ready...");

		if (IS_ENABLED(CONFIG_LED_VERBOSE_INDICATION)) {
			long_led_pattern(LED_WAITING);
		}

		(void)await_network_ready(K_FOREVER);

		LOG_INF("Network is ready");

		/* Wait for provisioning to complete, if the provisioning library is enabled. */
		if (IS_ENABLED(CONFIG_NRF_PROVISIONING)) {
			LOG_DBG("Awaiting provisioning idle");
			(void)await_provisioning_idle(K_FOREVER);
		}

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
