/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef DATE_TIME_H__
#define DATE_TIME_H__

#include <zephyr/types.h>
#include <time.h>

/**
 * @defgroup date_time Date Time Library
 * @{
 * @brief Library that maintains the current date time UTC which can be fetched
 *        in order to timestamp data.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Date time notification event types used to signal the application. */
enum date_time_evt_type {
	/** Date time library has obtained valid time from the modem. */
	DATE_TIME_OBTAINED_MODEM,
	/** Date time library has obtained valid time from NTP servers. */
	DATE_TIME_OBTAINED_NTP,
	/** Date time library has obtained valid time from external source. */
	DATE_TIME_OBTAINED_EXT,
	/** Date time library does not have valid time. */
	DATE_TIME_NOT_OBTAINED
};

/** @brief Struct with data received from the Date time library. */
struct date_time_evt {
	/** Type of event. */
	enum date_time_evt_type type;
};

/** @brief Date time library asynchronous event handler.
 *
 *  @param[in] evt The event and any associated parameters.
 */
typedef void (*date_time_evt_handler_t)(const struct date_time_evt *evt);

/** @brief Set the current date time.
 *
 *  @note See http://www.cplusplus.com/reference/ctime/tm/ for accepted input
 *        format.
 *
 *  @param[in] new_date_time Pointer to a tm structure.
 *
 *  @return 0        If the operation was successful.
 *  @return -EINVAL  If a member of the passing variable new_date_time does not
 *                   adhere to the tm structure format.
 */
int date_time_set(const struct tm *new_date_time);

/** @brief Get the date time UTC when the passing variable uptime was set.
 *         This function requires that k_uptime_get() has been called on the
 *         passing variable uptime prior to the function call.
 *
 *  @warning If the function fails, the passed in variable retains its
 *           old value.
 *
 *  @param[in, out] uptime Pointer to a previously set uptime.
 *
 *  @return 0        If the operation was successful.
 *  @return -ENODATA If the library does not have a valid date time UTC.
 *  @return -EINVAL  If the passing variable is too large or already converted.
 */
int date_time_uptime_to_unix_time_ms(int64_t *uptime);

/** @brief Get the current date time UTC.
 *
 *  @warning If the function fails, the passed in variable retains its
 *           old value.
 *
 *  @param[out] unix_time_ms Pointer to a variable to store the current date
 *                           time UTC.
 *
 *  @return 0        If the operation was successful.
 *  @return -ENODATA If the library does not have a valid date time UTC.
 */
int date_time_now(int64_t *unix_time_ms);

/** @brief Register an event handler for Date time library events.
 *
 *  @warning The library only allows for one event handler to be registered
 *           at a time. A passed in event handler in this function will
 *           overwrite the previously set event handler.
 *
 *  @param evt_handler Event handler. Handler is de-registered if parameter is
 *                     NULL.
 */
void date_time_register_handler(date_time_evt_handler_t evt_handler);

/** @brief Asynchronous update of internal date time UTC. This function
 *         initiates a date time update regardless of the internal update
 *         interval. If an event handler is provided it will be updated
 *         with library events, accordingly.
 *
 *  @param evt_handler Event handler. If the passed in pointer is NULL the
 *                     previous registered event handler is not de-registered.
 *                     This means that library events will still be received in
 *                     the previously registered event handler.
 *
 *  @return 0 If the operation was successful.
 */
int date_time_update_async(date_time_evt_handler_t evt_handler);

/** @brief Clear the current date time held by the library.
 *
 *  @return 0 If the operation was successful.
 */
int date_time_clear(void);

/** @brief Clear a timestamp in unix time ms.
 *
 *  @param[in, out] unix_timestamp Pointer to a unix timestamp.
 *
 *  @return 0 If the operation was successful.
 */
int date_time_timestamp_clear(int64_t *unix_timestamp);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DATE_TIME_H__ */
