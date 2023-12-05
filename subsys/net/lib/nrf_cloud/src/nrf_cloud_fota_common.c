/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <stdbool.h>
#if defined(CONFIG_NRF_MODEM)
#include <nrf_modem.h>
#endif
#if defined(CONFIG_NRF_MODEM_LIB)
#include <modem/nrf_modem_lib.h>
#endif
#include <dfu/dfu_target_full_modem.h>
#include <dfu/fmfu_fdev.h>
#if defined(CONFIG_FOTA_DOWNLOAD)
#include <net/fota_download.h>
#endif
#include <net/nrf_cloud.h>
#include "nrf_cloud_fota.h"

#if defined(CONFIG_NRF_MODEM_LIB)
static int dfu_result;

static void on_modem_lib_dfu(int dfu_res, void *ctx)
{
	dfu_result = dfu_res;
}

NRF_MODEM_LIB_ON_DFU_RES(nrf_cloud_fota_dfu_hook, on_modem_lib_dfu, NULL);
#endif /* CONFIG_NRF_MODEM_LIB*/

LOG_MODULE_REGISTER(nrf_cloud_fota_common, CONFIG_NRF_CLOUD_LOG_LEVEL);

/* Use the settings library to store FOTA job information to flash so
 * that the job status can be updated after a reboot
 */
#if defined(CONFIG_FOTA_USE_NRF_CLOUD_SETTINGS_AREA)
#define FOTA_SETTINGS_NAME		NRF_CLOUD_SETTINGS_FULL_FOTA
#define FOTA_SETTINGS_KEY_PENDING_JOB	NRF_CLOUD_SETTINGS_FOTA_JOB
#define FOTA_SETTINGS_FULL		NRF_CLOUD_SETTINGS_FULL_FOTA_JOB
#else
#define FOTA_SETTINGS_NAME		CONFIG_FOTA_SETTINGS_NAME
#define FOTA_SETTINGS_KEY_PENDING_JOB	CONFIG_FOTA_SETTINGS_KEY_PENDING_JOB
#define FOTA_SETTINGS_FULL		FOTA_SETTINGS_NAME "/" FOTA_SETTINGS_KEY_PENDING_JOB
#endif

/* Pending job info used with the settings library */
static struct nrf_cloud_settings_fota_job *pending_job;
static K_MUTEX_DEFINE(pending_job_mutex);

static int fota_settings_set(const char *key, size_t len_rd,
			    settings_read_cb read_cb, void *cb_arg);

SETTINGS_STATIC_HANDLER_DEFINE(fota, FOTA_SETTINGS_NAME, NULL, fota_settings_set, NULL, NULL);

static int settings_init(void)
{
	static bool initd;
	int ret = 0;

	if (!initd) {
		ret = settings_subsys_init();
		if (ret) {
			LOG_ERR("Settings init failed: %d", ret);
			return ret;
		}
		initd = true;
	}

	return 0;
}

int nrf_cloud_fota_settings_load(struct nrf_cloud_settings_fota_job *job)
{
	__ASSERT_NO_MSG(job != NULL);

	int ret = settings_init();

	if (ret) {
		return ret;
	}

	/* Set the job pointer, which will be used in the settings callback
	 * to receive the stored data
	 */
	k_mutex_lock(&pending_job_mutex, K_FOREVER);
	pending_job = job;

	/* Clear the job info and set to invalid type */
	memset(pending_job, 0, sizeof(*pending_job));
	pending_job->type = NRF_CLOUD_FOTA_TYPE__INVALID;

	/* Load FOTA settings */
	ret = settings_load_subtree(settings_handler_fota.name);

	/* Done with the pointer */
	pending_job = NULL;
	k_mutex_unlock(&pending_job_mutex);

	if (ret) {
		LOG_ERR("Cannot load settings: %d", ret);
	}

	return ret;
}

