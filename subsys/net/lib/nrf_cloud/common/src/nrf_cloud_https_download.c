/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nrf_cloud_download.h"
#include "nrf_cloud_downloads_internal.h"
#include <net/fota_download.h>

int nrf_cloud_download_start_internal(struct nrf_cloud_download_data *const dl)
{
#if defined(CONFIG_FOTA_DOWNLOAD)
	if (dl->type == NRF_CLOUD_DL_TYPE_FOTA) {
		return fota_download_start_with_image_type(
			dl->host, dl->path,
			dl->dl_host_conf.sec_tag_count ? dl->dl_host_conf.sec_tag_list[0] : -1,
			dl->dl_host_conf.pdn_id, dl->dl_host_conf.range_override,
			dl->fota.expected_type);
	} else {
#else
	{
#endif /* CONFIG_FOTA_DOWNLOAD */
		__ASSERT(dl->type == NRF_CLOUD_DL_TYPE_DL_CLIENT, "Unsupported download type");

		return downloader_get_with_host_and_file(dl->dl, &dl->dl_host_conf, dl->host,
							 dl->path, 0);
	}
}
