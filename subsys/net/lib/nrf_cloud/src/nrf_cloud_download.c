/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#if defined(CONFIG_FOTA_DOWNLOAD)
#include <net/fota_download.h>
#endif
#include "nrf_cloud_download.h"
LOG_MODULE_REGISTER(nrf_cloud_download, CONFIG_NRF_CLOUD_LOG_LEVEL);

static K_MUTEX_DEFINE(active_dl_mutex);
static struct nrf_cloud_download_data active_dl = { .type = NRF_CLOUD_DL_TYPE_NONE };

void nrf_cloud_download_end(void)
{
	k_mutex_lock(&active_dl_mutex, K_FOREVER);
	memset(&active_dl, 0, sizeof(active_dl));
	active_dl.type = NRF_CLOUD_DL_TYPE_NONE;
	k_mutex_unlock(&active_dl_mutex);
}

int nrf_cloud_download_start(struct nrf_cloud_download_data *const dl)
{
	if (!dl || !dl->path || (dl->type <= NRF_CLOUD_DL_TYPE_NONE) ||
	    (dl->type >= NRF_CLOUD_DL_TYPE_DL__LAST)) {
		return -EINVAL;
	}

	if (!IS_ENABLED(CONFIG_FOTA_DOWNLOAD) && (dl->type == NRF_CLOUD_DL_TYPE_FOTA)) {
		return -ENOTSUP;
	}

	int ret = 0;

	k_mutex_lock(&active_dl_mutex, K_FOREVER);

	/* FOTA has priority */
	if ((active_dl.type == NRF_CLOUD_DL_TYPE_FOTA) ||
	    ((active_dl.type != NRF_CLOUD_DL_TYPE_NONE) &&
	     (dl->type != NRF_CLOUD_DL_TYPE_FOTA))) {
		k_mutex_unlock(&active_dl_mutex);
		/* A download of equal or higher priority is already active. */
		return -EBUSY;
	}

	/* If a download is active, that means the incoming download request is a FOTA
	 * type, which has priority. Cancel the active download.
	 */
	if (active_dl.type == NRF_CLOUD_DL_TYPE_DL_CLIENT) {
		LOG_INF("Stopping active download, incoming FOTA update download has priority");
		ret = download_client_disconnect(active_dl.dlc);

		if (ret) {
			LOG_ERR("download_client_disconnect() failed, error %d", ret);
		}
	}

	if (dl->type == NRF_CLOUD_DL_TYPE_FOTA) {
#if defined(CONFIG_FOTA_DOWNLOAD)
		ret = fota_download_start_with_image_type(dl->host, dl->path,
			dl->dl_cfg.sec_tag_count ? dl->dl_cfg.sec_tag_list[0] : -1,
			dl->dl_cfg.pdn_id, dl->dl_cfg.frag_size_override, dl->fota.expected_type);
#endif
	} else if (dl->type == NRF_CLOUD_DL_TYPE_DL_CLIENT) {
		ret = download_client_get(dl->dlc, dl->host, &dl->dl_cfg,
					  dl->path, 0);
		if (ret) {
			(void)download_client_disconnect(dl->dlc);
		}
	} else {
		LOG_WRN("Unhandled download type: %d", dl->type);
		ret = -EFTYPE;
	}

	if (ret == 0) {
		active_dl = *dl;
	}
	k_mutex_unlock(&active_dl_mutex);

	return ret;
}
