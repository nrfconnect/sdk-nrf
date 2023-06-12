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

#define MAX_RESOURCE_LOCATOR_LEN 500

LOG_MODULE_REGISTER(fota_download, CONFIG_FOTA_DOWNLOAD_LOG_LEVEL);

static fota_download_callback_t callback;
static const char *dl_host;
static const char *dl_file;
static struct download_client dlc;
static struct k_work_delayable  dlc_with_offset_work;
static int socket_retries_left;
#ifdef CONFIG_DFU_TARGET_MCUBOOT
static uint8_t mcuboot_buf[CONFIG_FOTA_DOWNLOAD_MCUBOOT_FLASH_BUF_SZ] __aligned(4);
#endif
static enum dfu_target_image_type img_type;
static enum dfu_target_image_type img_type_expected = DFU_TARGET_IMAGE_TYPE_ANY;
enum flags_t {
	FLAG_DOWNLOADING,
	FLAG_FIRST_FRAGMENT,
	FLAG_RESUME,
	FLAG_NEW_URI,
	FLAG_CLOSED,
	FLAG_CANCEL,
};
static atomic_t flags;
static enum fota_download_error_cause error_state = FOTA_DOWNLOAD_ERROR_CAUSE_NO_ERROR;

static void send_evt(enum fota_download_evt_id id)
{
	__ASSERT(id != FOTA_DOWNLOAD_EVT_PROGRESS, "use send_progress");
	__ASSERT(id != FOTA_DOWNLOAD_EVT_ERROR, "use send_error_evt");
	const struct fota_download_evt evt = {
		.id = id
	};
	callback(&evt);
}

static void send_error_evt(void)
{
	__ASSERT(error_state != FOTA_DOWNLOAD_ERROR_CAUSE_NO_ERROR, "use a valid error cause");
	const struct fota_download_evt evt = {
		.id = FOTA_DOWNLOAD_EVT_ERROR,
		.cause = error_state
	};
	callback(&evt);
}

static void set_error_state(enum fota_download_error_cause cause)
{
	error_state = cause;
}

static bool is_error(void)
{
	return error_state != FOTA_DOWNLOAD_ERROR_CAUSE_NO_ERROR;
}

static void send_progress(int progress)
{
#ifdef CONFIG_FOTA_DOWNLOAD_PROGRESS_EVT
	const struct fota_download_evt evt = { .id = FOTA_DOWNLOAD_EVT_PROGRESS,
					       .progress = progress };
	callback(&evt);
#endif
}

