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
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_log.h>
#include <net/nrf_cloud_alert.h>
#include <zephyr/logging/log.h>
#include <date_time.h>
#include <zephyr/random/rand32.h>

#include <app_event_manager.h>
#include <caf/events/button_event.h>
#include <caf/events/led_event.h>
#define MODULE main
#include <caf/events/module_state_event.h>

#include "leds_def.h"

#if defined(CONFIG_NRF_PROVISIONING)
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <net/nrf_provisioning.h>
#include "nrf_provisioning_at.h"
/* Button number to check for provisioning commands */
#define BTN_NUM_PROV		2
#endif /* CONFIG_NRF_PROVISIONING */

LOG_MODULE_REGISTER(nrf_cloud_rest_device_message,
		    CONFIG_NRF_CLOUD_REST_DEVICE_MESSAGE_SAMPLE_LOG_LEVEL);

/* Button number to perform JITP or send BUTTON event device message */
#define BTN_NUM			1
#define LTE_LED_NUM		CONFIG_REST_DEVICE_MESSAGE_LTE_LED_NUM
#define SEND_LED_NUM		CONFIG_REST_DEVICE_MESSAGE_SEND_LED_NUM
#define JITP_REQ_WAIT_SEC	10

/* This does not match a predefined schema, but it is not a problem. */
#define SAMPLE_SIGNON_FMT "nRF Cloud REST Device Message Sample, version: %s"
#define SAMPLE_MSG_FMT	"{\"sample_message\":"\
			"\"Hello World, from the REST Device Message Sample! "\
			"Message ID: %lld\"}"
#define SAMPLE_MSG_BUF_SIZE (sizeof(SAMPLE_MSG_FMT) + 19)

/* Semaphore to indicate a button has been pressed */
static K_SEM_DEFINE(button_press_sem, 0, 1);

/* Semaphore to indicate that a network connection has been established */
static K_SEM_DEFINE(lte_connected, 0, 1);

/* nRF Cloud device ID */
static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];

#define REST_RX_BUF_SZ 2100
/* Buffer used for REST calls */
static char rx_buf[REST_RX_BUF_SZ];

/* nRF Cloud REST context */
static struct nrf_cloud_rest_context rest_ctx = {
	.connect_socket = -1,
	.keep_alive = false,
	.rx_buf = rx_buf,
	.rx_buf_len = sizeof(rx_buf),
	.fragment_size = 0
};

/* Flag to indicate if the user requested JITP to be performed */
static bool jitp_requested;

/* Register a listener for application events, specifically a button event */
static bool app_event_handler(const struct app_event_header *aeh);
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, button_event);

static void await_credentials(void);

#if defined(CONFIG_NRF_PROVISIONING)
static int modem_mode_cb(enum lte_lc_func_mode new_mode, void *user_data);
static void device_mode_cb(void *user_data);
static struct nrf_provisioning_dm_change dmode = { .cb = device_mode_cb };
static struct nrf_provisioning_mm_change mmode = { .cb = modem_mode_cb };

static int modem_mode_cb(enum lte_lc_func_mode new_mode, void *user_data)
{
	enum lte_lc_func_mode fmode;
	char time_buf[64];
	int ret;

	ARG_UNUSED(user_data);

	if (lte_lc_func_mode_get(&fmode)) {
		LOG_ERR("Failed to read modem functional mode");
		ret = -EFAULT;
		return ret;
	}

	if (fmode == new_mode) {
		ret = fmode;
	} else if (new_mode == LTE_LC_FUNC_MODE_NORMAL) {
		/* Use the blocking call, because in next step
		 * the service will create a socket and call connect()
		 */
		ret = lte_lc_connect();

		if (ret) {
			LOG_ERR("lte_lc_connect() failed %d", ret);
		}
		LOG_INF("Modem connection restored");

		LOG_INF("Waiting for modem to acquire network time...");

		do {
			k_sleep(K_SECONDS(3));
			ret = nrf_provisioning_at_time_get(time_buf, sizeof(time_buf));
		} while (ret != 0);

		LOG_INF("Network time obtained");
		ret = fmode;
	} else {
		ret = lte_lc_func_mode_set(new_mode);
		if (ret == 0) {
			LOG_DBG("Modem set to requested state %d", new_mode);
			ret = fmode;
		}
	}

	return ret;
}

static void device_mode_cb(void *user_data)
{
	(void)user_data;

	/* Disconnect from network gracefully */
	int ret = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);

	if (ret != 0) {
		LOG_ERR("Unable to set modem offline, error %d", ret);
	}

	LOG_INF("Provisioning done, rebooting...");
	while (log_process()) {
		;
	}

	sys_reboot(SYS_REBOOT_WARM);
}
#endif /* CONFIG_NRF_PROVISIONING */

