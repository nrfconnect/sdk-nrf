/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/toolchain.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <dfu/dfu_target_full_modem.h>
#include <dfu/fmfu_fdev.h>
#include <nrf_errno.h>
#include <nrf_socket.h>
#include <nrf_modem_bootloader.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_info.h>
#include <modem/lte_lc.h>
#include <modem/modem_key_mgmt.h>
#include <net/fota_download.h>

#define TLS_SEC_TAG 42

#ifdef CONFIG_USE_HTTPS
#define SEC_TAG (TLS_SEC_TAG)
#else
#define SEC_TAG (-1)
#endif

/* We assume that modem version strings (not UUID) will not be more than this. */
#define MAX_MODEM_VERSION_LEN 256
static char modem_version[MAX_MODEM_VERSION_LEN];

enum fota_state { IDLE, CONNECTED, UPDATE_DOWNLOAD, UPDATE_PENDING, UPDATE_APPLY, ERROR };
static enum fota_state state = IDLE;

static const struct device *flash_dev = DEVICE_DT_GET_ONE(jedec_spi_nor);
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec sw1 = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
static struct gpio_callback sw0_cb;
static struct gpio_callback sw1_cb;
static struct k_work fota_work;

static void fota_work_cb(struct k_work *work);
static int apply_state(enum fota_state new_state);
static int modem_configure_and_connect(void);

/* Buffer used as temporary storage when downloading the modem firmware, and
 * when loading the modem firmware from external flash to the modem.
 */
#define FMFU_BUF_SIZE (0x1000)
static uint8_t fmfu_buf[FMFU_BUF_SIZE];

BUILD_ASSERT(strlen(CONFIG_DOWNLOAD_MODEM_0_VERSION), "CONFIG_DOWNLOAD_MODEM_0_VERSION not set");
BUILD_ASSERT(strlen(CONFIG_DOWNLOAD_MODEM_0_FILE), "CONFIG_DOWNLOAD_MODEM_0_FILE not set");
BUILD_ASSERT(strlen(CONFIG_DOWNLOAD_MODEM_1_FILE), "CONFIG_DOWNLOAD_MODEM_1_FILE not set");

static bool current_version_is_0(void)
{
	return strncmp(modem_version, CONFIG_DOWNLOAD_MODEM_0_VERSION,
		       strlen(CONFIG_DOWNLOAD_MODEM_0_VERSION)) == 0;
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

static void dfu_download_btn_irq_enable(void)
{
	gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_EDGE_TO_ACTIVE);
}

static void dfu_download_btn_irq_disable(void)
{
	gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_DISABLE);
}

void dfu_download_btn_pressed(const struct device *gpiob, struct gpio_callback *cb, uint32_t pins)
{
	if (state != CONNECTED) {
		return;
	}

	apply_state(UPDATE_DOWNLOAD);
}

static void dfu_apply_btn_irq_disable(void)
{
	gpio_pin_interrupt_configure_dt(&sw1, GPIO_INT_DISABLE);
}

static void dfu_apply_btn_irq_enable(void)
{
	gpio_pin_interrupt_configure_dt(&sw1, GPIO_INT_EDGE_TO_ACTIVE);
}

void dfu_apply_btn_pressed(const struct device *gpiob, struct gpio_callback *cb, uint32_t pins)
{
	if (state != IDLE && state != CONNECTED && state != UPDATE_PENDING) {
		return;
	}

	apply_state(UPDATE_APPLY);
}

