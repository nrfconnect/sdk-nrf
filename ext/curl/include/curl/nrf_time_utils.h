/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_TIME_UTILS_H
#define NRF_TIME_UTILS_H

/**
 * @brief Returns current time since the last bootup.
 *
 * @details This is equvalent of time() function in standard C except that
 * this function returns time since last boot up while standard C implementation
 * returns time since beginning of year 1970.
 *
 * @param[out] t Time since last bootup.
 *
 * @return Time since last bootup.
 */
time_t nrf_time(time_t *t);

#endif /* NRF_TIME_UTILS_H */
