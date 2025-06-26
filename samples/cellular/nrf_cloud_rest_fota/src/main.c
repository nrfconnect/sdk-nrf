/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_fota_poll.h>
#include <net/nrf_cloud_defs.h>
#include <net/fota_download.h>
#include <date_time.h>

#include <dk_buttons_and_leds.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include "smp_reset.h"

LOG_MODULE_REGISTER(nrf_cloud_rest_fota, CONFIG_NRF_CLOUD_REST_FOTA_SAMPLE_LOG_LEVEL);

#define BTN_NUM		       CONFIG_REST_FOTA_BUTTON_EVT_NUM
#define LTE_LED_NUM	       CONFIG_REST_FOTA_LTE_LED_NUM
#define REST_FOTA_CFG_INTERVAL "fotaInterval"
#define INTERVAL_MINUTES_MAX   10080
#define REBOOT_DELAY_REQUIRED 5
#define REBOOT_DELAY_SUCCESS 10
#define REBOOT_DELAY_ERROR 30
#define LED_ON 1
#define LED_OFF 0

/* For simplicity, just build the JSON string here and add the interval with snprintk */
#define REP_CFG_TMPLT "{\"" NRF_CLOUD_JSON_KEY_REP "\":{" \
				"\"" NRF_CLOUD_JSON_KEY_CFG "\":{" \
					"\"" REST_FOTA_CFG_INTERVAL "\":%u}}}"

/* The transform is just a dotted string of the JSON keys */
#define TRANSFORM_DESIRED_CFG_INTERVAL	NRF_CLOUD_JSON_KEY_STATE "." \
					NRF_CLOUD_JSON_KEY_DES "." \
					NRF_CLOUD_JSON_KEY_CFG "." \
					REST_FOTA_CFG_INTERVAL

/* Size of the template plus three additional chars for the number value */
#define REP_CFG_BUFF_SZ (sizeof(REP_CFG_TMPLT) + 3)

/* The interval (in minutes) at which to check for FOTA jobs */
static uint32_t fota_check_rate = CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN;

/* Network states */
#define NETWORK_UP BIT(0)
#define EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

/* Semaphore to indicate a button has been pressed */
static K_EVENT_DEFINE(button_press_event);
#define BUTTON_PRESSED BIT(0)

/* Connection event */
static K_EVENT_DEFINE(connection_events);

/* nRF Cloud device ID */
static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];

/* Modem info */
static struct modem_param_info mdm_param;

#define REST_RX_BUF_SZ 2100
/* Buffer used for REST calls */
static char rx_buf[REST_RX_BUF_SZ];

/* nRF Cloud REST context */
static struct nrf_cloud_rest_context rest_ctx = {
	.connect_socket = -1,
	.keep_alive = true,
	.rx_buf = rx_buf,
	.rx_buf_len = sizeof(rx_buf),
	.fragment_size = 0,
	/* A JWT will be automatically generated using the configured nRF Cloud device ID.
	 * See CONFIG_NRF_CLOUD_REST_AUTOGEN_JWT and CONFIG_NRF_CLOUD_CLIENT_ID_SRC.
	 */
	.auth = NULL
};

static void sample_reboot(enum nrf_cloud_fota_reboot_status status);

/* FOTA support context */
static struct nrf_cloud_fota_poll_ctx fota_ctx = {
	.rest_ctx = &rest_ctx,
	.device_id = device_id,
	.reboot_fn = sample_reboot,
#if SMP_FOTA_ENABLED
	.smp_reset_cb = nrf52840_reset_api
#endif
};

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

static void get_modem_info(void)
{
	int err = modem_info_params_get(&mdm_param);

	if (err) {
		LOG_WRN("Unable to obtain modem info, error: %d", err);
	}
}

