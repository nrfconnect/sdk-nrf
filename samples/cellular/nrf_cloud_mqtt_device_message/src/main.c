/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <modem/modem_info.h>
#include <zephyr/settings/settings.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <helpers/nrfx_reset_reason.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_log.h>
#include <net/nrf_cloud_alert.h>
#include <net/nrf_cloud_defs.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <date_time.h>
#include <zephyr/random/random.h>
#include <app_version.h>
#include <dk_buttons_and_leds.h>
#include "nrf_cloud_shadow.h"

LOG_MODULE_REGISTER(nrf_cloud_mqtt_device_message,
		    CONFIG_NRF_CLOUD_MQTT_DEVICE_MESSAGE_SAMPLE_LOG_LEVEL);

/* Button number to send BUTTON event device message */
#define LTE_LED_NUM  DK_LED1
#define SEND_LED_NUM DK_LED2

/* Boot message */
#define SAMPLE_SIGNON_FMT "nRF Cloud MQTT Device Message Sample, version: %s"

/* Example message */
#define CUSTOM_TOPICM_FMT "sample_message"
#define SAMPLE_MSG_FMT                                                                             \
	"\"Hello World, from the MQTT Device Message Sample! "                                     \
	"Message ID: %lld\""
#define SAMPLE_MSG_BUF_SIZE (sizeof(SAMPLE_MSG_FMT) + 19)

/* Network states */
#define NETWORK_UP  BIT(0)
#define CLOUD_READY BIT(1)
#define EVENT_MASK  (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

/* Event to indicate a button has been pressed */
static K_EVENT_DEFINE(button_press_event);
#define BUTTON_PRESSED BIT(0)

/* Connection event */
static K_EVENT_DEFINE(connection_events);

/* nRF Cloud device ID */
static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];

static bool cred_check(struct nrf_cloud_credentials_status *const cs)
{
	int ret = 0;

	ret = nrf_cloud_credentials_check(cs);
	if (ret) {
		LOG_ERR("nRF Cloud credentials check failed, error: %d", ret);
		return false;
	}

	/* Since this is a CoAP sample, we only need two credentials:
	 *  - a CA for the TLS connections
	 *  - a private key to sign the JWT
	 */

	if (!cs->ca || !cs->ca_aws || !cs->prv_key) {
		LOG_WRN("Missing required nRF Cloud credential(s) in sec tag %u:", cs->sec_tag);
	}
	if (!cs->ca || !cs->ca_aws) {
		LOG_WRN("\t-CA Cert");
	}
	if (!cs->prv_key) {
		LOG_WRN("\t-Private Key");
	}

	return (cs->ca && cs->ca_aws && cs->prv_key);
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

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {
		k_event_post(&button_press_event, BUTTON_PRESSED);
	}
}

/**
 * @brief Construct a device message object with automatically generated timestamp
 *
 * The resultant JSON object will be conformal to the General Message Schema described in the
 * application-protocols repo:
 *
 * https://github.com/nRFCloud/application-protocols
 *
 * @param msg - The object to contain the message
 * @param appid - The appId for the device message
 * @param msg_type - The messageType for the device message
 * @return int - 0 on success, negative error code otherwise.
 */
static int create_timestamped_device_message(struct nrf_cloud_obj *const msg,
					     const char *const appid,
					     const char *const msg_type)
{
	int err;
	int64_t timestamp;

	/* Acquire timestamp */
	err = date_time_now(&timestamp);
	if (err) {
		LOG_ERR("Failed to obtain current time, error %d", err);
		return -ETIME;
	}

	/* Create message object */
	err = nrf_cloud_obj_msg_init(msg, appid,
				     IS_ENABLED(CONFIG_NRF_CLOUD_COAP) ? NULL : msg_type);
	if (err) {
		LOG_ERR("Failed to initialize message with appid %s and msg type %s",
			appid, msg_type);
		return err;
	}

	/* Add timestamp to message object */
	err = nrf_cloud_obj_ts_add(msg, timestamp);
	if (err) {
		LOG_ERR("Failed to add timestamp to data message with appid %s and msg type %s",
			appid, msg_type);
		nrf_cloud_obj_free(msg);
		return err;
	}

	return 0;
}

