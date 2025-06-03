/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <modem/modem_info.h>
#include <zephyr/settings/settings.h>
#include <helpers/nrfx_reset_reason.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_log.h>
#include <net/nrf_cloud_alert.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <date_time.h>
#include <zephyr/random/random.h>

#include <app_event_manager.h>
#include <caf/events/button_event.h>
#include <caf/events/led_event.h>
#define MODULE main
#include <caf/events/module_state_event.h>

#include <zephyr/net/conn_mgr_connectivity.h>

#include "leds_def.h"

LOG_MODULE_REGISTER(nrf_cloud_rest_device_message,
		    CONFIG_NRF_CLOUD_REST_DEVICE_MESSAGE_SAMPLE_LOG_LEVEL);

/* Button number to send BUTTON event device message */
#define BTN_NUM		1
#define LTE_LED_NUM	 CONFIG_REST_DEVICE_MESSAGE_LTE_LED_NUM
#define SEND_LED_NUM	 CONFIG_REST_DEVICE_MESSAGE_SEND_LED_NUM

/* This does not match a predefined schema, but it is not a problem. */
#define SAMPLE_SIGNON_FMT "nRF Cloud REST Device Message Sample, version: %s"
#define SAMPLE_MSG_FMT	"{\"sample_message\":"\
				"\"Hello World, from the REST Device Message Sample! "\
				"Message ID: %lld\"}"
#define SAMPLE_MSG_BUF_SIZE (sizeof(SAMPLE_MSG_FMT) + 19)

/* Network states */
#define NETWORK_UP		  BIT(0)
#define EVENT_MASK		  (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

/* Semaphore to indicate a button has been pressed */
static K_EVENT_DEFINE(button_press_event);
#define BUTTON_PRESSED BIT(0)

/* Connection event */
static K_EVENT_DEFINE(connection_events);

/* nRF Cloud device ID */
static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];

#define REST_RX_BUF_SZ 2100
/* Buffer used for REST calls */
static char rx_buf[REST_RX_BUF_SZ];

/* nRF Cloud REST context */
static struct nrf_cloud_rest_context rest_ctx = {.connect_socket = -1,
						 .keep_alive = false,
						 .rx_buf = rx_buf,
						 .rx_buf_len = sizeof(rx_buf),
						 .fragment_size = 0};

/* Register a listener for application events, specifically a button event */
static bool app_event_handler(const struct app_event_header *aeh);
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, button_event);

static int send_led_event(enum led_id led_idx, const int state)
{
	if (led_idx >= LED_ID_COUNT) {
		LOG_ERR("Invalid LED ID: %d", led_idx);
		return -EINVAL;
	}

	struct led_event *event = new_led_event();

	event->led_id = led_idx;
	event->led_effect = state != 0 ? &led_effect_on : &led_effect_off;
	APP_EVENT_SUBMIT(event);

	return 0;
}

static int set_led(const int led, const int state)
{
	int err = send_led_event(led, state);
	if (err) {
		LOG_ERR("Failed to set LED %d, error: %d", led, err);
		return err;
	}

	return 0;
}

static int set_leds_off(void)
{
	int err = set_led(LTE_LED_NUM, 0);
	if (err) {
		return err;
	}

	err = set_led(SEND_LED_NUM, 0);
	if (err) {
		return err;
	}

	return 0;
}

static bool cred_check(struct nrf_cloud_credentials_status *const cs)
{
	int ret = 0;

	ret = nrf_cloud_credentials_check(cs);
	if (ret) {
		LOG_ERR("nRF Cloud credentials check failed, error: %d", ret);
		return false;
	}

	/* Since this is a REST sample, we only need two credentials:
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

static bool app_event_handler(const struct app_event_header *aeh)
{
	struct button_event *event = is_button_event(aeh) ? cast_button_event(aeh) : NULL;

	if (!event) {
		return false;
	} else if (!event->pressed) {
		return true;
	}

	const uint16_t btn_num = event->key_id + 1;

	if (btn_num == BTN_NUM) {
		LOG_DBG("Button %d pressed", btn_num);
		k_event_post(&button_press_event, BUTTON_PRESSED);
	}
	return true;
}

static int send_message(const char *const msg)
{
	int ret = 0;

	/* Turn the SEND LED on for a bit */
	set_led(SEND_LED_NUM, 1);

	LOG_INF("Sending message:'%s'", msg);
	/* Send the message to nRF Cloud */
	ret = nrf_cloud_rest_send_device_message(&rest_ctx, device_id, msg, false, NULL);
	if (ret) {
		LOG_ERR("Failed to send device message via REST: %d", ret);
	} else {
		LOG_INF("Message sent");
	}

	/* Keep that LED on for at least 100ms */
	k_sleep(K_MSEC(100));

	/* Turn the LED back off */
	set_led(SEND_LED_NUM, 0);

	return ret;
}

