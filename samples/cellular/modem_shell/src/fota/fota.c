/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <modem/nrf_modem_lib.h>
#include <net/fota_download.h>

#include "fota.h"
#include "link.h"
#include "mosh_print.h"

static void modem_update_apply(void)
{
	int err;

	link_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE, true);

	mosh_print("FOTA: Shutting down modem to trigger DFU update...");

	err = nrf_modem_lib_shutdown();
	if (err) {
		mosh_warn("FOTA: Failed to shut down modem, err: %d", err);
	}

	err = nrf_modem_lib_init();
	switch (err) {
	case 0:
		mosh_error("FOTA: Expected DFU result from modem library, "
			   "but no DFU was triggered");
		goto exit;
	case NRF_MODEM_DFU_RESULT_OK:
		mosh_print("FOTA: Modem firmware update successful!");
		goto restart_modem;
	case NRF_MODEM_DFU_RESULT_UUID_ERROR:
	case NRF_MODEM_DFU_RESULT_AUTH_ERROR:
		mosh_error("FOTA: Modem firmware update failed, err: 0x%08x", err);
		goto restart_modem;
	case NRF_MODEM_DFU_RESULT_HARDWARE_ERROR:
	case NRF_MODEM_DFU_RESULT_INTERNAL_ERROR:
		mosh_error("FOTA: Fatal error, modem firmware update failed, err: 0x%08x", err);
		__ASSERT(false, "Modem firmware update failed on fatal error");
	case NRF_MODEM_DFU_RESULT_VOLTAGE_LOW:
		mosh_error("FOTA: Modem firmware update cancelled due to low voltage");
		mosh_error("FOTA: Please reboot once you have sufficient voltage for the DFU");
		__ASSERT(false, "Modem firmware update cancelled due to low voltage");
	default:
		/* Modem library initialization failed. */
		mosh_error("FOTA: Fatal error, could not initialize nrf_modem_lib, err: %d", err);
		__ASSERT(false, "Could not initialize nrf_modem_lib");
	}

restart_modem:
	mosh_print("FOTA: Restarting modem...");

	err = nrf_modem_lib_shutdown();
	if (err) {
		mosh_warn("FOTA: Failed to shut down modem, err: %d", err);
	}

	err = nrf_modem_lib_init();
	__ASSERT(!err, "FOTA: Failed to initialize modem, err: %d", err);

exit:
	link_func_mode_set(LTE_LC_FUNC_MODE_NORMAL, true);
}

static const char *get_error_cause(enum fota_download_error_cause cause)
{
	switch (cause) {
	case FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED:
		return "download failed";
	case FOTA_DOWNLOAD_ERROR_CAUSE_INVALID_UPDATE:
		return "invalid update";
	default:
		return "unknown cause value";
	}
}

static void fota_download_callback(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_PROGRESS:
		mosh_print("FOTA: Progress %d%%", evt->progress);
		break;
	case FOTA_DOWNLOAD_EVT_FINISHED:
		mosh_print("FOTA: Download finished");
		modem_update_apply();
		break;
	case FOTA_DOWNLOAD_EVT_ERASE_PENDING:
		mosh_print("FOTA: Still erasing...");
		break;
	case FOTA_DOWNLOAD_EVT_ERASE_DONE:
		mosh_print("FOTA: Erasing finished");
		break;
	case FOTA_DOWNLOAD_EVT_ERROR:
		mosh_error("FOTA: Error, %s", get_error_cause(evt->cause));
		break;
	default:
		mosh_error("FOTA: Unknown event %d", evt->id);
		break;
	}
}

int fota_init(void)
{
	return fota_download_init(&fota_download_callback);
}

int fota_start(const char *host, const char *file)
{
	return fota_download_start(host, file, CONFIG_NRF_CLOUD_SEC_TAG, 0, 0);
}