static int send_message(struct nrf_cloud_obj *msg)
{
	int ret = 0;

	/* Turn the SEND LED on for a bit */
	dk_set_led(SEND_LED_NUM, 1);

	struct nrf_cloud_tx_data mqtt_msg = {
		.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
		.obj = msg
	};

	/* Send message */
	ret = nrf_cloud_send(&mqtt_msg);

	/* Keep that LED on for at least 100ms */
	k_sleep(K_MSEC(100));

	/* Turn the LED back off */
	dk_set_led(SEND_LED_NUM, 0);

	return ret;
}

static void send_message_on_button(void)
{
	static unsigned int count;

	/* Wait for a button press */
	k_event_wait(&button_press_event, BUTTON_PRESSED, true, K_FOREVER);

	int ret;

	NRF_CLOUD_OBJ_JSON_DEFINE(msg_obj);

	/* Create a timestamped message container object for the button press event. */
	ret = create_timestamped_device_message(&msg_obj, NRF_CLOUD_JSON_APPID_VAL_BTN,
						NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA);
	if (ret) {
		LOG_ERR("Failed to create button press message object, error: %d", ret);
		return;
	}

	/* Populate the container object with the button value. */
	ret = nrf_cloud_obj_num_add(&msg_obj, NRF_CLOUD_JSON_DATA_KEY, 0, false);
	if (ret) {
		LOG_ERR("Failed to append value to %s sample container object ",
			NRF_CLOUD_JSON_APPID_VAL_BTN);
		nrf_cloud_obj_free(&msg_obj);
		return;
	}

	(void)send_message(&msg_obj);

	(void)nrf_cloud_log_send(LOG_LEVEL_INF, "Button pressed %u times", ++count);
}

static int send_hello_world_msg(void)
{
	int err = 0;
	int64_t time_now = 0;
	char buf[SAMPLE_MSG_BUF_SIZE];

	NRF_CLOUD_OBJ_JSON_DEFINE(msg_obj);

	/* Get the current timestamp */
	err = date_time_now(&time_now);
	if (err) {
		LOG_ERR("Failed to get timestamp, using random number");
		sys_rand_get(&time_now, sizeof(time_now));
	}

	err = snprintk(buf, SAMPLE_MSG_BUF_SIZE, SAMPLE_MSG_FMT, time_now);
	if (err < 0 || err > SAMPLE_MSG_BUF_SIZE) {
		LOG_ERR("Failed to create Hello World message.");
		return err;
	}

	/* Create a timestamped message container object for the string press event. */
	err = create_timestamped_device_message(&msg_obj, CUSTOM_TOPICM_FMT,
						NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA);
	if (err) {
		LOG_ERR("Failed to create Hello World message object, error: %d", err);
		return err;
	}

	/* Populate the container object with the string value. */
	err = nrf_cloud_obj_str_add(&msg_obj, NRF_CLOUD_JSON_DATA_KEY, buf, false);
	if (err) {
		LOG_ERR("Failed to append value to %s sample container object ",
			NRF_CLOUD_JSON_APPID_VAL_BTN);
		nrf_cloud_obj_free(&msg_obj);
		return err;
	}

	/* Send off a hello world message! */
	err = send_message(&msg_obj);
	if (err) {
		LOG_ERR("Failed to send Hello World message");
	} else {
		LOG_INF("Sent Hello World message with ID: %lld", time_now);
	}

	return 0;
}

static void print_reset_reason(void)
{
	int reset_reason = 0;

	reset_reason = nrfx_reset_reason_get();
	LOG_INF("Reset reason: 0x%x", reset_reason);
}

static void report_startup(void)
{
	int err = 0;
	int reset_reason = 0;
	const char *protocol = "MQTT";

	reset_reason = nrfx_reset_reason_get();
	nrfx_reset_reason_clear(reset_reason);
	LOG_INF("Reset reason: 0x%x", reset_reason);

	err = nrf_cloud_alert_send(ALERT_TYPE_DEVICE_NOW_ONLINE,
				   reset_reason, NULL);
	if (err) {
		LOG_ERR("Error sending alert to cloud: %d", err);
	}

	err = nrf_cloud_log_send(LOG_LEVEL_INF,
				SAMPLE_SIGNON_FMT,
				APP_VERSION_STRING,
				protocol);
	if (err) {
		LOG_ERR("Error sending direct log to cloud: %d", err);
	}
}

