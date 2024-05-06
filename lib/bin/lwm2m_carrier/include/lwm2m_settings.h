/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LWM2M_SETTINGS_H__
#define LWM2M_SETTINGS_H__

/**
 * @file lwm2m_settings.h
 *
 * @defgroup lwm2m_carrier_settings LwM2M custom init settings
 * @{
 */

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Check if automatic startup is enabled.
 *
 * @return true if auto startup is enabled.
 */
bool lwm2m_settings_auto_startup_get(void);

/**
 * @brief Enable or disable automatic startup.
 *
 * @param new_auto_startup Whether to enable automatic startup or not.
 *
 * @return int 0 on success, non-zero on failure.
 */
int lwm2m_settings_auto_startup_set(bool new_auto_startup);

/**
 * @brief Determines whether the LwM2M custom settings are enabled.
 *
 * @retval  true if LwM2M custom settings are enabled.
 */
bool lwm2m_settings_enable_custom_config_get(void);

/**
 * @brief Enable or disable the LwM2M custom settings.
 *
 * @param new_enable_custom_config Whether to enable LwM2M custom settings or not.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_enable_custom_config_set(bool new_enable_custom_config);

/**
 * @brief Determines which carriers are enabled.
 *
 * @retval  Bitmask corresponding to carrier oper_id values.
 */
uint32_t lwm2m_settings_carriers_enabled_get(void);

/**
 * @brief Set enabled carriers.
 *
 * @param new_carriers_enabled Bitmask corresponding to carrier oper_id values.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_carriers_enabled_set(uint32_t new_carriers_enabled);

/**
 * @brief Determines whether bootstrap from Smartcard mode is disabled.
 *
 * @retval true if bootstrap from Smartcard mode enabled.
 */
bool lwm2m_settings_bootstrap_from_smartcard_get(void);

/**
 * @brief Enable or disable bootstrap from Smartcard mode.
 *
 * @param new_bootstrap_from_smartcard Whether to enable bootstrap from Smartcard
 * mode or not.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_bootstrap_from_smartcard_set(bool new_bootstrap_from_smartcard);

/**
 * @brief Determines whether @c server_uri is an LwM2M Bootstrap-Server or an LwM2M Server.
 *
 * @retval true if @c server_uri is an LwM2M Bootstrap-Server, false if  @c server_uri is
 * an LwM2M Server.
 */
bool lwm2m_settings_is_bootstrap_server_get(void);

/**
 * @brief Indicate if @c server_uri is an LwM2M Bootstrap-Server or an LwM2M Server
 *
 * @param new_is_bootstrap_server true to enable server_uri as an LwM2M
 * Bootstrap-Server
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_is_bootstrap_server_set(bool new_is_bootstrap_server);

/**
 * @brief Retrieve the server URI from the custom LwM2M settings.
 *
 * @retval The server URI.
 */
const char *lwm2m_settings_server_uri_get(void);

/**
 * @brief Set or update the server URI in the custom LwM2M settings.
 *
 * @param[in] new_server_uri Server URI.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_server_uri_set(const char *new_server_uri);

/**
 * @brief Retrieve the server lifetime from the custom LwM2M settings.
 *
 * @retval The server lifetime (in seconds).
 */
int32_t lwm2m_settings_server_lifetime_get(void);

/**
 * @brief Set or update the server lifetime in the custom LwM2M settings.
 *
 * @param[in] new_server_lifetime Server lifetime (in seconds). Used only for an LwM2M Server.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_server_lifetime_set(const int32_t new_server_lifetime);

/**
 * @brief Retrieve the server binding from the custom LwM2M settings.
 *
 * @retval The server binding.
 */
uint8_t lwm2m_settings_server_binding_get(void);

/**
 * @brief Set or update the server binding in the custom LwM2M settings.
 *
 * @param[in] new_server_binding Server binding.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_server_binding_set(uint8_t new_server_binding);

/**
 * @brief Retrieve DTLS session idle timeout from the custom LwM2M settings.
 *
 * @retval The DTLS session idle timeout (in seconds).
 */
int32_t lwm2m_settings_session_idle_timeout_get(void);

/**
 * @brief Set or update the DTLS session idle timeout in the custom LwM2M settings.
 *
 * @param[in] new_session_idle_timeout DTLS session idle timeout (in seconds).
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_session_idle_timeout_set(const int32_t new_session_idle_timeout);

/**
 * @brief Retrieve CoAP CON interval from the custom LwM2M settings.
 *
 * @retval The CoAP CON interval (in seconds).
 */
int32_t lwm2m_settings_coap_con_interval_get(void);

/**
 * @brief Set or update the CoAP CON interval in the custom LwM2M settings.
 *
 * @param[in] new_coap_con_interval CoAP CON interval (in seconds).
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_coap_con_interval_set(const int32_t new_coap_con_interval);

/**
 * @brief Retrieve the security tag from the custom LwM2M settings.
 *
 * @retval The security tag.
 */
uint32_t lwm2m_settings_server_sec_tag_get(void);

/**
 * @brief Set or update the security tag in the custom LwM2M settings.
 *
 * @param[in] new_server_sec_tag Security tag.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_server_sec_tag_set(const uint32_t new_server_sec_tag);

/**
 * @brief Retrieve the APN from the custom LwM2M settings.
 *
 * @retval The APN.
 */
