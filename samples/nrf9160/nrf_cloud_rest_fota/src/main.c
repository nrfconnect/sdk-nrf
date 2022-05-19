/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <modem/modem_info.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/settings/settings.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_rest.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>
#include <net/fota_download.h>

LOG_MODULE_REGISTER(nrf_cloud_rest_fota, CONFIG_NRF_CLOUD_REST_FOTA_SAMPLE_LOG_LEVEL);

#if defined(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
#define EXT_FLASH_DEVICE DT_LABEL(DT_INST(0, jedec_spi_nor))
#endif

/* Use the settings library to store FOTA job information to flash so
 * that the job status can be updated after a reboot
 */
#if defined(CONFIG_REST_FOTA_USE_NRF_CLOUD_SETTINGS_AREA)
#define FOTA_SETTINGS_NAME		NRF_CLOUD_SETTINGS_FULL_FOTA
#define FOTA_SETTINGS_KEY_PENDING_JOB	NRF_CLOUD_SETTINGS_FOTA_JOB
#define FOTA_SETTINGS_FULL		NRF_CLOUD_SETTINGS_FULL_FOTA_JOB
#else
#define FOTA_SETTINGS_NAME		CONFIG_REST_FOTA_SETTINGS_NAME
#define FOTA_SETTINGS_KEY_PENDING_JOB	CONFIG_REST_FOTA_SETTINGS_KEY_PENDING_JOB
#define FOTA_SETTINGS_FULL		FOTA_SETTINGS_NAME "/" FOTA_SETTINGS_KEY_PENDING_JOB
#endif

#define FOTA_DL_FRAGMENT_SZ	1400
#define BTN_NUM			CONFIG_REST_FOTA_BUTTON_EVT_NUM
#define LED_NUM			CONFIG_REST_FOTA_LED_NUM
#define JITP_REQ_WAIT_SEC	10
#define FOTA_ENABLE_WAIT_SEC	10

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

/* Semaphore to indicate a button has been pressed */
static K_SEM_DEFINE(button_press_sem, 0, 1);

/* Semaphore to indicate that a network connection has been established */
static K_SEM_DEFINE(lte_connected, 0, 1);

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

/* nRF Cloud device ID */
static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];

#define REST_RX_BUF_SZ 2100
/* Buffer used for REST calls */
static char rx_buf[REST_RX_BUF_SZ];

#define JWT_DURATION_S	(60*5)
#define JWT_BUF_SZ	900
/* Buffer used for JSON Web Tokens (JWTs) */
static char jwt[JWT_BUF_SZ];

/* nRF Cloud REST context */
struct nrf_cloud_rest_context rest_ctx = {
	.connect_socket = -1,
	.keep_alive = true,
	.rx_buf = rx_buf,
	.rx_buf_len = sizeof(rx_buf),
	.fragment_size = 0
};

#if defined(CONFIG_REST_FOTA_DO_JITP)
/* Flag to indicate if the user requested JITP to be performed */
static bool jitp_requested;
#endif

/* Flag to indicate if FOTA should be enabled in the device shadow */
static bool enable_fota_requested;

/* Flag to indicate if full modem FOTA is enabled */
static bool full_modem_fota_initd;

static int rest_fota_settings_set(const char *key, size_t len_rd,
			    settings_read_cb read_cb, void *cb_arg);

SETTINGS_STATIC_HANDLER_DEFINE(rest_fota, FOTA_SETTINGS_NAME, NULL,
			       rest_fota_settings_set, NULL, NULL);

static int rest_fota_settings_set(const char *key, size_t len_rd, settings_read_cb read_cb,
				  void *cb_arg)
{
	ssize_t sz;

	if (!key) {
		LOG_DBG("Key is NULL");
		return -EINVAL;
	}

	LOG_DBG("Settings key: %s, size: %d", log_strdup(key), len_rd);

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
			log_strdup(pending_job.id), pending_job.type,
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
		LOG_INF("FOTA download erase pending");
		fota_status = NRF_CLOUD_FOTA_SUCCEEDED;
		k_sem_give(&fota_download_sem);
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

int set_led(const int state)
{
#if defined(CONFIG_REST_FOTA_ENABLE_LED)
	int err = dk_set_led(LED_NUM, state);

	if (err) {
		LOG_ERR("Failed to set LED, error: %d", err);
		return err;
	}
#else
	ARG_UNUSED(state);
#endif
	return 0;
}

int init_led(void)
{
#if defined(CONFIG_REST_FOTA_ENABLE_LED)
	int err = dk_leds_init();

	if (err) {
		LOG_ERR("LED init failed, error: %d", err);
		return err;
	}
	(void)set_led(0);
#endif
	return 0;
}

static bool pending_fota_job_exists(void)
{
	return (pending_job.validate != NRF_CLOUD_FOTA_VALIDATE_NONE);
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & BIT(BTN_NUM - 1)) {
		LOG_DBG("Button %d pressed", BTN_NUM);
		k_sem_give(&button_press_sem);
	}
}

