/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LWM2M_CARRIER_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**@file lwm2m_carrier.h
 *
 * @defgroup lwm2m_carrier_event LWM2M carrier library events
 * @{
 */
/** Modem library initialized. */
#define LWM2M_CARRIER_EVENT_BSDLIB_INIT   1
/** Connecting to the LTE network. */
#define LWM2M_CARRIER_EVENT_CONNECTING    2
/** Connected to the LTE network. */
#define LWM2M_CARRIER_EVENT_CONNECTED     3
/** Disconnecting from the LTE network. */
#define LWM2M_CARRIER_EVENT_DISCONNECTING 4
/** Disconnected from the LTE network. */
#define LWM2M_CARRIER_EVENT_DISCONNECTED  5
/** LWM2M carrier bootstrapped. */
#define LWM2M_CARRIER_EVENT_BOOTSTRAPPED  6
/** LWM2M carrier registered. */
#define LWM2M_CARRIER_EVENT_REGISTERED    7
/** LWM2M carrier operation is deferred. */
#define LWM2M_CARRIER_EVENT_DEFERRED      8
/** Modem update started. */
#define LWM2M_CARRIER_EVENT_FOTA_START    9
/** Application will reboot. */
#define LWM2M_CARRIER_EVENT_REBOOT        10
/**< LTE network is ready to be used. */
#define LWM2M_CARRIER_EVENT_LTE_READY     11
/** An error occurred. */
#define LWM2M_CARRIER_EVENT_ERROR         20

/**
 * @brief LWM2M carrier library event structure.
 */
typedef struct {
	/** Event type. */
	uint32_t type;
	/** Event data. Can be NULL, depending on event type. */
	const void *data;
} lwm2m_carrier_event_t;

/**
 * @brief LWM2M carrier library event deferred reason.
 */
/** No reason given. */
#define LWM2M_CARRIER_DEFERRED_NO_REASON           0
/** Failed to activate PDN. */
#define LWM2M_CARRIER_DEFERRED_PDN_ACTIVATE        1
/** No route to bootstrap server. */
#define LWM2M_CARRIER_DEFERRED_BOOTSTRAP_NO_ROUTE  2
/** Failed to connect to bootstrap server. */
#define LWM2M_CARRIER_DEFERRED_BOOTSTRAP_CONNECT   3
/** Bootstrap sequence not completed. */
#define LWM2M_CARRIER_DEFERRED_BOOTSTRAP_SEQUENCE  4
/** No route to server. */
#define LWM2M_CARRIER_DEFERRED_SERVER_NO_ROUTE     5
/** Failed to connect to server. */
#define LWM2M_CARRIER_DEFERRED_SERVER_CONNECT      6
/** Server registration sequence not completed. */
#define LWM2M_CARRIER_DEFERRED_SERVER_REGISTRATION 7

/*
 * @brief LwM2M carrier library deferred event structure.
 */
typedef struct {
	/** Deferred event reason. */
	uint32_t reason;
	/** Time before operation is resumed (seconds). */
	int32_t timeout;
} lwm2m_carrier_event_deferred_t;

/**
 * @brief LWM2M carrier library event error codes.
 */
/** No error. */
#define LWM2M_CARRIER_ERROR_NO_ERROR        0
/** Failed to connect to the LTE network. */
#define LWM2M_CARRIER_ERROR_CONNECT_FAIL    1
/** Failed to disconnect from the LTE network. */
#define LWM2M_CARRIER_ERROR_DISCONNECT_FAIL 2
/** LWM2M carrier bootstrap failed. */
#define LWM2M_CARRIER_ERROR_BOOTSTRAP       3
/** Update package rejected. */
#define LWM2M_CARRIER_ERROR_FOTA_PKG        4
/** Protocol error. */
#define LWM2M_CARRIER_ERROR_FOTA_PROTO      5
/** Connection error. */
#define LWM2M_CARRIER_ERROR_FOTA_CONN       6
/** Connection lost. */
#define LWM2M_CARRIER_ERROR_FOTA_CONN_LOST  7
/** Update failed. */
#define LWM2M_CARRIER_ERROR_FOTA_FAIL       8

/**
 * @brief LWM2M carrier library error event structure.
 */
typedef struct {
	/** Error event code. */
	uint32_t code;
	/** Error event value. */
	int32_t  value;
} lwm2m_carrier_event_error_t;
/**@} */

/**
 * @defgroup lwm2m_carrier_api LWM2M carrier library API
 * @brief LWM2M carrier library API functions.
 * @{
 */
/**
 * @brief LWM2M device power sources type.
 */
#define LWM2M_CARRIER_POWER_SOURCE_DC               0
#define LWM2M_CARRIER_POWER_SOURCE_INTERNAL_BATTERY 1
#define LWM2M_CARRIER_POWER_SOURCE_EXTERNAL_BATTERY 2
#define LWM2M_CARRIER_POWER_SOURCE_ETHERNET         4
#define LWM2M_CARRIER_POWER_SOURCE_USB              5
#define LWM2M_CARRIER_POWER_SOURCE_AC               6
#define LWM2M_CARRIER_POWER_SOURCE_SOLAR            7

/**
 * @brief LWM2M device error codes.
 */
#define LWM2M_CARRIER_ERROR_CODE_NO_ERROR                0
#define LWM2M_CARRIER_ERROR_CODE_LOW_CHARGE              1
#define LWM2M_CARRIER_ERROR_CODE_EXTERNAL_SUPPLY_OFF     2
#define LWM2M_CARRIER_ERROR_CODE_GPS_FAILURE             3
#define LWM2M_CARRIER_ERROR_CODE_LOW_SIGNAL              4
#define LWM2M_CARRIER_ERROR_CODE_OUT_OF_MEMORY           5
#define LWM2M_CARRIER_ERROR_CODE_SMS_FAILURE             6
#define LWM2M_CARRIER_ERROR_CODE_IP_CONNECTIVITY_FAILURE 7
#define LWM2M_CARRIER_ERROR_CODE_PERIPHERAL_MALFUNCTION  8

/**
 * @brief LWM2M device battery status.
 *
 * @note These values are only valid for the LWM2M Device INTERNAL_BATTERY if
 *       present.
 */
#define LWM2M_CARRIER_BATTERY_STATUS_NORMAL          0
#define LWM2M_CARRIER_BATTERY_STATUS_CHARGING        1
#define LWM2M_CARRIER_BATTERY_STATUS_CHARGE_COMPLETE 2
#define LWM2M_CARRIER_BATTERY_STATUS_DAMAGED         3
#define LWM2M_CARRIER_BATTERY_STATUS_LOW_BATTERY     4
#define LWM2M_CARRIER_BATTERY_STATUS_NOT_INSTALLED   5
#define LWM2M_CARRIER_BATTERY_STATUS_UNKNOWN         6

/**
 * @brief LWM2M portfolio identity types.
 */
#define LWM2M_CARRIER_IDENTITY_ID           0
#define LWM2M_CARRIER_IDENTITY_MANUFACTURER 1
#define LWM2M_CARRIER_IDENTITY_MODEL        2
#define LWM2M_CARRIER_IDENTITY_SW_VERSION   3

/**
 * @brief Structure holding LWM2M carrier library initialization parameters.
 */
typedef struct {
	/** URI of the bootstrap server, null-terminated string. */
	const char *bootstrap_uri;
	/** Pre-Shared Key for the bootstrap server,
	 *  null-terminated hexadecimal string.
	 */
	const char *psk;
	/** Optional custom APN, null-terminated string. */
	const char *apn;
	/** Connect to certification servers if true,
	 *  connect to production servers otherwise.
	 */
	bool certification_mode;
} lwm2m_carrier_config_t;

/**
 * @brief Initialize the LWM2M carrier library.
 *
 * @param[in] config Configuration parameters for the library.
 *
 * @note The library does not copy the contents of pointers in the config
 *       parameters. The application has to make sure that the provided
 *       parameters are valid throughout the application lifetime
 *       (i. e. placed in static memory or in flash).
 *
 * @note The first time this function is called after a modem firmware update,
 *       (FOTA) it may take several seconds to return in order to complete
 *       the procedure.
 *
 * @return 0 on success, negative error code on error.
 */
int lwm2m_carrier_init(const lwm2m_carrier_config_t *config);

/**
 * @brief LWM2M carrier library main function.
 *
 * This is a non-return function, intended to run on a separate thread.
 */
void lwm2m_carrier_run(void);

/**
 * @brief Function to read all time parameters.
 *
 * @param[out] utc_time   Pointer to time since Epoch in seconds.
 * @param[out] utc_offset Pointer to UTC offset in minutes.
 * @param[out] tz         Pointer to null-terminated timezone string pointer.
 */
void lwm2m_carrier_time_read(int32_t *utc_time, int *utc_offset,
			     const char **tz);

/**
 * @brief Function to read current UTC time
 *
 * @note This function can be implemented by the application, if custom time
 *       management is needed.
 *
 * @return  Current UTC time since Epoch in seconds.
 */
int32_t lwm2m_carrier_utc_time_read(void);

/**
 * @brief Function to read offset to UTC time.
 *
 * @note This function can be implemented by the application, if custom time
 *       management is needed.
 *
 * @return  UTC offset in minutes.
 */
int lwm2m_carrier_utc_offset_read(void);

/**
 * @brief Function to read timezone
 *
 * @note This function can be implemented by the application, if custom time
 *       management is needed.
 *
 * @return  Null-terminated timezone string pointer, IANA Timezone (TZ)
 *          database format.
 */
const char *lwm2m_carrier_timezone_read(void);

/**
 * @brief Function to write current UTC time (LWM2M server write operation)
 *
 * @note This function can be implemented by the application, if custom time
 *       management is needed.
 *
 * @param[in] time Time since Epoch in seconds.
 *
 * @return 0 on success, negative error code on error.
 */
int lwm2m_carrier_utc_time_write(int32_t time);

/**
 * @brief Function to write UTC offset (LWM2M server write operation)
 *
 * @note This function can be implemented by the application, if custom time
 *       management is needed.
 *
 * @param[in] offset UTC offset in minutes.
 *
 * @return 0 on success, negative error code on error.
 */
int lwm2m_carrier_utc_offset_write(int offset);

/**
 * @brief Function to write timezone (LWM2M server write operation).
 *
 * @note This function can be implemented by the application, if custom time
 *       management is needed.
 *
 * @param[in] tz Null-terminated timezone string pointer.
 *
 * @return 0 on success, negative error code on error.
 */
int lwm2m_carrier_timezone_write(const char *tz);

/**
 * @brief LWM2M carrier library event handler.
 *
 * This function will be called by the LWM2M carrier library whenever some event
 * significant for the application occurs.
 *
 * @note This function has to be implemented by the application.
 *
 * @param[in] event LWM2M carrier event that occurred.
 *
 * @return     In case of @c LWM2M_CARRIER_EVENT_REBOOT events
 *             if non-zero is returned, the LwM2M carrier library will not
 *             reboot the device. The application should to reboot at
 *             the earliest convenient time.
 */
int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event);
/**@} */

