/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_DOWNLOAD_H__
#define NRF_CLOUD_DOWNLOAD_H__

#include <dfu/dfu_target.h>
#include <net/downloader.h>

#ifdef __cplusplus
extern "C" {
#endif

enum nrf_cloud_download_type {
	NRF_CLOUD_DL_TYPE_NONE,

	/* Download a FOTA update using the fota_download library */
	NRF_CLOUD_DL_TYPE_FOTA,
	/* Download data using the downloader library */
	NRF_CLOUD_DL_TYPE_DL_CLIENT,

	NRF_CLOUD_DL_TYPE_DL__LAST
};

struct nrf_cloud_download_fota {
	/* FOTA update type */
	enum dfu_target_image_type expected_type;
	int img_sz;
	/* fota_download callback: fota_download_callback_t */
	void *cb;
};

struct nrf_cloud_download_data {
	/* Type of download to perform */
	enum nrf_cloud_download_type type;

	/* File download host */
	const char *host;
	/* File download path */
	const char *path;

	/* Downloader host configuration */
	struct downloader_host_cfg dl_host_conf;

	union {
		/* FOTA type data */
		struct nrf_cloud_download_fota fota;
		/* Downloader data */
		struct downloader *dl;
	};
};

/** @brief Start download. Only one download at a time is allowed. FOTA downloads have priority.
 *         If a FOTA download is started while a non-FOTA download is active, the non-FOTA
 *         download is stopped.
 */
int nrf_cloud_download_start(struct nrf_cloud_download_data *const cloud_dl);

/** @brief Cancel the active download.
 *         Call to stop the current download and reset the download state.
 */
void nrf_cloud_download_cancel(void);

/** @brief Reset the active download state. Call when download has ended. */
void nrf_cloud_download_end(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_DOWNLOAD_H__ */
