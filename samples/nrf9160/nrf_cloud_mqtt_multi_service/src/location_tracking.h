/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LOCATION_TRACKING_H_
#define _LOCATION_TRACKING_H_

/* Definition found in location.h */
struct location_data;

/**
 * @brief Check an MQTT message payload for AGPS data, and if AGPS data is present,
 * pass it along to the modem for use in GNSS fix acquisition.
 *
 * @param[in] buf The MQTT message payload buffer.
 * @param[in] len the length of the MQTT message payload.
 */
void location_assistance_data_handler(const char *buf, size_t len);

/**
 * @brief Callback to receive tracked locations.
 *
 * @param[in] location_data The tracked location data.
 */
typedef void (*location_update_cb_t)(const struct location_data * const location_data);

/**
 * @brief Start tracking our location at the given interval in seconds.
 *
 * @param handler_cb - Handler callback to receive location updates.
 * @param interval - The interval, in seconds, at which to track our location.
 */
int start_location_tracking(location_update_cb_t handler_cb, int interval);

/**
 * @brief Check whether one of the location tracking methods is enabled
 *
 * @return bool - Whether location tracking of any form is enabled
 */
static inline bool location_tracking_enabled(void)
{
	return IS_ENABLED(CONFIG_LOCATION_TRACKING_GNSS) ||
	       IS_ENABLED(CONFIG_LOCATION_TRACKING_CELLULAR);
}


#endif /* _LOCATION_TRACKING_H_ */
