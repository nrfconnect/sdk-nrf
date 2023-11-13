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

#define JOB_WAIT_S		30

/* Use the settings library to store FOTA job information to flash so
 * that the job status can be updated after a reboot
 */
#if defined(CONFIG_COAP_FOTA_USE_NRF_CLOUD_SETTINGS_AREA)
#define FOTA_SETTINGS_NAME		NRF_CLOUD_SETTINGS_FULL_FOTA
#define FOTA_SETTINGS_KEY_PENDING_JOB	NRF_CLOUD_SETTINGS_FOTA_JOB
#define FOTA_SETTINGS_FULL		NRF_CLOUD_SETTINGS_FULL_FOTA_JOB
#else
#define FOTA_SETTINGS_NAME		CONFIG_COAP_FOTA_SETTINGS_NAME
#define FOTA_SETTINGS_KEY_PENDING_JOB	CONFIG_COAP_FOTA_SETTINGS_KEY_PENDING_JOB
#define FOTA_SETTINGS_FULL		FOTA_SETTINGS_NAME "/" FOTA_SETTINGS_KEY_PENDING_JOB
#endif

#define FOTA_DL_FRAGMENT_SZ	1400

/* FOTA job status strings that provide additional details for nrf_cloud_fota_status values */
const char * const FOTA_STATUS_DETAILS_TIMEOUT = "Download did not complete in the allotted time";
const char * const FOTA_STATUS_DETAILS_DL_ERR  = "Error occurred while downloading the file";
const char * const FOTA_STATUS_DETAILS_MDM_REJ = "Modem rejected the update; invalid delta?";
const char * const FOTA_STATUS_DETAILS_MDM_ERR = "Modem was unable to apply the update";
const char * const FOTA_STATUS_DETAILS_MCU_REJ = "Device rejected the update";
const char * const FOTA_STATUS_DETAILS_MCU_ERR = "Update could not be validated";
const char * const FOTA_STATUS_DETAILS_SUCCESS = "FOTA update completed successfully";
const char * const FOTA_STATUS_DETAILS_NO_VALIDATE = "FOTA update completed without validation";
const char * const FOTA_STATUS_DETAILS_MISMATCH = "FW file does not match specified FOTA type";

/* Semaphore to indicate the FOTA download has arrived at a terminal state */
static K_SEM_DEFINE(fota_download_sem, 0, 1);

/* FOTA job info received from nRF Cloud */
static struct nrf_cloud_fota_job_info job;

/* FOTA job status */
static enum nrf_cloud_fota_status fota_status = NRF_CLOUD_FOTA_QUEUED;
static char const *fota_status_details = FOTA_STATUS_DETAILS_SUCCESS;

/* Pending job info used with the settings library */
static struct nrf_cloud_settings_fota_job pending_job = {
	.type = NRF_CLOUD_FOTA_TYPE__INVALID,
	.validate = NRF_CLOUD_FOTA_VALIDATE_NONE
};

#if defined(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
/* Flag to indicate if full modem FOTA is enabled */
static bool full_modem_fota_initd;
#endif

static int coap_fota_settings_set(const char *key, size_t len_rd,
			    settings_read_cb read_cb, void *cb_arg);

SETTINGS_STATIC_HANDLER_DEFINE(coap_fota, FOTA_SETTINGS_NAME, NULL,
			       coap_fota_settings_set, NULL, NULL);

static int coap_fota_settings_set(const char *key, size_t len_rd, settings_read_cb read_cb,
				  void *cb_arg)
{
	ssize_t sz;

	if (!key) {
		LOG_DBG("Key is NULL");
		return -EINVAL;
	}

	LOG_DBG("Settings key: %s, size: %d", key, len_rd);

	if (strncmp(key, FOTA_SETTINGS_KEY_PENDING_JOB, strlen(FOTA_SETTINGS_KEY_PENDING_JOB))) {
		return -ENOMSG;
	}

	if (len_rd > sizeof(pending_job)) {
		LOG_INF("FOTA settings size larger than expected");
		len_rd = sizeof(pending_job);
	}

	sz = read_cb(cb_arg, (void *)&pending_job, len_rd);
	if (sz == 0) {
		LOG_DBG("FOTA settings key-value pair has been deleted");
		return -EIDRM;
	} else if (sz < 0) {
		LOG_ERR("FOTA settings read error: %d", sz);
		return -EIO;
	}

	if (sz == sizeof(pending_job)) {
		LOG_INF("Saved job: %s, type: %d, validate: %d, bl: 0x%X",
			pending_job.id, pending_job.type,
			pending_job.validate, pending_job.bl_flags);
	} else {
		LOG_INF("FOTA settings size smaller than current, likely outdated");
	}

	return 0;
}

