/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OTA_DOWNLOAD_H_
#define OTA_DOWNLOAD_H_

#include <stdint.h>

/** Path of the downloaded OTA bundle on the external flash file system. */
#define OTA_BUNDLE_PATH "/ota/bundle.bin"

/**
 * @brief Download an OTA bundle to OTA_BUNDLE_PATH.
 *
 * Blocks until the download completes or fails. Any previous bundle is removed first,
 * and a partial bundle is removed on failure.
 *
 * @param url            HTTP(S) URL of the bundle.
 * @param sec_tags       TLS security tags, or NULL for plain HTTP.
 * @param sec_tag_count  Number of entries in @p sec_tags.
 *
 * @retval 0 on success, negative errno on failure.
 */
int ota_bundle_download(const char *url, const int *sec_tags, uint8_t sec_tag_count);

#endif /* OTA_DOWNLOAD_H_ */

/**
 * @brief Process the downloaded OTA bundle and reboot.
 *
 * @retval 0 on success, negative errno on failure.
 */
int ota_bundle_apply(void);