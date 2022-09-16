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
 * @note This API calls settings_subsys_init() so should
 *       only be called after the settings backend (Flash or FS)
 *       is ready.
 */
int lwm2m_init_security(struct lwm2m_ctx *ctx, char *endpoint, struct modem_mode_change *mmode);

/**
 * @brief Set security object to PSK mode.
 *
 * Any pointer can be given as a NULL, which means that data related to this field is set to
 * zero legth in the engine. Effectively, it causes that relative data is not written into
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
 * zero legth in the engine. Effectively, it causes that relative data is not written into
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
 * @brief Apply the firmware update.
 *
 * By default lwm2m firmware is applied when the update resource (5/0/2) is executed.
 * If application needs to control the update, it can set its own callback for the
 * update resource calling `lwm2m_firmware_set_update_cb`. After that, the application
 * can apply the firmware update when it is ready.
 *
 * @param[in] obj_inst_id Instance id of the firmware update object.
 */
int lwm2m_firmware_apply_update(uint16_t obj_inst_id);

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
int lwm2m_update_signal_meas_objects(const struct lte_lc_cells_info *const cells);
int lwm2m_ncell_handler_register(void);
void lwm2m_ncell_schedule_measurement(void);
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_CELL_CONN_OBJ_SUPPORT)
#define LWM2M_OBJECT_CELLULAR_CONNECTIVITY_ID 10
int lwm2m_init_cellular_connectivity_object(void);
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_GNSS_ASSIST_OBJ_SUPPORT)
#define GNSS_ASSIST_OBJECT_ID 33625
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS)

/**
 * @brief Set an A-GPS assistance request mask for the object
 *
 * @param request_mask A bitmask containing the requested parameters from the server
 */
void location_assist_agps_request_set(uint32_t request_mask);

/**
 * @brief Set the satellite elevation mask angle above the ground. Satellites
 *        below the angle will be filtered in the response.
 *
 * @param elevation_mask Elevation mask angle in degrees above the ground level
 */
void location_assist_agps_set_elevation_mask(int32_t elevation_mask);

/**
 * @brief Get the satellite elevation mask currently stored in the resource.
 *
 * @return int32_t
 */
int32_t location_assist_agps_get_elevation_mask(void);
#endif /* CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS */
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS)
/**
 * @brief Set prediction count for the P-GPS query
 *
 * @param predictions Amount of predictions requested
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_assist_pgps_set_prediction_count(int32_t predictions);

/**
 * @brief Set prediction interval as minutes for the P-GPS query
 *
 * @param interval Time in minutes between each query
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_assist_pgps_set_prediction_interval(int32_t interval);

/**
 * @brief Set starting GPS day as days since GPS epoch. Setting the day as 0,
 *        will use the default value for the request which will be current GPS day.
 *
 * @param gps_day Day as a GPS days since GPS epoch.
 */
void location_assist_pgps_set_start_gps_day(int32_t gps_day);

/**
 * @brief Get the GPS start day stored currently in the resource.
 *
 * @return Returns the currently stored value for GPS start day.
 */
int32_t location_assist_pgps_get_start_gps_day(void);

/**
 * @brief Set the GPS start time in seconds.
 *
 * @param start_time Time of day in seconds.
 * @return Returns a negative error code (errno.h) indicating
 *         reason of failure or 0 for success.
 */
int location_assist_pgps_set_start_time(int32_t start_time);

/**
 * @brief Set the GNSS request to P-GPS request.
 */
void location_assist_pgps_request_set(void);
#endif /* CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS */

/**
 * @brief Get the result code of the location request.
 *
 * @return int32_t Returns a result code from the LwM2M server.
 */
int32_t location_assist_gnss_get_result_code(void);
#endif /* CONFIG_LWM2M_CLIENT_UTILS_GNSS_ASSIST_OBJ_SUPPORT */

#if defined(CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT)
#define GROUND_FIX_OBJECT_ID 33626
/**
 * @brief Set if the server should report the location back to the object
 *        after it has attained it.
 *
 * @param report_back Boolean value showing if the server should report the location back.
 */
void ground_fix_set_report_back(bool report_back);

/**
 * @brief Get the result code of the location request.
 *
 * @return int32_t Returns a result code from the LwM2M server.
 */
int32_t ground_fix_get_result_code(void);
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_VISIBLE_WIFI_AP_OBJ_SUPPORT)
#define VISIBLE_WIFI_AP_OBJECT_ID 33627
#endif

#ifdef __cplusplus
}
#endif

#endif /* LWM2M_CLIENT_UTILS_H__ */

/**@} */
