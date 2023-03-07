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

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/lwm2m.h>
#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_SECURITY_OBJ_SUPPORT)
/**
 * @typedef modem_mode_cb_t
 * @brief Callback to request a modem state change, being it powering off, flight mode etc.
 *
 * @return 0 if mode was set successfully
 * @return positive value to indicate seconds before retry
 * @return negative error code in case of a failure
 */
typedef int (*modem_mode_cb_t)(enum lte_lc_func_mode new_mode, void *user_data);

/**
 * @struct modem_mode_change
 * @brief Callback used for querying permission from the app to proceed when modem's state changes
 *
 * @param cb        The callback function
 * @param user_data App specific data to be fed to the callback once it's called
 */
struct modem_mode_change {
	modem_mode_cb_t cb;
	void *user_data;
};

/**
 * @brief Initialize Security object support for nrf91
 *
 * This wrapper will install hooks that allows device to do a
 * proper bootstrap and store received server settings to permanent
 * storage using Zephyr settings API. Credential are stored to
 * modem and no keys would enter the flash.
 *
 * @note This API calls settings_subsys_init() and should
 *       only be called after the settings backend (Flash or FS)
 *       is ready.
 */
int lwm2m_init_security(struct lwm2m_ctx *ctx, char *endpoint, struct modem_mode_change *mmode);

/**
 * @brief Set security object to PSK mode.
 *
 * Any pointer can be given as a NULL, which means that data related to this field is set to
 * zero length in the engine. Effectively, it causes that relative data is not written into
 * the modem. This can be used if the given data is already provisioned to the modem.
 *
 * @param sec_obj_inst Security object ID to modify.
 * @param psk Pointer to PSK key, either in HEX or binary format.
 * @param psk_len Length of data in PSK pointer.
 *                If PSK is HEX string, should include string terminator.
 * @param psk_is_hex True if PSK points to data in HEX format. False if the data is binary.
 * @param psk_id PSK key ID in string format.
 * @return Zero if success, negative error code otherwise.
 */
int lwm2m_security_set_psk(uint16_t sec_obj_inst, const void *psk, int psk_len, bool psk_is_hex,
			   const char *psk_id);

/**
 * @brief Set security object to certificate mode.
 *
 * Any pointer can be given as a NULL, which means that data related to this field is set to
 * zero length in the engine. Effectively, it causes that relative data is not written into
 * the modem. This can be used if the given data is already provisioned to the modem.
 *
 * @param sec_obj_inst Security object ID to modify.
 * @param cert Pointer to certificate.
 * @param cert_len Certificate length.
 * @param private_key Pointer to private key.
 * @param key_len Private key length.
 * @param ca_chain Pointer to CA certificate or server certificate.
 * @param ca_len CA chain length.
 * @return Zero if success, negative error code otherwise.
 */
int lwm2m_security_set_certificate(uint16_t sec_obj_inst, const void *cert, int cert_len,
				   const void *private_key, int key_len, const void *ca_chain,
				   int ca_len);
/**
 * @brief Check if the client credentials are already stored.
 *
 * @return true If bootstrap is needed.
 * @return false If client credentials are already available.
 */
bool lwm2m_security_needs_bootstrap(void);

#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_DEVICE_OBJ_SUPPORT)
/**
 * @brief Initialize Device object
 */
int lwm2m_init_device(void);

/**
 * @brief Reboot handler for a device object
 *
 * All arguments are ignored.
 *
 * @param obj_inst_id Device object instance.
 * @param args Argument pointer's
 * @param args_len Length of argument's
 *
 * @return Zero if success, negative error code otherwise.
 */
int lwm2m_device_reboot_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len);
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
 * @param[in] update_state LwM2M Firmware Update object states
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
 * @param[in] cb A callback function to receive firmware update state changes or NULL for disable.
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
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_CONN_MON_OBJ_SUPPORT)
/**
 * @brief Initialize Connectivity Monitoring object. Called in SYS_INIT.
 *
 * @return Zero if success, negative error code otherwise.
 */
int lwm2m_init_connmon(const struct device *dev);
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_CELL_CONN_OBJ_SUPPORT)
#define LWM2M_OBJECT_CELLULAR_CONNECTIVITY_ID 10
int lwm2m_init_cellular_connectivity_object(void);
#endif

enum lwm2m_rai_mode {
	LWM2M_RAI_MODE_DISABLED	= 0,
	LWM2M_RAI_MODE_ENABLED	= 1
};

