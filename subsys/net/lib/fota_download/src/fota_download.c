/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/hash_function.h>
#include <net/fota_download.h>
#include <net/downloader.h>
#include <pm_config.h>
#include <zephyr/net/socket.h>

#include "fota_download_util.h"

#if defined(PM_S1_ADDRESS) || defined(CONFIG_DFU_TARGET_MCUBOOT)
/* MCUBoot support is required */
#include <fw_info.h>
#if CONFIG_TRUSTED_EXECUTION_NONSECURE
#include <tfm/tfm_ioctl_api.h>
#endif
#include <dfu/dfu_target_mcuboot.h>
#endif

LOG_MODULE_REGISTER(fota_download, CONFIG_FOTA_DOWNLOAD_LOG_LEVEL);

#if defined(CONFIG_FOTA_DOWNLOAD_EXTERNAL_DL)
static size_t ext_file_sz;
static size_t ext_rcvd_sz;
#endif

static fota_download_callback_t callback;
static const char *dl_host;
static const char *dl_file;
static uint32_t dl_host_hash;
static uint32_t dl_file_hash;

static struct downloader dl;
static int downloader_callback(const struct downloader_evt *event);
static char dl_buf[CONFIG_FOTA_DOWNLOAD_BUF_SZ];
static struct downloader_cfg dl_cfg = {
	.callback = downloader_callback,
	.buf = dl_buf,
	.buf_size = sizeof(dl_buf),
};
static struct downloader_host_cfg dl_host_cfg;
/** SMP MCUBoot image type */
static bool use_smp_dfu_target;
static struct k_work_delayable  dl_with_offset_work;
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
	FLAG_STOPPED,
	FLAG_CANCEL,
};
static atomic_t flags;
static enum fota_download_error_cause error_state = FOTA_DOWNLOAD_ERROR_CAUSE_NO_ERROR;
static bool initialized;

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

static void send_ext_resume(const size_t offset)
{
#if defined(CONFIG_FOTA_DOWNLOAD_EXTERNAL_DL)
	const struct fota_download_evt evt = { .id = FOTA_DOWNLOAD_EVT_RESUME_OFFSET,
					       .resume_offset = offset };
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
	case DFU_TARGET_EVT_ERASE_PENDING:
		send_evt(FOTA_DOWNLOAD_EVT_ERASE_PENDING);
		break;
	case DFU_TARGET_EVT_TIMEOUT:
		send_evt(FOTA_DOWNLOAD_EVT_ERASE_TIMEOUT);
		break;
	case DFU_TARGET_EVT_ERASE_DONE:
		send_evt(FOTA_DOWNLOAD_EVT_ERASE_DONE);
		break;
	}
}

static size_t file_size_get(size_t *size)
{
#if defined(CONFIG_FOTA_DOWNLOAD_EXTERNAL_DL)
	*size = ext_file_sz;
	return 0;
#endif
	return downloader_file_size_get(&dl, size);
}

static size_t downloaded_size_get(size_t *size)
{
#if defined(CONFIG_FOTA_DOWNLOAD_EXTERNAL_DL)
	*size = ext_rcvd_sz;
	return 0;
#endif
	return downloader_downloaded_size_get(&dl, size);
}

static int dl_cancel(void)
{
#if defined(CONFIG_FOTA_DOWNLOAD_EXTERNAL_DL)
	return 0;
#endif
	return downloader_cancel(&dl);
}

static int downloader_callback(const struct downloader_evt *event)
{
	static size_t file_size;
	size_t offset;
	int err;

	if (event == NULL) {
		return -EINVAL;
	}

	switch (event->id) {
	case DOWNLOADER_EVT_FRAGMENT: {
		if (atomic_test_and_clear_bit(&flags, FLAG_FIRST_FRAGMENT)) {
			err = file_size_get(&file_size);
			if (err != 0) {
				LOG_DBG("file_size_get err: %d", err);
				set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL);
				goto error_and_close;
			}
			if (use_smp_dfu_target) {
				/* Validate SMP target type */
				img_type = dfu_target_smp_img_type_check(event->fragment.buf,
									 event->fragment.len);
			} else {
				img_type = dfu_target_img_type(event->fragment.buf,
							       event->fragment.len);
			}

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
					(void)dl_cancel();
					k_work_schedule(&dl_with_offset_work, K_SECONDS(1));
					LOG_INF("Refuse fragment, restart with offset");
					return -1;
				}
			} else {
				atomic_clear_bit(&flags, FLAG_RESUME);
			}
		}

