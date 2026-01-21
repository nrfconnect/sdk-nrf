/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_defs.h>
#include <helpers/nrfx_reset_reason.h>
#include <app_version.h>
#include <dk_buttons_and_leds.h>
#ifdef CONFIG_NRF_CLOUD_FOTA_SMP
#include "smp_reset.h"
#endif

LOG_MODULE_REGISTER(nrf_cloud_mqtt_fota, CONFIG_NRF_CLOUD_MQTT_FOTA_SAMPLE_LOG_LEVEL);

#define NORMAL_REBOOT_S 10

/* LED to indicate LTE connectivity */
#define LTE_LED_NUM DK_LED1

/* Boot message */
#define SAMPLE_SIGNON_FMT "nRF Cloud MQTT FOTA Sample, version: %s"

/* Network states */
#define NETWORK_UP BIT(0)
#define CLOUD_READY BIT(1)
#define CLOUD_CONNECTED BIT(2)
#define CLOUD_DISCONNECTED BIT(3)
#define EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

static void ready_timeout_work_fn(struct k_work *work);
static void disconnect_cloud(void);

static K_WORK_DELAYABLE_DEFINE(ready_timeout_work, ready_timeout_work_fn);

/* Connectivity */
static K_EVENT_DEFINE(connection_events);
static struct net_mgmt_event_callback l4_callback;
static int reconnect_seconds = CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS;

/* Atomic status flag tracking whether an initial association is in progress. */
static atomic_t initial_association;

static bool device_deleted;

static bool cred_check(struct nrf_cloud_credentials_status *const cs)
{
	int ret = 0;

	ret = nrf_cloud_credentials_check(cs);
	if (ret) {
		LOG_ERR("nRF Cloud credentials check failed, error: %d", ret);
		return false;
	}

	/* For MQTT, we need three credentials:
	 *  - a CA for the TLS connection
	 *  - a client certificate for authentication
	 *  - a private key for authentication
	 */
	if (!cs->ca || !cs->client_cert || !cs->prv_key) {
		LOG_WRN("Missing required nRF Cloud credential(s) in sec tag %u:", cs->sec_tag);
	}
	if (!cs->ca) {
		LOG_WRN("\t-CA Cert");
	}
	if (!cs->client_cert) {
		LOG_WRN("\t-Client Cert");
	}
	if (!cs->prv_key) {
		LOG_WRN("\t-Private Key");
	}

	return (cs->ca && cs->client_cert && cs->prv_key);
}

static void await_credentials(void)
{
	struct nrf_cloud_credentials_status cs;

	while (!cred_check(&cs)) {
		LOG_INF("Waiting for credentials to be installed...");
		LOG_INF("Press the reset button once the credentials are installed");
		k_sleep(K_FOREVER);
	}

	LOG_INF("nRF Cloud credentials detected!");
}

static void print_reset_reason(void)
{
	int reset_reason = 0;

	reset_reason = nrfx_reset_reason_get();
	LOG_INF("Reset reason: 0x%x", reset_reason);
}