/**
 * @brief Initialize release assistance indication (RAI) module.
 *
 * @return Zero if success, negative error code otherwise.
 */
int lwm2m_init_rai(void);

/**
 * @brief Set socket option SO_RAI_NO_DATA to bypass
 * RRC Inactivity period and immediately switch to Idle mode.
 *
 * @return Zero if success, negative error code otherwise.
 */
int lwm2m_rai_no_data(void);

/**
 * @brief Set socket option SO_RAI_LAST and send dummy packet to bypass
 * RRC Inactivity period and immediately switch to Idle mode.
 *
 * @return Zero if success, negative error code otherwise.
 */
int lwm2m_rai_last(void);

/**
 * @brief Get the RAI mode.
 *
 * @param mode Pointer to RAI mode variable.
 *
 * @return Zero if success, negative error code otherwise.
 */
int lwm2m_rai_get(enum lwm2m_rai_mode *mode);

/**
 * @brief Function for requesting modem to enable or disable
 * use of AS RAI.
 *
 * @param mode Requested RAI mode.
 *
 * @return Zero if success, negative error code otherwise.
 */
int lwm2m_rai_req(enum lwm2m_rai_mode mode);

/* Advanced firmare object support */
uint8_t lwm2m_adv_firmware_get_update_state(uint16_t obj_inst_id);
void lwm2m_adv_firmware_set_update_state(uint16_t obj_inst_id, uint8_t state);
uint8_t lwm2m_adv_firmware_get_update_result(uint16_t obj_inst_id);
void lwm2m_adv_firmware_set_update_result(uint16_t obj_inst_id, uint8_t result);
void lwm2m_adv_firmware_set_write_cb(uint16_t obj_inst_id, lwm2m_engine_set_data_cb_t cb);
lwm2m_engine_set_data_cb_t lwm2m_adv_firmware_get_write_cb(uint16_t obj_inst_id);
void lwm2m_adv_firmware_set_update_cb(uint16_t obj_inst_id, lwm2m_engine_execute_cb_t cb);
lwm2m_engine_execute_cb_t lwm2m_adv_firmware_get_update_cb(uint16_t obj_inst_id);
int lwm2m_adv_firmware_create_inst(const char *component,
				   lwm2m_engine_set_data_cb_t write_callback,
				   lwm2m_engine_execute_cb_t update_callback);

#define LWM2M_OBJECT_ADV_FIRMWARE_ID 33629
#define RESULT_ADV_FOTA_CANCELLED 10
#define RESULT_ADV_FOTA_DEFERRED 11
#define RESULT_ADV_CONFLICT_STATE 12
#define RESULT_ADV_DEPENDENCY_ERR 13

/* Reboot execute possible argument's */
#define REBOOT_SOURCE_DEVICE_OBJ 0
#define REBOOT_SOURCE_FOTA_OBJ 1

/* Firmware resource IDs */
#define LWM2M_FOTA_PACKAGE_ID 0
#define LWM2M_FOTA_PACKAGE_URI_ID 1
#define LWM2M_FOTA_UPDATE_ID 2
#define LWM2M_FOTA_STATE_ID 3
#define LWM2M_FOTA_UPDATE_RESULT_ID 5
#define LWM2M_FOTA_PACKAGE_NAME_ID 6
#define LWM2M_FOTA_PACKAGE_VERSION_ID 7
#define LWM2M_FOTA_UPDATE_PROTO_SUPPORT_ID 8
#define LWM2M_FOTA_UPDATE_DELIV_METHOD_ID 9
/* LwM2M v1.2 extension which are common for advanced fota */
#define LWM2M_FOTA_CANCEL_ID 10
#define LWM2M_FOTA_SEVERITY_ID 11
#define LWM2M_FOTA_LAST_STATE_CHANGE_TIME_ID 12
#define LWM2M_FOTA_MAXIMUM_DEFERRED_PERIOD_ID 13
/* Unique resources for advanced fota */
#define LWM2M_ADV_FOTA_COMPONENT_NAME_ID 14
#define LWM2M_ADV_FOTA_CURRENT_VERSION_ID 15
#define LWM2M_ADV_FOTA_LINKED_INSTANCES_ID 16
#define LWM2M_ADV_FOTA_CONFLICTING_INSTANCES_ID 17

#ifdef __cplusplus
}
#endif

#endif /* LWM2M_CLIENT_UTILS_H__ */

/**@} */