#if defined(CONFIG_FOTA_DOWNLOAD_EXTERNAL_DL)
		ext_rcvd_sz += event->fragment.len;
#endif

		err = dfu_target_write(event->fragment.buf, event->fragment.len);
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
			err = downloaded_size_get(&offset);
			if (err != 0) {
				LOG_DBG("unable to get downloaded size err: %d", err);
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

	case DOWNLOADER_EVT_DONE:
		err = dfu_target_done(true);
		if (err == 0 && IS_ENABLED(CONFIG_FOTA_CLIENT_AUTOSCHEDULE_UPDATE)) {
			err = dfu_target_schedule_update(0);
		}

		if (err != 0) {
			LOG_ERR("dfu_target_done error: %d", err);
			set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL);
			goto error_and_close;
		}

		atomic_clear_bit(&flags, FLAG_DOWNLOADING);
		atomic_set_bit(&flags, FLAG_STOPPED);
		send_evt(FOTA_DOWNLOAD_EVT_FINISHED);

		break;

	case DOWNLOADER_EVT_ERROR:
		/* In case of socket errors we can return 0 to retry/continue,
		 * or non-zero to stop
		 */
		if ((socket_retries_left) && (event->error == -ECONNRESET)) {
			LOG_WRN("Download socket error. %d retries left...",
				socket_retries_left);
			socket_retries_left--;
			/* Fall through and return 0 below to tell
			 * downloader to retry
			 */
		} else if ((event->error == -ECONNABORTED) ||
			   (event->error == -ECONNREFUSED) ||
			   (event->error == -EHOSTUNREACH)) {
			LOG_ERR("Download client failed to connect to server");
			set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_CONNECT_FAILED);

			goto error_and_close;
		} else {
			LOG_ERR("Downloader error event %d", event->error);
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
	case DOWNLOADER_EVT_STOPPED:
		atomic_set_bit(&flags, FLAG_STOPPED);
		/* Only clear flags if we are not going to resume */
		if (!atomic_test_bit(&flags, FLAG_RESUME)) {
			stopped();
		}
		break;
	case DOWNLOADER_EVT_DEINITIALIZED:
		/* Not implemented in fota download */
	default:
		break;
	}

	return 0;

error_and_close:
	atomic_clear_bit(&flags, FLAG_RESUME);
	dfu_target_done(false);
	return -1;
}

static int get_from_offset(const size_t offset)
{
	if (IS_ENABLED(CONFIG_FOTA_DOWNLOAD_EXTERNAL_DL)) {
		send_ext_resume(offset);
		return 0;
	}

	int err = downloader_get_with_host_and_file(&dl, &dl_host_cfg, dl_host, dl_file, offset);

	if (err != 0) {
		LOG_ERR("%s failed to start download with error %d", __func__, err);
		set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED);
		return err;
	}

	LOG_INF("Downloading from offset: 0x%x", offset);
	return 0;
}

static void download_with_offset(struct k_work *unused)
{
	size_t offset;
	int err;

	if (!atomic_test_bit(&flags, FLAG_STOPPED)) {
		/* Re-schedule, wait for previous download to be stopped */
		k_work_schedule(&dl_with_offset_work, K_SECONDS(1));
		return;
	}

	atomic_clear_bit(&flags, FLAG_RESUME);

	err = dfu_target_offset_get(&offset);
	if (err != 0) {
		LOG_ERR("%s failed to get offset with error %d", __func__, err);
		set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL);
		goto stop_and_clear_flags;
	}

	err = get_from_offset(offset);
	if (err != 0) {
		goto stop_and_clear_flags;
	}

	return;

stop_and_clear_flags:
	stopped();
}

