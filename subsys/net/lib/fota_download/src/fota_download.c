/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdint.h>
#include <zephyr.h>
#include <flash.h>
#include <net/download_client.h>
#include <dfu/mcuboot.h>
#include <dfu/flash_img.h>
#include <pm_config.h>
#include <logging/log.h>
#include <net/fota_download.h>

LOG_MODULE_REGISTER(fota_download, CONFIG_FOTA_DOWNLOAD_LOG_LEVEL);

static		fota_download_callback_t callback;
static struct	flash_img_context flash_img;
static struct	download_client dfu;
static size_t	bytes_sent_to_flash;

static int download_client_callback(const struct download_client_evt *event)
{
	int err;

	if (event == NULL) {
		return -EINVAL;
	}

	switch (event->id) {
	case DOWNLOAD_CLIENT_EVT_FRAGMENT: {
		size_t size;

		err = download_client_file_size_get(&dfu, &size);
		if (err != 0) {
			LOG_ERR("download_client_file_size_get error %d", err);
			callback(FOTA_DOWNLOAD_EVT_ERROR);
			return err;
		}
		if (size > PM_MCUBOOT_SECONDARY_SIZE) {
			LOG_ERR("Requested file too big to fit in flash\n");
			callback(FOTA_DOWNLOAD_EVT_ERROR);
			return -EFBIG;
		}

		err = flash_img_buffered_write(&flash_img,
				(u8_t *)event->fragment.buf,
				event->fragment.len, false);
		if (err != 0) {
			LOG_ERR("flash_img_buffered_write error %d", err);
			callback(FOTA_DOWNLOAD_EVT_ERROR);
			return err;
		}
		bytes_sent_to_flash += event->fragment.len;
		break;
	}

	case DOWNLOAD_CLIENT_EVT_DONE:
		/* Write with 0 length to flush the write operation to flash. */
		err = flash_img_buffered_write(&flash_img,
				(u8_t *)event->fragment.buf,
				0, true);
		if (err != 0) {
			LOG_ERR("flash_img_buffered_write error %d", err);
			callback(FOTA_DOWNLOAD_EVT_ERROR);
			return err;
		}

		err = boot_request_upgrade(BOOT_UPGRADE_TEST);
		if (err != 0) {
			LOG_ERR("boot_request_upgrade error %d", err);
			callback(FOTA_DOWNLOAD_EVT_ERROR);
			return err;
		}
		err = download_client_disconnect(&dfu);
		if (err != 0) {
			LOG_ERR("download_client_disconnect error %d", err);
			callback(FOTA_DOWNLOAD_EVT_ERROR);
			return err;
		}
		callback(FOTA_DOWNLOAD_EVT_FINISHED);
		break;

	case DOWNLOAD_CLIENT_EVT_ERROR: {
		download_client_disconnect(&dfu);
		LOG_ERR("Download client error");
		callback(FOTA_DOWNLOAD_EVT_ERROR);
		return event->error;
	}
	default:
		break;
	}

	return 0;
}

static int fota_download_from_offset(char *host, char *file, size_t offset)
{
	int err;
	struct download_client_cfg config = {
		.sec_tag = -1, /* HTTP */
	};

	err = download_client_connect(&dfu, host, &config);
	if (err != 0) {
		LOG_ERR("download_client_connect error %d", err);
		return err;
	}

	err = download_client_start(&dfu, file, offset);
	if (err != 0) {
		LOG_ERR("download_client_start error %d", err);
		download_client_disconnect(&dfu);
		return err;
	}

	return 0;
}

int fota_download_start(char *host, char *file)
{
	int err;

	if (host == NULL || file == NULL || callback == NULL) {
		return -EINVAL;
	}

	/* Verify that a download is not already ongoing */
	if (dfu.fd != -1) {
		return -EALREADY;
	}

	err = flash_img_init(&flash_img);
	if (err != 0) {
		LOG_ERR("flash_img_init error %d", err);
		return err;
	}

	bytes_sent_to_flash = 0;

	return fota_download_from_offset(host, file, 0);
}

int fota_download_continue(char *host, char *file)
{
	if (host == NULL || file == NULL ||
	    callback == NULL || bytes_sent_to_flash == 0) {
		return -EINVAL;
	}

	/* If an existing socket it still open, close it in case that
	 * connection is stuck
	 */
	if (dfu.fd != -1) {
		int err = download_client_disconnect(&dfu);

		if (err != 0) {
			LOG_ERR("download_client_disconnect error %d", err);
			return err;
		}
	}

	return fota_download_from_offset(host, file, bytes_sent_to_flash);
}

int fota_download_init(fota_download_callback_t client_callback)
{
	int err;

	if (client_callback == NULL) {
		return -EINVAL;
	}

	callback = client_callback;

	err = download_client_init(&dfu, download_client_callback);
	if (err != 0) {
		LOG_ERR("download_client_init error %d", err);
		return err;
	}

	return 0;
}