static int button_init(void)
{
	int err;

	if (!device_is_ready(sw0.port)) {
		printk("SW0 GPIO port device not ready\n");
		return -ENODEV;
	}

	if (!device_is_ready(sw1.port)) {
		printk("SW1 GPIO port not ready\n");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&sw0, GPIO_INPUT);
	if (err < 0) {
		return err;
	}

	err = gpio_pin_configure_dt(&sw1, GPIO_INPUT);
	if (err < 0) {
		return err;
	}

	gpio_init_callback(&sw0_cb, dfu_download_btn_pressed, BIT(sw0.pin));
	gpio_init_callback(&sw1_cb, dfu_apply_btn_pressed, BIT(sw1.pin));

	err = gpio_add_callback(sw0.port, &sw0_cb);
	if (err < 0) {
		printk("Unable to configure SW0 GPIO pin!\n");
		return err;
	}

	err = gpio_add_callback(sw1.port, &sw1_cb);
	if (err < 0) {
		printk("Unable to configure SW1 GPIO pin!\n");
		return err;
	}

	return 0;
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

static void current_version_display(void)
{
	int err;
	int num_leds;

	err = modem_info_string_get(MODEM_INFO_FW_VERSION, modem_version,
			      MODEM_INFO_MAX_RESPONSE_SIZE);
	if (err < 0) {
		printk("Failed to get modem version\n");
		return;
	}

	printk("Current modem firmware version: %s\n", modem_version);

	num_leds = current_version_is_0() ? 1 : 2;
	leds_set(num_leds);
}

static int apply_state(enum fota_state new_state)
{
	__ASSERT(state != new_state, "State already set: %d", state);

	state = new_state;

	switch (new_state) {
	case IDLE:
		dfu_download_btn_irq_disable();
		dfu_apply_btn_irq_enable();
		modem_configure_and_connect();
		break;
	case CONNECTED:
		dfu_download_btn_irq_enable();
		dfu_apply_btn_irq_enable();
		printk("Press Button 1 or enter 'download' to download firmware update\n");
		printk("Press Button 2 or enter 'apply' to apply modem firmware update "
		       "from flash\n");
		break;
	case UPDATE_DOWNLOAD:
		dfu_download_btn_irq_disable();
		dfu_apply_btn_irq_disable();
		k_work_submit(&fota_work);
		break;
	case UPDATE_PENDING:
		dfu_apply_btn_irq_enable();
		printk("Press Button 2 or enter 'apply' to apply modem firmware update "
		       "from flash\n");
		break;
	case UPDATE_APPLY:
		dfu_download_btn_irq_disable();
		dfu_apply_btn_irq_disable();
		k_work_submit(&fota_work);
		break;
	case ERROR:
		break;
	}

	return 0;
}

static int apply_fmfu_from_ext_flash(bool valid_init)
{
	int err;

	printk("Applying full modem firmware update from external flash\n");

	if (valid_init) {
		err = nrf_modem_lib_shutdown();
		if (err != 0) {
			printk("nrf_modem_lib_shutdown() failed: %d\n", err);
			return err;
		}
	}

	err = nrf_modem_lib_bootloader_init();
	if (err != 0) {
		printk("nrf_modem_lib_bootloader_init() failed: %d\n", err);
		return err;
	}

	err = fmfu_fdev_load(fmfu_buf, sizeof(fmfu_buf), flash_dev, 0);
	if (err != 0) {
		printk("fmfu_fdev_load failed: %d\n", err);
		return err;
	}

	err = nrf_modem_lib_shutdown();
	if (err != 0) {
		printk("nrf_modem_lib_shutdown() failed: %d\n", err);
		return err;
	}

	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem library initialization failed, err %d\n", err);
		switch (err) {
		case -NRF_EPERM:
			printk("Modem is already initialized");
			break;
		default:
			printk("Reprogram the full modem firmware to recover\n");
			apply_state(UPDATE_PENDING);
			return err;
		}
	}

	printk("Modem firmware update completed.\n");

	current_version_display();

	return 0;
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

void fota_dl_handler(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_ERROR:
		printk("Received error from fota_download\n");
		/* Fallthrough */
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
	const struct dfu_target_full_modem_params params = {
		.buf = fmfu_buf,
		.len = sizeof(fmfu_buf),
		.dev = &(struct dfu_target_fmfu_fdev){
			.dev = flash_dev,
			.offset = 0,
			.size = 0
		}
	};

	err = fota_download_init(fota_dl_handler);
	if (err != 0) {
		printk("fota_download_init() failed, err %d\n", err);
		return err;
	}

	err = dfu_target_full_modem_cfg(&params);
	if (err != 0 && err != -EALREADY) {
		printk("dfu_target_full_modem_cfg failed: %d\n", err);
		return err;
	}

	err = modem_info_string_get(MODEM_INFO_FW_VERSION, modem_version,
				    MODEM_INFO_MAX_RESPONSE_SIZE);
	if (err < 0) {
		printk("Failed to get modem version\n");
		return err;
	}

	if (current_version_is_0()) {
		file = CONFIG_DOWNLOAD_MODEM_1_FILE;
	} else {
		file = CONFIG_DOWNLOAD_MODEM_0_FILE;
	}

	/* Functions for getting the host and file */
	err = fota_download_start(CONFIG_DOWNLOAD_HOST, file, SEC_TAG, 0, 0);
	if (err != 0) {
		printk("fota_download_start() failed, err %d\n", err);
		return err;
	}

	return 0;
}

static int shell_download(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	err = apply_state(UPDATE_DOWNLOAD);
	if (err) {
		printk("Could not start download, err %d\n", err);
		return err;
	}

	return 0;
}

static int shell_apply(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	err = apply_state(UPDATE_APPLY);
	if (err) {
		printk("Could not start fota, err %d\n", err);
		return err;
	}

	return 0;
}

SHELL_CMD_REGISTER(download, NULL, "Download modem DFU", shell_download);
SHELL_CMD_REGISTER(apply, NULL, "Apply modem DFU", shell_apply);

static void fota_work_cb(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	switch (state) {
	case UPDATE_DOWNLOAD:
		dfu_target_full_modem_reset();
		err = update_download();
		if (err) {
			printk("Download failed, err %d\n", err);
			apply_state(CONNECTED);
		}
		break;
	case UPDATE_APPLY:
		err = apply_fmfu_from_ext_flash(true);
		if (err) {
			printk("FMFU failed, err %d\n", err);
		}

		apply_state(IDLE);
		break;
	default:
		break;
	}
}

int main(void)
{
	int err;

	printk("HTTP full modem update sample started\n");

	if (!device_is_ready(flash_dev)) {
		printk("Flash device %s not ready\n", flash_dev->name);
		return -ENODEV;
	}

	k_work_init(&fota_work, fota_work_cb);

	err = button_init();
	if (err != 0) {
		printk("button_init() failed: %d\n", err);
		return err;
	}

	err = leds_init();
	if (err != 0) {
		printk("leds_init() failed: %d\n", err);
		return err;
	}

	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem initialization failed, err %d\n", err);
		switch (err) {
		case -NRF_EPERM:
			printk("Modem is already initialized");
			break;
		default:
			printk("Failed to initialize modem lib, err: %d\n", err);
			printk("This could indicate that an earlier update failed\n");
			printk("Reprogram the full modem firmware to recover\n");
			apply_state(UPDATE_PENDING);
			return err;
		}
	}

	err = modem_info_init();
	if (err != 0) {
		printk("modem_info_init failed: %d\n", err);
		return err;
	}

	current_version_display();

	err = modem_configure_and_connect();
	if (err) {
		printk("Modem configuration failed: %d\n", err);
		return err;
	}

	dfu_apply_btn_irq_enable();

	return 0;
}
