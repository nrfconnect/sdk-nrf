/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <logging/log.h>
#include <net/fota_download.h>
#include <net/download_client.h>
#include <dfu/dfu_target.h>
#include <pm_config.h>

#ifdef PM_S1_ADDRESS
/* MCUBoot support is required */
#include <fw_info.h>
#include <secure_services.h>
#include <dfu_target_mcuboot.h>
#endif

LOG_MODULE_REGISTER(fota_download, CONFIG_FOTA_DOWNLOAD_LOG_LEVEL);

static fota_download_callback_t callback;
static struct download_client   dlc;
static struct k_delayed_work    dlc_with_offset_work;
static int socket_retries_left;

static int download_client_callback(const struct download_client_evt *event)
{
	static bool first_fragment = true;
	size_t file_size;
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
				callback(FOTA_DOWNLOAD_EVT_ERROR);
				return err;
			}
			first_fragment = false;
			int img_type = dfu_target_img_type(event->fragment.buf,
							event->fragment.len);
			err = dfu_target_init(img_type, file_size);
			if ((err < 0) && (err != -EBUSY)) {
				LOG_ERR("dfu_target_init error %d", err);
				return err;
			}

			err = dfu_target_offset_get(&offset);
			LOG_INF("Offset: 0x%x", offset);

			if (offset != 0) {
				/* Abort current download procedure, and
				 * schedule new download from offset.
				 */
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
			err = download_client_disconnect(&dlc);
			callback(FOTA_DOWNLOAD_EVT_ERROR);
			return err;
		}
	break;
	}

	case DOWNLOAD_CLIENT_EVT_DONE:
		err = dfu_target_done(true);
		if (err != 0) {
			LOG_ERR("dfu_target_done error: %d", err);
			callback(FOTA_DOWNLOAD_EVT_ERROR);
			return err;
		}

		err = download_client_disconnect(&dlc);
		if (err != 0) {
			callback(FOTA_DOWNLOAD_EVT_ERROR);
			return err;
		}
		callback(FOTA_DOWNLOAD_EVT_FINISHED);
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
			if (err != 0) {
				LOG_ERR("Unable to deinitialze resources "
					"used by dfu_target.");
			}
			first_fragment = true;
			callback(FOTA_DOWNLOAD_EVT_ERROR);
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

	err = download_client_start(&dlc, dlc.file, offset);

	LOG_INF("Downloading from offset: 0x%x", offset);
	if (err != 0) {
		LOG_ERR("%s failed with error %d", __func__, err);
	}
}

int fota_download_start(char *host, char *file)
{
	int err = -1;

	struct download_client_cfg config = {
		.sec_tag = -1, /* HTTP */
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
	char *update;
	struct fw_info s0;
	struct fw_info s1;

	err = spm_firmware_info(PM_S0_ADDRESS, &s0);
	if (err != 0) {
		return err;
	}

	err = spm_firmware_info(PM_S1_ADDRESS, &s1);
	if (err != 0) {
		return err;
	}

	bool s0_active = s0.version >= s1.version;

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

	callback = client_callback;

	k_delayed_work_init(&dlc_with_offset_work, download_with_offset);

	int err = download_client_init(&dlc, download_client_callback);

	if (err != 0) {
		return err;
	}

	return 0;
}