/**
 * @brief      Set the available power sources supported and used by the LWM2M
 *             device.
 *
 * @note       It is necessary to call this function before any other device
 *             power source related functions listed in this file, as any
 *             updates of voltage/current measurements performed on power
 *             sources that have not been reported will be discarded.
 * @note       Upon consecutive calls of this function, the corresponding
 *             current and voltage measurements will be reset to 0. Similarly,
 *             the battery status will be set to UNKNOWN and the battery
 *             level to 0%.
 *
 * @param[in]  power_sources          Array of available device power sources.
 * @param[in]  power_source_count     Number of power sources currently used
 *                                    by the device.
 *
 * @retval     -E2BIG    If the reported number of power sources is bigger than
 *                       the maximum supported.
 * @retval     -EINVAL   If one or more of the power sources are not supported.
 * @retval     0         If the available power sources have been set
 *                       successfully.
 */
int lwm2m_carrier_avail_power_sources_set(const uint8_t *power_sources,
					  uint8_t power_source_count);

/**
 * @brief      Set or update the latest voltage measurements made on one of
 *             the available device power sources.
 *
 * @note       The voltage measurement needs to be specified in millivolts (mV)
 *             and is to be assigned to one of the available power sources.
 *
 * @param[in]  power_source    Power source to which the measurement
 *                             corresponds.
 * @param[in]  value           Voltage measurement expressed in mV.
 *
 * @retval     -EINVAL   If the power source is not supported.
 * @retval     -ENODEV   If the power source is not listed as an available
 *                       power source.
 * @retval     0         If the voltage measurements have been updated
 *                       successfully.
 */