static int fota_settings_set(const char *key, size_t len_rd, settings_read_cb read_cb,
				  void *cb_arg)
{
	/* Ignore if the pending job has not been set. This can occur when the
	 * base nRF Cloud settings are loaded.
	 */
	if (!pending_job) {
		return 0;
	}

	ssize_t sz;

	if (!key) {
		LOG_DBG("Key is NULL");
		return -EINVAL;
	}

	LOG_DBG("Settings key: %s, size: %d", key, len_rd);

	if (strncmp(key, FOTA_SETTINGS_KEY_PENDING_JOB, strlen(FOTA_SETTINGS_KEY_PENDING_JOB))) {
		return -ENOMSG;
	}

	if (len_rd > sizeof(*pending_job)) {
		LOG_INF("FOTA settings size larger than expected");
		len_rd = sizeof(*pending_job);
	}

	sz = read_cb(cb_arg, (void *)pending_job, len_rd);
	if (sz == 0) {
		LOG_DBG("FOTA settings key-value pair has been deleted");
		return -EIDRM;
	} else if (sz < 0) {
		LOG_ERR("FOTA settings read error: %d", sz);
		return -EIO;
	}

	if (sz == sizeof(*pending_job)) {
		LOG_INF("Saved job: %s, type: %d, validate: %d, bl: 0x%X",
			pending_job->id, pending_job->type,
			pending_job->validate, pending_job->bl_flags);
	} else {
		LOG_INF("FOTA settings size smaller than current, likely outdated");
	}

	return 0;
}

int nrf_cloud_fota_settings_save(struct nrf_cloud_settings_fota_job *job)
{
	__ASSERT_NO_MSG(job != NULL);

	int ret = settings_init();

	if (ret) {
		return ret;
	}

	ret = settings_save_one(FOTA_SETTINGS_FULL, job, sizeof(*job));

	if (ret) {
		LOG_ERR("Failed to save FOTA job to settings, error: %d", ret);
	}

	return ret;
}

#if defined(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
/* Buffer for loading/applying the full modem update */
static char fmfu_buf[CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE_BUF_SIZE];
/* Flash device for storing full modem image */
static struct dfu_target_fmfu_fdev fmfu_dev;
/* The DFU target library does not allow reconfiguration */
static bool fmfu_dev_set;

int nrf_cloud_fota_fmfu_dev_set(const struct dfu_target_fmfu_fdev *const fmfu_dev_inf)
{
	if (fmfu_dev_set) {
		LOG_DBG("Full modem FOTA flash device already set");
		return 1;
	}

	if (!IS_ENABLED(CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION)) {
		if (!fmfu_dev_inf) {
			return -EINVAL;
		}

		if (!fmfu_dev_inf->dev) {
			LOG_ERR("Flash device is NULL");
			return -ENODEV;
		}

		if (!device_is_ready(fmfu_dev_inf->dev)) {
			LOG_ERR("Flash device is not ready");
			return -EBUSY;
		}
	}

	int ret;
	const struct dfu_target_full_modem_params params = {
		.buf = fmfu_buf,
		.len = sizeof(fmfu_buf),
		/* If a partition is used, the flash device info is not needed */
		.dev = (IS_ENABLED(CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION) ?
		       NULL : (struct dfu_target_fmfu_fdev *)fmfu_dev_inf)
	};

	ret = dfu_target_full_modem_cfg(&params);
	if (ret) {
		LOG_ERR("Failed to initialize full modem FOTA: %d", ret);
		return ret;
	}

	ret = dfu_target_full_modem_fdev_get(&fmfu_dev);
	if (ret) {
		LOG_ERR("Failed to get flash device info: %d", ret);
		return ret;
	}

	fmfu_dev_set = true;

	return ret;
}

int nrf_cloud_fota_fmfu_apply(void)
{
	if (!fmfu_dev_set) {
		LOG_ERR("Flash device for full modem FOTA is not set");
		return -EACCES;
	}

	int err;

	err = nrf_modem_lib_shutdown();
	if (err != 0) {
		LOG_ERR("nrf_modem_lib_shutdown() failed: %d", err);
		return err;
	}

	err = nrf_modem_lib_bootloader_init();
	if (err != 0) {
		LOG_ERR("nrf_modem_lib_bootloader_init() failed: %d", err);
		(void)nrf_modem_lib_init();
		return err;
	}

	err = fmfu_fdev_load(fmfu_buf, sizeof(fmfu_buf), fmfu_dev.dev, fmfu_dev.offset);
	if (err != 0) {
		LOG_ERR("Failed to apply full modem update, error: %d", err);
		(void)nrf_modem_lib_init();
		return err;
	}

	err = nrf_modem_lib_shutdown();
	if (err != 0) {
		LOG_WRN("nrf_modem_lib_shutdown() failed: %d\n", err);
	}

	return 0;
}
#endif /* CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE */

