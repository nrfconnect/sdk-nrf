/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <logging/log.h>
#include <net/fota_download.h>
#include <net/download_client.h>
#include <dfu/dfu_target.h>
#include <pm_config.h>

#if defined(PM_S1_ADDRESS) || defined(CONFIG_DFU_TARGET_MCUBOOT)
/* MCUBoot support is required */
#include <fw_info.h>
#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE
#include <secure_services.h>
#endif
#include <dfu/dfu_target_mcuboot.h>
#endif

LOG_MODULE_REGISTER(fota_download, CONFIG_FOTA_DOWNLOAD_LOG_LEVEL);

static fota_download_callback_t callback;
static struct download_client   dlc;
static struct k_delayed_work    dlc_with_offset_work;
static int socket_retries_left;
#ifdef CONFIG_DFU_TARGET_MCUBOOT
static uint8_t mcuboot_buf[CONFIG_FOTA_DOWNLOAD_MCUBOOT_FLASH_BUF_SZ];
#endif
static void send_evt(enum fota_download_evt_id id)
{
	__ASSERT(id != FOTA_DOWNLOAD_EVT_PROGRESS, "use send_progress");
	__ASSERT(id != FOTA_DOWNLOAD_EVT_ERROR, "use send_error_evt");
	const struct fota_download_evt evt = {
		.id = id
	};
	callback(&evt);
}

static void send_error_evt(enum fota_download_error_cause cause)
{
	__ASSERT(cause != FOTA_DOWNLOAD_ERROR_CAUSE_NO_ERROR, "use a valid error cause");
	const struct fota_download_evt evt = {
		.id = FOTA_DOWNLOAD_EVT_ERROR,
		.cause = cause
	};
	callback(&evt);
}

static void send_progress(int progress)
{
#ifdef CONFIG_FOTA_DOWNLOAD_PROGRESS_EVT
	const struct fota_download_evt evt = { .id = FOTA_DOWNLOAD_EVT_PROGRESS,
					       .progress = progress };
	callback(&evt);
#endif
}