static void sample_reboot(void)
{
	LOG_INF("Rebooting...");
	(void)lte_lc_power_off();
	LOG_PANIC();
	k_sleep(K_SECONDS(NORMAL_REBOOT_S));
	sys_reboot(SYS_REBOOT_COLD);
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

static void clear_readiness_timeout(void)
{
	LOG_DBG("Stopping cloud connection readiness timeout");
	k_work_cancel_delayable(&ready_timeout_work);
}

/* This function causes the cloud to disconnect, and updates internal state accordingly.
 *
 * It is also triggered by cloud disconnection, to update internal state.
 *
 * In this latter case, an unnecessary "Disconnecting from nRF Cloud" and
 * "Already disconnected from nRF Cloud" will be printed.
 *
 * This is done to keep the sample simple, though the log messages may be a bit confusing.
 */
static void disconnect_cloud(void)
{
	int err = 0;

	/* Clear the readiness timeout in case it was running. */
	clear_readiness_timeout();

	/* Clear the Ready and Connected events, no longer accurate. */
	k_event_clear(&connection_events, CLOUD_READY | CLOUD_CONNECTED);
	LOG_DBG("Cleared CLOUD_READY and CLOUD_CONNECTED");

	/* Clear the initial association flag, no longer accurate. */
	atomic_set(&initial_association, false);

	/* Disconnect from nRF Cloud -- Blocking call
	 * Will no-op and return -EACCES if already disconnected.
	 */
	LOG_INF("Disconnecting from nRF Cloud");

	err = nrf_cloud_disconnect();

	/* nrf_cloud_disconnect returns -EACCES if we are not currently in a connected state. */
	if ((err == -EACCES) || (err == -ENOTCONN)) {
		LOG_DBG("Already disconnected from nRF Cloud");
	} else if (err) {
		LOG_ERR("Cannot disconnect from nRF Cloud, error: %d. Continuing anyways", err);
	} else {
		LOG_INF("Successfully disconnected from nRF Cloud");
	}

	/* Fire the disconnected event. */
	k_event_post(&connection_events, CLOUD_DISCONNECTED);
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

	return (k_event_wait(&connection_events, events, false, timeout) & CLOUD_CONNECTED) != 0;
}

/* Start the readiness timeout if readiness is not already achieved. */
static void start_readiness_timeout(void)
{
	/* It doesn't make sense to start the readiness timeout if we're already ready. */
	if (k_event_test(&connection_events, CLOUD_READY)) {
		LOG_DBG("Already ready.");
		return;
	}

	LOG_DBG("Starting cloud connection readiness timeout for %d seconds",
		CONFIG_CLOUD_READY_TIMEOUT_SECONDS);

	k_work_reschedule(&ready_timeout_work, K_SECONDS(CONFIG_CLOUD_READY_TIMEOUT_SECONDS));
}

/* Attempt to connect to nRF Cloud and update internal state accordingly.
 * Blocks until connection attempt either succeeds or fails.
 */
static bool connect_cloud(void)
{
	int err = 0;

	LOG_INF("Connecting to nRF Cloud");

	/* Clear the disconnected flag, no longer accurate. */
	k_event_clear(&connection_events, CLOUD_DISCONNECTED);

	/* Connect to nRF Cloud -- Non-blocking. State updates are handled in callbacks. */
	err = nrf_cloud_connect();

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

/* Handler for events from nRF Cloud Lib. */
static void cloud_event_handler(const struct nrf_cloud_evt *nrf_cloud_evt)
{
	int err = 0;

	switch (nrf_cloud_evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTED");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTING");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR: %d", nrf_cloud_evt->status);
		/* Disconnect from cloud immediately rather than wait for retry timeout. */
		err = nrf_cloud_disconnect();
		if ((err == -EACCES) || (err == -ENOTCONN)) {
			LOG_DBG("Already disconnected from nRF Cloud");
		} else if (err) {
			LOG_ERR("Cannot disconnect from nRF Cloud, error: %d. Continuing anyways",
				err);
		} else {
			LOG_INF("Successfully disconnected from nRF Cloud");
		}
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST");
		/* This event indicates that the user must associate the device with their
		 * nRF Cloud account in the nRF Cloud portal.
		 *
		 * The device must then disconnect and reconnect to nRF Cloud after association
		 * succeeds.
		 */
		LOG_INF("Please add this device to your nRF Cloud account in the portal.");
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
		device_deleted = false;
		if (atomic_get(&initial_association)) {
			/* Disconnect as is required.
			 * The connection loop will handle reconnection afterwards.
			 */
			LOG_INF("Device successfully associated with cloud! Reconnecting");
			reconnect_seconds = 1;
			disconnect_cloud();
		}
		break;
	case NRF_CLOUD_EVT_READY:
		LOG_DBG("NRF_CLOUD_EVT_READY");
		LOG_INF("Connection to nRF Cloud ready");
		/* Clear the readiness timeout, since we have become ready. */
		clear_readiness_timeout();

		reconnect_seconds = CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS;

		k_event_post(&connection_events, CLOUD_READY);
		break;
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		LOG_DBG("NRF_CLOUD_EVT_SENSOR_DATA_ACK");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED");
		/* The nRF Cloud library itself has disconnected for some reason.
		 * Disconnect from cloud immediately rather than wait for retry timeout.
		 */
		disconnect_cloud();

		break;
	case NRF_CLOUD_EVT_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_ERROR: %d", nrf_cloud_evt->status);
		break;
	case NRF_CLOUD_EVT_RX_DATA_DISCON:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA_DISCON");
		LOG_INF("Device was removed from your account.");
		device_deleted = true;
		break;
	case NRF_CLOUD_EVT_RX_DATA_SHADOW: {
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA_SHADOW");
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
			fota_type == NRF_CLOUD_FOTA_APPLICATION ? "Application" :
			fota_type == NRF_CLOUD_FOTA_MODEM_DELTA ? "Modem (delta)" :
			fota_type == NRF_CLOUD_FOTA_MODEM_FULL ? "Modem (full)"	:
			fota_type == NRF_CLOUD_FOTA_BOOTLOADER ? "Bootloader" :
										"Invalid");

		/* Proceed to reboot the device */
		sample_reboot();
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

static struct dfu_target_fmfu_fdev *get_full_modem_fota_fdev(void)
{
	if (IS_ENABLED(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)) {
		static struct dfu_target_fmfu_fdev ext_flash_dev = {
			.size = 0,
			.offset = 0,
			/* CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION is enabled,
			 * so no need to specify the flash device here
			 */
			.dev = NULL
		};

		return &ext_flash_dev;
	}

	return NULL;
}

/* Callback to track network connectivity */
static void l4_event_handler(struct net_mgmt_event_callback *cb, uint64_t event,
			     struct net_if *iface)
{
	if ((event & EVENT_MASK) != event) {
		return;
	}

	if (event == NET_EVENT_L4_CONNECTED) {
		/* Mark network as up. */
		dk_set_led(LTE_LED_NUM, 1);
		LOG_INF("Connected to LTE");
		k_event_post(&connection_events, NETWORK_UP);
	}

	if (event == NET_EVENT_L4_DISCONNECTED) {
		/* Mark network as down. */
		dk_set_led(LTE_LED_NUM, 0);
		LOG_INF("Network connectivity lost!");
	}
}

static int setup(void)
{
	int err = 0;

	print_reset_reason();

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)", err);
		return err;
	}

	/* Set the LEDs off after all modules are ready */
	err = dk_set_leds(0);
	if (err) {
		LOG_ERR("Failed to set LEDs off");
		return err;
	}

	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_callback, l4_event_handler, EVENT_MASK);
	net_mgmt_add_event_callback(&l4_callback);

	/* Init modem */
	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Failed to initialize modem library: 0x%X", err);
		return -EFAULT;
	}

	/* Ensure device has credentials installed before proceeding */
	await_credentials();

	/* Enable the connection manager for all interfaces and allow them to connect. */
	conn_mgr_all_if_up(true);

	/* Initiate Connection */
	LOG_INF("Enabling connectivity...");
	conn_mgr_all_if_connect(true);
	k_event_wait(&connection_events, NETWORK_UP, false, K_FOREVER);

	/* Initialize nrf_cloud library. */
	struct nrf_cloud_init_param params = {
		.event_handler = cloud_event_handler,
		.fmfu_dev_inf = get_full_modem_fota_fdev(),
#ifdef CONFIG_NRF_CLOUD_FOTA_SMP
		.smp_reset_cb = nrf52840_reset_api,
#endif
		.application_version = APP_VERSION_STRING
	};

	err = nrf_cloud_init(&params);
	if (err) {
		LOG_ERR("nRF Cloud library could not be initialized, error: %d", err);
		return err;
	}

	/* Clear the disconnected flag, no longer accurate. */
	k_event_clear(&connection_events, CLOUD_DISCONNECTED);

	LOG_INF("Connecting to nRF Cloud...");
	err = nrf_cloud_connect();
	/* If we were already connected, treat as a successful connection, but do nothing. */
	if (err == NRF_CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED) {
		LOG_WRN("Already connected to nRF Cloud");
		return 0;
	}

	/* If the connection attempt fails immediately, report and exit. */
	if (err != 0) {
		LOG_ERR("Could not connect to nRF Cloud, error: %d", err);
		return err;
	}

	/* Wait for the connection to either complete or fail. */
	if (!await_connection_result(K_FOREVER)) {
		LOG_ERR("Could not connect to nRF Cloud");
		return -EIO;
	}

	/* If connection succeeded and we aren't already ready, start the readiness timeout.
	 * (Readiness check is performed by start_readiness_timeout).
	 */
	start_readiness_timeout();

	return 0;
}

int main(void)
{
	int err = 0;

	LOG_INF(SAMPLE_SIGNON_FMT, APP_VERSION_STRING);

	err = setup();
	if (err) {
		LOG_ERR("Setup failed, stopping.");
		return 0;
	}

	while (1) {
		LOG_INF("Waiting for network ready...");
		k_event_wait(&connection_events, NETWORK_UP, false, K_FOREVER);
		LOG_INF("Network is ready");

		/* Attempt to connect to nRF Cloud. */
		if (connect_cloud()) {
			LOG_DBG("Monitoring nRF Cloud connection");

			/* and then wait patiently for a connection problem. */
			k_event_wait(&connection_events, CLOUD_DISCONNECTED, false, K_FOREVER);

			LOG_INF("Disconnected from nRF Cloud");
		}

		LOG_INF("Retrying in %d seconds...", reconnect_seconds);

		/* Wait a bit before trying again. */
		k_sleep(K_SECONDS(reconnect_seconds));
	}
}
