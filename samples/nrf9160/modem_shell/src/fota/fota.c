/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <net/fota_download.h>

#include "fota.h"
#include "mosh_print.h"

static void system_reboot_work(struct k_work *item)
{
	ARG_UNUSED(item);

	sys_reboot(SYS_REBOOT_WARM);
}

static K_WORK_DELAYABLE_DEFINE(system_reboot, system_reboot_work);

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
		mosh_print("FOTA: Download finished, rebooting in 5 seconds...");
		k_work_schedule(&system_reboot, K_SECONDS(5));
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