void init_provisioning(void)
{
#if defined(CONFIG_NRF_PROVISIONING)
	LOG_INF("Initializing the nRF Provisioning library...");

	int ret = nrf_provisioning_init(&mmode, &dmode);

	if (ret) {
		LOG_ERR("Failed to initialize provisioning client, error: %d", ret);
	}

	await_credentials();
#endif
}

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
	int ret;

	ret = nrf_cloud_credentials_check(cs);
	if (ret) {
		LOG_ERR("nRF Cloud credentials check failed, error: %d", ret);
		return false;
	}

	/* Since this is a REST sample, we only need two credentials:
	 *  - a CA for the TLS connections
	 *  - a private key to sign the JWT
	 */
	return (cs->ca && cs->prv_key);
}

static void await_credentials(void)
{
	struct nrf_cloud_credentials_status cs;

	if (!cred_check(&cs)) {
		LOG_WRN("Missing required nRF Cloud credential(s) in sec tag %u:",
			cs.sec_tag);

		if (!cs.ca) {
			LOG_WRN("\t-CA Cert");
		}

		if (!cs.prv_key) {
			LOG_WRN("\t-Private Key");
		}

#if defined(CONFIG_NRF_PROVISIONING)
		LOG_INF("Waiting for credentials to be remotely provisioned...");
		/* Device will reboot when credential provisioning is complete */
		k_sleep(K_FOREVER);
#else
		LOG_ERR("Cannot continue without required credentials");
		LOG_INF("Install credentials on the device and then "
			"provision the device or register its public key with nRF Cloud.");
		LOG_INF("Reboot device when complete");
		(void)lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);
		k_sleep(K_FOREVER);
#endif
	}

	LOG_INF("nRF Cloud credentials detected");
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

	LOG_INF("Button %d pressed", btn_num);
	if (btn_num == BTN_NUM) {
		k_sem_give(&button_press_sem);
	}
#if defined(CONFIG_NRF_PROVISIONING)
	else if (btn_num == BTN_NUM_PROV) {
		nrf_provisioning_trigger_manually();
	}
#endif
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
	}

	/* Keep that LED on for at least 100ms */
	k_sleep(K_MSEC(100));

	LOG_INF("Message sent");
	/* Turn the LED back off */
	set_led(SEND_LED_NUM, 0);

	return ret;
}

static void send_message_on_button(void)
{
	static unsigned int count;

	/* Wait for a button press */
	(void)k_sem_take(&button_press_sem, K_FOREVER);

	/* Send off a JSON message about the button press */
	/* This matches the nRF Cloud-defined schema for a "BUTTON" device message */
	rest_ctx.keep_alive = true;
	(void)send_message("{\"appId\":\"BUTTON\", \"messageType\":\"DATA\", \"data\":\"1\"}");
	rest_ctx.keep_alive = false;
	(void)nrf_cloud_rest_log_send(&rest_ctx, device_id, LOG_LEVEL_DBG,
				      "Button pressed %u times", ++count);
}

static int do_jitp(void)
{
	int ret = 0;

	LOG_INF("Performing JITP...");

	/* Turn the SEND LED, indicating in-progress JITP */
	set_led(SEND_LED_NUM, 1);

	ret = nrf_cloud_rest_jitp(CONFIG_NRF_CLOUD_SEC_TAG);

	if (ret == 0) {
		LOG_INF("Waiting 30s for cloud provisioning to complete...");
		k_sleep(K_SECONDS(30));
		k_sem_reset(&button_press_sem);
		LOG_INF("Associate device with nRF Cloud account and press button %d when complete",
			BTN_NUM);
		(void)k_sem_take(&button_press_sem, K_FOREVER);
	} else if (ret == 1) {
		LOG_INF("Device already provisioned");
		ret = 0;
	} else {
		LOG_ERR("JITP device provisioning failed: %d", ret);
	}

	/* Turn LED back off */
	set_led(SEND_LED_NUM, 0);

	return ret;
}