static int save_pending_job(void)
{
	int ret = settings_save_one(FOTA_SETTINGS_FULL, &pending_job, sizeof(pending_job));

	if (ret) {
		LOG_ERR("Failed to save FOTA job to settings, error: %d", ret);
	}

	return ret;
}

static void http_fota_dl_handler(const struct fota_download_evt *evt)
{
	LOG_DBG("evt: %d", evt->id);

	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_FINISHED:
		LOG_INF("FOTA download finished");
		fota_status = NRF_CLOUD_FOTA_SUCCEEDED;
		k_sem_give(&fota_download_sem);
		break;
	case FOTA_DOWNLOAD_EVT_ERASE_PENDING:
	case FOTA_DOWNLOAD_EVT_ERASE_TIMEOUT:
		LOG_INF("FOTA download erase ongoing");
		break;
	case FOTA_DOWNLOAD_EVT_ERASE_DONE:
		LOG_DBG("FOTA download erase done");
		break;
	case FOTA_DOWNLOAD_EVT_ERROR:
		LOG_INF("FOTA download error: %d", evt->cause);

		fota_status = NRF_CLOUD_FOTA_FAILED;
		fota_status_details = FOTA_STATUS_DETAILS_DL_ERR;

		if (evt->cause == FOTA_DOWNLOAD_ERROR_CAUSE_INVALID_UPDATE) {
			fota_status = NRF_CLOUD_FOTA_REJECTED;
			if (nrf_cloud_fota_is_type_modem(job.type)) {
				fota_status_details = FOTA_STATUS_DETAILS_MDM_REJ;
			} else {
				fota_status_details = FOTA_STATUS_DETAILS_MCU_REJ;
			}
		} else if (evt->cause == FOTA_DOWNLOAD_ERROR_CAUSE_TYPE_MISMATCH) {
			fota_status_details = FOTA_STATUS_DETAILS_MISMATCH;
		}
		k_sem_give(&fota_download_sem);
		break;
	case FOTA_DOWNLOAD_EVT_PROGRESS:
		LOG_INF("FOTA download percent: %d", evt->progress);
		break;
	default:
		break;
	}
}

static bool pending_fota_job_exists(void)
{
	return (pending_job.validate != NRF_CLOUD_FOTA_VALIDATE_NONE);
}

static void process_pending_job(void)
{
	bool reboot_required = false;
	int ret;

	LOG_INF("Checking for pending FOTA job");
	ret = nrf_cloud_pending_fota_job_process(&pending_job, &reboot_required);

	if ((ret == 0) && reboot_required) {
		/* Save validate status and reboot */
		(void)save_pending_job();

		fota_reboot(PENDING_REBOOT_S, false);
	}
}

int coap_fota_init(void)
{
	int err;

	LOG_INF("Loading FOTA settings...");
	err = settings_subsys_init();
	if (err) {
		LOG_ERR("Failed to initialize settings subsystem, error: %d", err);
		return err;
	}
	err = settings_load_subtree(settings_handler_coap_fota.name);
	if (err) {
		LOG_WRN("Failed to load settings, error: %d", err);
	}

#if defined(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
	err = nrf_cloud_fota_fmfu_dev_set(get_full_modem_fota_fdev());
	if (err < 0) {
		LOG_WRN("Full modem FOTA not initialized");
		return err;
	}

	full_modem_fota_initd = true;
#endif
	return err;
}

int coap_fota_begin(void)
{
	int err;

	/* This function may perform a reboot if a FOTA update is in progress */
	process_pending_job();

	err = fota_download_init(http_fota_dl_handler);
	if (err) {
		LOG_ERR("Failed to initialize FOTA download, error: %d", err);
		return err;
	}

	return 0;
}

