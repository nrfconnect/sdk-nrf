/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file sb_fota.h
 *
 * @brief Public APIs for the Softbank FOTA client.
 * @defgroup sb_fota Softbank FOTA client
 * @{
 */

#ifndef SB_FOTA_H_
#define SB_FOTA_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FOTA application events.
 *
 */
enum sb_fota_event {
	/** FOTA client starts networking.
	 * Application should close all TLS sockets. Modem may change the operation mode
	 * from Nb-IOT to LTE-M, so any network activity is recommended to be paused. */
	FOTA_EVENT_DOWNLOADING,
	/** FOTA client is now on idle.
	 * Application may resume normal network operations. */
	FOTA_EVENT_IDLE,
	/** Modem FW is going to be updated, so it is shut down.
	 * Application cannot use the network anymore until firmware is updated and
	 * device is booted. */
	FOTA_EVENT_MODEM_SHUTDOWN,
	/** Modem FW is now updated, reboot the device to resume network operations. */
	FOTA_EVENT_REBOOT_PENDING,
};

/**
 * @brief Modem FOTA application callback function.
 *
 * See also @ref sb_fota_event
 *
 * @param event_id Event ID.
 */
typedef void (*sb_fota_callback_t)(enum sb_fota_event event);

/**
 * @brief Initializes the modem FOTA client.
 *
 * Initializes the library and registers a callback for FOTA process.
 * Subsequent calls may be used to change the callback function.
 *
 * If Kconfig value CONFIG_SB_FOTA_AUTOINIT is set, then application is not required to call this
 * function, but might still want to use it for registering the callback.
 *
 * If there is no callback set, then the library automatically reboots the device in case of
 * FOTA_EVENT_REBOOT_PENDING event.
 *
 * @param callback Callback for the application events or NULL for no callback.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int sb_fota_init(sb_fota_callback_t callback);

/**
 * @brief Sets current time for the modem FOTA client and modem clock.
 *
 * Sets the current time used by the modem FOTA client. The given time is also
 * set to the modem. If LTE network time is not available, current time has
 * to be provided to the modem FOTA client using this function.
 *
 * LTE network time overrides the time set using this function. If time is
 * available from the LTE network, time doesn't have to be set using this
 * function.
 *
 * @param time_str Time as a null terminated string in format
 *                 "yy/MM/dd,hh:mm:ss±zz", where the characters, from left to
 *                 right, indicate year, month, day, hour, minutes, seconds and
 *                 time zone. Time zone indicates the difference, expressed in
 *                 quarters of an hour, between the local time and GMT (value
 *                 range -48...+48). For example "20/10/15,09:12:47+12".
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int sb_fota_set_clock(const char *time_str);

#ifdef __cplusplus
}
#endif

#endif /* SB_FOTA_H_ */

/** @} */
