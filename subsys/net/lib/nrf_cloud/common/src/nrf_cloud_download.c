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
#if defined(CONFIG_NRF_CLOUD_FOTA_SMP)
#include <mcumgr_smp_client.h>
#include <fota_download_util.h>
#endif
#include "nrf_cloud_download.h"
#include "nrf_cloud_downloads_internal.h"

LOG_MODULE_REGISTER(nrf_cloud_download, CONFIG_NRF_CLOUD_LOG_LEVEL);

static K_MUTEX_DEFINE(active_dl_mutex);
static struct nrf_cloud_download_data active_dl = {.type = NRF_CLOUD_DL_TYPE_NONE};

#if defined(CONFIG_FOTA_DOWNLOAD)
static int smp_fota_dl_cancel(void)
{
#if defined(CONFIG_NRF_CLOUD_FOTA_SMP)
	return mcumgr_smp_client_download_cancel();
#else
	return -ENOTSUP;
#endif /* CONFIG_NRF_CLOUD_FOTA_SMP */
}

static void smp_fota_dl_util_init(const struct nrf_cloud_download_fota *const fota)
{
#if defined(CONFIG_NRF_CLOUD_FOTA_SMP)
	fota_download_util_client_init(fota->cb, true);
#endif
}
#endif /* CONFIG_FOTA_DOWNLOAD */

static int fota_start(struct nrf_cloud_download_data *const dl)
{
#if defined(CONFIG_FOTA_DOWNLOAD)

	if (dl->fota.expected_type == DFU_TARGET_IMAGE_TYPE_SMP) {
		/* This needs to be called to set the SMP flag in fota_download */
		smp_fota_dl_util_init(&dl->fota);
	} else {
		/* This needs to be called to clear the SMP flag */
		(void)fota_download_init(dl->fota.cb);
	}

	return nrf_cloud_download_start_internal(dl);

#endif /* CONFIG_FOTA_DOWNLOAD */

	return -ENOTSUP;
}

static int dl_start(struct nrf_cloud_download_data *const dl)
{
	__ASSERT(dl->dl != NULL, "Download client is NULL");

	return nrf_cloud_download_start_internal(dl);
}

static int dl_disconnect(struct nrf_cloud_download_data *const dl)
{
	return downloader_cancel(dl->dl);
}

static void active_dl_reset(void)
{
	memset(&active_dl, 0, sizeof(active_dl));
	active_dl.type = NRF_CLOUD_DL_TYPE_NONE;
}

static int fota_dl_cancel(struct nrf_cloud_download_data *const dl)
{
	int ret = -ENOTSUP;

#if defined(CONFIG_FOTA_DOWNLOAD)
	if (dl->fota.expected_type == DFU_TARGET_IMAGE_TYPE_SMP) {
		ret = smp_fota_dl_cancel();
	} else {
		ret = fota_download_cancel();
	}
#endif /* CONFIG_FOTA_DOWNLOAD */
	return ret;
}

void nrf_cloud_download_cancel(void)
{
	int ret = 0;

	k_mutex_lock(&active_dl_mutex, K_FOREVER);

	if (active_dl.type == NRF_CLOUD_DL_TYPE_FOTA) {
		ret = fota_dl_cancel(&active_dl);
	} else if (active_dl.type == NRF_CLOUD_DL_TYPE_DL_CLIENT) {
		ret = dl_disconnect(&active_dl);
	} else {
		LOG_WRN("No active download to cancel");
	}

	if (ret) {
		LOG_WRN("Error canceling download: %d", ret);
	}

	active_dl_reset();
	k_mutex_unlock(&active_dl_mutex);
}

void nrf_cloud_download_end(void)
{
	k_mutex_lock(&active_dl_mutex, K_FOREVER);
	active_dl_reset();
	k_mutex_unlock(&active_dl_mutex);
}

static bool check_fota_file_path_len(char const *const file_path)
{
#if defined(CONFIG_FOTA_DOWNLOAD)
	/* Check that the null-terminator is found in the allowable buffer length */
	return memchr(file_path, '\0', CONFIG_FOTA_DOWNLOAD_RESOURCE_LOCATOR_LENGTH) != NULL;
#endif
	return true;
}

int nrf_cloud_download_start(struct nrf_cloud_download_data *const cloud_dl)
{
	int ret = 0;

	if (!cloud_dl || !cloud_dl->path || (cloud_dl->type <= NRF_CLOUD_DL_TYPE_NONE) ||
	    (cloud_dl->type >= NRF_CLOUD_DL_TYPE_DL__LAST)) {
		return -EINVAL;
	}

	if (cloud_dl->type == NRF_CLOUD_DL_TYPE_FOTA) {
		if (!IS_ENABLED(CONFIG_FOTA_DOWNLOAD)) {
			return -ENOTSUP;
		}
		if (!cloud_dl->fota.cb) {
			return -ENOEXEC;
		}
		if ((cloud_dl->fota.expected_type == DFU_TARGET_IMAGE_TYPE_SMP) &&
		    !IS_ENABLED(CONFIG_NRF_CLOUD_FOTA_SMP)) {
			return -ENOSYS;
		}
	}

	if (!check_fota_file_path_len(cloud_dl->path)) {
		LOG_ERR("FOTA download file path is too long");
		return -E2BIG;
	}

	k_mutex_lock(&active_dl_mutex, K_FOREVER);

	/* FOTA has priority */
	if ((active_dl.type == NRF_CLOUD_DL_TYPE_FOTA) ||
	    ((active_dl.type != NRF_CLOUD_DL_TYPE_NONE) &&
	     (cloud_dl->type != NRF_CLOUD_DL_TYPE_FOTA))) {
		k_mutex_unlock(&active_dl_mutex);
		/* A download of equal or higher priority is already active. */
		return -EBUSY;
	}

	/* If a download is active, that means the incoming download request is a FOTA
	 * type, which has priority. Cancel the active download.
	 */
	if (active_dl.type == NRF_CLOUD_DL_TYPE_DL_CLIENT) {
		LOG_INF("Stopping active download, incoming FOTA update download has priority");
		ret = dl_disconnect(&active_dl);

		if (ret) {
			LOG_ERR("Download disconnect failed, error %d", ret);
		}
	}

	active_dl = *cloud_dl;

	if (active_dl.type == NRF_CLOUD_DL_TYPE_FOTA) {
		ret = fota_start(&active_dl);
	} else if (active_dl.type == NRF_CLOUD_DL_TYPE_DL_CLIENT) {
		ret = dl_start(&active_dl);
		if (ret) {
			(void)dl_disconnect(&active_dl);
		}
	} else {
		LOG_WRN("Unhandled download type: %d", active_dl.type);
		ret = -EFTYPE;
	}

	if (ret != 0) {
		active_dl_reset();
	}

	k_mutex_unlock(&active_dl_mutex);

	return ret;
}