static void send_device_status(void)
{
	int err;
	/* Enable FOTA for bootloader, modem and application */
	struct nrf_cloud_svc_info_fota fota = {
		.bootloader = 1,
		.modem = 1,
		.application = 1,
		.modem_full = fota_ctx.full_modem_fota_supported,
		.smp = SMP_FOTA_ENABLED
	};

	struct nrf_cloud_svc_info svc_inf = {
		.fota = &fota,
		/* Deprecated: The "ui" section is no longer used by nRF Cloud */
		.ui = NULL
	};

	struct nrf_cloud_modem_info mdm_inf = {
		/* Include all available modem info */
		.device = NRF_CLOUD_INFO_SET,
		.network = NRF_CLOUD_INFO_SET,
		.sim = NRF_CLOUD_INFO_SET,
		/* Use the modem info already obtained */
		.mpi = &mdm_param,
		/* Include the application version */
		.application_version = CONFIG_REST_FOTA_SAMPLE_VERSION
	};

	struct nrf_cloud_device_status dev_status = {
		.modem = &mdm_inf,
		.svc = &svc_inf,
		.conn_inf = NRF_CLOUD_INFO_SET
	};

	LOG_INF("Sending device status...");
	err = nrf_cloud_rest_shadow_device_status_update(&rest_ctx, device_id, &dev_status);
	if (err) {
		LOG_ERR("Failed to send device status, FOTA not enabled; error: %d", err);
	} else {
		LOG_INF("FOTA enabled in device shadow");
	}
}

static void check_config(void)
{
	/* Ensure the reported section gets sent once */
	static bool reported_sent;
	static char cfg_buf[REP_CFG_BUFF_SZ];
	long desired_check_rate = 0;
	bool update_reported = false;
	int err;

	/* Check if there is a value in the desired config */
	err = nrf_cloud_rest_shadow_transform_request(&rest_ctx, device_id,
						      TRANSFORM_DESIRED_CFG_INTERVAL);
	if (err) {
		LOG_ERR("Failed to request desired FOTA interval, error: %u", err);
	} else if ((rest_ctx.response_len == 0) || !rest_ctx.response) {
		err = -ENOMSG;
		LOG_DBG("No desired FOTA interval exists");
	} else {
		char *endptr;

		/* Parse the desired interval */
		desired_check_rate = strtol(rest_ctx.response, &endptr, 10);
		if ((endptr == rest_ctx.response) || (errno)) {
			LOG_ERR("Failed to parse desired FOTA interval value");
			err = -EIO;
		}
	}

	if (!err) {
		/* Validate desired interval */
		LOG_INF("Desired interval: %ld", desired_check_rate);

		if (desired_check_rate > INTERVAL_MINUTES_MAX) {
			LOG_INF("Desired interval exceeds max value, setting to %d",
				INTERVAL_MINUTES_MAX);
			desired_check_rate = INTERVAL_MINUTES_MAX;
		} else if (desired_check_rate <= 0) {
			LOG_INF("Desired interval must be greater than zero, setting to %d",
				CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN);
			desired_check_rate = CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN;
		}
		/* Use the desired value */
		fota_check_rate = (uint32_t)desired_check_rate;
		/* Update the reported value if changed */
		update_reported = (fota_check_rate != desired_check_rate);
	}

	if (update_reported || !reported_sent) {
		/* Format the JSON for the reported config */
		err = snprintk(cfg_buf, REP_CFG_BUFF_SZ, REP_CFG_TMPLT, fota_check_rate);

		if ((err < 0) || (err >= REP_CFG_BUFF_SZ)) {
			LOG_ERR("Could not format reported config JSON");
		} else {
			/* Update the shadow's reported config with the interval value */
			err = nrf_cloud_rest_shadow_state_update(&rest_ctx, device_id, cfg_buf);
			if (!err) {
				reported_sent = true;
			} else {
				LOG_ERR("Failed to update reported config, error: %d", err);
			}
		}
	}
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

static int set_led(const int state)
{
	int err = dk_set_led(LTE_LED_NUM, state);

	if (err) {
		LOG_ERR("Failed to set LED, error: %d", err);
		return err;
	}
	return 0;
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & BIT(BTN_NUM - 1)) {
		LOG_DBG("Button %d pressed", BTN_NUM);
		k_event_post(&button_press_event, BUTTON_PRESSED);
	}
}

static int init_led(void)
{
	int err = dk_leds_init();

	if (err) {
		LOG_ERR("LED init failed, error: %d", err);
		return err;
	}
	(void)set_led(LED_OFF);
	return 0;
}

