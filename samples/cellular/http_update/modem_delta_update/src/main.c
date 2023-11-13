/*
 * Copyright (c) 2020-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/toolchain.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/shell/shell.h>

#include <nrf_errno.h>
#include <nrf_modem.h>
#include <nrf_modem_at.h>
#include <nrf_socket.h>
#include <modem/lte_lc.h>
#include <modem/modem_key_mgmt.h>
#include <modem/modem_info.h>
#include <modem/nrf_modem_lib.h>
#include <net/fota_download.h>

#define FOTA_TEST "FOTA-TEST"

#define TLS_SEC_TAG 42

#ifdef CONFIG_USE_HTTPS
#define SEC_TAG (TLS_SEC_TAG)
#else
#define SEC_TAG (-1)
#endif

/* We assume that modem version strings (not UUID) will not be more than this */
#define MAX_MODEM_VERSION_LEN 256
static char modem_version[MAX_MODEM_VERSION_LEN];

enum fota_state { IDLE, CONNECTED, UPDATE_DOWNLOAD, UPDATE_PENDING, UPDATE_APPLY, ERROR };
static volatile enum fota_state state = IDLE;

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback sw0_cb;
static struct k_work fota_work;

BUILD_ASSERT(strlen(CONFIG_SUPPORTED_BASE_VERSION), "CONFIG_SUPPORTED_BASE_VERSION not set");
BUILD_ASSERT(strlen(CONFIG_DOWNLOAD_FILE_FOTA_TEST_TO_BASE),
	     "CONFIG_DOWNLOAD_FILE_FOTA_TEST_TO_BASE not set");
BUILD_ASSERT(strlen(CONFIG_DOWNLOAD_FILE_BASE_TO_FOTA_TEST),
	     "CONFIG_DOWNLOAD_FILE_BASE_TO_FOTA_TEST not set");

static int apply_state(enum fota_state new_state);

static bool is_test_firmware(void)
{
	return strstr(modem_version, FOTA_TEST) != NULL;
}

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

static int leds_set(int num)
{
	switch (num) {
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

static void dfu_button_irq_disable(void)
{
	gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_DISABLE);
}

static void dfu_button_irq_enable(void)
{
	gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_EDGE_TO_ACTIVE);
}

static void dfu_button_pressed(const struct device *gpiob, struct gpio_callback *cb,
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

	gpio_init_callback(&sw0_cb, dfu_button_pressed, BIT(sw0.pin));

	err = gpio_add_callback(sw0.port, &sw0_cb);
	if (err < 0) {
		printk("Unable to configure SW0 GPIO pin!\n");
		return err;
	}

	return 0;
}

static void current_version_display(void)
{
	int err;
	int num_leds;

	err = modem_info_string_get(MODEM_INFO_FW_VERSION, modem_version,
				    MAX_MODEM_VERSION_LEN);
	if (err < 0) {
		printk("Failed to get modem version\n");
		return;
	}

	num_leds = is_test_firmware() ? 1 : 2;
	leds_set(num_leds);
	printk("Current modem firmware version: %s\n", modem_version);
}

void fota_dl_handler(const struct fota_download_evt *evt)
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