static void dfu_target_callback_handler(enum dfu_target_evt_id evt)
{
	switch (evt) {
	case DFU_TARGET_EVT_TIMEOUT:
		send_evt(FOTA_DOWNLOAD_EVT_ERASE_PENDING);
		break;
	case DFU_TARGET_EVT_ERASE_DONE:
		send_evt(FOTA_DOWNLOAD_EVT_ERASE_DONE);
		break;
	default:
		send_error_evt(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
	}
}

static int download_client_callback(const struct download_client_evt *event)
{
	static bool first_fragment = true;
	static size_t file_size;
	size_t offset;
	int err;

	if (event == NULL) {
		return -EINVAL;
	}

	switch (event->id) {
	case DOWNLOAD_CLIENT_EVT_FRAGMENT: {
		if (first_fragment) {
			err = download_client_file_size_get(&dlc, &file_size);
			if (err != 0) {
				LOG_DBG("download_client_file_size_get err: %d",
					err);
				send_error_evt(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
				return err;
			}
			first_fragment = false;
			int img_type = dfu_target_img_type(event->fragment.buf,
							event->fragment.len);
			err = dfu_target_init(img_type, file_size,
					      dfu_target_callback_handler);
			if ((err < 0) && (err != -EBUSY)) {
				LOG_ERR("dfu_target_init error %d", err);
				send_error_evt(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
				int res = dfu_target_reset();

				if (res != 0) {
					LOG_ERR("Unable to reset DFU target");
				}
				first_fragment = true;
				return err;
			}

			err = dfu_target_offset_get(&offset);
			if (err != 0) {
				LOG_DBG("unable to get dfu target offset err: "
					"%d", err);
				send_error_evt(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
			}

			if (offset != 0) {
				/* Abort current download procedure, and
				 * schedule new download from offset.
				 */
				(void)download_client_disconnect(&dlc);
				k_delayed_work_submit(&dlc_with_offset_work,
						K_SECONDS(1));
				LOG_INF("Refuse fragment, restart with offset");

				return -1;
			}
		}

		err = dfu_target_write(event->fragment.buf,
				       event->fragment.len);
		if (err != 0) {
			LOG_ERR("dfu_target_write error %d", err);
			int res = dfu_target_done(false);

			if (res != 0) {
				LOG_ERR("Unable to free DFU target resources");
			}
			first_fragment = true;
			(void) download_client_disconnect(&dlc);
			send_error_evt(FOTA_DOWNLOAD_ERROR_CAUSE_INVALID_UPDATE);
			return err;
		}

		if (IS_ENABLED(CONFIG_FOTA_DOWNLOAD_PROGRESS_EVT) &&
		    !first_fragment) {
			err = dfu_target_offset_get(&offset);
			if (err != 0) {
				LOG_DBG("unable to get dfu target "
						"offset err: %d", err);
				send_error_evt(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
				return err;
			}

			if (file_size == 0) {
				LOG_DBG("invalid file size: %d", file_size);
				send_error_evt(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
				return err;
			}

			send_progress((offset * 100) / file_size);
			LOG_DBG("Progress: %d/%d bytes", offset, file_size);
		}
	break;
	}

	case DOWNLOAD_CLIENT_EVT_DONE:
		err = dfu_target_done(true);
		if (err != 0) {
			LOG_ERR("dfu_target_done error: %d", err);
			send_error_evt(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
			return err;
		}

		err = download_client_disconnect(&dlc);
		if (err != 0) {
			send_error_evt(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
			return err;
		}
		send_evt(FOTA_DOWNLOAD_EVT_FINISHED);
		first_fragment = true;
		break;

	case DOWNLOAD_CLIENT_EVT_ERROR: {
		/* In case of socket errors we can return 0 to retry/continue,
		 * or non-zero to stop
		 */
		if ((socket_retries_left) && ((event->error == -ENOTCONN) ||
					      (event->error == -ECONNRESET))) {
			LOG_WRN("Download socket error. %d retries left...",
				socket_retries_left);
			socket_retries_left--;
			/* Fall through and return 0 below to tell
			 * download_client to retry
			 */
		} else {
			download_client_disconnect(&dlc);
			LOG_ERR("Download client error");
			err = dfu_target_done(false);
			if (err == -EACCES) {
				LOG_DBG("No DFU target was initialized");
			} else if (err != 0) {
				LOG_ERR("Unable to deinitialze resources "
					"used by dfu_target.");
			}
			first_fragment = true;
			send_error_evt(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
			/* Return non-zero to tell download_client to stop */
			return event->error;
		}
	}
	default:
		break;
	}

	return 0;
}

static void download_with_offset(struct k_work *unused)
{
	int offset;
	int err = dfu_target_offset_get(&offset);
	if (err != 0) {
		LOG_ERR("%s failed to get offset with error %d", __func__, err);
		return;
	}

	err = download_client_connect(&dlc, dlc.host, &dlc.config);
	if (err != 0) {
		LOG_ERR("%s failed to connect with error %d", __func__, err);
		return;
	}

	err = download_client_start(&dlc, dlc.file, offset);
	if (err != 0) {
		LOG_ERR("%s failed to start download  with error %d", __func__,
			err);
		return;
	}
	LOG_INF("Downloading from offset: 0x%x", offset);
	return;
}

int fota_download_start(const char *host, const char *file, int sec_tag,
			const char *apn, size_t fragment_size)
{
	int err = -1;

	struct download_client_cfg config = {
		.sec_tag = sec_tag,
		.apn = apn,
		.frag_size_override = fragment_size,
		.set_tls_hostname = (sec_tag != -1),
	};

	if (host == NULL || file == NULL || callback == NULL) {
		return -EINVAL;
	}

	socket_retries_left = CONFIG_FOTA_SOCKET_RETRIES;

#ifdef PM_S1_ADDRESS
	/* B1 upgrade is supported, check what B1 slot is active,
	 * (s0 or s1), and update file to point to correct candidate if
	 * space separated file is given.
	 */
	const char *update;
	bool s0_active;
#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE

	err = spm_s0_active(PM_S0_ADDRESS, PM_S1_ADDRESS, &s0_active);
	if (err != 0) {
		return err;
	}
#else /* CONFIG_TRUSTED_EXECUTION_NONSECURE */
	const struct fw_info *tmp_info;

	tmp_info = fw_info_find(PM_S0_ADDRESS);
	if (tmp_info == NULL) {
		return -EFAULT;
	}
	memcpy(&s0, tmp_info, sizeof(s0));

	tmp_info = fw_info_find(PM_S1_ADDRESS);
	if (tmp_info == NULL) {
		return -EFAULT;
	}
	memcpy(&s1, tmp_info, sizeof(s1));
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */

	err = dfu_ctx_mcuboot_set_b1_file(file, s0_active, &update);
	if (err != 0) {
		return err;
	}

	if (update != NULL) {
		LOG_INF("B1 update, selected file:\n%s", update);
		file = update;
	}
#endif /* PM_S1_ADDRESS */

	err = download_client_connect(&dlc, host, &config);
	if (err != 0) {
		return err;
	}

	err = download_client_start(&dlc, file, 0);
	if (err != 0) {
		download_client_disconnect(&dlc);
		return err;
	}

	return 0;
}

int fota_download_init(fota_download_callback_t client_callback)
{
	if (client_callback == NULL) {
		return -EINVAL;
	}

	int err;

	callback = client_callback;

#ifdef CONFIG_DFU_TARGET_MCUBOOT
	/* Set the required buffer for MCUboot targets */
	err = dfu_target_mcuboot_set_buf(mcuboot_buf, sizeof(mcuboot_buf));
	if (err) {
		LOG_ERR("%s failed to set MCUboot flash buffer %d",
			__func__, err);
		return err;
	}
#endif

	k_delayed_work_init(&dlc_with_offset_work, download_with_offset);

	err = download_client_init(&dlc, download_client_callback);
	if (err != 0) {
		return err;
	}

	return 0;
}