static int generate_jwt(void)
{
	int err = nrf_cloud_jwt_generate(JWT_DURATION_S, jwt, sizeof(jwt));

	if (err < 0) {
		LOG_ERR("Failed to generate JWT, error: %d", err);
		return err;
	}

	LOG_DBG("JWT:\n%s", log_strdup(jwt));
	rest_ctx.auth = jwt;

	return 0;
}

static void modem_time_wait(void)
{
	int err;
	char time_buf[64];

	LOG_INF("Waiting for modem to acquire network time...");

	do {
		k_sleep(K_SECONDS(3));
		err = nrf_modem_at_cmd(time_buf, sizeof(time_buf), "AT%%CCLK?");
	} while (err != 0);

	LOG_INF("Network time obtained");
}

#if defined(CONFIG_REST_FOTA_DO_JITP)
static void request_jitp(void)
{
	if (pending_fota_job_exists()) {
		/* Skip request if a FOTA job already exists */
		return;
	}

	int ret;

	jitp_requested = false;

	k_sem_reset(&button_press_sem);

	LOG_INF("---> Press button %d to request just-in-time provisioning", BTN_NUM);
	LOG_INF("     Waiting %d seconds...", JITP_REQ_WAIT_SEC);

	ret = k_sem_take(&button_press_sem, K_SECONDS(JITP_REQ_WAIT_SEC));
	if (ret == 0) {
		jitp_requested = true;
		LOG_INF("JITP will be performed after network connection is obtained");
		/* FOTA needs to be enabled for a newly JITP'd device */
		enable_fota_requested = true;
	} else {
		if (ret != -EAGAIN) {
			LOG_ERR("k_sem_take error: %d", ret);
		}

		LOG_INF("JITP will be skipped");
	}
}

static void do_jitp(void)
{
	int ret = 0;

	LOG_INF("Performing JITP...");
	ret = nrf_cloud_rest_jitp(CONFIG_NRF_CLOUD_SEC_TAG);

	if (ret == 0) {
		LOG_INF("Waiting 30s for cloud provisioning to complete...");
		k_sleep(K_SECONDS(30));
		k_sem_reset(&button_press_sem);
		LOG_INF("Associate device with nRF Cloud account and press button %d when complete",
			BTN_NUM);
		(void)k_sem_take(&button_press_sem, K_FOREVER);
	} else if (ret == 1) {
		LOG_INF("Device already provisioned");
	} else {
		LOG_ERR("Device provisioning failed");
	}
}
#endif

static void request_fota_enable(void)
{
	if (pending_fota_job_exists()) {
		/* Skip request if a FOTA job already exists */
		return;
	}

	if (!enable_fota_requested) {
		int ret;

		k_sem_reset(&button_press_sem);

		LOG_INF("---> Press button %d to enable FOTA in the device shadow", BTN_NUM);
		LOG_INF("     Waiting %d seconds...", FOTA_ENABLE_WAIT_SEC);

		ret = k_sem_take(&button_press_sem, K_SECONDS(FOTA_ENABLE_WAIT_SEC));

		if (ret == 0) {
			enable_fota_requested = true;
		} else {
			if (ret != -EAGAIN) {
				LOG_ERR("k_sem_take error: %d", ret);
			}
			LOG_INF("Shadow will not be updated");
		}
	}

	if (enable_fota_requested) {
		LOG_INF("FOTA will be enabled after network connection is obtained");
	}
}

static void do_fota_enable(void)
{
	int err;
	/* Enable FOTA for bootloader, modem and application */
	struct nrf_cloud_svc_info_fota fota = {
		.bootloader = 1,
		.modem = 1,
		.application = 1,
		.modem_full = full_modem_fota_initd
	};

	struct nrf_cloud_svc_info svc_inf = {
		.fota = &fota,
		.ui = NULL
	};

	err = generate_jwt();
	if (err) {
		LOG_ERR("Failed to generated JWT; FOTA not enabled.");
		return;
	}

	LOG_INF("Enabling FOTA...");
	err = nrf_cloud_rest_shadow_service_info_update(&rest_ctx, device_id, &svc_inf);
	if (err) {
		LOG_ERR("Failed to enable FOTA, error: %d", err);
	} else {
		LOG_INF("FOTA enabled in device shadow");
	}
}

