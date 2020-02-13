/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file fota_download.h
 *
 * @defgroup fota_download Firmware over-the-air download library
 * @{
 * @brief  Library for downloading a firmware update over the air.
 *
 * @details Downloads the specified file from the specified host to the
 * secondary partition of MCUboot. After the file has been downloaded,
 * the secondary slot is tagged as having valid firmware inside it.
 */

#ifndef FOTA_DOWNLOAD_H_
#define FOTA_DOWNLOAD_H_

#include <zephyr.h>
#include <zephyr/types.h>
#include <net/download_client.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FOTA download event IDs.
 */
enum fota_download_evt_id {
	/** FOTA download progress report. */
	FOTA_DOWNLOAD_EVT_PROGRESS,
	/** FOTA download finished. */
	FOTA_DOWNLOAD_EVT_FINISHED,
	/** FOTA download erase pending. */
	FOTA_DOWNLOAD_EVT_ERASE_PENDING,
	/** FOTA download erase done. */
	FOTA_DOWNLOAD_EVT_ERASE_DONE,
	/** FOTA download error. */
	FOTA_DOWNLOAD_EVT_ERROR,
};

/**
 * @brief FOTA download event data.
 */
struct fota_download_evt {
	enum fota_download_evt_id id;
	/** Download progress % */
	int progress;
};

/**
 * @brief FOTA download asynchronous callback function.
 *
 * @param event_id Event ID.
 *
 */
typedef void (*fota_download_callback_t)(const struct fota_download_evt *evt);

/**@brief Initialize the firmware over-the-air download library.
 *
 * @param client_callback Callback for the generated events.
 *
 * @retval 0 If successfully initialized.
 *           Otherwise, a negative value is returned.
 */
int fota_download_init(fota_download_callback_t client_callback);

/**@brief Start downloading the given file from the given host.
 *
 * When the download is complete, the secondary slot of MCUboot is tagged as having
 * valid firmware inside it. The completion is reported through an event.
 *
 * @retval 0	     If download has started successfully.
 * @retval -EALREADY If download is already ongoing.
 *                   Otherwise, a negative value is returned.
 */
int fota_download_start(const char *host, const char *file);

#ifdef __cplusplus
}
#endif

#endif /* FOTA_DOWNLOAD_H_ */

/**@} */
