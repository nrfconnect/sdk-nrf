/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file lwm2m_settings.h
 *
 * @defgroup lwm2m_carrier_settings LwM2M custom init settings
 * @{
 */

#ifndef LWM2M_SETTINGS_H__
#define LWM2M_SETTINGS_H__

#include <modem/lte_lc.h>
#include <lwm2m_carrier.h>

/**
 *
 * @brief Initialize the LwM2M carrier custom settings.
 *
 * @param[in] config Configuration parameters for the library.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_init(lwm2m_carrier_config_t *config);

/**
 * @brief Determines whether the LwM2M custom settings are enabled.
 *
 * @retval  true if LwM2M custom settings are enabled.
 */
bool lwm2m_settings_enable_custom_config_get(void);

 /**
  * Enable or disable the LwM2M custom settings.
  *
  * @param new_enable_custom_config Whether to enable LwM2M custom settings or not.
  *
  * @retval 0 on success, non-zero on failure.
  */
int lwm2m_settings_enable_custom_config_set(bool new_enable_custom_config);

/**
 * @brief Determines whether certification mode is enabled.
 *
 * @retval true if certification mode is enabled.
 */
bool lwm2m_settings_certification_mode_get(void);

/**
 * @brief Enable or disable certification mode.
 *
 * @param new_certification_mode Whether to enable certification mode or not.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_certification_mode_set(bool new_certification_mode);

/**
 * @brief Determines whether bootstrap from Smartcard mode is disabled.
 *
 * @retval true if bootstrap from Smartcard mode is disabled.
 */
bool lwm2m_settings_disable_bootstrap_from_smartcard_get(void);

/**
 * @brief Enable or disable bootstrap from Smartcard mode.
 *
 * @param new_disable_bootstrap_from_smartcard Whether to enable bootstrap from Smartcard
 * mode or not.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_disable_bootstrap_from_smartcard_set(bool new_disable_bootstrap_from_smartcard);

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
 * @param new_disable_bootstrap_from_smartcard true to enable server_uri as an LwM2M
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
 * @brief Retrieve the Pre-Shared Key from the custom LwM2M settings.
 *
 * @retval The Pre-Shared Key.
 */
const char *lwm2m_settings_server_psk_get(void);

/**
 * @brief Set or update the Pre-Shared Key in the custom LwM2M settings.
 *
 * @param[in] new_server_psk Pre-Shared Key.
 *
 * @retval 0 on success, non-zero on failure.
 */
int lwm2m_settings_server_psk_set(const char *new_server_psk);

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

#endif /* LWM2M_SETTINGS_H__ */
