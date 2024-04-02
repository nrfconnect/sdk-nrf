/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_fota_poll.h>
#include <net/fota_download.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(nrf_cloud_rest_fota, CONFIG_NRF_CLOUD_REST_FOTA_SAMPLE_LOG_LEVEL);

#define BTN_NUM			CONFIG_REST_FOTA_BUTTON_EVT_NUM
#define LED_NUM			CONFIG_REST_FOTA_LED_NUM
#define JITP_REQ_WAIT_SEC	10

/* Semaphore to indicate a button has been pressed */
static K_SEM_DEFINE(button_press_sem, 0, 1);

/* Semaphore to indicate that a network connection has been established */
static K_SEM_DEFINE(lte_connected, 0, 1);

/* nRF Cloud device ID */
static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];

/* Modem info */
static struct modem_param_info mdm_param;

#define REST_RX_BUF_SZ 2100
/* Buffer used for REST calls */
static char rx_buf[REST_RX_BUF_SZ];

#define JWT_DURATION_S	(60*5)
#define JWT_BUF_SZ	900
/* Buffer used for JSON Web Tokens (JWTs) */
static char jwt[JWT_BUF_SZ];

/* nRF Cloud REST context */
static struct nrf_cloud_rest_context rest_ctx = {
	.connect_socket = -1,
	.keep_alive = true,
	.rx_buf = rx_buf,
	.rx_buf_len = sizeof(rx_buf),
	.fragment_size = 0,
	.auth = jwt
};

static void sample_reboot(enum nrf_cloud_fota_reboot_status status);

/* FOTA support context */
static struct nrf_cloud_fota_poll_ctx fota_ctx = {
	.rest_ctx = &rest_ctx,
	.device_id = device_id,
	.reboot_fn = sample_reboot
};

#if defined(CONFIG_REST_FOTA_DO_JITP)
/* Flag to indicate if the user requested JITP to be performed */
static bool jitp_requested;
#endif

int set_led(const int state)
{
#if defined(CONFIG_REST_FOTA_ENABLE_LED)
	int err = dk_set_led(LED_NUM, state);

	if (err) {
		LOG_ERR("Failed to set LED, error: %d", err);
		return err;
	}
#else
	ARG_UNUSED(state);
#endif
	return 0;
}

int init_led(void)
{
#if defined(CONFIG_REST_FOTA_ENABLE_LED)
	int err = dk_leds_init();

	if (err) {
		LOG_ERR("LED init failed, error: %d", err);
		return err;
	}
	(void)set_led(0);
#endif
	return 0;
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & BIT(BTN_NUM - 1)) {
		LOG_DBG("Button %d pressed", BTN_NUM);
		k_sem_give(&button_press_sem);
	}
}

static int generate_jwt(struct nrf_cloud_rest_context *ctx)
{
	int err = nrf_cloud_jwt_generate(JWT_DURATION_S, ctx->auth, JWT_BUF_SZ);

	if (err < 0) {
		LOG_ERR("Failed to generate JWT, error: %d", err);
		return err;
	}

	LOG_DBG("JWT:\n%s", ctx->auth);

	return 0;
}

static void modem_time_wait(void)
{
	int err;
	char time_buf[64];

	LOG_INF("Waiting for modem to acquire network time...");

	do {
		k_sleep(K_SECONDS(3));
		err = nrf_modem_at_cmd(time_buf, sizeof(time_buf), "AT%%CCLK?");
	} while (err != 0);

	LOG_INF("Network time obtained");
}

static void get_modem_info(void)
{
	int err = modem_info_params_get(&mdm_param);

	if (!err) {
		LOG_INF("Modem FW Ver: %s", mdm_param.device.modem_fw.value_string);
	} else {
		LOG_WRN("Unable to obtain modem info, error: %d", err);
	}
}

#if defined(CONFIG_REST_FOTA_DO_JITP)
static void request_jitp(void)
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

static void do_jitp(void)
{
	int ret = 0;

	LOG_INF("Performing JITP...");
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
	} else {
		LOG_ERR("Device provisioning failed");
	}
}
#endif

