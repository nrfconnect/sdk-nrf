/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/dfu/mcuboot.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <nrf_modem_at.h>
#include <memfault/components.h>
#include <memfault/ports/zephyr/fota.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_defs.h>
#include <net/nrf_cloud_coap.h>
#include <net/fota_download.h>
#include <date_time.h>

#include <dk_buttons_and_leds.h>

#define COAP_SHADOW_MAX_SIZE 512

#include <app_version.h>

LOG_MODULE_REGISTER(nrf_cloud_coap_fota_sample, CONFIG_NRF_CLOUD_COAP_FOTA_SAMPLE_LOG_LEVEL);

#define BTN_NUM		       CONFIG_COAP_FOTA_BUTTON_EVT_NUM
#define LTE_LED_NUM	       CONFIG_COAP_FOTA_LTE_LED_NUM
#define FOTA_CFG_INTERVAL "fotaInterval"
#define INTERVAL_MINUTES_MAX   10080
#define LED_ON 1
#define LED_OFF 0


/* The interval (in minutes) at which to check for FOTA jobs */
static uint32_t fota_check_rate = CONFIG_COAP_FOTA_JOB_CHECK_RATE_MIN;

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

/* Custom FOTA download callback to power off LTE before rebooting to apply the
 * update. Without this, the modem may count the cold reset as an unclean boot
 * and temporarily disable LTE to protect itself.
 */
void memfault_fota_download_callback(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_FINISHED:
		LOG_INF("FOTA download complete, rebooting to apply update...");
		(void)nrf_cloud_coap_disconnect();
		(void)lte_lc_power_off();
		LOG_PANIC(); // flush logs before rebooting
		sys_reboot(SYS_REBOOT_COLD);
		break;
	case FOTA_DOWNLOAD_EVT_ERROR:
	case FOTA_DOWNLOAD_EVT_CANCELLED:
		LOG_ERR("FOTA download error: %d", evt->id);
		break;
	default:
		break;
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

static void get_modem_info(void)
{
	int err = modem_info_params_get(&mdm_param);

	if (err) {
		LOG_WRN("Unable to obtain modem info, error: %d", err);
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

	/* Confirm the running image so MCUBoot does not roll back after a
	 * successful FOTA update reboot
	 */
	if (!boot_is_img_confirmed()) {
		LOG_INF("Confirming OTA image");
		boot_write_img_confirmed();
	}

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

	memfault_device_info_dump();

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

	LOG_INF("nRF Cloud CoAP FOTA Sample, version: %s", APP_VERSION_STRING);

	err = init();
	if (err) {
		LOG_ERR("Initialization failed");
		return 0;
	}

	/* log in to nRF Cloud CoAP */
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

	while (1) {

		/* Check Memfault for a pending OTA update */
		err = memfault_zephyr_fota_start();
		if (err < 0) {
			LOG_ERR("FOTA check failed: %d", err);
		} else if (err == 0) {
			LOG_INF("Device is up to date");
		} else {
			/* Download started. memfault_fota_download_callback() will
			* reboot the device if the download completes successfully.
			*/
			LOG_INF("FOTA download started");
		}

		LOG_INF("Checking for FOTA job in %d minute(s) or when button %d is pressed",
			fota_check_rate, CONFIG_COAP_FOTA_BUTTON_EVT_NUM);

		/* Wait for the configured duration or a button press */
		k_event_wait(&button_press_event, BUTTON_PRESSED, true, K_MINUTES(fota_check_rate));
	}
}
