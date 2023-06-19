/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef ZEPHYR_INCLUDE_MODEM_BATTERY_H_
#define ZEPHYR_INCLUDE_MODEM_BATTERY_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file modem_battery.h
 *
 * @defgroup modem_battery Modem battery
 *
 * @{
 *
 * @brief Public APIs for modem battery.
 */

/**
 * @brief Battery voltage levels for power-off warnings.
 */
enum pofwarn_level {
	POFWARN_3000 = 30,
	POFWARN_3100 = 31,
	POFWARN_3200 = 32,
	POFWARN_3300 = 33
};

/**
 * @brief Type definition of event handler for battery voltage low level notifications.
 *
 * @param battery_voltage Battery voltage provided by the low level notification.
 *                        Unit of measure mV.
 */
typedef void(*modem_battery_low_level_handler_t)(int battery_voltage);

/**
 * @brief Function to set an event handler for battery voltage low level notifications.
 *
 * @param handler Event handler.
 *
 * @retval 0            If execution was successful.
 * @retval -EINVAL      If handler is a NULL pointer.
 */
int modem_battery_low_level_handler_set(modem_battery_low_level_handler_t handler);

/**
 * @brief Type definition of Event handler for power-off warnings.
 */
typedef void(*modem_battery_pofwarn_handler_t)(void);

/**
 * @brief Function to set an event handler for power-off warnings.
 *
 * @param handler Event handler.
 *
 * @note Only supported with modem firmware v1.3.1 and higher.
 *
 * @retval 0            If execution was successful.
 * @retval -EINVAL      If handler is a NULL pointer.
 */
int modem_battery_pofwarn_handler_set(modem_battery_pofwarn_handler_t handler);

/** @brief Configure power-off warnings from the modem.
 *
 * @note The warning is received as an unsolicited AT notification.
 *
 * @note When the power-off warning is sent, the modem sets itself to Offline mode and sends
 *       unsolicited notifications.
 *
 * @note The application is responsible for detecting possible increase in the battery voltage
 *       level and for restarting the LTE protocol activities. This can be detected by calling
 *       @ref modem_battery_voltage_get. If the level is acceptable again (>3000 mV),
 *       the application can proceed with initialization of the modem.
 *
 * @note Only supported with modem firmware v1.3.1 and higher.
 *
 * @param level Battery voltage level for the power-off warnings.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int modem_battery_pofwarn_enable(enum pofwarn_level level);

/** @brief Disable power-off warnings from the modem.
 *
 * @note Only supported with modem firmware v1.3.1 and higher.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int modem_battery_pofwarn_disable(void);

/** @brief Subscribe unsolicited notifications of battery voltage low level.
 *
 * @note The command configuration is stored to NVM approximately every 48 hours and when the modem
 *       is powered off with the +CFUN=0 command.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int modem_battery_low_level_enable(void);

/** @brief Unsubscribe unsolicited notifications of battery voltage low level.
 *
 * @note The command configuration is stored to NVM approximately every 48 hours and when the modem
 *       is powered off with the +CFUN=0 command.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int modem_battery_low_level_disable(void);

/** @brief Set the battery voltage low level for the modem.
 *
 * @note This function sends an AT command for setting the battery voltage low level for the modem.
 *       If notifications of battery voltage low level have been subscribed
 *       (see @ref modem_battery_low_level_enable), the modem sends clients a
 *       notification when the measured battery voltage is below the defined level.
 *       The modem reads sensors periodically in connected mode. The default period is
 *       60 seconds. If the temperature or voltage gets close to the set threshold, a shorter
 *       period is used.
 *       Battery voltage low level has range 3100-5000 mV. Factory default is 3300 mV.
 *
 * @param battery_level Battery voltage low level. Unit of measure mV.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if AT command failed.
 */
int modem_battery_low_level_set(int battery_level);

/** @brief Function for retrieving the latest voltage measured automatically during modem wakeup or
 *         reception. During modem inactivity, the modem measures battery voltage when this
 *         function is called.
 *
 * @note This function sends the Nordic-proprietary `%XVBAT` command that reads battery voltage.
 *       When the modem is active (either LTE communication or GNSS receiver), the `%XVBAT` command
 *       returns the latest voltage measured automatically during modem wakeup or reception.
 *       The voltage measured during transmission is not reported. During modem inactivity,
 *       the modem measures battery voltage when the `%XVBAT` command is received.
 *
 * @param[out] battery_voltage Pointer to the variable for battery voltage.
 *                             Battery voltage in mV, with a resolution of 4mV.
 *
 * @retval 0 if successful.
 * @retval -EINVAL if input argument was invalid.
 * @retval -EFAULT if AT command failed.
 */
int modem_battery_voltage_get(int *battery_voltage);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MODEM_BATTERY_H_ */
