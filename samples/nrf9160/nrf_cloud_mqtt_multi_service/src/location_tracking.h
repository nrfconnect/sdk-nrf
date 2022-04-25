/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LOCATION_TRACKING_H_
#define _LOCATION_TRACKING_H_

#include <modem/location.h>

/**
 * @brief Check an MQTT message payload for AGPS data, and if AGPS data is present,
 * pass it along to the modem for use in GNSS fix acquisition.
 *
 * @param[in] buf The MQTT message payload buffer.
 * @param[in] len the length of the MQTT message payload.
 */
void location_agps_data_handler(const char *buf, size_t len);

/**
 * @brief Callback to receive tracked locations.
 *
 * @param[in] location_data The tracked location data.
 */
typedef void (*location_update_cb_t)(const struct location_data location_data);

/**
 * @brief Start tracking our location at the given interval in seconds.
 *
 * @param handler_cb - Handler callback to receive location updates.
 * @param interval - The interval, in seconds, at which to track our location.
 */
int start_location_tracking(location_update_cb_t handler_cb, int interval);

#endif /* _LOCATION_TRACKING_H_ */