static enum nrf_cloud_fota_validate_status boot_fota_validate_get(
	const enum nrf_cloud_fota_bootloader_status_flags bl_flags)
{
	enum nrf_cloud_fota_validate_status validate = NRF_CLOUD_FOTA_VALIDATE_UNKNOWN;

	/* Rebooted, compare active slot with previous, if set */
	if (bl_flags & NRF_CLOUD_FOTA_BL_STATUS_S0_FLAG_SET) {
#if defined(CONFIG_FOTA_DOWNLOAD)
		/* If the slot has changed, so has the (b1) bootloader */
		bool s0_active;
		bool s0_prev = bl_flags & NRF_CLOUD_FOTA_BL_STATUS_S0_WAS_ACTIVE;
		int err = fota_download_s0_active_get(&s0_active);

		if (err) {
			LOG_WRN("Active slot unknown, error: %d", err);
		} else if (s0_active != s0_prev) {
			LOG_INF("Bootloader slot changed, FOTA update validated");
			validate = NRF_CLOUD_FOTA_VALIDATE_PASS;
		} else {
			LOG_WRN("Bootloader slot unchanged, FOTA update invalidated");
			validate = NRF_CLOUD_FOTA_VALIDATE_FAIL;
		}
#endif
	}

	if (validate == NRF_CLOUD_FOTA_VALIDATE_UNKNOWN) {
		LOG_WRN("Bootloader FOTA update complete but not validated");
	}

	return validate;
}

static enum nrf_cloud_fota_validate_status app_fota_validate_get(void)
{
	enum nrf_cloud_fota_validate_status validate = NRF_CLOUD_FOTA_VALIDATE_UNKNOWN;

#if defined(CONFIG_MCUBOOT_IMG_MANAGER)
	if (!boot_is_img_confirmed()) {
		int err = boot_write_img_confirmed();

		if (err) {
			LOG_ERR("Application FOTA update confirmation failed: %d", err);
			/* If this fails then MCUBOOT will revert
			* to the previous image on reboot
			*/
			validate = NRF_CLOUD_FOTA_VALIDATE_FAIL;
			LOG_INF("Previous version will be restored on next boot");
		} else {
			LOG_INF("App FOTA update confirmed");
			validate = NRF_CLOUD_FOTA_VALIDATE_PASS;
		}
	}
#endif

	return validate;
}

static enum nrf_cloud_fota_validate_status modem_delta_fota_validate_get(void)
{
#if defined(CONFIG_NRF_MODEM_LIB)
	switch (dfu_result) {
	case NRF_MODEM_DFU_RESULT_OK:
		LOG_INF("Modem FOTA update confirmed");
		return NRF_CLOUD_FOTA_VALIDATE_PASS;
	case NRF_MODEM_DFU_RESULT_UUID_ERROR:
	case NRF_MODEM_DFU_RESULT_AUTH_ERROR:
		LOG_ERR("Modem FOTA error: 0x%x. Running old firmware", dfu_result);
		return NRF_CLOUD_FOTA_VALIDATE_FAIL;
	case NRF_MODEM_DFU_RESULT_HARDWARE_ERROR:
	case NRF_MODEM_DFU_RESULT_INTERNAL_ERROR:
		LOG_ERR("Modem FOTA fatal error: 0x%x. Modem failure", dfu_result);
		return NRF_CLOUD_FOTA_VALIDATE_FAIL;
	case NRF_MODEM_DFU_RESULT_VOLTAGE_LOW:
		LOG_ERR("Modem FOTA cancelled: 0x%x.", dfu_result);
		LOG_ERR("Please reboot once you have sufficient power for the DFU");
		return NRF_CLOUD_FOTA_VALIDATE_FAIL;
	default:
		/* Unknown DFU result code */
		LOG_WRN("Modem FOTA result unknown: 0x%x", dfu_result);
		break;
	}
#endif /* CONFIG_NRF_MODEM_LIB*/

