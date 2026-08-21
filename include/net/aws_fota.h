/*
 *Copyright (c) 2019 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 *@brief AWS FOTA library header.
 */

#ifndef AWS_FOTA_H__
#define AWS_FOTA_H__

#include <dfu/dfu_target.h>
#include <zephyr/net/mqtt.h>

/**
 * @defgroup aws_fota AWS FOTA library
 * @{
 * @brief Library for performing FOTA with MQTT and HTTP.
 */

#ifdef __cplusplus
extern "C" {
#endif

enum aws_fota_evt_id {
	/** AWS FOTA has started */
	AWS_FOTA_EVT_START,
	/** AWS FOTA complete and status reported to job document.
	 *  Payload of type @ref dfu_target_image_type (image).
	 *
	 *  If the image parameter type is of type DFU_TARGET_IMAGE_TYPE_MCUBOOT the device needs to
	 *  reboot to apply the new application image.
	 *
	 *  If the image parameter type is of type DFU_TARGET_IMAGE_TYPE_MODEM_DELTA the modem
	 *  needs to be reinitialized to apply the new modem image.
	 */
	AWS_FOTA_EVT_DONE,
	/** AWS FOTA error.
	 *  Payload of type @ref aws_fota_error_cause (cause).
	 */
	AWS_FOTA_EVT_ERROR,
	/** AWS FOTA Erase pending*/
	AWS_FOTA_EVT_ERASE_PENDING,
	/** AWS FOTA Erase done*/
	AWS_FOTA_EVT_ERASE_DONE,
	/** AWS FOTA download progress */
	AWS_FOTA_EVT_DL_PROGRESS,
};

/** @brief Cause of a failed FOTA job, reported with the @ref AWS_FOTA_EVT_ERROR event.
 *
 *  The same cause is reported to AWS IoT Jobs in the ``statusDetails`` field of the job
 *  execution update that marks the job as ``FAILED``.
 */
enum aws_fota_error_cause {
	/** No error. Used when the event ID is not @ref AWS_FOTA_EVT_ERROR. */
	AWS_FOTA_ERROR_CAUSE_NO_ERROR = 0,
	/** Connecting to the firmware server failed. A possible reason could be wrong
	 *  TLS credentials. A retry with the same credentials is unlikely to help.
	 */
	AWS_FOTA_ERROR_CAUSE_CONNECT_FAILED,
	/** Downloading the update failed. The download may be retried. */
	AWS_FOTA_ERROR_CAUSE_DOWNLOAD_FAILED,
	/** The update is invalid and was rejected. A retry will not help. */
	AWS_FOTA_ERROR_CAUSE_INVALID_UPDATE,
	/** The actual firmware type does not match the expected type. A retry will not help. */
	AWS_FOTA_ERROR_CAUSE_TYPE_MISMATCH,
	/** Generic error on the device side. */
	AWS_FOTA_ERROR_CAUSE_INTERNAL,
	/** Error reported by the DFU library. */
	AWS_FOTA_ERROR_CAUSE_DFU,
	/** The transfer protocol requested in the job document is not supported. */
	AWS_FOTA_ERROR_CAUSE_PROTO_NOT_SUPPORTED,
	/** Invalid URI or invalid download configuration. */
	AWS_FOTA_ERROR_CAUSE_INVALID_CONFIGURATION,
	/** The received job document could not be parsed. A retry will not help. */
	AWS_FOTA_ERROR_CAUSE_INVALID_JOB_DOCUMENT,
	/** The URL in the job document did not fit in the configured buffers.
	 *  See :kconfig:option:`CONFIG_DOWNLOADER_MAX_HOSTNAME_SIZE` and
	 *  :kconfig:option:`CONFIG_DOWNLOADER_MAX_FILENAME_SIZE`.
	 */
	AWS_FOTA_ERROR_CAUSE_URL_TOO_LONG,
	/** The job document requested HTTPS, but no security tag is configured.
	 *  See :kconfig:option:`CONFIG_AWS_FOTA_DOWNLOAD_SECURITY_TAG`.
	 */
	AWS_FOTA_ERROR_CAUSE_NO_SEC_TAG,
	/** The firmware download could not be started. */
	AWS_FOTA_ERROR_CAUSE_DOWNLOAD_START_FAILED,
	/** The job execution update was rejected by AWS IoT Jobs. */
	AWS_FOTA_ERROR_CAUSE_JOB_UPDATE_REJECTED,
	/** The job execution could not be updated after a successful download. */
	AWS_FOTA_ERROR_CAUSE_JOB_UPDATE_FAILED,
	/** Unknown or unmapped error cause. */
	AWS_FOTA_ERROR_CAUSE_UNKNOWN,
};

#define AWS_FOTA_EVT_DL_COMPLETE_VAL 100
struct aws_fota_event_dl {
	int progress; /* Download progress percent, 0-100 */
};

struct aws_fota_event {
	enum aws_fota_evt_id id;
	union {
		struct aws_fota_event_dl dl;
		enum dfu_target_image_type image;
		/** Cause of the failure, set when the event ID is @ref AWS_FOTA_EVT_ERROR. */
		enum aws_fota_error_cause cause;
	};
};

typedef void (*aws_fota_callback_t)(struct aws_fota_event *fota_evt);

/**@brief Initialize the AWS Firmware Over the Air library.
 *
 * @param evt_handler  Callback function for events emitted by the aws_fota
 *                     library.
 *
 * @retval 0       If successfully initialized.
 * @retval -EINVAL If the passed in event handler is NULL.
 * @retval -EPERM  If the library has already been initialized.
 * @return         Negative value on error.
 */
int aws_fota_init(aws_fota_callback_t evt_handler);

/**@brief AWS Firmware over the air mqtt event handler.
 *
 * @param client Pointer to the mqtt_client instance.
 * @param evt          Pointer to the received mqtt_evt.
 *
 * @retval 0 If successful and the application can skip handling this event.
 * @retval 1 If successful but wants the application to handle the event.
 * @return   A negative value on error.
 */
int aws_fota_mqtt_evt_handler(struct mqtt_client *const client, const struct mqtt_evt *evt);

/**@brief Get the null-terminated job id string.
 *
 * @param job_id_buf Buffer to which the job id will be copied.
 * @param buf_size   Size of the buffer.
 *
 * @return Length of the job id string (not counting the terminating
 *         null character) or a negative value on error.
 */
int aws_fota_get_job_id(uint8_t *const job_id_buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* AWS_FOTA_H__ */
