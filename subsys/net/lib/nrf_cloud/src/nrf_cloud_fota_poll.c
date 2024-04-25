/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_fota_poll.h>
#if defined(CONFIG_NRF_CLOUD_COAP)
#include <net/nrf_cloud_coap.h>
#endif
#include <net/nrf_cloud_rest.h>
#include <zephyr/logging/log.h>
#include <net/fota_download.h>
#include "nrf_cloud_download.h"
#include "nrf_cloud_fota.h"

LOG_MODULE_REGISTER(nrf_cloud_fota_poll, CONFIG_NRF_CLOUD_FOTA_POLL_LOG_LEVEL);

#define FOTA_DL_FRAGMENT_SZ	1400
#define JOB_WAIT_S		30

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

static bool initialized;

/* Semaphore to indicate the FOTA download has arrived at a terminal state */
static K_SEM_DEFINE(fota_download_sem, 0, 1);

/* FOTA job info received from nRF Cloud */
static struct nrf_cloud_fota_job_info job;

/* Pending job information saved to flash */
static struct nrf_cloud_settings_fota_job pending_job = { .type = NRF_CLOUD_FOTA_TYPE__INVALID };

/* FOTA job status */
static enum nrf_cloud_fota_status fota_status = NRF_CLOUD_FOTA_QUEUED;
static char const *fota_status_details = FOTA_STATUS_DETAILS_SUCCESS;

/****************************************************/
/* Define wrappers for transport-specific functions */
/****************************************************/

static bool check_ctx(struct nrf_cloud_fota_poll_ctx *ctx)
{
	if (!ctx || !ctx->reboot_fn) {
		return false;
	}
	if (IS_ENABLED(CONFIG_NRF_CLOUD_REST) && (!ctx->rest_ctx || !ctx->device_id)) {
		return false;
	}
	return true;
}

static int fota_job_get(struct nrf_cloud_fota_poll_ctx *ctx,
		 struct nrf_cloud_fota_job_info *const job)
{
#if defined(CONFIG_NRF_CLOUD_REST)
	return nrf_cloud_rest_fota_job_get(ctx->rest_ctx, ctx->device_id, job);
#elif defined(CONFIG_NRF_CLOUD_COAP)
	return nrf_cloud_coap_fota_job_get(job);
#endif
	return -ENOTSUP;
}

static int fota_job_update(struct nrf_cloud_fota_poll_ctx *ctx, const char *const job_id,
			   const enum nrf_cloud_fota_status status, const char * const details)
{
#if defined(CONFIG_NRF_CLOUD_REST)
	return nrf_cloud_rest_fota_job_update(ctx->rest_ctx, ctx->device_id, job_id,
					      status, details);
#elif defined(CONFIG_NRF_CLOUD_COAP)
	return nrf_cloud_coap_fota_job_update(job_id, status, details);
#endif
	return -ENOTSUP;
}

static int disconnect(struct nrf_cloud_fota_poll_ctx *ctx)
{
	int err;

#if defined(CONFIG_NRF_CLOUD_REST)
	err = nrf_cloud_rest_disconnect(ctx->rest_ctx);
#elif defined(CONFIG_NRF_CLOUD_COAP)
	err = nrf_cloud_coap_disconnect();
#else
	err = -ENOTSUP;
#endif
#if defined(CONFIG_LTE_LINK_CONTROL)
	(void)lte_lc_power_off();
#endif

	return err;
}

static void cleanup(void)
{
#if defined(CONFIG_NRF_CLOUD_REST)
	nrf_cloud_rest_fota_job_free(&job);
#elif defined(CONFIG_NRF_CLOUD_COAP)
	nrf_cloud_coap_fota_job_free(&job);
#endif
}

/****************************************************/
/* End of transport-specific wrappers.              */
/****************************************************/
static void http_fota_dl_handler(const struct fota_download_evt *evt)
{
	LOG_DBG("evt: %d", evt->id);

	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_FINISHED:
		LOG_INF("FOTA download finished");
		nrf_cloud_download_end();
		fota_status = NRF_CLOUD_FOTA_SUCCEEDED;
		fota_status_details = FOTA_STATUS_DETAILS_SUCCESS;
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

		nrf_cloud_download_end();
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
		LOG_DBG("FOTA download percent: %d%%", evt->progress);
		break;
	case FOTA_DOWNLOAD_EVT_RESUME_OFFSET:
		/* TODO: for now, just fail the download.
		 * Unable to resume with CoAP until NRFCDP-423 is complete.
		 */
		nrf_cloud_download_end();
		fota_status = NRF_CLOUD_FOTA_FAILED;
		fota_status_details = FOTA_STATUS_DETAILS_DL_ERR;
		k_sem_give(&fota_download_sem);
		break;
	default:
		break;
	}
}

