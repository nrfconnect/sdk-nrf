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
	/** AWS FOTA error */
	AWS_FOTA_EVT_ERROR,
	/** AWS FOTA Erase pending*/
	AWS_FOTA_EVT_ERASE_PENDING,
	/** AWS FOTA Erase done*/
	AWS_FOTA_EVT_ERASE_DONE,
	/** AWS FOTA download progress */
	AWS_FOTA_EVT_DL_PROGRESS,
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
	};
};

typedef void (*aws_fota_callback_t)(struct aws_fota_event *fota_evt);

/**@brief Initialize the AWS Firmware Over the Air library.
 *
 * @param client       Pointer to an initialized MQTT instance.
 * @param evt_handler  Callback function for events emitted by the aws_fota
 *                     library.
 *
 * @retval 0       If successfully initialized.
 * @retval -EINVAL If any of the input values are invalid.
 * @return         Negative value on error.
 */
int aws_fota_init(struct mqtt_client *const client,
		  aws_fota_callback_t evt_handler);

/**@brief AWS Firmware over the air mqtt event handler.
 *
 * @param client Pointer to the mqtt_client instance.
 * @param evt          Pointer to the received mqtt_evt.
 *
 * @retval 0 If successful and the application can skip handling this event.
 * @retval 1 If successful but wants the application to handle the event.
 * @return   A negative value on error.
 */
int aws_fota_mqtt_evt_handler(struct mqtt_client *const client,
			      const struct mqtt_evt *evt);

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