	return NRF_CLOUD_FOTA_VALIDATE_UNKNOWN;
}

static enum nrf_cloud_fota_validate_status modem_full_fota_validate_get(void)
{
	int err = -1;

#if defined(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
	err = nrf_cloud_fota_fmfu_apply();
	if (err) {
		LOG_ERR("Full modem FOTA was not applied, error: %d", err);
	}
#endif

	return (err ? NRF_CLOUD_FOTA_VALIDATE_FAIL :
		      NRF_CLOUD_FOTA_VALIDATE_PASS);
}

bool nrf_cloud_fota_is_type_modem(const enum nrf_cloud_fota_type type)
{
	return ((type == NRF_CLOUD_FOTA_MODEM_DELTA) ||
		(type == NRF_CLOUD_FOTA_MODEM_FULL));
}

int nrf_cloud_bootloader_fota_slot_set(struct nrf_cloud_settings_fota_job * const job)
{
	int err = -ENOTSUP;

	if (!job) {
		return -EINVAL;
	}

	/* Only set the slot flag once for bootloader updates */
	if (job->type == NRF_CLOUD_FOTA_BOOTLOADER &&
	    !(job->bl_flags & NRF_CLOUD_FOTA_BL_STATUS_S0_FLAG_SET)) {
		bool s0_active = false;

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

	if ((job->validate != NRF_CLOUD_FOTA_VALIDATE_PENDING) ||
	    (job->type == NRF_CLOUD_FOTA_TYPE__INVALID)) {
		return -ENODEV;
	}

	if (job->type == NRF_CLOUD_FOTA_MODEM_DELTA) {
		job->validate = modem_delta_fota_validate_get();

		*reboot_required = true;
		LOG_INF("Modem (delta) FOTA complete on modem library reinit or reboot");

	} else if (job->type == NRF_CLOUD_FOTA_MODEM_FULL) {

		job->validate = modem_full_fota_validate_get();

		if (IS_ENABLED(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)) {
			*reboot_required = true;
			LOG_INF("Modem (full) FOTA complete on modem library reinit or reboot");
		} else {
			LOG_ERR("Not configured for full modem FOTA");
			return -ESRCH;
		}
	} else if (job->type == NRF_CLOUD_FOTA_APPLICATION) {
		job->validate = app_fota_validate_get();

		if (job->validate == NRF_CLOUD_FOTA_VALIDATE_FAIL) {
			*reboot_required = true;
		}
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

		job->validate = boot_fota_validate_get(job->bl_flags);
	} else {
		LOG_ERR("Unknown FOTA job type: %d", job->type);
		return -ENOENT;
	}

	return 0;
}

bool nrf_cloud_fota_is_type_enabled(const enum nrf_cloud_fota_type type)
{
	if (!IS_ENABLED(CONFIG_NRF_CLOUD_FOTA) &&
	    !IS_ENABLED(CONFIG_NRF_CLOUD_REST) &&
	    !IS_ENABLED(CONFIG_NRF_CLOUD_COAP)) {
		return false;
	}

	switch (type) {
	case NRF_CLOUD_FOTA_APPLICATION:
		return IS_ENABLED(CONFIG_BOOTLOADER_MCUBOOT);
	case NRF_CLOUD_FOTA_BOOTLOADER:
		return IS_ENABLED(CONFIG_BOOTLOADER_MCUBOOT) &&
		       IS_ENABLED(CONFIG_BUILD_S1_VARIANT) &&
		       IS_ENABLED(CONFIG_SECURE_BOOT);
	case NRF_CLOUD_FOTA_MODEM_DELTA:
		return IS_ENABLED(CONFIG_NRF_MODEM);
	case NRF_CLOUD_FOTA_MODEM_FULL:
		return IS_ENABLED(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE) &&
		       IS_ENABLED(CONFIG_NRF_MODEM);
	default:
		return false;
	}
}