static bool pending_fota_job_exists(void)
{
	return (pending_job.validate != NRF_CLOUD_FOTA_VALIDATE_NONE);
}

static void process_pending_job(struct nrf_cloud_fota_poll_ctx *ctx)
{
	bool reboot_required = false;
	int ret;

	ret = nrf_cloud_pending_fota_job_process(&pending_job, &reboot_required);

	if ((ret == 0) && reboot_required) {
		/* Save validate status and reboot */
		(void)nrf_cloud_fota_settings_save(&pending_job);

		if (ctx->reboot_fn) {
			ctx->reboot_fn(FOTA_REBOOT_REQUIRED);
		}
	}
}

int nrf_cloud_fota_poll_init(struct nrf_cloud_fota_poll_ctx *ctx)
{
	int err;

	if (!check_ctx(ctx)) {
		LOG_ERR("Invalid ctx");
		return -EINVAL;
	}

	ctx->full_modem_fota_supported = false;

	err = nrf_cloud_fota_settings_load(&pending_job);
	if (err) {
		return err;
	}

#if defined(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
	struct dfu_target_fmfu_fdev fmfu_dev_inf = {
		.size = 0,
		.offset = 0,
		.dev = IS_ENABLED(CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION) ?
		       NULL : DEVICE_DT_GET_ONE(jedec_spi_nor)
	};

	err = nrf_cloud_fota_fmfu_dev_set(&fmfu_dev_inf);
	if (err < 0) {
		LOG_WRN("Full modem FOTA not initialized");
		return err;
	}

	ctx->full_modem_fota_supported = true;
#endif

	err = fota_download_init(http_fota_dl_handler);
	if (err) {
		LOG_ERR("Failed to initialize FOTA download, error: %d", err);
		return err;
	}

	initialized = true;
	return 0;
}

int nrf_cloud_fota_poll_process_pending(struct nrf_cloud_fota_poll_ctx *ctx)
{
	if (!check_ctx(ctx) || !initialized) {
		return -EINVAL;
	}

	/* This function may perform a reboot if a FOTA update is in progress */
	process_pending_job(ctx);

	return pending_job.type;
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

		return true;
	}

	return false;
}

static int check_for_job(struct nrf_cloud_fota_poll_ctx *ctx)
{
	int err;

	LOG_INF("Checking for FOTA job...");

	err = fota_job_get(ctx, &job);
	if (err) {
		LOG_ERR("Failed to fetch FOTA job, error: %d", err);
		return -ENOENT;
	}

	if (job.type == NRF_CLOUD_FOTA_TYPE__INVALID) {
		LOG_INF("No pending FOTA job");
		return 1;
	}

	LOG_INF("FOTA Job: %s, type: %d\n", job.id, job.type);
	return 0;
}

static int update_job_status(struct nrf_cloud_fota_poll_ctx *ctx)
{
	int err;
	bool is_job_pending = pending_fota_job_exists();

	/* Send FOTA job status to nRF Cloud and, if successful, clear pending job in settings */
	LOG_INF("Updating FOTA job status...");
	err = fota_job_update(ctx, is_job_pending ? pending_job.id : job.id,
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
			(void)nrf_cloud_fota_settings_save(&pending_job);
		}
	}

	return err;
}