static void send_message_on_button(void)
{
	static unsigned int count;

	/* Wait for a button press */
	k_event_wait(&button_press_event, BUTTON_PRESSED, true, K_FOREVER);

	/* Send off a JSON message about the button press */
	/* This matches the nRF Cloud-defined schema for a "BUTTON" device message */
	rest_ctx.keep_alive = IS_ENABLED(CONFIG_REST_DEVICE_MESSAGE_KEEP_ALIVE);
	(void)send_message("{\"appId\":\"BUTTON\", \"messageType\":\"DATA\", \"data\":\"1\"}");
	rest_ctx.keep_alive = false;
	(void)nrf_cloud_rest_log_send(&rest_ctx, device_id, LOG_LEVEL_DBG,
				      "Button pressed %u times", ++count);
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

	/* Set the keep alive flag to true;
	 * connection will remain open to allow for multiple REST calls
	 */
	rest_ctx.keep_alive = true;

	reset_reason = nrfx_reset_reason_get();
	nrfx_reset_reason_clear(reset_reason);
	LOG_INF("Reset reason: 0x%x", reset_reason);

	err = nrf_cloud_rest_alert_send(&rest_ctx, device_id,
								ALERT_TYPE_DEVICE_NOW_ONLINE,
								reset_reason, NULL);
	if (err) {
		LOG_ERR("Error sending alert to cloud: %d", err);
	}

	err = nrf_cloud_rest_log_send(&rest_ctx, device_id,
							LOG_LEVEL_INF,
							SAMPLE_SIGNON_FMT,
							CONFIG_REST_DEVICE_MESSAGE_SAMPLE_VERSION);
	if (err) {
		LOG_ERR("Error sending direct log to cloud: %d", err);
	}
}

int send_hello_world_msg(void)
{
	int err = 0;
	int64_t time_now = 0;
	char buf[SAMPLE_MSG_BUF_SIZE];

	/* Get the current timestamp */
	err = date_time_now(&time_now);
	if (err) {
		LOG_ERR("Failed to get timestamp, using random number");
		sys_rand_get(&time_now, sizeof(time_now));
	}

	/* Send off a hello world message! */
	err = snprintk(buf, SAMPLE_MSG_BUF_SIZE, SAMPLE_MSG_FMT, time_now);
	if (err < 0 || err > SAMPLE_MSG_BUF_SIZE) {
		LOG_ERR("Failed to create Hello World message.");
		return err;
	}

	err = send_message(buf);
	if (err) {
		LOG_ERR("Failed to send Hello World message");
	} else {
		LOG_INF("Sent Hello World message with ID: %lld", time_now);
	}

	return err;
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

static int setup(void)
{
	int err = 0;

	print_reset_reason();

	/* Application event manager is used for button press and LED
	 * events from the common application framework (CAF) library
	 */
	err = app_event_manager_init();
	if (err) {
		LOG_ERR("Application Event Manager could not be initialized, error: %d", err);
		return err;
	}

	/* Inform the app event manager that this module is ready to receive events */
	module_set_state(MODULE_STATE_READY);

	/* Set the LEDs off after all modules are ready */
	err = set_leds_off();
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

    /* Initialize the nRF Cloud logging subsystem */
	nrf_cloud_log_init();

#if defined(CONFIG_NRF_CLOUD_LOG_BACKEND)
	nrf_cloud_log_rest_context_set(&rest_ctx, device_id);
#endif
	nrf_cloud_log_enable(nrf_cloud_log_control_get() != LOG_LEVEL_NONE);

	return 0;
}

/* Callback to track network connectivity */
static struct net_mgmt_event_callback l4_callback;
static void l4_event_handler(struct net_mgmt_event_callback *cb, uint32_t event,
			     struct net_if *iface)
{
	if ((event & EVENT_MASK) != event) {
		return;
	}

	if (event == NET_EVENT_L4_CONNECTED) {
		/* Mark network as up. */
		(void)set_led(LTE_LED_NUM, 1);
		LOG_INF("Connected to LTE");
		k_event_post(&connection_events, NETWORK_UP);
	}

	if (event == NET_EVENT_L4_DISCONNECTED) {
		/* Mark network as down. */
		(void)set_led(LTE_LED_NUM, 0);
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

	LOG_INF(SAMPLE_SIGNON_FMT, CONFIG_REST_DEVICE_MESSAGE_SAMPLE_VERSION);

	err = setup();
	if (err) {
		LOG_ERR("Setup failed, stopping.");
		return 0;
	}

	/* Send alert and log message to the cloud. */
	report_startup();

	/* Set the keep alive flag to false;
	 * connection will be closed after the next REST call
	 */
	rest_ctx.keep_alive = false;

	err = send_hello_world_msg();

	/* -EBADMSG can indicate that nRF Cloud rejected the device's JWT in the REST call */
	if (err == -EBADMSG) {
		LOG_ERR("Ensure device is provisioned or "
				"has its public key registered with nRF Cloud");
	}

	while (1) {
		send_message_on_button();
	}
}
