/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <nrf_modem.h>
#include <modem/nrf_modem_lib.h>
#include <net/fota_download.h>
#include <modem/at_cmd.h>
#include "update.h"

#define FOTA_TEST "FOTA-TEST"

static char version[256];

static bool is_test_firmware(void)
{
	static bool version_read;
	int err;

	if (!version_read) {
		err = at_cmd_init();
		if (err != 0) {
			printk("at_cmd_init failed: %d\n", err);
			return false;
		}

		err = at_cmd_write("AT+CGMR", version, sizeof(version), NULL);
		if (err != 0) {
			printk("Unable to read modem version: %d\n", err);
			return false;
		}

		if (strstr(version, CONFIG_SUPPORTED_BASE_VERSION) == NULL) {
			printk("Unsupported base modem version: %s\n", version);
			printk("Supported base version (set in prj.conf): %s\n",
			       CONFIG_SUPPORTED_BASE_VERSION);
			return false;
		}

		version_read = true;
	}

	return strstr(version, FOTA_TEST) != NULL;
}

static const char *get_file(void)
{
	if (is_test_firmware()) {
		return CONFIG_DOWNLOAD_FILE_FOTA_TEST_TO_BASE;
	} else {
		return CONFIG_DOWNLOAD_FILE_BASE_TO_FOTA_TEST;
	}
}

void fota_dl_handler(const struct fota_download_evt *evt)
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

void update_start(void)
{
	int err;
	char *apn = NULL;

	/* Functions for getting the host and file */
	err = fota_download_start(CONFIG_DOWNLOAD_HOST, get_file(), SEC_TAG,
				  apn, 0);
	if (err != 0) {
		update_sample_done();
		printk("fota_download_start() failed, err %d\n", err);
	}
}

static int num_leds(void)
{
	if (is_test_firmware()) {
		return 2;
	} else {
		return 1;
	}
}

void main(void)
{
	int err;

	printk("HTTP delta modem update sample started\n");

	printk("Initializing modem library\n");
#if !defined(CONFIG_NRF_MODEM_LIB_SYS_INIT)
	err = nrf_modem_lib_init(NORMAL_MODE);
#else
	/* If nrf_modem_lib is initialized on post-kernel we should
	 * fetch the returned error code instead of nrf_modem_lib_init
	 */
	err = nrf_modem_lib_get_init_ret();
#endif
	switch (err) {
	case MODEM_DFU_RESULT_OK:
		printk("Modem firmware update successful!\n");
		printk("Modem will run the new firmware after reboot\n");
		k_thread_suspend(k_current_get());
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		printk("Modem firmware update failed\n");
		printk("Modem will run non-updated firmware on reboot.\n");
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		printk("Modem firmware update failed\n");
		printk("Fatal error.\n");
		break;
	case -1:
		printk("Could not initialize momdem library.\n");
		printk("Fatal error.\n");
		return;
	default:
		break;
	}
	printk("Initialized modem library\n");

	err = fota_download_init(fota_dl_handler);
	if (err != 0) {
		printk("fota_download_init() failed, err %d\n", err);
		return;
	}

	err = update_sample_init(&(struct update_sample_init_params){
					.update_start = update_start,
					.num_leds = num_leds()
				});
	if (err != 0) {
		printk("update_sample_init() failed, err %d\n", err);
		return;
	}

	printk("Current modem firmware version: %s\n", version);

	printk("Press Button 1 for modem delta update\n");
}