const char *lwm2m_settings_apn_get(void);

/**
 * @brief Set or update the APN in the custom LwM2M settings.
 *
 * @param[in] new_apn APN.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_apn_set(const char *new_apn);

/**
 * @brief Retrieve PDN type from the custom LwM2M settings.
 *
 * @retval The PDN type.
 */
int32_t lwm2m_settings_pdn_type_get(void);

/**
 * @brief Set or update the PDN type in the custom LwM2M settings.
 *
 * @param[in] new_pdn_type PDN type.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_pdn_type_set(uint32_t new_pdn_type);

/**
 * @brief Determines whether the LwM2M custom device settings are enabled.
 *
 * @retval  true if LwM2M custom device settings are enabled.
 */
bool lwm2m_settings_enable_custom_device_config_get(void);

/**
 * @brief Enable or disable the LwM2M device custom settings.
 *
 * @param new_enable_custom_device_config Whether to enable LwM2M custom device settings or not.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_enable_custom_device_config_set(bool new_enable_custom_device_config);

/**
 * @brief Retrieve the LwM2M device manufacturer name from the custom LwM2M settings.
 *
 * @retval The LwM2M device manufacturer name.
 */
const char *lwm2m_settings_manufacturer_get(void);

/**
 * @brief Set or update the LwM2M device manufacturer name in the custom LwM2M settings.
 *
 * @param[in] new_manufacturer LwM2M device manufacturer name.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_manufacturer_set(const char *new_manufacturer);

/**
 * @brief Retrieve the LwM2M device model number from the custom LwM2M settings.
 *
 * @retval The LwM2M device model number.
 */
const char *lwm2m_settings_model_number_get(void);

/**
 * @brief Set or update the LwM2M device model number in the custom LwM2M settings.
 *
 * @param[in] new_model_number LwM2M device model number.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_model_number_set(const char *new_model_number);

/**
 * @brief Retrieve the LwM2M device type from the custom LwM2M settings.
 *
 * @retval The LwM2M device type.
 */
const char *lwm2m_settings_device_type_get(void);

/**
 * @brief Set or update the LwM2M device type in the custom LwM2M settings.
 *
 * @param[in] new_device_type LwM2M device type.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_device_type_set(const char *new_device_type);

/**
 * @brief Retrieve the LwM2M device hardware version from the custom LwM2M settings.
 *
 * @retval The LwM2M device hardware version.
 */
const char *lwm2m_settings_hardware_version_get(void);

/**
 * @brief Set or update the LwM2M device hardware version in the custom LwM2M settings.
 *
 * @param[in] new_hardware_version LwM2M device hardware version.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_hardware_version_set(const char *new_hardware_version);

/**
 * @brief Retrieve the LwM2M device software version from the custom LwM2M settings.
 *
 * @retval The LwM2M device software version.
 */
const char *lwm2m_settings_software_version_get(void);

/**
 * @brief Set or update the LwM2M device software version in the custom LwM2M settings.
 *
 * @param[in] new_software_version LwM2M device software version.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_software_version_set(const char *new_software_version);

/**
 * @brief Retrieve the LG U+ Service Code from the custom LwM2M settings.
 *
 * @retval The LG U+ Service Code.
 */
const char *lwm2m_settings_service_code_get(void);

/**
 * @brief Set or update the LG U+ Service Code in the custom LwM2M settings.
 *
 * @param[in] new_service_code LG U+ Service Code.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_service_code_set(const char *new_service_code);

/**
 * @brief Determines the LG U+ Device Serial Number type to be used.
 *
 * @retval The LG U+ Device Serial Number type.
 */
bool lwm2m_settings_device_serial_no_type_get(void);

/**
 * @brief Indicate the LG U+ Device Serial Number type to be used.
 *
 * @param new_device_serial_no_type  The LG U+ Device Serial Number type to be used.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_device_serial_no_type_set(bool new_device_serial_no_type);

/**
 * @brief Retrieve the firmware download timeout from the custom LwM2M settings.
 *
 * @retval The firmware download timeout (in minutes).
 */
uint16_t lwm2m_settings_firmware_download_timeout_get(void);

/**
 * @brief Set or update the firmware download timeout in the custom LwM2M settings.
 *
 * @param[in] new_firmware_download_timeout Timeout (in minutes).
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_firmware_download_timeout_set(const uint16_t new_firmware_download_timeout);

/**
 * @brief Determines whether queue mode is disabled.
 *
 * @retval true if queue mode enabled.
 */
bool lwm2m_settings_queue_mode_get(void);

/**
 * @brief Enable or disable queue mode.
 *
 * @param new_queue_mode Whether to enable queue mode or not.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_queue_mode_set(bool new_queue_mode);

/**
 * @brief Determines whether automatic registration on LTE Attach is disabled.
 *
 * @retval true if automatic registration on LTE Attach enabled.
 */
bool lwm2m_settings_auto_register_get(void);

/**
 * @brief Enable or disable automatic registration on LTE Attach.
 *
 * @param new_auto_register Whether to enable automatic registration on LTE Attach or not.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_auto_register_set(bool new_auto_register);

/** @} */

#endif /* LWM2M_SETTINGS_H__ */
