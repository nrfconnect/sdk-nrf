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
int lwm2m_init_image(uint16_t obj_inst_id);

/**
 * @brief Verifies modem firmware update
 */
void lwm2m_verify_modem_fw_update(uint16_t obj_inst_id);

/**
 * @brief Reports the update status according to update counters
 */
void lwm2m_report_firmware_update_status(uint16_t obj_inst_id);
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

#ifdef __cplusplus
}
#endif

#endif /* LWM2M_CLIENT_UTILS_H__ */

/**@} */
