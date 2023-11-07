/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 *@brief Azure IoT Hub Device Provisioning Service.
 */

#ifndef AZURE_IOT_HUB_DPS__
#define AZURE_IOT_HUB_DPS__

#include <stdint.h>
#include <net/azure_iot_hub.h>

/**
 * @defgroup azure_iot_hub_dps Azure IoT Hub DPS library
 * @{
 * @brief Library to connect a device to Azure IoT Hub Device Provisioning Service (DPS).
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Device Provisioning Service registration status. */
enum azure_iot_hub_dps_reg_status {
	/** The DPS library has been initialized and the registration process can now be started. */
	AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED,

	AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNING,
	AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNED,
	AZURE_IOT_HUB_DPS_REG_STATUS_FAILED,
};

/** @brief The handler will be called when registration is done, be it
 *	   successfully or if it fails.
 */
typedef void (*azure_iot_hub_dps_handler_t)(enum azure_iot_hub_dps_reg_status status);

/** @brief Device Provisioning Service configuration structure. */
struct azure_iot_hub_dps_config {
	/** Handler to receive updates on registration status. */
	azure_iot_hub_dps_handler_t handler;

	/** ID scope to use in the provisioning request.
	 *  If the pointer is NULL or the length is zero, the compile-time option
	 *  :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE` is used.
	 */
	struct azure_iot_hub_buf id_scope;

	/** Registration ID to use in the provisioning request.
	 *  If the pointer is NULL or the length is zero, the compile-time option
	 *  :kconfig:option:`CONFIG_AZURE_IOT_HUB_REG_ID` is used.
	 */
	struct azure_iot_hub_buf reg_id;
};

/** @brief Initialize Azure IoT hub Device Provisioning Service (DPS).
 *
 *  @param[in] cfg Pointer to DPS configuration structure.
 *
 *  @note The buffers provided in the configuration structure must remain unchanged until the DPS
 *	  attempt has been completed. If the content of the buffers is changed while provisioning
 *	  is ongoing, the behavior is undefined.
 *
 *  @retval 0 if successul.
 *  @retval -EINVAL if configuration or event handler was NULL.
 */
int azure_iot_hub_dps_init(struct azure_iot_hub_dps_config *cfg);

/* @brief Start Device Provisioning Service (DPS) to obtain device ID and assigmnent
 *	  to an IoT hub. The information can later be used to connect to the assigned IoT hub.
 *
 * @retval 0 if successful.
 * @retval -EACCESS if the library is in the wrong state to start DPS.
 * @retval -EALREADY if DPS has already started and is ongoing.
 */
int azure_iot_hub_dps_start(void);

/** @brief Get the hostname of the currently assigned Azure IoT Hub as a temrinated string.
 *
 *  @param[in, out] buf Pointer to buffer where the hostname will be stored.
 *			The buffer will upon success be populated to point to the assigned IoT Hub
 *			and its size.
 *
 *  @retval 0 if a valid hostname was found and populated to the provided buffer.
 *  @retval -EINVAL if an invalid buffer pointer was provided.
 *  @retval -EMSGSIZE if the provided buffer is too small.
 *  @retval -ENOENT if no hostname has been registered using DPS.
 */
int azure_iot_hub_dps_hostname_get(struct azure_iot_hub_buf *buf);

/** @brief Delete the hostname of the currently assigned Azure IoT Hub.
 *
 *  @retval 0 if a valid hostname was found and deleted, or of none existed.
 *  @retval -EFAULT if there was in issue when erasing an already assigned IoT Hub from flash.
 */
int azure_iot_hub_dps_hostname_delete(void);

/** @brief Get the currently assigned device ID.
 *
 *  @param[in, out] buf Pointer to buffer where the device ID will be stored.
 *			The buffer will upon success be populated to point to the assigned device ID
 *			and its size.
 *
 *  @retval 0 if a valid device ID was found and populated to the provided buffer.
 *  @retval -EINVAL if an invalid buffer pointer was provided.
 *  @retval -EMSGSIZE if the provided buffer is too small.
 *  @retval -ENOENT if no device ID has been registered using DPS.
 */
int azure_iot_hub_dps_device_id_get(struct azure_iot_hub_buf *buf);

/** @brief Delete the assigned device ID.
 *
 *  @retval 0 if a valid device ID was found and deleted, or of none existed.
 *  @retval -EFAULT if there was in issue when erasing an already assigned IoT Hub from flash.
 */
int azure_iot_hub_dps_device_id_delete(void);

/** @brief Reset DPS library. All stored information about assigned IoT Hub is deleted.
 *	  After this call, the library is put into an uninitialized state and must be initialized
 *	  again before the next provisioning attempt.
 *
 *  @retval 0 if successul, otherwise a negative error code.
 *  @retval -EACCES if a DPS request is already in progress.
 *  @retval -EFAULT if there was in issue when erasing an already assigned IoT Hub from flash.
 */
int azure_iot_hub_dps_reset(void);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* AZURE_IOT_HUB_DPS__ */
