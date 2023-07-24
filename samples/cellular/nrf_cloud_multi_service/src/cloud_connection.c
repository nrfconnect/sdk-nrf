/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <stdio.h>
#include <date_time.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_log.h>
#if defined(CONFIG_NRF_CLOUD_COAP)
#include <net/nrf_cloud_coap.h>
#include "fota_support_coap.h"
#endif

#include "cloud_connection.h"
#include "fota_support.h"
#include "location_tracking.h"
#include "led_control.h"

LOG_MODULE_REGISTER(cloud_connection, CONFIG_MULTI_SERVICE_LOG_LEVEL);

/* Internal state */

/* Pendable events that threads can wait for. */
#define NETWORK_READY			BIT(0)
#define CLOUD_READY			BIT(1)
#define CLOUD_DISCONNECTED		BIT(2)
#define DATE_TIME_KNOWN			BIT(3)
static K_EVENT_DEFINE(cloud_events);

/* Atomic status flag tracking whether an initial association is in progress. */
atomic_t initial_association;

static void cloud_ready(void);
static void update_shadow(void);

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
	err = nrf_cloud_coap_connect();
	if (!err) {
		cloud_ready();
		update_shadow();
		return true;
	}
#endif

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
	static bool updated;
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
		.log = nrf_cloud_is_text_logging_enabled(),
		.dictionary_log = nrf_cloud_is_dict_logging_enabled()
	};
	struct nrf_cloud_svc_info service_info = {
		.fota = &fota_info,
		.ui = &ui_info
	};
#if defined(CONFIG_NRF_MODEM)
	struct nrf_cloud_modem_info modem_info = {
		.device = NRF_CLOUD_INFO_SET,
		.network = NRF_CLOUD_INFO_SET,
		.sim = IS_ENABLED(CONFIG_MODEM_INFO_ADD_SIM) ? NRF_CLOUD_INFO_SET : 0,
		.mpi = NULL,
		/* Include the application version */
		.application_version = CONFIG_APP_VERSION
	};
	struct nrf_cloud_modem_info *mdm = &modem_info;
#else
	struct nrf_cloud_modem_info *mdm = NULL;
#endif
	struct nrf_cloud_device_status device_status = {
		.modem = mdm,
		.svc = &service_info
	};

	if (updated) {
		return; /* It is not necessary to do this more than once per boot. */
	}
	LOG_DBG("Updating shadow");
#if defined(CONFIG_NRF_CLOUD_MQTT)
	err = nrf_cloud_shadow_device_status_update(&device_status);
#elif defined(CONFIG_NRF_CLOUD_COAP)
	err = nrf_cloud_coap_shadow_device_status_update(&device_status);
#endif

	if (err) {
		LOG_ERR("Failed to update device shadow, error: %d", err);
	} else {
		LOG_DBG("Updated shadow");
		updated = true;
	}
}

/* External event handlers */

/* Handler for L4/connectivity events
 * This allows the cloud module to react to network gain and loss.
 * The conn_mgr subsystem is responsible for seeking / maintaining network connectivity and
 * firing these events.
 */
struct net_mgmt_event_callback l4_callback;
static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t event, struct net_if *iface)
{
	if (event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connectivity gained!");

		/* Set the network ready flag */
		k_event_post(&cloud_events, NETWORK_READY);


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
#if defined(CONFIG_NRF_CLOUD_COAP_FOTA)
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
#endif /* CONFIG_NRF_CLOUD_COAP_FOTA */
	err = nrf_cloud_coap_init();
	if (err) {
		LOG_ERR("Failed to initialize CoAP client: %d", err);
		return err;
	}
#endif /* CONFIG_NRF_CLOUD_COAP */

	return 0;
}

void cloud_connection_thread_fn(void)
{
	long_led_pattern(LED_WAITING);

	LOG_INF("Setting up nRF Cloud library...");
	if (setup_cloud()) {
		LOG_ERR("Fatal: nRF Cloud library setup failed");
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
