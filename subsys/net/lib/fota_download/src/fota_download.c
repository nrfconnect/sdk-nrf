/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <net/fota_download.h>
#include <net/download_client.h>
#include <pm_config.h>
#include <zephyr/net/socket.h>

#include "fota_download_util.h"

#if defined(PM_S1_ADDRESS) || defined(CONFIG_DFU_TARGET_MCUBOOT)
/* MCUBoot support is required */
#include <fw_info.h>
#if CONFIG_BUILD_WITH_TFM
#include <tfm_ioctl_api.h>
#endif
#include <dfu/dfu_target_mcuboot.h>
#endif

/* If bootloader upgrades are supported we need room for two file strings. */
#ifdef PM_S1_ADDRESS
/* One file string for each of s0 and s1, and a space separator */
#define FILE_BUF_LEN ((CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE*2)+1)
#else
#define FILE_BUF_LEN (CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE)
#endif

LOG_MODULE_REGISTER(fota_download, CONFIG_FOTA_DOWNLOAD_LOG_LEVEL);

static fota_download_callback_t callback;
static struct download_client dlc;
static struct k_work_delayable  dlc_with_offset_work;
static int socket_retries_left;
#ifdef CONFIG_DFU_TARGET_MCUBOOT
static uint8_t mcuboot_buf[CONFIG_FOTA_DOWNLOAD_MCUBOOT_FLASH_BUF_SZ] __aligned(4);
#endif
static enum dfu_target_image_type img_type;
static enum dfu_target_image_type img_type_expected = DFU_TARGET_IMAGE_TYPE_ANY;
static bool first_fragment;
static bool downloading;

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
	first_fragment = true;
	downloading = false;
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
	static size_t file_size;
	size_t offset;
	int err;

	if (event == NULL) {
		return -EINVAL;
	}

	switch (event->id) {
	case DOWNLOAD_CLIENT_EVT_FRAGMENT: {
		if (first_fragment) {
			enum fota_download_error_cause err_cause =
				FOTA_DOWNLOAD_ERROR_CAUSE_NO_ERROR;

			err = download_client_file_size_get(&dlc, &file_size);
			if (err != 0) {
				LOG_DBG("download_client_file_size_get err: %d",
					err);
				send_error_evt(FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL);
				return err;
			}
			first_fragment = false;
			img_type = dfu_target_img_type(event->fragment.buf,
							event->fragment.len);

			if (img_type == DFU_TARGET_IMAGE_TYPE_NONE) {
				LOG_ERR("Unknown image type");
				err_cause = FOTA_DOWNLOAD_ERROR_CAUSE_INVALID_UPDATE;
				err = -EFAULT;
			} else if ((img_type & img_type_expected) != img_type) {
				LOG_ERR("FOTA image type %d does not match expected type %d",
					img_type, img_type_expected);
				err_cause = FOTA_DOWNLOAD_ERROR_CAUSE_TYPE_MISMATCH;
				err = -EPROTOTYPE;
			} else {
				err = dfu_target_init(img_type, 0, file_size,
						      dfu_target_callback_handler);
				if (err == -EFBIG) {
					LOG_ERR("Image too big");
					err_cause = FOTA_DOWNLOAD_ERROR_CAUSE_INVALID_UPDATE;
				} else if (err < 0) {
					LOG_ERR("dfu_target_init error %d", err);
					err_cause = FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL;
				}
			}

			if (err_cause != FOTA_DOWNLOAD_ERROR_CAUSE_NO_ERROR) {
				(void)download_client_disconnect(&dlc);
				send_error_evt(err_cause);
				int res = dfu_target_reset();

				if (res != 0) {
					LOG_ERR("Unable to reset DFU target, err: %d",
						res);
				}
				first_fragment = true;
				return err;
			}

			err = dfu_target_offset_get(&offset);
			if (err != 0) {
				LOG_DBG("unable to get dfu target offset err: "
					"%d", err);
				send_error_evt(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);

				return -1;
			}

			if (offset != 0) {
				/* Abort current download procedure, and
				 * schedule new download from offset.
				 */
				(void)download_client_disconnect(&dlc);
				k_work_schedule(&dlc_with_offset_work,
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
		if (err == 0 && IS_ENABLED(CONFIG_FOTA_CLIENT_AUTOSCHEDULE_UPDATE)) {
			err = dfu_target_schedule_update(0);
		}

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
		downloading = false;
		break;

	case DOWNLOAD_CLIENT_EVT_ERROR: {
		/* In case of socket errors we can return 0 to retry/continue,
		 * or non-zero to stop
		 */
		if ((socket_retries_left) && ((event->error == -ENOTCONN) ||
					      (event->error == -ECONNRESET) ||
					      (event->error == -ETIMEDOUT))) {
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
		send_error_evt(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
		return;
	}

	err = download_client_get(&dlc, dlc.host, &dlc.config, dlc.file, offset);
	if (err != 0) {
		LOG_ERR("%s failed to start download  with error %d", __func__,
			err);
		send_error_evt(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
		return;
	}
	LOG_INF("Downloading from offset: 0x%x", offset);
	downloading = true;
	return;
}

int fota_download_start(const char *host, const char *file, int sec_tag,
	uint8_t pdn_id, size_t fragment_size)
{
	return fota_download_start_with_image_type(host, file, sec_tag, pdn_id,
		fragment_size, DFU_TARGET_IMAGE_TYPE_ANY);
}

static bool is_ip_address(const char *host)
{
	struct sockaddr sa;

	if (zsock_inet_pton(AF_INET, host, sa.data) == 1) {
		return true;
	} else if (zsock_inet_pton(AF_INET6, host, sa.data) == 1) {
		return true;
	}

	return false;
}

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
static int read_s0_active(uint32_t s0_address, uint32_t s1_address,
			  bool *const s0_active)
{
	const struct fw_info *s0;
	const struct fw_info *s1;

	if (!s0_active) {
		return -EINVAL;
	}

	s0 = fw_info_find(s0_address);
	if (s0 == NULL) {
		return -EFAULT;
	}

	s1 = fw_info_find(s1_address);
	if (s1 == NULL) {
		/* No s1 found, s0 is active */
		*s0_active = true;
	} else {
		/* Both s0 and s1 found, check who is active */
		*s0_active = s0->version >= s1->version;
	}

	return 0;
}
#endif /* !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) */

int fota_download_s0_active_get(bool *const s0_active)
{
#ifdef PM_S1_ADDRESS
	int err;

#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE
#if CONFIG_SPM_SERVICE_S0_ACTIVE
	err = spm_s0_active(PM_S0_ADDRESS, PM_S1_ADDRESS, s0_active);
#elif CONFIG_BUILD_WITH_TFM
	err = tfm_platform_s0_active(PM_S0_ADDRESS, PM_S1_ADDRESS, s0_active);
#else
#error "Not possible to read s0 active status"
#endif
#else /* CONFIG_TRUSTED_EXECUTION_NONSECURE */
	err = read_s0_active(PM_S0_ADDRESS, PM_S1_ADDRESS, s0_active);
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */
	return err;
#else /* PM_S1_ADDRESS */
	return -ENOENT;
#endif /* PM_S1_ADDRESS */
}

int fota_download_start_with_image_type(const char *host, const char *file,
	int sec_tag, uint8_t pdn_id, size_t fragment_size,
	const enum dfu_target_image_type expected_type)
{
	/* We need a static file buffer since the download client structure
	 * only keeps a pointer to the file buffer. This is problematic when
	 * a download needs to be restarted for some reason (e.g. if
	 * continuing a download operation from an offset).
	 */
	static char file_buf[FILE_BUF_LEN];
	const char *file_buf_ptr = file_buf;
	int err = -1;

	struct download_client_cfg config = {
		.sec_tag = sec_tag,
		.pdn_id = pdn_id,
		.frag_size_override = fragment_size,
	};

	if (host == NULL || file == NULL || callback == NULL) {
		return -EINVAL;
	}

	if (downloading) {
		return -EALREADY;
	}

	if (sec_tag != -1 && !is_ip_address(host)) {
		config.set_tls_hostname = true;
	}

	socket_retries_left = CONFIG_FOTA_SOCKET_RETRIES;

	strncpy(file_buf, file, sizeof(file_buf) - 1);
	file_buf[sizeof(file_buf) - 1] = '\0';

#ifdef PM_S1_ADDRESS
	/* B1 upgrade is supported, check what B1 slot is active,
	 * (s0 or s1), and update file to point to correct candidate if
	 * space separated file is given.
	 */
	const char *update;
	bool s0_active;

	err = fota_download_s0_active_get(&s0_active);
	if (err != 0) {
		return err;
	}

	err = fota_download_parse_dual_resource_locator(file_buf, s0_active, &update);
	if (err != 0) {
		return err;
	}

	if (update != NULL) {
		LOG_INF("B1 update, selected file:\n%s", update);
		file_buf_ptr = update;
	}
#endif /* PM_S1_ADDRESS */

	img_type_expected = expected_type;

	err = download_client_get(&dlc, host, &config, file_buf_ptr, 0);
	if (err != 0) {
		download_client_disconnect(&dlc);
		return err;
	}

	downloading = true;

	return 0;
}

int fota_download_init(fota_download_callback_t client_callback)
{
	if (client_callback == NULL) {
		return -EINVAL;
	}

	int err;

	callback = client_callback;

#ifdef CONFIG_FOTA_DOWNLOAD_NATIVE_TLS
	/* Enable native TLS for the download client socket
	 * if configured.
	 */
	dlc.set_native_tls = CONFIG_FOTA_DOWNLOAD_NATIVE_TLS;
#endif

#ifdef CONFIG_DFU_TARGET_MCUBOOT
	/* Set the required buffer for MCUboot targets */
	err = dfu_target_mcuboot_set_buf(mcuboot_buf, sizeof(mcuboot_buf));
	if (err) {
		LOG_ERR("%s failed to set MCUboot flash buffer %d",
			__func__, err);
		return err;
	}
#endif

	k_work_init_delayable(&dlc_with_offset_work, download_with_offset);

	err = download_client_init(&dlc, download_client_callback);
	if (err != 0) {
		return err;
	}

	first_fragment = true;
	return 0;
}

int fota_download_cancel(void)
{
	int err;

	if (!downloading) {
		/* Download not started, aborted or completed */
		LOG_WRN("%s invalid state", __func__);
		return -EAGAIN;
	}

	err = download_client_disconnect(&dlc);
	if (err) {
		LOG_ERR("%s failed to disconnect: %d", __func__, err);
		return err;
	}
	downloading = false;

	err = dfu_target_done(false);
	if (err && err != -EACCES) {
		LOG_ERR("%s failed to clean up: %d", __func__, err);
	} else {
		first_fragment = true;
		send_evt(FOTA_DOWNLOAD_EVT_CANCELLED);
	}

	return err;
}

int fota_download_target(void)
{
	return img_type;
}
