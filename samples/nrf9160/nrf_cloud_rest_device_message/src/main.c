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
#include <net/nrf_cloud_alerts.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>
#include <date_time.h>
#include <zephyr/random/rand32.h>

LOG_MODULE_REGISTER(nrf_cloud_rest_device_message,
		    CONFIG_NRF_CLOUD_REST_DEVICE_MESSAGE_SAMPLE_LOG_LEVEL);

#define BTN_NUM			CONFIG_REST_DEVICE_MESSAGE_BUTTON_EVT_NUM
#define LTE_LED_NUM		CONFIG_REST_DEVICE_MESSAGE_LTE_LED_NUM
#define SEND_LED_NUM		CONFIG_REST_DEVICE_MESSAGE_SEND_LED_NUM
#define JITP_REQ_WAIT_SEC	10

/* This does not match a predefined schema, but it is not a problem. */
#define SAMPLE_MSG_FMT		"{\"sample_message\":"\
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

NRF_MODEM_LIB_ON_INIT(nrf_cloud_rest_device_message_init_hook,
		      on_modem_lib_init, NULL);

/* Initialized to value different than success (0) */
static int modem_lib_init_result = -1;

static void on_modem_lib_init(int ret, void *ctx)
{
	modem_lib_init_result = ret;
}

static int set_led(const int led, const int state)
{
	int err = dk_set_led(led, state);

	if (err) {
		LOG_ERR("Failed to set LED %d, error: %d", led, err);
		return err;
	}
	return 0;
}

static int init_leds(void)
{
	int err = dk_leds_init();

	if (err) {
		LOG_ERR("LED init failed, error: %d", err);
		return err;
	}

	err = set_led(LTE_LED_NUM, 0);
	if (err) {
		return err;
	}

	err = set_led(SEND_LED_NUM, 0);
	if (err) {
		return err;
	}

	return err;
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & BIT(BTN_NUM - 1)) {
		LOG_DBG("Button %d pressed", BTN_NUM);
		k_sem_give(&button_press_sem);
	}
}

static int send_message(const char *const msg)
{
	int ret = 0;

	/* Turn the SEND LED on for a bit */
	set_led(SEND_LED_NUM, 1);

	LOG_DBG("Sending message:'%s'", msg);
	/* Send the message to nRF Cloud */
	ret = nrf_cloud_rest_send_device_message(&rest_ctx, device_id, msg, false, NULL);
	if (ret) {
		LOG_ERR("Failed to send device message via REST: %d", ret);
	}

	/* Keep that LED on for at least 100ms */
	k_sleep(K_MSEC(100));

	LOG_DBG("Message sent");
	/* Turn the LED back off */
	set_led(SEND_LED_NUM, 0);

	return ret;
}

static void send_message_on_button(void)
{
	/* Wait for a button press */
	(void)k_sem_take(&button_press_sem, K_FOREVER);

	/* Send off a JSON message about the button press */
	/* This matches the nRF Cloud-defined schema for a "BUTTON" device message */
	(void)send_message("{\"appId\":\"BUTTON\", \"messageType\":\"DATA\", \"data\":\"1\"}");
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

	/* Init the LEDs */
	err = init_leds();
	if (err) {
		LOG_ERR("LED initialization failed");
		return err;
	}

	/* Init modem */
	if (!IS_ENABLED(CONFIG_NRF_MODEM_LIB_SYS_INIT)) {
		modem_lib_init_result = nrf_modem_lib_init();
	}
	if (modem_lib_init_result) {
		LOG_ERR("Failed to initialize modem library: 0x%X", modem_lib_init_result);
		return -EFAULT;
	}

	/* Init the button */
	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Failed to initialize button: error: %d", err);
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

void main(void)
{
	int err;

	LOG_INF("nRF Cloud REST Device Message Sample, version: %s",
		CONFIG_REST_DEVICE_MESSAGE_SAMPLE_VERSION);

	err = setup();
	if (err) {
		LOG_ERR("Setup failed, stopping.");
		return;
	}

	rest_ctx.keep_alive = true;
	(void)nrf_cloud_rest_alert_send(&rest_ctx, device_id,
					ALERT_TYPE_DEVICE_NOW_ONLINE, 0, NULL);
	rest_ctx.keep_alive = false;

	err = send_hello_world_msg();
	if (err) {
		LOG_ERR("Hello World failed, stopping.");
		return;
	}

	while (1) {
		send_message_on_button();
	}
}