static void send_device_status(void)
{
	int err;
	/* Enable FOTA for bootloader, modem and application */
	struct nrf_cloud_svc_info_fota fota = {
		.bootloader = 1,
		.modem = 1,
		.application = 1,
		.modem_full = fota_ctx.full_modem_fota_supported
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

	err = generate_jwt(&rest_ctx);
	if (err) {
		LOG_ERR("Failed to generated JWT; device status not sent");
		return;
	}

	LOG_INF("Sending device status...");
	err = nrf_cloud_rest_shadow_device_status_update(&rest_ctx, device_id, &dev_status);
	if (err) {
		LOG_ERR("Failed to send device status, FOTA not enabled; error: %d", err);
	} else {
		LOG_INF("FOTA enabled in device shadow");
	}
}

int init(void)
{
	int err = init_led();
	enum nrf_cloud_fota_type pending_job_type = NRF_CLOUD_FOTA_TYPE__INVALID;

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
		LOG_ERR("Failed to set device ID, error: %d", err);
		return err;
	}

	LOG_INF("Device ID: %s", device_id);

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Failed to initialize button: error: %d", err);
		return err;
	}

#if defined(CONFIG_REST_FOTA_DO_JITP)
	/* Present option for JITP via REST if there is no pending job */
	if (pending_job_type == NRF_CLOUD_FOTA_TYPE__INVALID) {
		request_jitp();
	}
#endif

	return 0;
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

static int connect_to_network(void)
{
	int err;

	LOG_INF("Waiting for network...");

	k_sem_reset(&lte_connected);

	err = lte_lc_connect_async(lte_handler);
	if (err) {
		LOG_ERR("Failed to init modem, error: %d", err);
	} else {
		(void)k_sem_take(&lte_connected, K_FOREVER);
		(void)set_led(1);
		LOG_INF("Connected");
	}

	return err;
}

static void sample_reboot(enum nrf_cloud_fota_reboot_status status)
{
	int seconds;

	switch (status) {
	case FOTA_REBOOT_REQUIRED:
		LOG_INF("Rebooting...");
		seconds = 5;
		break;
	case FOTA_REBOOT_SUCCESS:
		seconds = 10;
		LOG_INF("Rebooting in %ds to complete FOTA update...", seconds);
		break;
	case FOTA_REBOOT_SYS_ERROR:
	default:
		seconds = 30;
		LOG_INF("Rebooting in %ds...", seconds);
	}
	(void)nrf_cloud_rest_disconnect(&rest_ctx);
	(void)lte_lc_power_off();
	k_sleep(K_SECONDS(seconds));
	sys_reboot(SYS_REBOOT_COLD);
}

static void check_credentials(void)
{
	int err = nrf_cloud_credentials_configured_check();

	if (err == -ENOTSUP) {
		LOG_ERR("Required nRF Cloud credentials were not found");
		LOG_INF("Install credentials and then reboot the device");
		k_sleep(K_FOREVER);
	} else if (err) {
		LOG_ERR("nrf_cloud_credentials_configured_check() failed, error: %d", err);
		LOG_WRN("Continuing without verifying that credentials are installed");
	}
}

int main(void)
{
	int err;

	LOG_INF("nRF Cloud REST FOTA Sample, version: %s",
		CONFIG_REST_FOTA_SAMPLE_VERSION);

	err = init();
	if (err) {
		LOG_ERR("Initialization failed");
		return 0;
	}

	/* Before connecting, ensure nRF Cloud credentials are installed */
	check_credentials();

	err = connect_to_network();
	if (err) {
		return 0;
	}

	/* Now that the device is connected to the network, get the modem info */
	get_modem_info();

	/* Modem must have valid time/date in order to generate JWTs with
	 * an expiration time
	 */
	modem_time_wait();

#if defined(CONFIG_REST_FOTA_DO_JITP)
	if (jitp_requested) {
		/* Perform JITP via REST */
		do_jitp();
	}
#endif

	/* Send the device status which contains HW/FW version info,
	 * details about the network connection, and FOTA service info.
	 * The FOTA service info is required by nRF Cloud to enable FOTA
	 * for the device.
	 */
	send_device_status();

	while (1) {

		/* Generate a JWT for each REST call */
		err = generate_jwt(&rest_ctx);
		if (err) {
			sample_reboot(FOTA_REBOOT_SYS_ERROR);
		}

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

		/* Wait for the configured duration or a button press */
		(void)nrf_cloud_rest_disconnect(&rest_ctx);

		LOG_INF("Retrying in %d minute(s) or when button %d is pressed",
			CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN,
			CONFIG_REST_FOTA_BUTTON_EVT_NUM);

		(void)k_sem_take(&button_press_sem, K_MINUTES(CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN));
	}
}