static void stopped(void)
{
	atomic_clear_bit(&flags, FLAG_DOWNLOADING);
	if (is_error()) {
		send_error_evt();
	} else if (atomic_test_and_clear_bit(&flags, FLAG_CANCEL)) {
		send_evt(FOTA_DOWNLOAD_EVT_CANCELLED);
	} else {
		send_evt(FOTA_DOWNLOAD_EVT_FINISHED);
	}
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
		if (atomic_test_and_clear_bit(&flags, FLAG_FIRST_FRAGMENT)) {
			err = download_client_file_size_get(&dlc, &file_size);
			if (err != 0) {
				LOG_DBG("download_client_file_size_get err: %d",
					err);
				set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL);
				goto error_and_close;
			}
			img_type = dfu_target_img_type(event->fragment.buf,
							event->fragment.len);

			if (img_type == DFU_TARGET_IMAGE_TYPE_NONE) {
				LOG_ERR("Unknown image type");
				set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INVALID_UPDATE);
				err = -EFAULT;
			} else if ((img_type & img_type_expected) != img_type) {
				LOG_ERR("FOTA image type %d does not match expected type %d",
					img_type, img_type_expected);
				set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_TYPE_MISMATCH);
				err = -EPROTOTYPE;
			} else {
				err = dfu_target_init(img_type, 0, file_size,
						      dfu_target_callback_handler);
				if (err == -EFBIG) {
					LOG_ERR("Image too big");
					set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INVALID_UPDATE);
				} else if (err < 0) {
					LOG_ERR("dfu_target_init error %d", err);
					set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL);
				}
			}

			if (err) {
				goto error_and_close;
			}

			err = dfu_target_offset_get(&offset);
			if (err != 0) {
				LOG_DBG("unable to get dfu target offset err: "
					"%d", err);
				set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
				goto error_and_close;
			}

			/* Is there a DFU already running? */
			if (offset != 0) {
				if (atomic_test_bit(&flags, FLAG_NEW_URI)) {
					atomic_clear_bit(&flags, FLAG_RESUME);
					/* Image is different, reset DFU target */
					err = dfu_target_reset();

					if (err != 0) {
						LOG_ERR("Unable to reset DFU target, err: %d", err);
						set_error_state(
							FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
						goto error_and_close;
					}
					err = dfu_target_init(img_type, 0, file_size,
							      dfu_target_callback_handler);
					if (err != 0) {
						LOG_ERR("Failed to re-initialize target, err: %d",
							err);
						set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL);
						goto error_and_close;
					}
				} else {
					/* Abort current download procedure, and
					 * schedule new download from offset.
					 */
					atomic_set_bit(&flags, FLAG_RESUME);
					(void)download_client_disconnect(&dlc);
					k_work_schedule(&dlc_with_offset_work,
							K_SECONDS(1));
					LOG_INF("Refuse fragment, restart with offset");

					return -1;
				}
			} else {
				atomic_clear_bit(&flags, FLAG_RESUME);
			}
		}

		err = dfu_target_write(event->fragment.buf,
				       event->fragment.len);
		if (err && err == -EINVAL) {
			LOG_INF("Image refused");
			set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INVALID_UPDATE);
			goto error_and_close;
		} else if (err != 0) {
			LOG_ERR("dfu_target_write error %d", err);
			set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL);
			goto error_and_close;
		}

		if (IS_ENABLED(CONFIG_FOTA_DOWNLOAD_PROGRESS_EVT)) {
			err = dfu_target_offset_get(&offset);
			if (err != 0) {
				LOG_DBG("unable to get dfu target "
					"offset err: %d",
					err);
				set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL);
				goto error_and_close;
			}

			if (file_size == 0) {
				LOG_DBG("invalid file size: %d", file_size);
				set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL);
				goto error_and_close;
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
			set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL);
			goto error_and_close;
		}

		err = download_client_disconnect(&dlc);
		if (err != 0) {
			set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL);
			goto error_and_close;
		}
		break;

	case DOWNLOAD_CLIENT_EVT_ERROR:
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
			LOG_ERR("Download client error");
			err = dfu_target_done(false);
			if (err == -EACCES) {
				LOG_DBG("No DFU target was initialized");
			} else if (err != 0) {
				LOG_ERR("Unable to deinitialze resources "
					"used by dfu_target.");
			}
			set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
			goto error_and_close;
		}
		break;
	case DOWNLOAD_CLIENT_EVT_CLOSED:
		atomic_set_bit(&flags, FLAG_CLOSED);
		/* Only clear flags if we are not going to resume */
		if (!atomic_test_bit(&flags, FLAG_RESUME)) {
			stopped();
		}
		break;
	default:
		break;
	}

	return 0;

error_and_close:
	atomic_clear_bit(&flags, FLAG_RESUME);
	(void)download_client_disconnect(&dlc);
	dfu_target_done(false);
	return -1;
}

static void download_with_offset(struct k_work *unused)
{
	int offset;
	int err;

	if (!atomic_test_bit(&flags, FLAG_CLOSED)) {
		/* Re-schedule, wait for socket close */
		k_work_schedule(&dlc_with_offset_work, K_SECONDS(1));
		return;
	}

	atomic_clear_bit(&flags, FLAG_RESUME);

	err = dfu_target_offset_get(&offset);
	if (err != 0) {
		LOG_ERR("%s failed to get offset with error %d", __func__, err);
		set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL);
		goto stop_and_clear_flags;
	}

	err = download_client_get(&dlc, dl_host, &dlc.config, dl_file, offset);
	if (err != 0) {
		LOG_ERR("%s failed to start download  with error %d", __func__,
			err);
		set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
		goto stop_and_clear_flags;
	}
	LOG_INF("Downloading from offset: 0x%x", offset);
	return;