int lwm2m_carrier_power_source_voltage_set(uint8_t power_source,
					   int32_t value);

/**
 * @brief      Set or update the latest current measurements made on one of
 *             the available device power sources.
 *
 * @note       The current measurement needs to be specified in
 *             milliamperes (mA) and is to be assigned to one of the available
 *             power sources.
 *
 * @param[in]  power_source           Power source to which the measurement
 *                                    corresponds.
 * @param[in]  value                  Current measurement expressed in mA.
 *
 * @retval     -EINVAL   If the power source is not supported.
 * @retval     -ENODEV   If the power source is not listed as an available
 *                       power source.
 * @retval     0         If the current measurements have been updated
 *                       successfully.
 */
int lwm2m_carrier_power_source_current_set(uint8_t power_source,
					   int32_t value);

/**
 * @brief      Set or update the latest battery level (internal battery).
 *
 * @note       The battery level is to be specified as a percentage, hence
 *             values outside the range 0-100 will be ignored.
 *
 * @note       The value is only valid for the Device internal battery if
 *             present.
 *
 * @param[in]  battery_level          Internal battery level percentage.
 *
 * @retval     -EINVAL   If the specified battery level lies outside the
 *                       0-100% range.
 * @retval     -ENODEV   If internal battery is not listed as an available
 *                       power source.
 * @retval     0         If the battery level has been updated successfully.
 */
