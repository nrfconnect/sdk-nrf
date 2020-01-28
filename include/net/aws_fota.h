/*
 *Copyright (c) 2019 Nordic Semiconductor ASA
 *
 *SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file
 *@brief AWS FOTA library header.
 */

#ifndef AWS_FOTA_H__
#define AWS_FOTA_H__

/**
 * @defgroup aws_fota AWS FOTA library
 * @{
 * @brief Library for performing FOTA with MQTT and HTTP.
 */

#ifdef __cplusplus
extern "C" {
#endif

enum aws_fota_evt_id {
	/** AWS FOTA complete and status reported to job document */
	AWS_FOTA_EVT_DONE,
	/** AWS FOTA error */
	AWS_FOTA_EVT_ERROR
};

typedef void (*aws_fota_callback_t)(enum aws_fota_evt_id evt_id);

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
 * @param evt          Pointer to the recived mqtt_evt.
 *
 * @retval 0 If successful but wants the application to handle the event.
 * @retval 1 If successful and the application can skip handling this event.
 * @return   A negative value on error.
 */
int aws_fota_mqtt_evt_handler(struct mqtt_client *const client,
			      const struct mqtt_evt *evt);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* AWS_FOTA_H__ */