static void process_pending_job(void)
{
	bool reboot_required = false;
	int ret;

	ret = nrf_cloud_pending_fota_job_process(&pending_job, &reboot_required);

	if ((ret == 0) && reboot_required) {
		/* Save validate status and reboot */
		(void)save_pending_job();

		/* Sleep to display log message */
		LOG_INF("Rebooting...");
		k_sleep(K_SECONDS(5));
		sys_reboot(SYS_REBOOT_COLD);
	}
}

int init(void)
{
	struct modem_param_info mdm_param;
	int modem_lib_init_result;
	int err = init_led();

	if (err) {
		LOG_ERR("LED initialization failed");
		return err;
	}
	err = settings_subsys_init();
	if (err) {
		LOG_ERR("Failed to initialize settings subsystem, error: %d", err);
		return err;
	}
	err = settings_load_subtree(settings_handler_rest_fota.name);
	if (err) {
		LOG_WRN("Failed to load settings, error: %d", err);
	}

#if defined(CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
	struct dfu_target_fmfu_fdev fmfu_dev_inf = {
		.size = 0,
		.offset = 0,
		.dev = device_get_binding(EXT_FLASH_DEVICE)
	};

	if (fmfu_dev_inf.dev) {
		err = nrf_cloud_fota_fmfu_dev_set(&fmfu_dev_inf);
		if (err < 0) {
			return err;
		}
		full_modem_fota_initd = true;
	} else {
		LOG_WRN("Full modem FOTA not initialized; flash device not specified");
	}
#endif

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_SYS_INIT)) {
		modem_lib_init_result = nrf_modem_lib_get_init_ret();
	} else {
		modem_lib_init_result = nrf_modem_lib_init(NORMAL_MODE);
	}

	/* This function may perform a reboot if a FOTA update is in progress */
	process_pending_job();

	if (modem_lib_init_result) {
		LOG_ERR("Failed to initialize modem library: 0x%X", modem_lib_init_result);
		return -EFAULT;
	}

	err = fota_download_init(http_fota_dl_handler);
	if (err) {
		LOG_ERR("Failed to initialize FOTA download, error: %d", err);
		return err;
	}

	err = modem_info_init();
	if (err) {
		LOG_WRN("Modem info initialization failed, error: %d", err);
		return err;
	}

	err = modem_info_params_init(&mdm_param);
	if (!err) {
		LOG_INF("Application Name: %s",
			log_strdup(mdm_param.device.app_name));
		LOG_INF("nRF Connect SDK version: %s",
			log_strdup(mdm_param.device.app_version));

		err = modem_info_params_get(&mdm_param);
		if (!err) {
			LOG_INF("Modem FW Ver: %s",
				log_strdup(mdm_param.device.modem_fw.value_string));
		} else {
			LOG_WRN("Unable to obtain modem info, error: %d", err);
		}
	} else {
		LOG_WRN("Modem info params initialization failed, error: %d", err);
	}

	err = nrf_cloud_client_id_get(device_id, sizeof(device_id));
	if (err) {
		LOG_ERR("Failed to set device ID, error: %d", err);
		return err;
	}

	LOG_INF("Device ID: %s", log_strdup(device_id));

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Failed to initialize button: error: %d", err);
		return err;
	}

#if defined(CONFIG_REST_FOTA_DO_JITP)
	/* Present option for JITP via REST */
	request_jitp();
#endif

	/* Present option to enable FOTA in device shadow via REST */
	request_fota_enable();

	return 0;
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		LOG_DBG("LTE_LC_EVT_NW_REG_STATUS: %d", evt->nw_reg_status);
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		LOG_DBG("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_DBG("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}

