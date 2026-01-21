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
#include <net/nrf_cloud_log.h>
#include <net/nrf_cloud_alert.h>
#include <net/nrf_cloud_defs.h>
#include <net/nrf_cloud_coap.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <date_time.h>
#include <zephyr/random/random.h>
#include <app_version.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(nrf_cloud_coap_device_message,
		    CONFIG_NRF_CLOUD_COAP_DEVICE_MESSAGE_SAMPLE_LOG_LEVEL);

/* Button number to send BUTTON event device message */
#define LTE_LED_NUM	DK_LED1
#define SEND_LED_NUM	DK_LED2

/* Boot message */
#define SAMPLE_SIGNON_FMT "nRF Cloud CoAP Device Message Sample, version: %s"

/* Example message */
#define SAMPLE_MSG_FMT	"{\"sample_message\":"\
				"\"Hello World, from the CoAP Device Message Sample! "\
				"Message ID: %lld\"}"
#define SAMPLE_MSG_BUF_SIZE (sizeof(SAMPLE_MSG_FMT) + 19)

/* Network states */
#define NETWORK_UP		  BIT(0)
#define EVENT_MASK		  (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

/* Event to indicate a button has been pressed */
static K_EVENT_DEFINE(button_press_event);
#define BUTTON_PRESSED BIT(0)

/* Connection event */
static K_EVENT_DEFINE(connection_events);

/* nRF Cloud device ID */
static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];

#define COAP_SHADOW_MAX_SIZE 512


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

static int send_message(const char *const msg)
{
	int ret = 0;

	/* Turn the SEND LED on for a bit */
	dk_set_led(SEND_LED_NUM, 1);

	LOG_INF("Sending message:'%s'", msg);

	/* Send the message to nRF Cloud */
	ret = nrf_cloud_coap_json_message_send(msg, false, true);
	if (ret) {
		LOG_ERR("Failed to send device message via REST: %d", ret);
	} else {
		LOG_INF("Message sent");
	}

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

	(void)send_message("{\"appId\":\"BUTTON\", \"messageType\":\"DATA\", \"data\":\"1\"}");

	(void)nrf_cloud_log_send(LOG_LEVEL_INF, "Button pressed %u times", ++count);
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


	reset_reason = nrfx_reset_reason_get();
	nrfx_reset_reason_clear(reset_reason);
	LOG_INF("Reset reason: 0x%x", reset_reason);

	err = nrf_cloud_alert_send(ALERT_TYPE_DEVICE_NOW_ONLINE,
				   reset_reason, NULL);

	if (err) {
		LOG_ERR("Error sending alert to cloud: %d", err);
	}

	(void)nrf_cloud_log_send(LOG_LEVEL_INF,
				 SAMPLE_SIGNON_FMT,
				 APP_VERSION_STRING);

	if (err) {
		LOG_ERR("Error sending direct log to cloud: %d", err);
	}
}

static int send_hello_world_msg(void)
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


static int shadow_support_coap_obj_send(struct nrf_cloud_obj *const shadow_obj, const bool reported)
{
	/* Encode the data for the cloud */
	int err = nrf_cloud_obj_cloud_encode(shadow_obj);

	/* Free the object */
	(void)nrf_cloud_obj_free(shadow_obj);

	if (!err) {
		/* Send the encoded data */
		if (!reported) {
			err = nrf_cloud_coap_shadow_desired_update(shadow_obj->encoded_data.ptr);
		} else {
			err = nrf_cloud_coap_shadow_state_update(shadow_obj->encoded_data.ptr);
		}
	} else {
		LOG_ERR("Failed to encode cloud data, err: %d", err);
		return err;
	}

	/* Free the encoded data */
	(void)nrf_cloud_obj_cloud_encoded_free(shadow_obj);

	return err;
}

static void send_initial_log_level(void)
{
	NRF_CLOUD_OBJ_JSON_DEFINE(root_obj);
	NRF_CLOUD_OBJ_JSON_DEFINE(ctrl_obj);
	int err;

	err = nrf_cloud_obj_init(&root_obj);
	if (err) {
		LOG_ERR("Failed to initialize root object: %d", err);
		goto cleanup;
	}

	err = nrf_cloud_obj_init(&ctrl_obj);
	if (err) {
		LOG_ERR("Failed to initialize config object: %d", err);
		(void)nrf_cloud_obj_free(&root_obj);
		goto cleanup;
	}

	/* Create the config object */
	err = nrf_cloud_obj_num_add(&ctrl_obj, NRF_CLOUD_JSON_KEY_LOG,
		(double)nrf_cloud_log_control_get(), false);
		if (err) {
			LOG_ERR("Failed to add reported log level, error: %d", err);
			goto cleanup;
		}

	/* Add the config object to the root object */
	err = nrf_cloud_obj_object_add(&root_obj, NRF_CLOUD_JSON_KEY_CTRL, &ctrl_obj, false);
	if (err) {
		LOG_ERR("Failed to add config object to root object: %d", err);
		goto cleanup;
	}

	/* Send the initial config */
	err = shadow_support_coap_obj_send(&root_obj, true);
	if (err) {
		LOG_ERR("Failed to send initial config, error: %d", err);
	} else {
		LOG_INF("Initial config sent");
	}
cleanup:
	(void)nrf_cloud_obj_free(&ctrl_obj);
	(void)nrf_cloud_obj_free(&root_obj);
}