static bool validate_in_progress_job(void)
{
	/* Update in progress job status if validation has been done */
	if (pending_fota_job_exists()) {

		if (pending_job.validate == NRF_CLOUD_FOTA_VALIDATE_PASS) {
			fota_status = NRF_CLOUD_FOTA_SUCCEEDED;
			fota_status_details = FOTA_STATUS_DETAILS_SUCCESS;
		} else if (pending_job.validate == NRF_CLOUD_FOTA_VALIDATE_FAIL) {
			fota_status = NRF_CLOUD_FOTA_FAILED;
			if (nrf_cloud_fota_is_type_modem(pending_job.type)) {
				fota_status_details = FOTA_STATUS_DETAILS_MDM_ERR;
			} else {
				fota_status_details = FOTA_STATUS_DETAILS_MCU_ERR;
			}
		} else {
			fota_status = NRF_CLOUD_FOTA_SUCCEEDED;
			fota_status_details = FOTA_STATUS_DETAILS_NO_VALIDATE;
		}
		LOG_DBG("FOTA job completed");
		return true;
	}

	LOG_DBG("No FOTA job to update");
	return false;
}

static int check_for_job(void)
{
	int err;

	LOG_INF("Checking for FOTA job...");
	err = nrf_cloud_coap_fota_job_get(&job);
	if (err == -ENOMSG) {
		LOG_INF("No pending FOTA job");
		return 1;
	} else if (err == -EACCES) {
		LOG_INF("Not connected yet.");
		return err;
	} else if (err < 0) {
		LOG_ERR("Failed to fetch FOTA job, error: %d", err);
		return -ENOENT;
	} else if (err > 0) {
		return err;
	}

	if (job.type == NRF_CLOUD_FOTA_TYPE__INVALID) {
		LOG_INF("No pending FOTA job");
		return 1;
	}

	LOG_INF("FOTA Job: %s, type: %d\n", job.id ? job.id : "<none>", job.type);
	return 0;
}

static int update_job_status(void)
{
	int err;
	bool is_job_pending = pending_fota_job_exists();

	/* Send FOTA job status to nRF Cloud and, if successful, clear pending job in settings */
	LOG_INF("Updating FOTA job status...");
	err = nrf_cloud_coap_fota_job_update(is_job_pending ? pending_job.id : job.id,
					     fota_status, fota_status_details);

	pending_job.validate	= NRF_CLOUD_FOTA_VALIDATE_NONE;
	pending_job.type	= NRF_CLOUD_FOTA_TYPE__INVALID;
	pending_job.bl_flags	= NRF_CLOUD_FOTA_BL_STATUS_CLEAR;
	memset(pending_job.id, 0, NRF_CLOUD_FOTA_JOB_ID_SIZE);

	if (err) {
		LOG_ERR("Failed to update FOTA job, error: %d", err);
	} else {
		LOG_INF("FOTA job updated, status: %d", fota_status);

		/* Clear the pending job in settings */
		if (is_job_pending) {
			(void)save_pending_job();
		}
	}

	return err;
}

static int start_download(void)
{
	enum dfu_target_image_type img_type;

	/* Start the FOTA download, specifying the job/image type */
	switch (job.type) {
	case NRF_CLOUD_FOTA_BOOTLOADER:
	case NRF_CLOUD_FOTA_APPLICATION:
		img_type = DFU_TARGET_IMAGE_TYPE_MCUBOOT;
		break;
	case NRF_CLOUD_FOTA_MODEM_DELTA:
		img_type = DFU_TARGET_IMAGE_TYPE_MODEM_DELTA;
		break;
	case NRF_CLOUD_FOTA_MODEM_FULL:
		img_type = DFU_TARGET_IMAGE_TYPE_FULL_MODEM;
		break;
	default:
		LOG_ERR("Unhandled FOTA type: %d", job.type);
		return -EFTYPE;
	}

	LOG_INF("Starting FOTA download of %s/%s", job.host, job.path);
	int err = fota_download_start_with_image_type(job.host, job.path,
		CONFIG_NRF_CLOUD_SEC_TAG, 0, FOTA_DL_FRAGMENT_SZ,
		img_type);

	if (err != 0) {
		LOG_ERR("Failed to start FOTA download, error: %d", err);
		return -ENODEV;
	}

	return 0;
}

static int wait_for_download(void)
{
	LOG_INF("Waiting for FOTA download to complete");
	int err = k_sem_take(&fota_download_sem,
			K_MINUTES(CONFIG_COAP_FOTA_DL_TIMEOUT_MIN));
	if (err == -EAGAIN) {
		fota_download_cancel();
		return -ETIMEDOUT;
	} else if (err != 0) {
		LOG_ERR("k_sem_take error: %d", err);
		return -ENOLCK;
	}

	return 0;
}

