/*
 * Copyright (c) 2019-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/toolchain.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/shell/shell.h>

#include <dfu/dfu_target_mcuboot.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <modem/modem_key_mgmt.h>
#include <net/fota_download.h>
#include <nrf_socket.h>

#if defined(CONFIG_LWM2M_CARRIER)
#include <lwm2m_carrier.h>
#endif

#if CONFIG_APPLICATION_VERSION == 2
#define NUM_LEDS 2
#else
#define NUM_LEDS 1
#endif

#define TLS_SEC_TAG 42

#ifdef CONFIG_USE_HTTPS
#define SEC_TAG (TLS_SEC_TAG)
#else
#define SEC_TAG (-1)
#endif

enum fota_state { IDLE, CONNECTED, UPDATE_DOWNLOAD, UPDATE_PENDING, UPDATE_APPLY };
static enum fota_state state = IDLE;

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback sw0_cb;
static struct k_work fota_work;

static void fota_work_cb(struct k_work *work);
static void apply_state(enum fota_state new_state);
static int modem_configure_and_connect(void);
static int update_download(void);

/**
 * @brief Handler for LTE link control events
 */
static void lte_lc_handler(const struct lte_lc_evt *const evt)
{
	static bool connected;

	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		    (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			if (!connected) {
				break;
			}

			printk("LTE network is disconnected.\n");
			connected = false;
			if (state == CONNECTED) {
				apply_state(IDLE);
			}
			break;
		}

		connected = true;

		if (state == IDLE) {
			printk("LTE Link Connected!\n");
			apply_state(CONNECTED);
		}
		break;
	default:
		break;
	}
}

static int leds_init(void)
{
	if (!device_is_ready(led0.port)) {
		printk("Led0 GPIO port not ready\n");
		return -ENODEV;
	}

	if (!device_is_ready(led1.port)) {
		printk("Led1 GPIO port not ready\n");
		return -ENODEV;
	}

	return 0;
}

/* We use the LEDs to indicate which version of the application we have installed */
static int leds_set(int num_leds)
{
	switch (num_leds) {
	case 0:
		gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
		gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
		break;
	case 1:
		gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
		gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
		break;
	case 2:
		gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
		gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void app_dfu_btn_irq_disable(void)
{
	gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_DISABLE);
}

static void app_dfu_btn_irq_enable(void)
{
	gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_EDGE_TO_ACTIVE);
}

static void app_dfu_button_pressed(const struct device *gpiob, struct gpio_callback *cb,
			uint32_t pins)
{
	switch (state) {
	case CONNECTED:
		apply_state(UPDATE_DOWNLOAD);
		break;
	case UPDATE_PENDING:
		apply_state(UPDATE_APPLY);
		break;
	default:
		break;
	}

}

static int button_init(void)
{
	int err;

	if (!device_is_ready(sw0.port)) {
		printk("SW0 GPIO port device not ready\n");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&sw0, GPIO_INPUT);
	if (err < 0) {
		return err;
	}

	gpio_init_callback(&sw0_cb, app_dfu_button_pressed, BIT(sw0.pin));

	err = gpio_add_callback(sw0.port, &sw0_cb);
	if (err < 0) {
		printk("Unable to configure SW0 GPIO pin!\n");
		return err;
	}

	return 0;
}

static void apply_state(enum fota_state new_state)
{
	__ASSERT(state != new_state, "State already set: %d", state);

	state = new_state;

	switch (new_state) {
	case IDLE:
		app_dfu_btn_irq_disable();
#if !defined(CONFIG_LWM2M_CARRIER)
		modem_configure_and_connect();
#endif /* !CONFIG_LWM2M_CARRIER */
		break;
	case CONNECTED:
		app_dfu_btn_irq_enable();
		printk("Press Button 1 or enter 'download' to download new application firmware\n");
		break;
	case UPDATE_DOWNLOAD:
		app_dfu_btn_irq_disable();
		k_work_submit(&fota_work);
		break;
	case UPDATE_PENDING:
		app_dfu_btn_irq_enable();
		printk("Press Button 1 or enter 'apply' to apply new application firmware\n");
		break;
	case UPDATE_APPLY:
		app_dfu_btn_irq_disable();
		k_work_submit(&fota_work);
		break;
	}
}