int fota_download_b1_file_parse(char *s0_s1_files)
{
#if !defined(PM_S1_ADDRESS)
	return -EOPNOTSUPP;
#endif
	/* B1 upgrade is supported, check what B1 slot is active,
	 * (s0 or s1), and update file to point to correct candidate if
	 * space separated file is given.
	 */
	const char *update;
	bool s0_active;
	int err;

	err = fota_download_s0_active_get(&s0_active);
	if (err != 0) {
		return -EIO;
	}

	err = fota_download_parse_dual_resource_locator(s0_s1_files, s0_active, &update);
	if (err != 0) {
		return -ENOEXEC;
	}

	if (update == NULL) {
		/* No delimeter found */
		return -ENOMSG;
	}

	LOG_INF("B1 update, selected s%u file:\n%s", s0_active ? 1 : 0, update);

	if (update != s0_s1_files) {
		/* Move the last file to the front */
		memmove(s0_s1_files, update, strlen(update) + 1);
	}

	return 0;
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
	err = tfm_platform_s0_active(PM_S0_ADDRESS, PM_S1_ADDRESS, s0_active);
#else /* CONFIG_TRUSTED_EXECUTION_NONSECURE */
	err = read_s0_active(PM_S0_ADDRESS, PM_S1_ADDRESS, s0_active);
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */
	return err;
#else /* PM_S1_ADDRESS */
	return -ENOENT;
#endif /* PM_S1_ADDRESS */
}

int fota_download_any(const char *host, const char *file, const int *sec_tag_list,
		      uint8_t sec_tag_count, uint8_t pdn_id, size_t fragment_size)
{
	return fota_download(host, file, sec_tag_list, sec_tag_count, pdn_id, fragment_size,
			     DFU_TARGET_IMAGE_TYPE_ANY);
}

static void set_host_and_file(char const *const host, char const *const file)
{
	uint32_t host_hash;
	uint32_t file_hash;

	host_hash = sys_hash32(host, strlen(host));
	file_hash = sys_hash32(file, strlen(file));

	LOG_DBG("URI checksums %d,%d,%d,%d\r\n", host_hash, file_hash, dl_host_hash, dl_file_hash);

	/* Verify if the URI is same as last time, if not, prevent resuming. */
	if (dl_host_hash != host_hash || dl_file_hash != file_hash) {
		atomic_set_bit(&flags, FLAG_NEW_URI);
	} else {
		atomic_clear_bit(&flags, FLAG_NEW_URI);
	}

	dl_host_hash = host_hash;
	dl_file_hash = file_hash;

	dl_host = host;
	dl_file = file;
}

#if defined(CONFIG_FOTA_DOWNLOAD_EXTERNAL_DL)
int fota_download_external_evt_handle(struct downloader_evt const *const evt)
{
	return downloader_callback(evt);
}

int fota_download_external_start(const char *host, const char *file,
				 const enum dfu_target_image_type expected_type,
				 const size_t image_size)
{
	if (host == NULL || file == NULL || callback == NULL || image_size == 0) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&flags, FLAG_DOWNLOADING)) {
		return -EALREADY;
	}

	atomic_clear_bit(&flags, FLAG_STOPPED);
	atomic_clear_bit(&flags, FLAG_RESUME);
	set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_NO_ERROR);

	set_host_and_file(host, file);

	socket_retries_left = CONFIG_FOTA_SOCKET_RETRIES;

	img_type_expected = expected_type;
	ext_file_sz = image_size;
	ext_rcvd_sz = 0;

	atomic_set_bit(&flags, FLAG_FIRST_FRAGMENT);

	return 0;
}
#endif /* CONFIG_FOTA_DOWNLOAD_EXTERNAL_DL */

int fota_download(const char *host, const char *file,
	const int *sec_tag_list, uint8_t sec_tag_count, uint8_t pdn_id, size_t fragment_size,
	const enum dfu_target_image_type expected_type)
{
	__ASSERT_NO_MSG(!IS_ENABLED(CONFIG_FOTA_DOWNLOAD_EXTERNAL_DL));

	if (host == NULL || file == NULL || callback == NULL) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&flags, FLAG_DOWNLOADING)) {
		return -EALREADY;
	}

	int err;
	static int sec_tag_list_copy[CONFIG_FOTA_DOWNLOAD_SEC_TAG_LIST_SIZE_MAX];
	dl_host_cfg.pdn_id = pdn_id;
	dl_host_cfg.range_override = fragment_size;

	if (sec_tag_count > ARRAY_SIZE(sec_tag_list_copy)) {
		return -E2BIG;
	}

	atomic_clear_bit(&flags, FLAG_STOPPED);
	atomic_clear_bit(&flags, FLAG_RESUME);
	set_error_state(FOTA_DOWNLOAD_ERROR_CAUSE_NO_ERROR);

	set_host_and_file(host, file);

	if ((sec_tag_list != NULL) && (sec_tag_count > 0)) {
		memcpy(sec_tag_list_copy, sec_tag_list, sec_tag_count * sizeof(sec_tag_list[0]));

		dl_host_cfg.sec_tag_count = sec_tag_count;
		dl_host_cfg.sec_tag_list = sec_tag_list_copy;
	}

	socket_retries_left = CONFIG_FOTA_SOCKET_RETRIES;