static int start_download(void)
{
	enum dfu_target_image_type img_type;
	static const int sec_tag = CONFIG_NRF_CLOUD_SEC_TAG;
	int ret = 0;

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
		if (IS_ENABLED(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)) {
			img_type = DFU_TARGET_IMAGE_TYPE_FULL_MODEM;
		} else {
			LOG_ERR("Not configured for full modem FOTA");
			ret = -EFTYPE;
		}
		break;
	default:
		LOG_ERR("Unhandled FOTA type: %d", job.type);
		return -EFTYPE;
	}

	if (ret == -EFTYPE) {
		fota_status = NRF_CLOUD_FOTA_REJECTED;
		fota_status_details = FOTA_STATUS_DETAILS_MCU_REJ;
		return ret;
	}

	LOG_INF("Starting FOTA download of %s/%s", job.host, job.path);

	struct nrf_cloud_download_data dl = {
		.type = NRF_CLOUD_DL_TYPE_FOTA,
		.host = job.host,
		.path = job.path,
		.dl_cfg = {
			.sec_tag_list = &sec_tag,
			.sec_tag_count = (sec_tag < 0 ? 0 : 1),
			.pdn_id = 0,
			.frag_size_override = FOTA_DL_FRAGMENT_SZ,
		},
		.fota = {
			.expected_type = img_type,
			.img_sz = job.file_size
		}
	};

	ret = nrf_cloud_download_start(&dl);
	if (ret) {
		LOG_ERR("Failed to start FOTA download, error: %d", ret);
		return -ENODEV;
	}

	return 0;
}

static int wait_for_download(void)
{
	int err = k_sem_take(&fota_download_sem, K_MINUTES(CONFIG_FOTA_DL_TIMEOUT_MIN));

	if (err == -EAGAIN) {
		fota_download_cancel();
		return -ETIMEDOUT;
	} else if (err != 0) {
		LOG_ERR("k_sem_take error: %d", err);
		return -ENOLCK;
	}

	return 0;
}

static void handle_download_succeeded_and_reboot(struct nrf_cloud_fota_poll_ctx *ctx)
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

	disconnect(ctx);

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

	err = nrf_cloud_fota_settings_save(&pending_job);
	if (err) {
		LOG_WRN("FOTA job will be marked as successful without validation");
		fota_status_details = FOTA_STATUS_DETAILS_NO_VALIDATE;
		(void)update_job_status(ctx);
	}

	if (ctx->reboot_fn) {
		ctx->reboot_fn(FOTA_REBOOT_SUCCESS);
	}
}

static void wait_after_job_update(void)
{
	/* Job operations can take up to 30s to be processed */
	LOG_INF("Waiting %u seconds for job update to be processed by nRF Cloud...", JOB_WAIT_S);
	cleanup();
	k_sleep(K_SECONDS(JOB_WAIT_S));
}

int nrf_cloud_fota_poll_process(struct nrf_cloud_fota_poll_ctx *ctx)
{
	int err;

	if (!check_ctx(ctx) || !initialized) {
		return -EINVAL;
	}

	/* If a FOTA job is in progress, handle it first */
	if (validate_in_progress_job()) {
		err = update_job_status(ctx);
		if (err) {
			return -ENOTRECOVERABLE;
		}

		wait_after_job_update();
		return -ENOENT;
	}

	/* Check for a new FOTA job */
	err = check_for_job(ctx);
	if (err < 0) {
		return -ENOTRECOVERABLE;
	} else if (err > 0) {
		/* No job. */
		cleanup();
		return -EAGAIN;
	}

	/* Start the FOTA download process and wait for completion (or timeout) */
	err = start_download();
	if (err) {
		LOG_ERR("Failed to start FOTA download");
		return -ENOTRECOVERABLE;
	}

	err = wait_for_download();
	if (err == -ETIMEDOUT) {
		LOG_ERR("Timeout; FOTA download took longer than %d minutes",
			CONFIG_FOTA_DL_TIMEOUT_MIN);
		fota_status = NRF_CLOUD_FOTA_TIMED_OUT;
		fota_status_details = FOTA_STATUS_DETAILS_TIMEOUT;
	}

	/* On download success, save job info and reboot to complete installation.
	 * Job status will be sent to nRF Cloud after reboot and validation.
	 */
	if (fota_status == NRF_CLOUD_FOTA_SUCCEEDED) {
		handle_download_succeeded_and_reboot(ctx);
		/* Application was expected to reboot... */
		return -EBUSY;
	}

	/* Job was not successful, send status to nRF Cloud */
	err = update_job_status(ctx);
	if (err) {
		return -ENOTRECOVERABLE;
	}

	wait_after_job_update();
	return -EFAULT;
}