static void handle_download_succeeded_and_reboot(void)
{
	int err;

	/* Save the pending FOTA job info to flash */
	memcpy(pending_job.id, job.id, NRF_CLOUD_FOTA_JOB_ID_SIZE);
	pending_job.type = job.type;
	pending_job.validate = NRF_CLOUD_FOTA_VALIDATE_PENDING;
	pending_job.bl_flags = NRF_CLOUD_FOTA_BL_STATUS_CLEAR;

	err = nrf_cloud_bootloader_fota_slot_set(&pending_job);
	if (err) {
		LOG_WRN("Failed to set B1 slot flag, BOOT FOTA validation may be incorrect");
	}

	(void)lte_lc_power_off();

#if defined(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
	if (job.type == NRF_CLOUD_FOTA_MODEM_FULL) {
		LOG_INF("Applying full modem FOTA update...");
		err = nrf_cloud_fota_fmfu_apply();
		if (err) {
			LOG_ERR("Failed to apply full modem FOTA update %d", err);
			pending_job.validate = NRF_CLOUD_FOTA_VALIDATE_FAIL;
		} else {
			pending_job.validate = NRF_CLOUD_FOTA_VALIDATE_PASS;
		}
	}
#endif

	err = save_pending_job();
	if (err) {
		LOG_WRN("FOTA job will be marked as successful without validation");
		fota_status_details = FOTA_STATUS_DETAILS_NO_VALIDATE;
		(void)update_job_status();
	}

	LOG_INF("Rebooting in %us to complete FOTA update...", FOTA_REBOOT_S);
	fota_reboot(FOTA_REBOOT_S, false);
}

static void cleanup(void)
{
	nrf_cloud_coap_fota_job_free(&job);
}

static void wait_after_job_update(void)
{
	/* Job operations can take up to 30s to be processed */
	LOG_INF("Waiting %u seconds for job update to be processed by nRF Cloud...", JOB_WAIT_S);
	cleanup();
	k_sleep(K_SECONDS(JOB_WAIT_S));
}

int coap_fota_process(void)
{
	int err;

	/* Wait until we are able to communicate. */
	LOG_DBG("Waiting for valid connection before handling FOTA");
	(void)await_cloud_ready(K_FOREVER);

	/* If a FOTA job is in progress, handle it first */
	if (validate_in_progress_job()) {
		err = update_job_status();
		if (err) {
			LOG_ERR("Error updating FOTA job status: %d", err);
			fota_reboot(ERROR_REBOOT_S, true);
		}
		wait_after_job_update();
		return -ENOENT;
	}

	/* Check for a new FOTA job */
	err = check_for_job();
	if (err < 0) {
		return err;
	}

	if (err > 0) {
		/* No job. Wait for the configured duration. */
		cleanup();

		LOG_DBG("Retrying in %d minute(s)", CONFIG_COAP_FOTA_JOB_CHECK_RATE_MINUTES);
		return -EAGAIN;
	}

	/* Start the FOTA download process and wait for completion (or timeout) */
	err = start_download();
	if (err) {
		return err;
	}
	err = wait_for_download();
	if (err == -ETIMEDOUT) {
		LOG_ERR("Timeout; FOTA download took longer than %d minutes",
			CONFIG_COAP_FOTA_DL_TIMEOUT_MIN);
		fota_status = NRF_CLOUD_FOTA_TIMED_OUT;
		fota_status_details = FOTA_STATUS_DETAILS_TIMEOUT;
	}

	/* On download success, save job info and reboot to complete installation.
	 * Job status will be sent to nRF Cloud after reboot and validation.
	 */
	if (fota_status == NRF_CLOUD_FOTA_SUCCEEDED) {
		LOG_INF("FOTA download succeeded");
		handle_download_succeeded_and_reboot();
	}

	/* Job was not successful, send status to nRF Cloud */
	err = update_job_status();
	if (err) {
		LOG_ERR("Error updating job status: %d", err);
	}

	wait_after_job_update();

	return err;
}

#define FOTA_THREAD_DELAY_S 10

int coap_fota_thread_fn(void)
{
	int err;

	while (1) {
		err = coap_fota_process();
		if (err == -EAGAIN) {
			k_sleep(K_MINUTES(CONFIG_COAP_FOTA_JOB_CHECK_RATE_MINUTES));
		} else {
			k_sleep(K_SECONDS(FOTA_THREAD_DELAY_S));
		}
	}
	return 0;
}
