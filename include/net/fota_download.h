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

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <net/download_client.h>
#include <dfu/dfu_target.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FOTA download event IDs.
 *
 * After FINISHED, ERROR or CANCELLED event, the FOTA client is ready to
 * accept new request.
 */
enum fota_download_evt_id {
	/** FOTA download progress report. */
	FOTA_DOWNLOAD_EVT_PROGRESS,

	/** FOTA download was completed successfully. */
	FOTA_DOWNLOAD_EVT_FINISHED,

	/** Deletion of the downloaded FOTA image is in progress (after upgrade success or failure).
	 *
	 * Not fired for all FOTA types.
	 */
	FOTA_DOWNLOAD_EVT_ERASE_PENDING,

	/** Deletion of the downloaded FOTA image has reached the defined timeout. It will continue.
	 *
	 * Not fired for all FOTA types.
	 */
	FOTA_DOWNLOAD_EVT_ERASE_TIMEOUT,

	/** Deletion of the downloaded FOTA image is complete.
	 *
	 * Not fired for all FOTA types.
	 */
	FOTA_DOWNLOAD_EVT_ERASE_DONE,

	/** FOTA download abandoned due to an error. */
	FOTA_DOWNLOAD_EVT_ERROR,

	/** FOTA download abandoned due to a cancellation request. */
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
	/** Generic error on device side. */
	FOTA_DOWNLOAD_ERROR_CAUSE_INTERNAL,
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

/**@brief Download the given file with the specified image type from the given host.
 *
 * Validate that the file type matches the expected type before proceeding with the download.
 * When the download is complete, the secondary slot of MCUboot is tagged as having
 * valid firmware inside it. The completion is reported through an event.
 *
 * URI parameters (host and file) are not copied, so pointers must stay valid
 * until download is finished.
 *
 * @param host Name of host to start downloading from. Can include scheme
 *             and port number, for example https://google.com:443
 * @param file Path to the file you wish to download. See fota_download_any()
 *             for details on expected format.
 * @param sec_tag_list Security tags that you want to use with HTTPS. Pass NULL to disable TLS.
 * @param sec_tag_count Number of TLS security tags in list. Pass 0 to disable TLS.
 * @param pdn_id Packet Data Network ID to use for the download, or 0 to use the default.
 * @param fragment_size Fragment size to be used for the download.
 *			If 0, @kconfig{CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE} is used.
 * @param expected_type Type of firmware file to be downloaded and installed.
 *
 * @retval 0	     If download has started successfully.
 * @retval -EALREADY If download is already ongoing.
 * @retval -E2BIG    If sec_tag_count is larger than
 *		     @kconfig{CONFIG_FOTA_DOWNLOAD_SEC_TAG_LIST_SIZE_MAX}
 *                   Otherwise, a negative value is returned.
 */
int fota_download(const char *host, const char *file, const int *sec_tag_list,
		  uint8_t sec_tag_count, uint8_t pdn_id, size_t fragment_size,
		  const enum dfu_target_image_type expected_type);


/**@brief Start downloading the given file of any image type from the given host.
 *
 * When the download is complete, the secondary slot of MCUboot is tagged as having
 * valid firmware inside it. The completion is reported through an event.
 *
 * URI parameters (host and file) are not copied, so pointers must stay valid
 * until download is finished.
 *
 * @param host Name of host to start downloading from. Can include scheme
 *             and port number, e.g. https://google.com:443
 * @param file Path to the file you wish to download. May be either a single
 *              file path, relative to host (for example "path/to/binary.bin"),
 *              or, if bootloader FOTA is enabled, can be two paths (both relative
 *              to host), separated by a space (for example "path/to/s0.bin path/to/s1.bin").
 *              If two paths are provided, the download will be assumed to be a bootloader
 *              download, both paths will be treated as upgradable bootloader slot 0
 *              and slot 1 binaries respectively, and only the binary corresponding to
 *              the currently inactive bootloader slot will be selected and downloaded.
 *              See <a href="https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/ug_bootloader.html">
 *              Secure Bootloader Chain Docs</a> for details regarding the upgradable
 *              bootloader slots.
 * @param sec_tag_list Security tags that you want to use with HTTPS. Pass NULL to disable TLS.
 * @param sec_tag_count Number of TLS security tags in list. Pass 0 to disable TLS.
 * @param pdn_id Packet Data Network ID to use for the download, or 0 to use the default.
 * @param fragment_size Fragment size to be used for the download.
 *			If 0, @kconfig{CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE} is used.
 *
 * @retval 0	     If download has started successfully.
 * @retval -EALREADY If download is already ongoing.
 * @retval -E2BIG    If sec_tag_count is larger than
 *		     @kconfig{CONFIG_FOTA_DOWNLOAD_SEC_TAG_LIST_SIZE_MAX}
 *                   Otherwise, a negative value is returned.
 */
int fota_download_any(const char *host, const char *file, const int *sec_tag_list,
		      uint8_t sec_tag_count, uint8_t pdn_id, size_t fragment_size);

/**@brief Start downloading the given file of any image type from the given host.
 *
 * When the download is complete, the secondary slot of MCUboot is tagged as having
 * valid firmware inside it. The completion is reported through an event.
 *
 * URI parameters (host and file) are not copied, so pointers must stay valid
 * until download is finished.
 *
 * @param host Name of host to start downloading from. Can include scheme
 *             and port number, e.g. https://google.com:443
 * @param file Path to the file you wish to download. See fota_download_any()
 *             for details on expected format.
 * @param sec_tag Security tag you want to use with HTTPS. Pass -1 to disable TLS.
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

/**@brief Download the given file with the specified image type from the given host.
 *
 * Validate that the file type matches the expected type before proceeding with the download.
 * When the download is complete, the secondary slot of MCUboot is tagged as having
 * valid firmware inside it. The completion is reported through an event.
 *
 * URI parameters (host and file) are not copied, so pointers must stay valid
 * until download is finished.
 *
 * @param host Name of host to start downloading from. Can include scheme
 *             and port number, for example https://google.com:443
 * @param file Path to the file you wish to download. See fota_download_any()
 *             for details on expected format.
 * @param sec_tag Security tag you want to use with HTTPS. Pass -1 to disable TLS.
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

/**@brief Get the active bootloader (B1) slot, s0 or s1.
 *
 * @param[out] s0_active Set to 'true' if s0 is active slot, 'false' otherwise
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int fota_download_s0_active_get(bool *const s0_active);

#ifdef __cplusplus
}
#endif

#endif /* FOTA_DOWNLOAD_H_ */

/**@} */