static int init(void)
{
	int err = 0;
	enum nrf_cloud_fota_type pending_job_type = NRF_CLOUD_FOTA_TYPE__INVALID;

	err = init_led();
	if (err) {
		LOG_ERR("LED initialization failed: %d", err);
		return err;
	}

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Failed to initialize modem library: 0x%X", err);
		return -EFAULT;
	}

	err = nrf_cloud_fota_poll_init(&fota_ctx);
	if (err) {
		LOG_ERR("FOTA support init failed: %d", err);
		return err;
	}

	/* This function may perform a reboot if a FOTA update is in progress */
	err = nrf_cloud_fota_poll_process_pending(&fota_ctx);
	if (err < 0) {
		LOG_ERR("Failed to process pending FOTA job, error: %d", err);
		return err;
	}
	pending_job_type = (enum nrf_cloud_fota_type)err;

	err = modem_info_init();
	if (err) {
		LOG_WRN("Modem info initialization failed, error: %d", err);
		return err;
	}

	err = modem_info_params_init(&mdm_param);
	if (!err) {
		LOG_INF("Application Name: %s", mdm_param.device.app_name);
		LOG_INF("nRF Connect SDK version: %s", mdm_param.device.app_version);
	} else {
		LOG_WRN("Modem info params initialization failed, error: %d", err);
	}

	err = nrf_cloud_client_id_get(device_id, sizeof(device_id));
	if (err) {
		LOG_ERR("Failed to get device ID, error: %d", err);
		return err;
	}

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Failed to initialize button: error: %d", err);
		return err;
	}

	/* Ensure device has credentials installed before proceeding */
	await_credentials();

	/* Initiate Connection */
	LOG_INF("Enabling connectivity...");
	conn_mgr_all_if_connect(true);
	k_event_wait(&connection_events, NETWORK_UP, false, K_FOREVER);

	/* Now that the device is connected to the network, get the modem info */
	get_modem_info();

	/* Modem must have valid time/date in order to generate JWTs with
	 * an expiration time
	 */
	modem_time_wait();

	return err;
}

static void sample_reboot(enum nrf_cloud_fota_reboot_status status)
{
	int seconds;

	switch (status) {
	case FOTA_REBOOT_REQUIRED:
		LOG_INF("Rebooting...");
		seconds = REBOOT_DELAY_REQUIRED;
		break;
	case FOTA_REBOOT_SUCCESS:
		seconds = REBOOT_DELAY_SUCCESS;
		LOG_INF("Rebooting in %ds to complete FOTA update...", seconds);
		break;
	case FOTA_REBOOT_SYS_ERROR:
	default:
		seconds = REBOOT_DELAY_ERROR;
		LOG_INF("Rebooting in %ds...", seconds);
	}
	(void)nrf_cloud_rest_disconnect(&rest_ctx);

	/* We must do this before we reboot, otherwise we might trip LTE boot loop
	 * prevention, and the modem will be temporarily disable itself.
	 */
	(void)lte_lc_power_off();

	LOG_PANIC();
	k_sleep(K_SECONDS(seconds));
	sys_reboot(SYS_REBOOT_COLD);
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
		(void)set_led(LED_ON);
		LOG_INF("Connected to LTE");
		k_event_post(&connection_events, NETWORK_UP);
	}

	if (event == NET_EVENT_L4_DISCONNECTED) {
		/* Network is down. */
		(void)set_led(LED_OFF);
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
	int err;

	LOG_INF("nRF Cloud REST FOTA Sample, version: %s", CONFIG_REST_FOTA_SAMPLE_VERSION);

	err = init();
	if (err) {
		LOG_ERR("Initialization failed");
		return 0;
	}

	/* Send the device status which contains HW/FW version info,
	 * details about the network connection, and FOTA service info.
	 * The FOTA service info is required by nRF Cloud to enable FOTA
	 * for the device.
	 */
	send_device_status();

	while (1) {

		/* Query for any pending FOTA jobs. If one is found, download and install
		 * it. This is a blocking operation which can take a long time.
		 * This function is likely to reboot in order to complete the FOTA update.
		 */
		while (true) {
			err = nrf_cloud_fota_poll_process(&fota_ctx);
			if (err == -ENOTRECOVERABLE) {
				sample_reboot(FOTA_REBOOT_SYS_ERROR);
			} else if ((err == -ENOENT) || (err == -EFAULT)) {
				/* A job has finished or failed, check again for another */
				continue;
			}

			break;
		}

		/* Check the configuration in the shadow to determine the FOTA check interval */
		check_config();

		/* Wait for the configured duration or a button press */
		(void)nrf_cloud_rest_disconnect(&rest_ctx);

		LOG_INF("Checking for FOTA job in %d minute(s) or when button %d is pressed",
			fota_check_rate, CONFIG_REST_FOTA_BUTTON_EVT_NUM);

		k_event_wait(&button_press_event, BUTTON_PRESSED, true, K_MINUTES(fota_check_rate));
	}
}
