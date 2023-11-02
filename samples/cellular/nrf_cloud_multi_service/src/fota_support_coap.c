/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <zephyr/settings/settings.h>
#include <net/nrf_cloud.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <net/fota_download.h>
#include <net/nrf_cloud_coap.h>
#include "cloud_connection.h"
#include "fota_support.h"
#include "fota_support_coap.h"

LOG_MODULE_REGISTER(fota_support_coap, CONFIG_MULTI_SERVICE_LOG_LEVEL);

#define FOTA_THREAD_DELAY_S 10

static void sample_reboot(enum nrf_cloud_fota_reboot_status status);

/* FOTA support context */
static struct nrf_cloud_fota_poll_ctx ctx = {
	.reboot_fn = sample_reboot
};

void sample_reboot(enum nrf_cloud_fota_reboot_status status)
{
	int seconds = 0;
	bool error = false;

	switch (status) {
	case FOTA_REBOOT_REQUIRED:
		LOG_INF("Rebooting...");
		seconds = PENDING_REBOOT_S;
		break;
	case FOTA_REBOOT_SUCCESS:
		seconds = FOTA_REBOOT_S;
		LOG_INF("Rebooting in %ds to complete FOTA update...", seconds);
		break;
	case FOTA_REBOOT_FAIL:
		seconds = ERROR_REBOOT_S;
		LOG_INF("Rebooting in %ds...", seconds);
		error = true;
		break;
	case FOTA_REBOOT_SYS_ERROR:
	default:
		seconds = ERROR_REBOOT_S;
		LOG_INF("Rebooting in %ds...", seconds);
		error = true;
		break;
	}
	fota_reboot(seconds, error);
}

int coap_fota_init(void)
{
	return nrf_cloud_fota_poll_init(&ctx);
}

int coap_fota_begin(void)
{
	return nrf_cloud_fota_poll_start(&ctx);
}

int coap_fota_thread_fn(void)
{
	int err;

	while (1) {
		/* Query for any pending FOTA jobs. If one is found, download and install
		 * it. This is a blocking operation which can take a long time.
		 * This function is likely to reboot in order to complete the FOTA update.
		 */
		err = nrf_cloud_fota_poll_process(&ctx);
		if (err == -EAGAIN) {
			LOG_DBG("Retrying in %d minute(s)",
				CONFIG_COAP_FOTA_JOB_CHECK_RATE_MINUTES);
			k_sleep(K_MINUTES(CONFIG_COAP_FOTA_JOB_CHECK_RATE_MINUTES));
		} else {
			k_sleep(K_SECONDS(FOTA_THREAD_DELAY_S));
		}
	}
	return 0;
}
