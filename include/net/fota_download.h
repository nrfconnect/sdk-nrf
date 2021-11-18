/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
#include <dfu/dfu_target.h>

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
	/** FOTA download cancelled. */
	FOTA_DOWNLOAD_EVT_CANCELLED
};

/**
 * @brief FOTA download error cause values.
 */
enum fota_download_error_cause {
	/** No error, used when event ID is not FOTA_DOWNLOAD_EVT_ERROR. */
	FOTA_DOWNLOAD_ERROR_CAUSE_NO_ERROR,
	/** Downloading the update failed. The download may be retried. */
	FOTA_DOWNLOAD_ERROR_CAUSE_DOWNLOAD_FAILED,
	/** The update is invalid and was rejected. Retry will not help. */
	FOTA_DOWNLOAD_ERROR_CAUSE_INVALID_UPDATE,
	/** Actual firmware type does not match expected. Retry will not help. */
	FOTA_DOWNLOAD_ERROR_CAUSE_TYPE_MISMATCH,
};

/**
 * @brief FOTA download event data.
 */
struct fota_download_evt {
	enum fota_download_evt_id id;

	union {
		/** Error cause. */
		enum fota_download_error_cause cause;
		/** Download progress %. */
		int progress;
	};
};

/**
 * @brief FOTA download asynchronous callback function.
 *
 * @param evt Event.
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
 * @param host Name of host to start downloading from. Can include scheme
 *             and port number, e.g. https://google.com:443
 * @param file Filepath to the file you wish to download.
 * @param sec_tag Security tag you want to use with HTTPS set to -1 to Disable.
 * @param pdn_id Packet Data Network ID to use for the download, or 0 to use the default.
 * @param fragment_size Fragment size to be used for the download.
 *			If 0, @kconfig{CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE} is used.
 *
 * @retval 0	     If download has started successfully.
 * @retval -EALREADY If download is already ongoing.
 *                   Otherwise, a negative value is returned.
 */
int fota_download_start(const char *host, const char *file, int sec_tag,
			uint8_t pdn_id, size_t fragment_size);

/**@brief Start downloading the given file from the given host. Validate that the
 * file type matches the expected type before starting the installation.
 *
 * When the download is complete, the secondary slot of MCUboot is tagged as having
 * valid firmware inside it. The completion is reported through an event.
 *
 * @param host Name of host to start downloading from. Can include scheme
 *             and port number, e.g. https://google.com:443
 * @param file Filepath to the file you wish to download.
 * @param sec_tag Security tag you want to use with HTTPS set to -1 to Disable.
 * @param pdn_id Packet Data Network ID to use for the download, or 0 to use the default.
 * @param fragment_size Fragment size to be used for the download.
 *			If 0, @kconfig{CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE} is used.
 * @param expected_type Type of firmware file to be downloaded and installed.
 *
 * @retval 0	     If download has started successfully.
 * @retval -EALREADY If download is already ongoing.
 *                   Otherwise, a negative value is returned.
 */
int fota_download_start_with_image_type(const char *host, const char *file,
			int sec_tag, uint8_t pdn_id, size_t fragment_size,
			const enum dfu_target_image_type expected_type);

/**@brief Cancel FOTA image downloading.
 *
 * @retval 0       If FOTA download is cancelled successfully.
 * @retval -EAGAIN If download is not started, aborted or completed.
 *                 Otherwise, a negative value is returned.
 */
int fota_download_cancel(void);

/**@brief Get target image type.
 *
 * Image type becomes known after download starts.
 *
 * @retval 0 Unknown type before download starts.
 *           Otherwise, a type defined in enum dfu_target_image_type.
 */
int fota_download_target(void);

#ifdef __cplusplus
}
#endif

#endif /* FOTA_DOWNLOAD_H_ */

/**@} */
