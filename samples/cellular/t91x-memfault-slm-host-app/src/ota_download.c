/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#include <net/downloader.h>

#include "ota_download.h"

LOG_MODULE_REGISTER(ota_download, LOG_LEVEL_INF);

static struct downloader dl;
static struct fs_file_t ota_file;
static char dl_buf[2048];
static K_SEM_DEFINE(dl_done, 0, 1);
static int dl_result;

static int dl_callback(const struct downloader_evt *evt)
{
	int err;

	switch (evt->id) {
	case DOWNLOADER_EVT_FRAGMENT:
		err = fs_write(&ota_file, evt->fragment.buf, evt->fragment.len);
		if (err < 0) {
			LOG_ERR("fs_write failed: %d", err);
			dl_result = err;
			return 1; /* non-zero stops the download */
		}
		return 0;

	case DOWNLOADER_EVT_ERROR:
		LOG_ERR("Download error: %d", evt->error);
		dl_result = evt->error;
		return 1; /* stop; zero would let the downloader reconnect and resume */

	case DOWNLOADER_EVT_DONE:
		dl_result = 0;
		k_sem_give(&dl_done);
		return 0;

	case DOWNLOADER_EVT_STOPPED:
		if (dl_result == 0) {
			dl_result = -ECANCELED;
		}
		k_sem_give(&dl_done);
		return 0;

	default:
		return 0;
	}
}

int ota_bundle_download(const char *url, const int *sec_tags, uint8_t sec_tag_count)
{
	struct downloader_cfg cfg = {
		.callback = dl_callback,
		.buf = dl_buf,
		.buf_size = sizeof(dl_buf),
	};
	struct downloader_host_cfg host_cfg = {
		.sec_tag_list = sec_tags,
		.sec_tag_count = sec_tag_count,
		.set_native_tls = true, /* TLS terminates on this MCU, the modem only carries PPP */
		.range_override = sizeof(dl_buf),
	};
	struct fs_statvfs sv;
	int err;

	/* Free the whole partition for the new bundle */
	(void)fs_unlink(OTA_BUNDLE_PATH);

	fs_file_t_init(&ota_file);
	err = fs_open(&ota_file, OTA_BUNDLE_PATH, FS_O_CREATE | FS_O_WRITE);
	if (err) {
		LOG_ERR("fs_open failed: %d", err);
		return err;
	}

	err = downloader_init(&dl, &cfg);
	if (err) {
		LOG_ERR("downloader_init failed: %d", err);
		goto out_close;
	}

	k_sem_reset(&dl_done);
	dl_result = 0;

	err = downloader_get(&dl, &host_cfg, url, 0);
	if (err) {
		LOG_ERR("downloader_get failed: %d", err);
		goto out_deinit;
	}

	k_sem_take(&dl_done, K_FOREVER);
	err = dl_result;

out_deinit:
	downloader_deinit(&dl);
out_close:
	fs_sync(&ota_file);
	fs_close(&ota_file);

	if (err) {
		(void)fs_unlink(OTA_BUNDLE_PATH); /* don't keep a partial bundle */
	} else if (fs_statvfs("/ota", &sv) == 0) {
		LOG_INF("Bundle stored, %lu of %lu blocks free", sv.f_bfree, sv.f_blocks);
	}

	return err;
}