static void check_desired_log_level(void)
{
	/* Ensure the reported section gets sent once */
	static bool reported_sent;
	/* Only request full shadow the first time */
	static bool request_delta;
	char buf[COAP_SHADOW_MAX_SIZE] = {0};
	size_t buf_len = sizeof(buf);
	double desired_log_level = -1;
	bool update_reported = false;
	struct nrf_cloud_data in_data = {
		.ptr = buf
	};
	struct nrf_cloud_obj delta_obj = {0};
	int err;

	LOG_INF("Checking for shadow delta...");
	err = nrf_cloud_coap_shadow_get(buf, &buf_len, false, COAP_CONTENT_FORMAT_APP_JSON);
	if (err == -EACCES) {
		LOG_DBG("Not connected yet.");
		goto exit;
	} else if (err) {
		LOG_ERR("Failed to request shadow delta: %d", err);
		goto exit;
	}
	LOG_DBG("Shadow: len:%zd, %s", strnlen(buf, buf_len), buf);
	request_delta = true;
	in_data.len = buf_len;

	if (strnlen(buf, buf_len) == 0) {
		LOG_DBG("No shadow delta available");
		goto exit;
	}

	/* Convert string into nrf_cloud_obj */
	err = nrf_cloud_coap_shadow_delta_process(&in_data, &delta_obj);
	if (err < 0) {
		LOG_ERR("Failed to process shadow delta, err: %d", err);
		goto exit;
	} else if (err == 0) {
		/* No application specific delta data */
		goto exit;
	}

	NRF_CLOUD_OBJ_JSON_DEFINE(ctrl_obj);

	/* Get the config object */
	err = nrf_cloud_obj_object_detach(&delta_obj, NRF_CLOUD_JSON_KEY_CTRL, &ctrl_obj);
	if (err == -ENODEV) {
		/* No config in the delta */
		goto exit;
	}

	/* Process incoming config */
	(void) nrf_cloud_obj_num_get(&ctrl_obj, NRF_CLOUD_JSON_KEY_LOG, &desired_log_level);

	/* Validate desired log level */
	LOG_INF("Desired log level: %d", (int)desired_log_level);

	if (desired_log_level < LOG_LEVEL_NONE || desired_log_level > LOG_LEVEL_DBG) {
		LOG_ERR("Invalid desired log level: %d", (int)desired_log_level);
		desired_log_level = CONFIG_NRF_CLOUD_COAP_DEVICE_MESSAGE_SAMPLE_LOG_LEVEL;
	} else {
		update_reported = (desired_log_level != nrf_cloud_log_control_get());
	}
	nrf_cloud_log_control_set(desired_log_level);

	/* Create config object for response */
	/* Note: this is simpler than trying to handle unsupported entries directly. */
	nrf_cloud_obj_free(&ctrl_obj);
	nrf_cloud_obj_init(&ctrl_obj);
	err = nrf_cloud_obj_num_add(&ctrl_obj, NRF_CLOUD_JSON_KEY_LOG,
				    desired_log_level, false);
	if (err) {
		LOG_ERR("Failed to add reported log level, error: %d", err);
		goto exit;
	}

	/* Add current config data */
	if (nrf_cloud_obj_object_add(&delta_obj, NRF_CLOUD_JSON_KEY_CFG, &ctrl_obj, false)) {
		goto exit;
	}

	/* Send Shadow update */
	if (update_reported || !reported_sent) {
		err = shadow_support_coap_obj_send(&delta_obj, update_reported);

		if (err) {
			LOG_ERR("Failed to send updated shadow delta, error: %d", err);
		} else {
			reported_sent = true;
			LOG_INF("Updated shadow delta sent");
		}
	}
exit:
	/* Free the objects */
	nrf_cloud_obj_free(&ctrl_obj);
	nrf_cloud_obj_free(&delta_obj);

	/* If the reported section was not sent, send the initial log level */
	if (!reported_sent) {
		send_initial_log_level();
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

	/* nRF Cloud CoAP requires login */
	err = nrf_cloud_coap_init();
	if (err) {
		LOG_ERR("Failed to initialize CoAP client: %d", err);
		return err;
	}
	err = nrf_cloud_coap_connect(NULL);
	if (err) {
		LOG_ERR("Connecting to nRF Cloud failed, error: %d", err);
		return 0;
	}

    /* Initialize the nRF Cloud logging subsystem */
	nrf_cloud_log_init();

	check_desired_log_level();

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

	/* Send alert and log message to the cloud. */
	report_startup();

	err = send_hello_world_msg();

	while (1) {
		send_message_on_button();
		check_desired_log_level();
	}
}