static void modem_time_wait(void)
{
	int err;
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

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		LOG_DBG("LTE_LC_EVT_NW_REG_STATUS: %d", evt->nw_reg_status);
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		LOG_DBG("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_DBG("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}

static int connect_to_lte(void)
{
	int err;

	LOG_INF("Waiting for network...");

	k_sem_reset(&lte_connected);

	err = lte_lc_init_and_connect_async(lte_handler);
	if (err) {
		LOG_ERR("Failed to init modem, error: %d", err);
		return err;
	}

	k_sem_take(&lte_connected, K_FOREVER);
	(void)set_led(LTE_LED_NUM, 1);
	LOG_INF("Connected to LTE");
	return 0;
}


static int setup_connection(void)
{
	int err;

	/* Get the device ID */
	err = nrf_cloud_client_id_get(device_id, sizeof(device_id));
	if (err) {
		LOG_ERR("Failed to get device ID, error: %d", err);
		return err;
	}

	LOG_INF("Device ID: %s", device_id);

	/* Connect to LTE */
	err = connect_to_lte();
	if (err) {
		LOG_ERR("Failed to connect to cellular network: %d", err);
		return err;
	}

	/* Wait until we know what time it is (necessary for JSON Web Token generation) */
	modem_time_wait();

	nrf_cloud_log_init();
#if defined(CONFIG_NRF_CLOUD_LOG_BACKEND)
	nrf_cloud_log_rest_context_set(&rest_ctx, device_id);
#endif
	nrf_cloud_log_enable(nrf_cloud_log_control_get() != LOG_LEVEL_NONE);

	/* Perform JITP if enabled and requested */
	if (IS_ENABLED(CONFIG_REST_DEVICE_MESSAGE_DO_JITP) && jitp_requested) {
		err = do_jitp();
		if (err) {
			LOG_ERR("Failed to perform JITP, %d", err);
			return err;
		}
	}

	return 0;
}

static void offer_jitp(void)
{
	int ret;

	jitp_requested = false;

	k_sem_reset(&button_press_sem);

	LOG_INF("---> Press button %d to request just-in-time provisioning", BTN_NUM);
	LOG_INF("     Waiting %d seconds...", JITP_REQ_WAIT_SEC);

	ret = k_sem_take(&button_press_sem, K_SECONDS(JITP_REQ_WAIT_SEC));
	if (ret == 0) {
		jitp_requested = true;
		LOG_INF("JITP will be performed after network connection is obtained");
	} else {
		if (ret != -EAGAIN) {
			LOG_ERR("k_sem_take error: %d", ret);
		}

		LOG_INF("JITP will be skipped");
	}
}

static int init(void)
{
	int err;

	/* Application event manager is used for button press and LED
	 * events from the common application framework (CAF) library
	 */
	err = app_event_manager_init();
	if (err) {
		LOG_ERR("Application Event Manager could not be initialized, error: %d", err);
		return err;
	}

	/* Init modem */
	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Failed to initialize modem library: 0x%X", err);
		return -EFAULT;
	}

	/* If provisioning library is disabled, ensure device has credentials installed
	 * before proceeding
	 */
	if (!IS_ENABLED(CONFIG_NRF_PROVISIONING)) {
		await_credentials();
	}

	/* Inform the app event manager that this module is ready to receive events */
	module_set_state(MODULE_STATE_READY);

	/* Set the LEDs off after all modules are ready */
	err = set_leds_off();
	if (err) {
		LOG_ERR("Failed to set LEDs off");
		return err;
	}

	return 0;
}

static int setup(void)
{
	int err;

	/* Initialize libraries and hardware */
	err = init();
	if (err) {
		LOG_ERR("Initialization failed.");
		return err;
	}

	/* Ask user if JITP via REST is desired */
	if (IS_ENABLED(CONFIG_REST_DEVICE_MESSAGE_DO_JITP)) {
		offer_jitp();
	}

	/* Initiate Connection */
	err = setup_connection();
	if (err) {
		LOG_ERR("Connection set-up failed.");
		return err;
	}

	init_provisioning();

	return 0;
}

int send_hello_world_msg(void)
{
	int err;
	int64_t time_now;
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

int main(void)
{
	int err;

	LOG_INF(SAMPLE_SIGNON_FMT,
		CONFIG_REST_DEVICE_MESSAGE_SAMPLE_VERSION);

	err = setup();
	if (err) {
		LOG_ERR("Setup failed, stopping.");
		return 0;
	}

	/* Set the keep alive flag to true;
	 * connection will remain open to allow for multiple REST calls
	 */
	rest_ctx.keep_alive = true;
	err = nrf_cloud_rest_alert_send(&rest_ctx, device_id,
					ALERT_TYPE_DEVICE_NOW_ONLINE, 0, NULL);

	if (err) {
		LOG_ERR("Error sending alert to cloud: %d", err);
	}

	err = nrf_cloud_rest_log_send(&rest_ctx, device_id, LOG_LEVEL_INF,
				      SAMPLE_SIGNON_FMT,
				      CONFIG_REST_DEVICE_MESSAGE_SAMPLE_VERSION);

	if (err) {
		LOG_ERR("Error sending direct log to cloud: %d", err);
	}

	/* Set the keep alive flag to false; connection will be closed after the next REST call */
	rest_ctx.keep_alive = false;

	err = send_hello_world_msg();

	/* -EBADMSG can indicate that nRF Cloud rejected the device's JWT in the REST call */
	if (err == -EBADMSG) {
		LOG_ERR("Ensure device is provisioned or "
			"has its public key registered with nRF Cloud");
	}

	k_sem_reset(&button_press_sem);

	while (1) {
		send_message_on_button();
	}
}