#if defined(CONFIG_USE_HTTPS)
static int cert_provision(void)
{
	static const char cert[] = {
		#include "../cert/AmazonRootCA1"
	};
	BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");

	int err;
	bool exists;

	err = modem_key_mgmt_exists(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (err) {
		printk("Failed to check for certificates err %d\n", err);
		return err;
	}

	if (exists) {
		/* For the sake of simplicity we delete what is provisioned
		 * with our security tag and reprovision our certificate.
		 */
		err = modem_key_mgmt_delete(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			printk("Failed to delete existing certificate, err %d\n",
			       err);
		}
	}

	printk("Provisioning certificate\n");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(TLS_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert,
				   sizeof(cert) - 1);
	if (err) {
		printk("Failed to provision certificate, err %d\n", err);
		return err;
	}

	return 0;
}
#endif /* defined(CONFIG_USE_HTTPS) */

/**
 * @brief Configures modem to provide LTE link.
 */
static int modem_configure_and_connect(void)
{
	int err;

#if defined(CONFIG_USE_HTTPS)
	err = cert_provision();
	if (err) {
		printk("Could not provision root CA to %d", TLS_SEC_TAG);
		return err;
	}

#endif /* CONFIG_USE_HTTPS */

	printk("LTE Link Connecting ...\n");
	err = lte_lc_connect_async(lte_lc_handler);
	if (err) {
		printk("LTE link could not be established.");
		return err;
	}

	return 0;
}

static void fota_work_cb(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	switch (state) {
	case UPDATE_DOWNLOAD:
		err = update_download();
		if (err) {
			printk("Download failed, err %d\n", err);
			apply_state(CONNECTED);
		}
		break;
	case UPDATE_APPLY:
#if !defined(CONFIG_LWM2M_CARRIER)
		lte_lc_power_off();
#endif
		sys_reboot(SYS_REBOOT_WARM);
		break;
	default:
		break;
	}

}

static int shell_download(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (state != CONNECTED) {
		return -EPERM;
	}

	apply_state(UPDATE_DOWNLOAD);

	return 0;
}

SHELL_CMD_REGISTER(download, NULL, "For downloading app firmware update", shell_download);

static int shell_apply(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (state != UPDATE_PENDING) {
		return -EPERM;
	}

	apply_state(UPDATE_APPLY);

	return 0;
}

SHELL_CMD_REGISTER(apply, NULL, "For applying app firmware update", shell_apply);

static void fota_dl_handler(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_ERROR:
		printk("Received error from fota_download\n");
		apply_state(CONNECTED);
		break;
	case FOTA_DOWNLOAD_EVT_FINISHED:
		apply_state(UPDATE_PENDING);
		break;
	default:
		break;
	}
}

static int update_download(void)
{
	int err;
	const char *file;

	err = fota_download_init(fota_dl_handler);
	if (err) {
		printk("fota_download_init() failed, err %d\n", err);
		return 0;
	}

#if CONFIG_APPLICATION_VERSION == 2
	file = CONFIG_DOWNLOAD_FILE_V1;
#else
	file = CONFIG_DOWNLOAD_FILE_V2;
#endif

	/* Functions for getting the host and file */
	err = fota_download_start(CONFIG_DOWNLOAD_HOST, file, SEC_TAG, 0, 0);
	if (err) {
		app_dfu_btn_irq_enable();
		printk("fota_download_start() failed, err %d\n", err);
		return err;
	}

	return 0;
}

#if defined(CONFIG_LWM2M_CARRIER)

static void print_err(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_error_t *err = evt->data.error;

	static const char * const strerr[] = {
		[LWM2M_CARRIER_ERROR_NO_ERROR] =
			"No error",
		[LWM2M_CARRIER_ERROR_BOOTSTRAP] =
			"Bootstrap error",
		[LWM2M_CARRIER_ERROR_LTE_LINK_UP_FAIL] =
			"Failed to connect to the LTE network",
		[LWM2M_CARRIER_ERROR_LTE_LINK_DOWN_FAIL] =
			"Failed to disconnect from the LTE network",
		[LWM2M_CARRIER_ERROR_FOTA_FAIL] =
			"Modem firmware update failed",
		[LWM2M_CARRIER_ERROR_CONFIGURATION] =
			"Illegal object configuration detected",
		[LWM2M_CARRIER_ERROR_INIT] =
			"Initialization failure",
		[LWM2M_CARRIER_ERROR_RUN] =
			"Configuration failure",
	};

	printk("%s, reason %d\n", strerr[err->type], err->value);
}

