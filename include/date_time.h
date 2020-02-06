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
 *  @param[in] new_date_time Pointer to a tm structure.
 */
void date_time_set(const struct tm *new_date_time);

/** @brief Get the date time UTC when the passing variable uptime was set.
 *         This function requires that k_uptime_get() has been called on the
 *         passing variable uptime prior to the function call.
 *
 *  @param[in, out] uptime Pointer to a previously set uptime.
 *
 *  @return 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int date_time_uptime_to_unix_time_ms(s64_t *uptime);

/** @brief Get the current date time UTC.
 *
 *  @param[out] unix_time_ms Pointer to a variable to store the current date
 *                           time UTC.
 *
 *  @return 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int date_time_now(s64_t *unix_time_ms);

/** @brief Asynchronous update of internal date time UTC. This function
 *         initiates a date time update regardless of the internal update
 *         interval.
 */
void date_time_update(void);

#ifdef __cplusplus
}
#endif

#endif /* DATE_TIME_H__ */
