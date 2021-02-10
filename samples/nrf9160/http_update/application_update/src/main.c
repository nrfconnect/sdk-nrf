/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr.h>
#include <dfu/mcuboot.h>
#include <dfu/dfu_target_mcuboot.h>
#include <modem/nrf_modem_lib.h>
#include <net/fota_download.h>
#include <stdio.h>
#include "update.h"

#if CONFIG_APPLICATION_VERSION == 2
#define NUM_LEDS 2
#else
#define NUM_LEDS 1
#endif

static uint8_t fota_buf[512];

static void fota_dl_handler(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_ERROR:
		printk("Received error from fota_download\n");
		/* Fallthrough */
	case FOTA_DOWNLOAD_EVT_FINISHED:
		update_sample_done();
		break;

	default:
		break;
	}
}

static void update_start(void)
{
	int err;
	char *apn = NULL;

	err = fota_download_start(CONFIG_DOWNLOAD_HOST, CONFIG_DOWNLOAD_FILE,
				  SEC_TAG, apn, 0);
	if (err != 0) {
		update_sample_done();
		printk("fota_download_start() failed, err %d\n", err);
	}
}

void main(void)
{
	int err;

	printk("HTTP application update sample started\n");

	err = nrf_modem_lib_init(NORMAL_MODE);
	if (err) {
		printk("Failed to initialize modem library!");
		return;
	}
	/* This is needed so that MCUBoot won't revert the update */
	boot_write_img_confirmed();

	err = dfu_target_mcuboot_set_buf(fota_buf, sizeof(fota_buf));
	if (err != 0) {
		printk("dfu_target_mcuboot_set_buf() failed, err %d\n", err);
		return;
	}

	err = fota_download_init(fota_dl_handler);
	if (err != 0) {
		printk("fota_download_init() failed, err %d\n", err);
		return;
	}

	err = update_sample_init(&(struct update_sample_init_params){
					.update_start = update_start,
					.num_leds = NUM_LEDS
				});
	if (err != 0) {
		printk("update_sample_init() failed, err %d\n", err);
		return;
	}

	printk("Press Button 1 to perform application firmware update\n");
}
