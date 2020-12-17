/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <modem/at_cmd.h>
#include <modem/nrf_modem_lib.h>
#include <dfu/dfu_target_full_modem.h>
#include <net/fota_download.h>
#include <nrf_modem_full_dfu.h>
#include <modem/modem_info.h>
#include <dfu/fmfu_fdev.h>
#include <drivers/gpio.h>
#include <stdio.h>
#include <string.h>
#include <update.h>

/* We assume that modem version strings (not UUID) will not be more than this */
#define MAX_MODEM_VERSION_LEN 256
#define EXT_FLASH_DEVICE DT_LABEL(DT_INST(0, jedec_spi_nor))
#define SW1_PIN (DT_GPIO_PIN(DT_ALIAS(sw1), gpios))
#define SW1_FLAGS (DT_GPIO_FLAGS(DT_ALIAS(sw1), gpios))

static struct k_work fmfu_work;
static const struct device *gpiob;
static struct gpio_callback gpio_cb;
static const struct device *flash_dev;
static char modem_version[MAX_MODEM_VERSION_LEN];

/* Buffer used as temporary storage when downloading the modem firmware, and
 * when loading the modem firmware from external flash to the modem.
 */
#define FMFU_BUF_SIZE (0x1000)
static uint8_t fmfu_buf[FMFU_BUF_SIZE];

static void fmfu_button_irq_disable(void)
{
	gpio_pin_interrupt_configure(gpiob, SW1_PIN, GPIO_INT_DISABLE);
}

static void fmfu_button_irq_enable(void)
{
	gpio_pin_interrupt_configure(gpiob, SW1_PIN, GPIO_INT_EDGE_TO_ACTIVE);
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

	err = nrf_modem_lib_init(FULL_DFU_MODE);
	if (err != 0) {
		printk("nrf_modem_lib_init(FULL_DFU_MODE) failed: %d\n", err);
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

	err = nrf_modem_lib_init(NORMAL_MODE);
	if (err != 0) {
		printk("nrf_modem_lib_init() failed: %d\n", err);
		return;
	}

	printk("Modem firmware update completed\n");
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

	gpiob = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(sw1), gpios));
	if (gpiob == 0) {
		printk("Nordic nRF GPIO driver was not found!\n");
		return 1;
	}
	err = gpio_pin_configure(gpiob, SW1_PIN, GPIO_INPUT | SW1_FLAGS);
	if (err == 0) {
		gpio_init_callback(&gpio_cb, fmfu_button_pressed, BIT(SW1_PIN));
		err = gpio_add_callback(gpiob, &gpio_cb);
	}
	if (err == 0) {
		fmfu_button_irq_enable();
	}
	if (err != 0) {
		printk("Unable to configure SW1 GPIO pin!\n");
		return 1;
	}
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
				  NULL, 0);
	if (err != 0) {
		update_sample_done();
		printk("fota_download_start() failed, err %d\n", err);
	}
}

static int num_leds(void)
{
	return current_version_is_0() ? 1 : 2;
}

void main(void)
{
	int err;

	printk("HTTP full modem update sample started\n");

	flash_dev = device_get_binding(EXT_FLASH_DEVICE);
	if (flash_dev == NULL) {
		printk("Failed to get flash device: %s\n", EXT_FLASH_DEVICE);
		return;
	}

	err = nrf_modem_lib_init(NORMAL_MODE);
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
		return;
	}

	err = fota_download_init(fota_dl_handler);
	if (err != 0) {
		printk("fota_download_init() failed, err %d\n", err);
		return;
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
		return;
	}

	err = at_cmd_init();
	if (err != 0) {
		printk("at_cmd_init failed: %d\n", err);
		return;
	}

	err = modem_info_init();
	if (err != 0) {
		printk("modem_info_init failed: %d\n", err);
		return;
	}

	modem_info_string_get(MODEM_INFO_FW_VERSION, modem_version,
			      MODEM_INFO_MAX_RESPONSE_SIZE);
	printk("Current modem firmware version: %s\n", modem_version);

	err = update_sample_init(&(struct update_sample_init_params){
					.update_start = update_start,
					.num_leds = num_leds()
				});
	if (err != 0) {
		printk("update_sample_init() failed, err %d\n", err);
		return;
	}

	printk("Press Button 1 to download and apply full modem firmware "
	       "update\n");
	printk("Press Button 2 to apply modem firmware update (no download)\n");
}