static void modem_time_wait(void)
{
	int err = 0;
	char time_buf[64];

	LOG_INF("Waiting for modem to acquire network time...");

	do {
		k_sleep(K_SECONDS(3));

		err = nrf_modem_at_cmd(time_buf, sizeof(time_buf), "AT%%CCLK?");
		if (err) {
			LOG_DBG("AT Clock Command Error %d... Retrying in 3 seconds.", err);
		}
	} while (err != 0);

	LOG_INF("Network time obtained");
}

/* Handler for events from nRF Cloud Lib. */
static void cloud_event_handler(const struct nrf_cloud_evt *nrf_cloud_evt)
{
	int err;

	switch (nrf_cloud_evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTED");
		shadow_config_cloud_connected();
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
		break;
	case NRF_CLOUD_EVT_READY:
		LOG_DBG("NRF_CLOUD_EVT_READY");
		LOG_INF("Connection to nRF Cloud ready");
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
	case NRF_CLOUD_EVT_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_ERROR: %d", nrf_cloud_evt->status);
		break;
	case NRF_CLOUD_EVT_RX_DATA_SHADOW: {
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA_SHADOW");
		handle_shadow_event(nrf_cloud_evt->shadow);
		break;
	}
	default:
		LOG_DBG("Unknown event type: %d", nrf_cloud_evt->type);
		break;
	}
}

static int setup(void)
{
	int err = 0;

	print_reset_reason();

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)\n", err);
		return 0;
	}

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("dk_buttons_init, error: %d", err);
	}

	/* Set the LEDs off after all modules are ready */
	err = dk_set_leds(0);
	if (err) {
		LOG_ERR("Failed to set LEDs off");
		return err;
	}

	/* Init modem */
	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Failed to initialize modem library: 0x%X", err);
		return -EFAULT;
	}

	/* Ensure device has credentials installed before proceeding */
	await_credentials();

	/* Get the device ID */
	err = nrf_cloud_client_id_get(device_id, sizeof(device_id));
	if (err) {
		LOG_ERR("Failed to get device ID, error: %d", err);
		return err;
	}

	/* Initiate Connection */
	LOG_INF("Enabling connectivity...");
	conn_mgr_all_if_connect(true);
	k_event_wait(&connection_events, NETWORK_UP, false, K_FOREVER);

	/* Wait until we know what time it is (necessary for JSON Web Token generation) */
	modem_time_wait();

	/* Initialize nrf_cloud library. */
	struct nrf_cloud_init_param params = {
		.event_handler = cloud_event_handler,
		.application_version = APP_VERSION_STRING
	};

	err = nrf_cloud_init(&params);
	if (err) {
		LOG_ERR("nRF Cloud library could not be initialized, error: %d", err);
		return err;
	}

	LOG_INF("Connecting to nRF Cloud...");
	err = nrf_cloud_connect();
	k_event_wait(&connection_events, CLOUD_READY, false, K_FOREVER);
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

	/* Initialize the nRF Cloud logging subsystem */
	nrf_cloud_log_init();
	nrf_cloud_log_control_set(CONFIG_NRF_CLOUD_LOG_OUTPUT_LEVEL);

	return 0;
}

/* Callback to track network connectivity */
static struct net_mgmt_event_callback l4_callback;
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

/* Start tracking network availability */
static int prepare_network_tracking(void)
{
	net_mgmt_init_event_callback(&l4_callback, l4_event_handler, EVENT_MASK);
	net_mgmt_add_event_callback(&l4_callback);

	return 0;
}

SYS_INIT(prepare_network_tracking, APPLICATION, 0);

int main(void)
{
	int err = 0;

	LOG_INF(SAMPLE_SIGNON_FMT, APP_VERSION_STRING);

	err = setup();
	if (err) {
		LOG_ERR("Setup failed, stopping.");
		return 0;
	}

	/* Send alert and log message to nRF Cloud. */
	report_startup();

	/* Send Hello World message to nRF Cloud */
	err = send_hello_world_msg();
	if (err) {
		LOG_ERR("Sending Hello World message to nRF Cloud failed, stopping.");
		return 0;
	}

	while (1) {
		send_message_on_button();
	}
}