int lwm2m_carrier_battery_level_set(uint8_t battery_level);

/**
 * @brief      Set or update the latest battery status (internal battery).
 *
 * @note       The value is only valid for the Device internal battery.
 *
 * @param[in]  battery_status         Internal battery status to be reported.
 *
 * @retval     -EINVAL   If the battery status is not supported.
 * @retval     -ENODEV   If internal battery is not listed as an available
 *                       power source.
 * @retval     0         If the battery status has been updated successfully.
 */
int lwm2m_carrier_battery_status_set(int32_t battery_status);

/**
 * @brief      Set the LWM2M device type.
 *
 * @note       Type of the LWM2M device specified by the manufacturer.
 *
 * @param[in]  device_type      Null terminated string specifying the type of
 *                              the LWM2M device.
 *
 * @retval     -EINVAL          If the input argument is a NULL pointer or
 *                              an empty string.
 * @retval     -E2BIG           If the input string is too long.
 * @retval     -ENOMEM          If it was not possible to allocate memory
 *                              storage to hold the string.
 * @retval     0                If the device type has been set successfully.
 */
int lwm2m_carrier_device_type_set(const char *device_type);

/**
 * @brief      Set the LWM2M device hardware version.
 *
 * @note       LWM2M device hardware version.
 *
 * @param[in]  hardware_version     Null terminated string specifying the
 *                                  hardware version of the LWM2M device.
 *
 * @retval     -EINVAL              If the input argument is a NULL pointer or
 *                                  an empty string.
 * @retval     -E2BIG               If the input string is too long.
 * @retval     -ENOMEM              If it was not possible to allocate memory
 *                                  storage to hold the string.
 * @retval     0                    If the hardware version has been set
 *                                  successfully.
 */
int lwm2m_carrier_hardware_version_set(const char *hardware_version);

/**
 * @brief      Set the LWM2M device software version.
 *
 * @note       High level device software version (application).
 *
 * @param[in]  software_version     Null terminated string specifying the
 *                                  current software version of the LWM2M
 *                                  device.
 *
 * @retval     -EINVAL              If the input argument is a NULL pointer or
 *                                  an empty string.
 * @retval     -E2BIG               If the input string is too long.
 * @retval     -ENOMEM              If it was not possible to allocate memory
 *                                  storage to hold the string.
 * @retval     0                    If the software version has been set
 *                                  successfully.
 */
int lwm2m_carrier_software_version_set(const char *software_version);

/**
 * @brief      Update the device object instance error code by adding an
 *             individual error.
 *
 * @note       Upon initialization of the device object instance, the error
 *             code is specified as 0, indicating no error. The error code is to
 *             be updated whenever a new error occurs.
 * @note       If the reported error is NO_ERROR, all existing error codes will
 *             be reset.
 * @note       If the reported error is already present, the error code will
 *             remain unchanged.
 *
 * @param[in]  error                Individual error to be added.
 *
 * @retval     -EINVAL              If the error code is not supported.
 * @retval     0                    If the error code has been added
 *                                  successfully.
 */
