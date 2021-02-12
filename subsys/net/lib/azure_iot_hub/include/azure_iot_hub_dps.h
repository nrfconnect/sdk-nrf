/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 *@brief Azure IoT Hub Device Provisioning Service.
 */

#ifndef AZURE_IOT_HUB_DPS__
#define AZURE_IOT_HUB_DPS__

#include <stdio.h>
#include <net/azure_iot_hub.h>

#include "azure_iot_hub_topic.h"

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Device Provisioning Service registration states. */
enum dps_reg_state {
	DPS_STATE_NOT_STARTED,
	DPS_STATE_REGISTERING,
	DPS_STATE_REGISTERED,
	DPS_STATE_FAILED,
};

/* @brief The handler will be called when registration is done, be it
 * successfully or if it fails.
 */
typedef void (*dps_handler_t)(enum dps_reg_state state);

/* @brief Device Provisioning Service configuration structure. */
struct dps_config {
	const struct mqtt_client *mqtt_client;
	dps_handler_t handler;
	const char *reg_id;
	size_t reg_id_len;
};

/* @brief Initialize Device Provisioning Service module.
 *
 * @param cfg Pointer to DPS configuration structure.
 *
 * @retval 0 if successul, otherwise a negative error code.
 */
int dps_init(struct dps_config *cfg);

/* @brief Parse incoming MQTT message to see if it's DPS related and process
 *	  accordingly.
 *
 * @param evt Pointer to Azure IoT Hub event.
 * @param topic_data Pointer to parsed topic data. If NULL, only the event
 *		     data will be processed.
 *
 * @retval true The message was DPS-related and is consumed.
 * @retval false The message was not DPS-related.
 */
bool dps_process_message(struct azure_iot_hub_evt *evt,
			 struct topic_parser_data *topic_data);

/* @brief Start Azure Device Provisioning Service process.
 *
 * @retval 0 if Azure device provisioning was successfully started.
 * @retval -EFAULT if settings could not be loaded from flash
 * @retval -EALREADY if the device has already been registered and is ready to
 *	    connect to an IoT hub.
 */
int dps_start(void);

/* @brief Send device registration request to Device Provisioning Service.
 *
 * @retval 0 if successul, otherwise a negative error code.
 */
int dps_send_reg_request(void);

/* @brief Subscribes to DPS related topics.
 *
 * @retval 0 if successul, otherwise a negative error code.
 */
int dps_subscribe(void);

/* @brief Get the hostname of the currently assigned Azure IoT Hub.
 *
 * @return Pointer to the assigned IoT hub's hostname.
 *	   If the device has not been assigned to an IoT hub using DPS, the
 *	   function returns NULL.
 */
char *dps_hostname_get(void);

/* @brief Get the current statue of DPS registration.
 *
 * @return The current DPS registration state.
 */
enum dps_reg_state dps_get_reg_state(void);

/* @brief Convenience function to check if DPS registration is currently in
 *	  progress.
 *
 * @retval True if DPS registration is in progress, otherwise false.
 */
bool dps_reg_in_progress(void);

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* AZURE_IOT_HUB_DPS__ */