stop_and_clear_flags:
	stopped();
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

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) && defined(PM_S1_ADDRESS)
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
	static int sec_tag_list[1];
	int err = -1;

	struct download_client_cfg config = {
		.pdn_id = pdn_id,
		.frag_size_override = fragment_size,
	};

	if (host == NULL || file == NULL || callback == NULL) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&flags, FLAG_DOWNLOADING)) {
		return -EALREADY;
	}

	atomic_clear_bit(&flags, FLAG_CLOSED);
	atomic_clear_bit(&flags, FLAG_RESUME);
	set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_NO_ERROR);

	/* Verify if the URI is same as last time, if not, prevent resuming. */
	if ((!dl_host || strcmp(dl_host, host) != 0) || (!dl_file || strcmp(dl_file, file) != 0)) {
		atomic_set_bit(&flags, FLAG_NEW_URI);
	} else {
		atomic_clear_bit(&flags, FLAG_NEW_URI);
	}
	dl_host = host;
	dl_file = file;

	if (sec_tag != -1 && !is_ip_address(host)) {
		config.set_tls_hostname = true;
	}

	if (sec_tag != -1) {
		sec_tag_list[0] = sec_tag;
		config.sec_tag_list = sec_tag_list;
		config.sec_tag_count = 1;
	}

	socket_retries_left = CONFIG_FOTA_SOCKET_RETRIES;

#ifdef PM_S1_ADDRESS
	/* B1 upgrade is supported, check what B1 slot is active,
	 * (s0 or s1), and update file to point to correct candidate if
	 * space separated file is given.
	 */
	const char *update;
	bool s0_active;
	/* I need modifiable copy of the filename so I can split it */
	static char file_buf[MAX_RESOURCE_LOCATOR_LEN];

	strncpy(file_buf, file, sizeof(file_buf) - 1);
	file_buf[sizeof(file_buf) - 1] = '\0';
	dl_file = file_buf;

	err = fota_download_s0_active_get(&s0_active);
	if (err != 0) {
		atomic_clear_bit(&flags, FLAG_DOWNLOADING);
		return err;
	}

	err = fota_download_parse_dual_resource_locator(file_buf, s0_active, &update);
	if (err != 0) {
		atomic_clear_bit(&flags, FLAG_DOWNLOADING);
		return err;
	}

	if (update != NULL) {
		LOG_INF("B1 update, selected file:\n%s", update);
		dl_file = update;
	}
#endif /* PM_S1_ADDRESS */

	img_type_expected = expected_type;

	atomic_set_bit(&flags, FLAG_FIRST_FRAGMENT);

	err = download_client_get(&dlc, dl_host, &config, dl_file, 0);
	if (err != 0) {
		atomic_clear_bit(&flags, FLAG_DOWNLOADING);
		download_client_disconnect(&dlc);
		return err;
	}

	return 0;
}

int fota_download_init(fota_download_callback_t client_callback)
{
	int err;
	static bool initialized;

	if (client_callback == NULL) {
		return -EINVAL;
	}

	callback = client_callback;

	if (initialized) {
		return 0;
	}

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

	initialized = true;
	return 0;
}

int fota_download_cancel(void)
{
	int err;

	if (!atomic_test_bit(&flags, FLAG_DOWNLOADING)) {
		/* Download not started, aborted or completed */
		LOG_WRN("%s invalid state", __func__);
		return -EAGAIN;
	}

	atomic_set_bit(&flags, FLAG_CANCEL);

	err = download_client_disconnect(&dlc);
	if (err) {
		LOG_ERR("%s failed to disconnect: %d", __func__, err);
		return err;
	}

	err = dfu_target_done(false);
	if (err && err != -EACCES) {
		LOG_ERR("%s failed to clean up: %d", __func__, err);
	}

	while (atomic_test_bit(&flags, FLAG_DOWNLOADING)) {
		k_msleep(10);
	}
	return err;
}

int fota_download_target(void)
{
	return img_type;
}
