/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef GPS_MODULE_H__
#define GPS_MODULE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise GPS driver.
 *
 * @return int 0 if successful, negative error code if not.
 */
int initialise_gps(void);

/**
 * @brief Start GPS search.
 *
 * @return int 0 if successful, negative error code if not.
 */
int start_gps_search(void);

#ifdef __cplusplus
}
#endif

#endif /* GPS_MODULE_H__ */
