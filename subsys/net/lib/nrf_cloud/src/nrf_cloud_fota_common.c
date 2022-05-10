/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <nrf_modem.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/dfu/mcuboot.h>
#if defined(CONFIG_FOTA_DOWNLOAD)
#include <net/fota_download.h>
#endif
#include <zephyr/logging/log.h>
#include <net/nrf_cloud.h>

LOG_MODULE_REGISTER(nrf_cloud_fota_common, CONFIG_NRF_CLOUD_LOG_LEVEL);

int nrf_cloud_bootloader_fota_slot_set(struct nrf_cloud_settings_fota_job * const job)
{
	int err = -ENOTSUP;

	if (!job) {
		return -EINVAL;
	}

	/* Only set the slot flag once for bootloader updates */
	if (job->type == NRF_CLOUD_FOTA_BOOTLOADER &&
	    !(job->bl_flags & NRF_CLOUD_FOTA_BL_STATUS_S0_FLAG_SET)) {
		bool s0_active;

#if defined(CONFIG_FOTA_DOWNLOAD)
		err = fota_download_s0_active_get(&s0_active);
#endif

		if (err) {
			LOG_ERR("Unable to determine active B1 slot, error: %d", err);
			return err;
		}

		if (s0_active) {
			job->bl_flags |= NRF_CLOUD_FOTA_BL_STATUS_S0_WAS_ACTIVE;
		} else {
			job->bl_flags &= ~NRF_CLOUD_FOTA_BL_STATUS_S0_WAS_ACTIVE;
		}

		job->bl_flags |= NRF_CLOUD_FOTA_BL_STATUS_S0_FLAG_SET;
	}

	return 0;
}

int nrf_cloud_pending_fota_job_process(struct nrf_cloud_settings_fota_job * const job,
				       bool * const reboot_required)
{
	if (!job || !reboot_required) {
		return -EINVAL;
	}

	if (job->validate != NRF_CLOUD_FOTA_VALIDATE_PENDING) {
		return -ENODEV;
	}

	int err;

	if (job->type == NRF_CLOUD_FOTA_MODEM) {
#if defined(CONFIG_NRF_MODEM_LIB)
		int modem_lib_init_result = nrf_modem_lib_get_init_ret();

		switch (modem_lib_init_result) {
		case MODEM_DFU_RESULT_OK:
			LOG_INF("Modem FOTA update confirmed");
			job->validate = NRF_CLOUD_FOTA_VALIDATE_PASS;
			break;
		case MODEM_DFU_RESULT_UUID_ERROR:
		case MODEM_DFU_RESULT_AUTH_ERROR:
		case MODEM_DFU_RESULT_HARDWARE_ERROR:
		case MODEM_DFU_RESULT_INTERNAL_ERROR:
			LOG_ERR("Modem FOTA error: %d", modem_lib_init_result);
			job->validate = NRF_CLOUD_FOTA_VALIDATE_FAIL;
			break;
		default:
			LOG_INF("Modem FOTA result unknown: %d", modem_lib_init_result);
			job->validate = NRF_CLOUD_FOTA_VALIDATE_UNKNOWN;
			break;
		}
#else
		job->validate = NRF_CLOUD_FOTA_VALIDATE_UNKNOWN;
#endif
		*reboot_required = true;

		LOG_INF("Modem FOTA update complete on reboot");

	} else if (job->type == NRF_CLOUD_FOTA_APPLICATION) {

		job->validate = NRF_CLOUD_FOTA_VALIDATE_UNKNOWN;

#if defined(CONFIG_MCUBOOT_IMG_MANAGER)
		if (!boot_is_img_confirmed()) {
			err = boot_write_img_confirmed();
			if (err) {
				LOG_ERR("Application FOTA update confirmation failed: %d",
					err);
				/* If this fails then MCUBOOT will revert
				 * to the previous image on reboot
				 */
				job->validate = NRF_CLOUD_FOTA_VALIDATE_FAIL;
				*reboot_required = true;
				LOG_INF("Rebooting to revert to previous version...");
			} else {
				LOG_INF("App FOTA update confirmed");
				job->validate = NRF_CLOUD_FOTA_VALIDATE_PASS;
			}
		}
#endif
	} else if (job->type == NRF_CLOUD_FOTA_BOOTLOADER) {
		/* The first boot after the completed download will execute
		 * the old MCUBOOT image. One more reboot is required.
		 * Don't yet send confirmation to the cloud.
		 */
		if (!(job->bl_flags & NRF_CLOUD_FOTA_BL_STATUS_REBOOTED)) {
			job->bl_flags |= NRF_CLOUD_FOTA_BL_STATUS_REBOOTED;
			*reboot_required = true;
			LOG_INF("Bootloader FOTA update will be processed on reboot");
			return 0;
		}

		err = -1;

		/* Rebooted, compare active slot with previous, if set */
		if (job->bl_flags & NRF_CLOUD_FOTA_BL_STATUS_S0_FLAG_SET) {
#if defined(CONFIG_FOTA_DOWNLOAD)
			/* If the slot has changed, so has the (b1) bootloader */
			bool s0_active;
			bool s0_prev = job->bl_flags & NRF_CLOUD_FOTA_BL_STATUS_S0_WAS_ACTIVE;

			err = fota_download_s0_active_get(&s0_active);
			if (err) {
				LOG_WRN("Active slot unknown, error: %d", err);
			} else if (s0_active != s0_prev) {
				LOG_INF("Bootloader slot changed, FOTA update validated");
				job->validate = NRF_CLOUD_FOTA_VALIDATE_PASS;
			} else {
				LOG_WRN("Bootloader slot unchanged, FOTA update invalidated");
				job->validate = NRF_CLOUD_FOTA_VALIDATE_FAIL;
			}
#endif
		}

		if (err) {
			job->validate = NRF_CLOUD_FOTA_VALIDATE_UNKNOWN;
			LOG_WRN("Bootloader FOTA update complete but not validated");
		}
	}

	return 0;
}