#if defined(CONFIG_USE_HTTPS)
static int cert_provision(void)
{
	static const char cert[] = {
		#include "../cert/AmazonRootCA1"
	};
	BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");

	int err;
	bool exists;

	err = modem_key_mgmt_exists(TLS_SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (err) {
		printk("Failed to check for certificates err %d\n", err);
		return err;
	}

	if (exists) {
		/* For the sake of simplicity we delete what is provisioned
		 * with our security tag and reprovision our certificate.
		 */
		err = modem_key_mgmt_delete(TLS_SEC_TAG,
					    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
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
#endif /* CONFIG_USE_HTTPS */

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

static int update_download(void)
{
	int err;
	const char *file;

	err = modem_info_string_get(MODEM_INFO_FW_VERSION, modem_version,
				    MAX_MODEM_VERSION_LEN);
	if (err < 0) {
		printk("Failed to get modem version\n");
		return false;
	}

	if (is_test_firmware()) {
		file = CONFIG_DOWNLOAD_FILE_FOTA_TEST_TO_BASE;
	} else {
		file = CONFIG_DOWNLOAD_FILE_BASE_TO_FOTA_TEST;
	}

	err = fota_download_init(fota_dl_handler);
	if (err) {
		printk("fota_download_init() failed, err %d\n", err);
		return err;
	}

	/* Functions for getting the host and file */
	err = fota_download_start(CONFIG_DOWNLOAD_HOST, file, SEC_TAG, 0, 0);
	if (err) {
		printk("fota_download_start() failed, err %d\n", err);
		return err;
	}

	return 0;
}

static int apply_state(enum fota_state new_state)
{
	__ASSERT(state != new_state, "State already set: %d", state);

	state = new_state;

	switch (state) {
	case IDLE:
		dfu_button_irq_disable();
		modem_configure_and_connect();
		break;
	case CONNECTED:
		dfu_button_irq_enable();
		printk("Press Button 1 or enter 'download' to download firmware update\n");
		break;
	case UPDATE_DOWNLOAD:
		dfu_button_irq_disable();
		k_work_submit(&fota_work);
		break;
	case UPDATE_PENDING:
		dfu_button_irq_enable();
		printk("Press Button 1 or enter 'apply' to apply the firmware update\n");
		break;
	case UPDATE_APPLY:
		dfu_button_irq_disable();
		k_work_submit(&fota_work);
		break;
	case ERROR:
		break;
	}

	return 0;
}

static int shell_download(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (state != CONNECTED) {
		printk("Not allowed\n");
		return -EPERM;
	}

	apply_state(UPDATE_DOWNLOAD);

	return 0;
}

SHELL_CMD_REGISTER(download, NULL, "Download modem firmware", shell_download);

static int shell_apply(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (state != UPDATE_PENDING) {
		printk("Not allowed\n");
		return -EPERM;
	}

	apply_state(UPDATE_APPLY);

	return 0;
}

SHELL_CMD_REGISTER(apply, NULL, "Apply modem firmware", shell_apply);

static void fota_work_cb(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	switch (state) {
	case UPDATE_DOWNLOAD:
		err = update_download();
		if (err) {
			apply_state(CONNECTED);
		}
		break;
	case UPDATE_APPLY:
		printk("Applying firmware update. This can take a while.\n");
		lte_lc_power_off();
		/* Re-initialize the modem to apply the update. */
		err = nrf_modem_lib_shutdown();
		if (err) {
			printk("Failed to shutdown modem, err %d\n", err);
		}

		err = nrf_modem_lib_init();
		if (err) {
			printk("Modem initialization failed, err %d\n", err);
			switch (err) {
			case -NRF_EPERM:
				printk("Modem is already initialized");
				break;
			case -NRF_EAGAIN:
				printk("Modem update failed due to low voltage. Try again later\n");
				apply_state(UPDATE_PENDING);
				return;
			default:
				printk("Reprogram the full modem firmware to recover\n");
				apply_state(ERROR);
				return;
			}
		}

		current_version_display();

		apply_state(IDLE);
		break;
	default:
		break;
	}
}

static void on_modem_lib_dfu(int dfu_res, void *ctx)
{
	switch (dfu_res) {
	case NRF_MODEM_DFU_RESULT_OK:
		printk("Modem firmware update successful!\n");
		printk("Modem is initializing with the new firmware\n");
		break;
	case NRF_MODEM_DFU_RESULT_UUID_ERROR:
	case NRF_MODEM_DFU_RESULT_AUTH_ERROR:
		printk("Modem firmware update failed\n");
		printk("Modem is initializing with the previous firmware\n");
		break;
	case NRF_MODEM_DFU_RESULT_HARDWARE_ERROR:
	case NRF_MODEM_DFU_RESULT_INTERNAL_ERROR:
		printk("Modem firmware update failed\n");
		printk("Fatal error.\n");
		break;
	case NRF_MODEM_DFU_RESULT_VOLTAGE_LOW:
		printk("Modem firmware update failed\n");
		printk("Please reboot once you have sufficient power for the DFU.\n");
		break;
	default:
		printk("Unknown error.\n");
		break;
	}
}

NRF_MODEM_LIB_ON_DFU_RES(main_dfu_hook, on_modem_lib_dfu, NULL);

int main(void)
{
	int err;

	printk("HTTP delta modem update sample started\n");

	k_work_init(&fota_work, fota_work_cb);

	err = button_init();
	if (err) {
		return err;
	}

	err = leds_init();
	if (err) {
		return err;
	}

	printk("Initializing modem library\n");

	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem library initialization failed, err %d\n", err);
		switch (err) {
		case -NRF_EPERM:
			printk("Modem is already initialized");
			break;
		case -NRF_EAGAIN:
			printk("Modem update failed due to low voltage. Try again later\n");
			apply_state(UPDATE_PENDING);
			return 0;
		default:
			printk("Reprogram the full modem firmware to recover\n");
			apply_state(ERROR);
			return err;
		}
	}

	printk("Initialized modem library\n");

	err = modem_info_init();
	if (err) {
		printk("modem_info_init failed: %d\n", err);
		return err;
	}

	current_version_display();

	if (strstr(modem_version, CONFIG_SUPPORTED_BASE_VERSION) == NULL) {
		printk("Unsupported base modem version: %s\n", modem_version);
		printk("Supported base version (set in prj.conf): %s\n",
		       CONFIG_SUPPORTED_BASE_VERSION);
		return -EINVAL;
	}

	err = modem_configure_and_connect();
	if (err) {
		printk("Modem configuration failed: %d\n", err);
		return err;
	}

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