static void print_deferred(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_deferred_t *def = evt->data.deferred;

	static const char * const strdef[] = {
		[LWM2M_CARRIER_DEFERRED_NO_REASON] =
			"No reason given",
		[LWM2M_CARRIER_DEFERRED_PDN_ACTIVATE] =
			"Failed to activate PDN",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_NO_ROUTE] =
			"No route to bootstrap server",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_CONNECT] =
			"Failed to connect to bootstrap server",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_SEQUENCE] =
			"Bootstrap sequence not completed",
		[LWM2M_CARRIER_DEFERRED_SERVER_NO_ROUTE] =
			"No route to server",
		[LWM2M_CARRIER_DEFERRED_SERVER_CONNECT] =
			"Failed to connect to server",
		[LWM2M_CARRIER_DEFERRED_SERVER_REGISTRATION] =
			"Server registration sequence not completed",
		[LWM2M_CARRIER_DEFERRED_SERVICE_UNAVAILABLE] =
			"Server in maintenance mode",
		[LWM2M_CARRIER_DEFERRED_SIM_MSISDN] =
			"Waiting for SIM MSISDN",
	};

	printk("Reason: %s, timeout: %d seconds\n", strdef[def->reason],
		def->timeout);
}

int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	int err = 0;

	switch (event->type) {
	case LWM2M_CARRIER_EVENT_LTE_LINK_UP:
		printk("LWM2M_CARRIER_EVENT_LTE_LINK_UP\n");
		err = lte_lc_connect_async(lte_lc_handler);
		break;
	case LWM2M_CARRIER_EVENT_LTE_LINK_DOWN:
		printk("LWM2M_CARRIER_EVENT_LTE_LINK_DOWN\n");
		err = lte_lc_offline();
		break;
	case LWM2M_CARRIER_EVENT_LTE_POWER_OFF:
		printk("LWM2M_CARRIER_EVENT_LTE_POWER_OFF\n");
		err = lte_lc_power_off();
		break;
	case LWM2M_CARRIER_EVENT_BOOTSTRAPPED:
		printk("LWM2M_CARRIER_EVENT_BOOTSTRAPPED\n");
		break;
	case LWM2M_CARRIER_EVENT_REGISTERED:
		printk("LWM2M_CARRIER_EVENT_REGISTERED\n");
		break;
	case LWM2M_CARRIER_EVENT_DEFERRED:
		printk("LWM2M_CARRIER_EVENT_DEFERRED\n");
		print_deferred(event);
		break;
	case LWM2M_CARRIER_EVENT_FOTA_START:
		printk("LWM2M_CARRIER_EVENT_FOTA_START\n");
		break;
	case LWM2M_CARRIER_EVENT_FOTA_SUCCESS:
		printk("LWM2M_CARRIER_EVENT_FOTA_SUCCESS\n");
		break;
	case LWM2M_CARRIER_EVENT_REBOOT:
		printk("LWM2M_CARRIER_EVENT_REBOOT\n");
		break;
	case LWM2M_CARRIER_EVENT_MODEM_INIT:
		printk("LWM2M_CARRIER_EVENT_MODEM_INIT\n");
		err = nrf_modem_lib_init();
		break;
	case LWM2M_CARRIER_EVENT_MODEM_SHUTDOWN:
		printk("LWM2M_CARRIER_EVENT_MODEM_SHUTDOWN\n");
		err = nrf_modem_lib_shutdown();
		break;
	case LWM2M_CARRIER_EVENT_ERROR:
		printk("LWM2M_CARRIER_EVENT_ERROR\n");
		print_err(event);
		break;
	}

	return err;
}
#endif /* CONFIG_LWM2M_CARRIER */

int main(void)
{
	int err;

	printk("HTTP application update sample started\n");
	printk("Using version %d\n", CONFIG_APPLICATION_VERSION);

	/* This is needed so that MCUBoot won't revert the update */
	boot_write_img_confirmed();

	err = nrf_modem_lib_init();
	if (err < 0) {
		printk("Failed to initialize modem library!\n");
		return err;
	}

	k_work_init(&fota_work, fota_work_cb);

	err = button_init();
	if (err) {
		return err;
	}

	err = leds_init();
	if (err) {
		return err;
	}

	err = modem_configure_and_connect();
	if (err) {
		printk("Modem configuration failed: %d\n", err);
		return err;
	}

	leds_set(NUM_LEDS);
	app_dfu_btn_irq_enable();

	return 0;
}