int lwm2m_carrier_error_code_add(int32_t error);

/**
 * @brief      Update the device object instance error code by removing an
 *             individual error.
 *
 * @note       Upon initialization of the device object instance, the error code
 *             is specified as 0, indicating no error. The error code is to be
 *             updated whenever an error is no longer present. When all the
 *             errors are removed, the error code is specified as 0, hence
 *             indicating no error again.
 *
 * @param[in]  error            Individual error code to be removed.
 *
 * @retval     -EINVAL          If the error code is not supported.
 * @retval     -ENOENT          If the error to be removed is not present on the
 *                              error code list.
 * @retval     0                If the error has been removed successfully.
 */
int lwm2m_carrier_error_code_remove(int32_t error);

/**
 * @brief      Set the total amount of storage space to store data and software
 *             in the LWM2M Device.
 *
 * @note       The value is expressed in kilobytes (kB).
 *
 * @param[in]  memory_total     Total amount of storage space in kilobytes.
 *
 * @retval     -EINVAL          It the reported value is bigger than INT32_MAX.
 * @retval     0                If the total amount of storage space has been
 *                              set successfully.
 */
int lwm2m_carrier_memory_total_set(uint32_t memory_total);

/**
 * @brief      Read the estimated current available amount of storage space to
 *             store data and software in the LWM2M Device.
 *
 * @note       This function must be implemented by the application in order to
 *             support the reporting of memory free, otherwise the returned
 *             value will be 0.
 *
 * @return     Available amount of storage space expressed in kB.
 */
int lwm2m_carrier_memory_free_read(void);

/**
 * @brief      Read the Identity field of a given Portfolio object instance.
 *
 * @note       If the provided buffer is NULL, the function will perform a dry
 *             run to determine the required buffer size (including the null
 *             terminator).
 *
 * @param[in]     instance_id    Portfolio object instance identifier.
 * @param[in]     identity_type  Type of Identity field to be read.
 * @param[inout]  buffer         Buffer where the null-terminated response
 *                               will be stored.
 * @param[inout]  buffer_len     Length of the provided buffer. Will return the
 *                               number of bytes of the full response.
 *
 * @retval        -ENOENT        If the instance does not exist.
 * @retval        -EINVAL        If the provided buffer length reference is NULL
 *                               or the identity type is invalid.
 * @retval        -ENOMEM        If the provided buffer is too small.
 * @retval        0              If the operation was successful.
 */
int lwm2m_carrier_identity_read(uint16_t instance_id, uint16_t identity_type,
					  char *buffer, uint16_t *buffer_len);

/**
 * @brief      Set the corresponding Identity field of a Portfolio object
 *             instance to a given value.
 *
 * @param[in]  instance_id    Portfolio object instance identifier.
 * @param[in]  identity_type  Type of Identity field to be written.
 * @param[in]  value          Null terminated string to be written into the
 *                            Identity field.
 *
 * @retval     -EPERM         If the specified object instance ID corresponds
 *                            to the Primary Host identity.
 * @retval     -EINVAL        If the input argument is a NULL pointer or
 *                            an empty string, or the identity type is invalid.
 * @retval     -ENOENT        If the instance does not exist.
 * @retval     -E2BIG         If the input string is too long.
 * @retval     -ENOMEM        If it was not possible to allocate memory
 *                            storage to hold the string.
 * @retval     0              If the Identity field has been updated
 *                            successfully.
 */
int lwm2m_carrier_identity_write(uint16_t instance_id, uint16_t identity_type,
					  const char *value);

/**
 * @brief      Create a new instance of the Portfolio object.
 *
 * @param[in]  instance_id    The identifier to be used for the new instance.
 *
 * @retval     -ENOMEM        If it was not possible to create the instance
 *                            because the maximum number of supported
 *                            object instances has already been reached.
 * @retval     -EINVAL        If the provided instance identifier is already in
 *                            use.
 * @retval     0              If the instance has been created successfully.
 */
int lwm2m_carrier_portfolio_instance_create(uint16_t instance_id);

#ifdef __cplusplus
}
#endif

#endif /* LWM2M_CARRIER_H__ */