#ifdef PM_S1_ADDRESS
	/* Need a modifiable copy of the filename for splitting */
	static char file_buf[CONFIG_FOTA_DOWNLOAD_RESOURCE_LOCATOR_LENGTH];

	strncpy(file_buf, file, sizeof(file_buf) - 1);
	file_buf[sizeof(file_buf) - 1] = '\0';

	err = fota_download_b1_file_parse(file_buf);
	if (err == 0) {
		/* Save the parsed file */
		dl_file = file_buf;
	} else if ((err == -EIO) || (err == -ENOEXEC)) {
		atomic_clear_bit(&flags, FLAG_DOWNLOADING);
		return err;
	}
#endif /* PM_S1_ADDRESS */

	img_type_expected = expected_type;

	atomic_set_bit(&flags, FLAG_FIRST_FRAGMENT);

	err = downloader_get_with_host_and_file(&dl, &dl_host_cfg, dl_host, dl_file, 0);
	if (err != 0) {
		atomic_clear_bit(&flags, FLAG_DOWNLOADING);
		(void)dl_cancel();
		return err;
	}

	return 0;
}

int fota_download_start(const char *host, const char *file, int sec_tag, uint8_t pdn_id,
			size_t fragment_size)
{
	int sec_tag_list[1] = { sec_tag };
	uint8_t sec_tag_count = sec_tag < 0 ? 0 : 1;

	return fota_download_any(host, file, sec_tag_list, sec_tag_count, pdn_id, fragment_size);
}

int fota_download_start_with_image_type(const char *host, const char *file,
					int sec_tag, uint8_t pdn_id, size_t fragment_size,
					const enum dfu_target_image_type expected_type)
{
	int sec_tag_list[1] = { sec_tag };
	uint8_t sec_tag_count = sec_tag < 0 ? 0 : 1;

	return fota_download(host, file, sec_tag_list, sec_tag_count, pdn_id, fragment_size,
			     expected_type);
}

static int fota_download_object_init(void)
{
	int err;

#ifdef CONFIG_FOTA_DOWNLOAD_NATIVE_TLS
	/* Enable native TLS for the download client socket
	 * if configured.
	 */
	dl_host_cfg.native_tls = CONFIG_FOTA_DOWNLOAD_NATIVE_TLS;
#endif

	k_work_init_delayable(&dl_with_offset_work, download_with_offset);

	err = downloader_init(&dl, &dl_cfg);
	if (err != 0) {
		return err;
	}

	initialized = true;
	return 0;
}

int fota_download_util_client_init(fota_download_callback_t client_callback, bool smp_image_type)
{
	int err;

	if (client_callback == NULL) {
		return -EINVAL;
	}

	callback = client_callback;
	err = fota_download_util_stream_init();
	if (err) {
		LOG_ERR("%s failed to init stream %d", __func__, err);
		return err;
	}
	use_smp_dfu_target = smp_image_type;

	if (initialized) {
		return 0;
	}

	return fota_download_object_init();
}

int fota_download_init(fota_download_callback_t client_callback)
{
	if (client_callback == NULL) {
		return -EINVAL;
	}

	callback = client_callback;

#ifdef CONFIG_DFU_TARGET_MCUBOOT
	int err;

	/* Set the required buffer for MCUboot targets */
	err = dfu_target_mcuboot_set_buf(mcuboot_buf, sizeof(mcuboot_buf));
	if (err) {
		LOG_ERR("%s failed to set MCUboot flash buffer %d", __func__, err);
		return err;
	}
#endif

	use_smp_dfu_target = false;
	if (initialized) {
		return 0;
	}

	return fota_download_object_init();
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

	err = dl_cancel();
	if (err) {
		LOG_ERR("%s failed to stop download: %d", __func__, err);
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
