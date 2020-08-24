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

/** @brief Asynchronous update of internal date time UTC. This function
 *         initiates a date time update regardless of the internal update
 *         interval.
 *
 *  @return 0 If the operation was successful.
 */
int date_time_update_async(void);

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
