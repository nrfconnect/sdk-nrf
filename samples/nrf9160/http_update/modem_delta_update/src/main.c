/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <nrf_modem.h>
#include <modem/nrf_modem_lib.h>
#include <net/fota_download.h>
#include <nrf_modem_at.h>
#include "update.h"

#define FOTA_TEST "FOTA-TEST"

NRF_MODEM_LIB_ON_INIT(modem_delta_update_init_hook,
		      on_modem_lib_init, NULL);

/* Initialized to value different than success (0) */
static int modem_lib_init_result = -1;

static void on_modem_lib_init(int ret, void *ctx)
{
	modem_lib_init_result = ret;
}

static char version[256];

static bool is_test_firmware(void)
{
	static bool version_read;
	int err;

	if (!version_read) {
		err = nrf_modem_at_cmd(version, sizeof(version), "AT+CGMR");
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
		update_sample_stop();
		break;

	case FOTA_DOWNLOAD_EVT_FINISHED:
		update_sample_done();
		printk("Press 'Reset' button or enter 'reset' to apply new firmware\n");
		break;

	default:
		break;
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
	err = modem_lib_init_result;
#endif
	switch (err) {
	case MODEM_DFU_RESULT_OK:
		printk("Modem firmware update successful!\n");
		printk("Modem will run the new firmware after reboot\n");
		printk("Press 'Reset' button or enter 'reset' to apply new firmware\n");
		k_thread_suspend(k_current_get());
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		printk("Modem firmware update failed\n");
		printk("Modem will run non-updated firmware on reboot.\n");
		printk("Press 'Reset' button or enter 'reset'\n");
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		printk("Modem firmware update failed\n");
		printk("Fatal error.\n");
		break;
	case MODEM_DFU_RESULT_VOLTAGE_LOW:
		printk("Modem firmware update failed\n");
		printk("Please reboot once you have sufficient power for the DFU.\n");
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
					.update_start = update_sample_start,
					.num_leds = num_leds(),
					.filename = get_file()
				});
	if (err != 0) {
		printk("update_sample_init() failed, err %d\n", err);
		return;
	}

	printk("Current modem firmware version: %s\n", version);

	printk("Press Button 1 for enter 'download' to download modem delta update\n");
}
