/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file lwm2m_client_utils.h
 *
 * @defgroup lwm2m_client_utils LwM2M client utilities
 * @{
 * @brief LwM2M Client utilities to build an application
 *
 * @details The client provides APIs for:
 *  - connecting to a remote server
 *  - setting up default resources
 *    * Firmware
 *    * Connection monitoring
 *    * Device
 *    * Location
 *    * Security
 */

#ifndef LWM2M_CLIENT_UTILS_H__
#define LWM2M_CLIENT_UTILS_H__

#include <zephyr.h>
#include <net/lwm2m.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_SECURITY_OBJ_SUPPORT)
/**
 * @brief Initialize Security object
 */
int lwm2m_init_security(struct lwm2m_ctx *ctx, char *endpoint);

/**
 * @brief Check if we already have client credentials stored
 *
 * @return true If we need bootstrap.
 * @return false If we already have client credentials.
 */
bool lwm2m_security_needs_bootstrap(void);

#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_DEVICE_OBJ_SUPPORT)
/**
 * @brief Initialize Device object
 */
int lwm2m_init_device(void);
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_OBJ_SUPPORT)
/**
 * @brief Initialize Location object
 */
int lwm2m_init_location(void);
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_UPDATE_OBJ_SUPPORT)
/**
 * @brief Firmware update state change event callback.
 *
 * @param[in] update_state LWM2M Firmware Update object states
 *
 * @return Callback returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
typedef int (*lwm2m_firmware_get_update_state_cb_t)(uint8_t update_state);

/**
 * @brief Set event callback for firmware update changes.
 *
 * LwM2M clients use this function to register a callback for receiving the
 * update state changes when performing a firmware update.
 *
 * @param[in] cb A callback function to receive firmware update state changes.
 */
void lwm2m_firmware_set_update_state_cb(lwm2m_firmware_get_update_state_cb_t cb);

/**
 * @brief Firmware read callback
 */
void *firmware_read_cb(uint16_t obj_inst_id, size_t *data_len);
/**
 * @brief Verify active firmware image
 */
int lwm2m_init_firmware(void);

/**
 * @brief Initialize Image Update object
 */
int lwm2m_init_image(void);

/**
 * @brief Verifies modem firmware update
 */
void lwm2m_verify_modem_fw_update(void);
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_CONN_MON_OBJ_SUPPORT)
/**
 * @brief Initialize Connectivity Monitoring object
 */
int lwm2m_init_connmon(void);

/**
 * @brief Update Connectivity Monitoring object
 */
int lwm2m_update_connmon(void);
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT)
#define ECID_SIGNAL_MEASUREMENT_INFO_OBJECT_ID 10256

int lwm2m_signal_meas_info_inst_id_to_index(uint16_t obj_inst_id);
int lwm2m_signal_meas_info_index_to_inst_id(int index);
int init_neighbour_cell_info(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* LWM2M_CLIENT_UTILS_H__ */

/**@} */