static int connect_to_network(void)
{
	int err;

	LOG_INF("Waiting for network...");

	k_sem_reset(&lte_connected);

	err = lte_lc_init_and_connect_async(lte_handler);
	if (err) {
		LOG_ERR("Failed to init modem, error: %d", err);
	} else {
		k_sem_take(&lte_connected, K_FOREVER);
		(void)set_led(1);
		LOG_INF("Connected");
	}

	return err;
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

static int check_for_job(void)
{
	int err;

	LOG_INF("Checking for FOTA job...");

	err = nrf_cloud_rest_fota_job_get(&rest_ctx, device_id, &job);
	if (err) {
		LOG_ERR("Failed to fetch FOTA job, error: %d", err);
		return -ENOENT;
	}

	if (job.type == NRF_CLOUD_FOTA_TYPE__INVALID) {
		LOG_INF("No pending FOTA job");
		return 1;
	}

	LOG_INF("FOTA Job: %s, type: %d\n", log_strdup(job.id), job.type);
	return 0;
}

static int update_job_status(void)
{
	int err;
	bool is_job_pending = pending_fota_job_exists();

	/* Send FOTA job status to nRF Cloud and, if successful, clear pending job in settings */
	LOG_INF("Updating FOTA job status...");
	err = nrf_cloud_rest_fota_job_update(&rest_ctx, device_id,
					     is_job_pending ? pending_job.id : job.id,
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
	int err = k_sem_take(&fota_download_sem,
			K_MINUTES(CONFIG_REST_FOTA_DL_TIMEOUT_MIN));
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

	(void)nrf_cloud_rest_disconnect(&rest_ctx);
	(void)lte_lc_deinit();

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
		err = 0;
		fota_status_details = FOTA_STATUS_DETAILS_NO_VALIDATE;
		(void)update_job_status();
	}

	LOG_INF("Rebooting in 10s to complete FOTA update...");
	k_sleep(K_SECONDS(10));
	sys_reboot(SYS_REBOOT_COLD);
}

static void cleanup(void)
{
	nrf_cloud_rest_fota_job_free(&job);
}

static void wait_after_job_update(void)
{
	/* Job operations can take up to 30s to be processed */
	LOG_INF("Checking for next FOTA update in 30s...");
	cleanup();
	k_sleep(K_SECONDS(30));
}

static void error_reboot(void)
{
	LOG_INF("Rebooting in 30s...");
	(void)nrf_cloud_rest_disconnect(&rest_ctx);
	(void)lte_lc_deinit();
	k_sleep(K_SECONDS(30));
	sys_reboot(SYS_REBOOT_COLD);
}

void main(void)
{
	int err;

	LOG_INF("nRF Cloud REST FOTA Sample, version: %s",
		CONFIG_REST_FOTA_SAMPLE_VERSION);

	err = init();
	if (err) {
		LOG_ERR("Initialization failed");
		return;
	}

	err = connect_to_network();
	if (err) {
		return;
	}

	/* Modem must have valid time/date in order to generate JWTs with
	 * an expiration time
	 */
	modem_time_wait();

#if defined(CONFIG_REST_FOTA_DO_JITP)
	if (jitp_requested) {
		/* Perform JITP via REST */
		do_jitp();
	}
#endif

	if (enable_fota_requested) {
		/* Enable FOTA in shadow */
		do_fota_enable();
	}

	while (1) {

		/* Generate a JWT for each REST call */
		err = generate_jwt();
		if (err) {
			error_reboot();
		}

		/* If a FOTA job is in progress, handle it first */
		if (validate_in_progress_job()) {
			err = update_job_status();
			if (err) {
				error_reboot();
			}

			wait_after_job_update();
			continue;
		}

		/* Check for a new FOTA job */
		err = check_for_job();
		if (err < 0) {
			error_reboot();
		} else if (err > 0) {
			/* No job. Wait for the configured duration or a button press */
			cleanup();
			(void)nrf_cloud_rest_disconnect(&rest_ctx);

			LOG_INF("Retrying in %d minute(s) or when button %d is pressed",
				CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN,
				CONFIG_REST_FOTA_BUTTON_EVT_NUM);

			(void)k_sem_take(&button_press_sem,
				K_MINUTES(CONFIG_REST_FOTA_JOB_CHECK_RATE_MIN));
			continue;
		}

		/* Start the FOTA download process and wait for completion (or timeout) */
		err = start_download();
		if (err) {
			LOG_ERR("Failed to start FOTA download");
			error_reboot();
		}

		err = wait_for_download();
		if (err == -ETIMEDOUT) {
			LOG_ERR("Timeout; FOTA download took longer than %d minutes",
				CONFIG_REST_FOTA_DL_TIMEOUT_MIN);
			fota_status = NRF_CLOUD_FOTA_TIMED_OUT;
			fota_status_details = FOTA_STATUS_DETAILS_TIMEOUT;
		}

		/* On download success, save job info and reboot to complete installation.
		 * Job status will be sent to nRF Cloud after reboot and validation.
		 */
		if (fota_status == NRF_CLOUD_FOTA_SUCCEEDED) {
			handle_download_succeeded_and_reboot();
		}

		/* Job was not successful, send status to nRF Cloud */
		err = update_job_status();
		if (err) {
			error_reboot();
		}

		wait_after_job_update();
	}
}
