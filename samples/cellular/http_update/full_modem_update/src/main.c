/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>
#include <dfu/dfu_target_full_modem.h>
#include <net/fota_download.h>
#include <zephyr/shell/shell.h>
#include <nrf_modem_bootloader.h>
#include <modem/modem_info.h>
#include <dfu/fmfu_fdev.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>
#include <string.h>
#include <update.h>

/* We assume that modem version strings (not UUID) will not be more than this */
#define MAX_MODEM_VERSION_LEN 256

static struct k_work fmfu_work;
static const struct gpio_dt_spec sw1 = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
static struct gpio_callback sw1_cb;
static const struct device *flash_dev = DEVICE_DT_GET_ONE(jedec_spi_nor);
static char modem_version[MAX_MODEM_VERSION_LEN];

/* Buffer used as temporary storage when downloading the modem firmware, and
 * when loading the modem firmware from external flash to the modem.
 */
#define FMFU_BUF_SIZE (0x1000)
static uint8_t fmfu_buf[FMFU_BUF_SIZE];

static void fmfu_button_irq_disable(void)
{
	gpio_pin_interrupt_configure_dt(&sw1, GPIO_INT_DISABLE);
}

static void fmfu_button_irq_enable(void)
{
	gpio_pin_interrupt_configure_dt(&sw1, GPIO_INT_EDGE_TO_ACTIVE);
}

void fmfu_button_pressed(const struct device *gpiob, struct gpio_callback *cb,
			 uint32_t pins)
{
	k_work_submit(&fmfu_work);
	fmfu_button_irq_disable();
}

static void apply_fmfu_from_ext_flash(bool valid_init)
{
	int err;

	printk("Applying full modem firmware update from external flash\n");

	if (valid_init) {
		err = nrf_modem_lib_shutdown();
		if (err != 0) {
			printk("nrf_modem_lib_shutdown() failed: %d\n", err);
			return;
		}
	}

	err = nrf_modem_lib_bootloader_init();
	if (err != 0) {
		printk("nrf_modem_lib_bootloader_init() failed: %d\n", err);
		return;
	}

	err = fmfu_fdev_load(fmfu_buf, sizeof(fmfu_buf), flash_dev, 0);
	if (err != 0) {
		printk("fmfu_fdev_load failed: %d\n", err);
		return;
	}

	err = nrf_modem_lib_shutdown();
	if (err != 0) {
		printk("nrf_modem_lib_shutdown() failed: %d\n", err);
		return;
	}

	err = nrf_modem_lib_init();
	if (err != 0) {
		printk("nrf_modem_lib_init() failed: %d\n", err);
		return;
	}

	printk("Modem firmware update completed.\n");
	printk("Press 'Reset' button or enter 'reset' to apply new firmware\n");
}

static void fmfu_work_cb(struct k_work *work)
{
	ARG_UNUSED(work);

	apply_fmfu_from_ext_flash(true);

	modem_info_string_get(MODEM_INFO_FW_VERSION, modem_version,
			      MODEM_INFO_MAX_RESPONSE_SIZE);
	printk("Current modem firmware version: %s\n", modem_version);
}

static int button_init(void)
{
	int err;

	if (!device_is_ready(sw1.port)) {
		printk("SW1 GPIO port not ready\n");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&sw1, GPIO_INPUT);
	if (err < 0) {
		return err;
	}

	gpio_init_callback(&sw1_cb, fmfu_button_pressed, BIT(sw1.pin));
	err = gpio_add_callback(sw1.port, &sw1_cb);
	if (err < 0) {
		printk("Unable to configure SW1 GPIO pin!\n");
		return err;
	}

	fmfu_button_irq_enable();

	return 0;
}

static bool current_version_is_0(void)
{
	return strncmp(modem_version, CONFIG_DOWNLOAD_MODEM_0_VERSION,
		       strlen(CONFIG_DOWNLOAD_MODEM_0_VERSION)) == 0;
}

static const char *get_file(void)
{
	const char *file = CONFIG_DOWNLOAD_MODEM_0_FILE;

	if (current_version_is_0()) {
		file = CONFIG_DOWNLOAD_MODEM_1_FILE;
	}

	return file;
}

void fota_dl_handler(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_ERROR:
		printk("Received error from fota_download\n");
		/* Fallthrough */
	case FOTA_DOWNLOAD_EVT_FINISHED:
		apply_fmfu_from_ext_flash(true);
		break;

	default:
		break;
	}
}

void update_start(void)
{
	int err;

	/* Functions for getting the host and file */
	err = fota_download_start(CONFIG_DOWNLOAD_HOST, get_file(), SEC_TAG,
				  0, 0);
	if (err != 0) {
		update_sample_stop();
		printk("fota_download_start() failed, err %d\n", err);
	}
}

static int num_leds(void)
{
	return current_version_is_0() ? 1 : 2;
}

static int shell_flash(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	k_work_submit(&fmfu_work);
	fmfu_button_irq_disable();

	return 0;
}

SHELL_CMD_REGISTER(flash, NULL, "For rebooting device", shell_flash);

int main(void)
{
	int err;

	printk("HTTP full modem update sample started\n");

	if (!device_is_ready(flash_dev)) {
		printk("Flash device %s not ready\n", flash_dev->name);
		return 0;
	}

	err = nrf_modem_lib_init();
	if (err) {
		printk("Failed to initialize modem lib, err: %d\n", err);
		printk("This could indicate that an earlier update failed\n");
		printk("Trying to apply modem update from ext flash\n");
		apply_fmfu_from_ext_flash(false);
	}

	k_work_init(&fmfu_work, fmfu_work_cb);

	err = button_init();
	if (err != 0) {
		printk("button_init() failed: %d\n", err);
		return 0;
	}

	err = fota_download_init(fota_dl_handler);
	if (err != 0) {
		printk("fota_download_init() failed, err %d\n", err);
		return 0;
	}

	const struct dfu_target_full_modem_params params = {
		.buf = fmfu_buf,
		.len = sizeof(fmfu_buf),
		.dev = &(struct dfu_target_fmfu_fdev){ .dev = flash_dev,
						       .offset = 0,
						       .size = 0 }
	};

	err = dfu_target_full_modem_cfg(&params);
	if (err != 0) {
		printk("dfu_target_full_modem_cfg failed: %d\n", err);
		return 0;
	}

	err = modem_info_init();
	if (err != 0) {
		printk("modem_info_init failed: %d\n", err);
		return 0;
	}

	modem_info_string_get(MODEM_INFO_FW_VERSION, modem_version,
			      MODEM_INFO_MAX_RESPONSE_SIZE);
	printk("Current modem firmware version: %s\n", modem_version);

	err = update_sample_init(&(struct update_sample_init_params){
					.update_start = update_start,
					.num_leds = num_leds(),
					.filename = get_file()
				});
	if (err != 0) {
		printk("update_sample_init() failed, err %d\n", err);
		return 0;
	}

	printk("Press Button 1 or enter 'download' to download firmware update\n");
	printk("Press Button 2 or enter 'flash' to apply modem firmware update from flash\n");

	return 0;
}
