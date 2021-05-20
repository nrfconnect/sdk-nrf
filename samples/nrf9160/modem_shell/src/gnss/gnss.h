/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_GNSS_H
#define MOSH_GNSS_H

#include <zephyr/types.h>

enum gnss_duty_cycling_policy {
	GNSS_DUTY_CYCLING_DISABLED,
	GNSS_DUTY_CYCLING_PERFORMANCE,
	GNSS_DUTY_CYCLING_POWER
};

enum gnss_data_delete {
	GNSS_DATA_DELETE_NONE,
	GNSS_DATA_DELETE_EPHEMERIDES,
	GNSS_DATA_DELETE_ALL
};

/* Common functions */

int gnss_set_lna_enabled(bool enabled);

/* Functions implemented by different API implementations
 * (GNSS socket/GPS driver)
 */

/**
 * @brief Starts GNSS.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_start(void);

/**
 * @brief Stops GNSS.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_stop(void);

/**
 * @brief Sets continuous tracking mode.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_continuous_mode(void);

/**
 * @brief Sets single fix mode.
 *
 * @param fix_retry Fix retry period (in seconds). Indicates how long
 *                  GNSS tries to get a fix before giving up. Value 0
 *                  denotes unlimited retry period.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_single_fix_mode(uint16_t fix_retry);

/**
 * @brief Sets periodic fix mode controlled by application.
 *
 * @param fix_interval Delay between fixes (in seconds).
 * @param fix_retry    Fix retry period (in seconds). Indicates how long
 *                     GNSS tries to get a fix before giving up. Value 0
 *                     denotes unlimited retry period.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_periodic_fix_mode(uint32_t fix_interval, uint16_t fix_retry);

/**
 * @brief Sets periodic fix mode controlled by GNSS.
 *
 * @param fix_interval Delay between fixes (in seconds). Allowed values are
 *                     10...1800.
 * @param fix_retry    Fix retry period (in seconds). Indicates how long
 *                     GNSS tries to get a fix before giving up. Value 0
 *                     denotes unlimited retry period.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_periodic_fix_mode_gnss(uint16_t fix_interval, uint16_t fix_retry);

/**
 * @brief Sets duty cycling policy for continuous tracking mode.
 *
 * Duty cycled tracking saves power by operating the GNSS receiver in on/off
 * cycles. Two different duty cycling modes are supported, one which saves
 * power without significant performance degradation and one which saves even
 * more power with an acceptable performance degradation.
 *
 * @param policy Duty cycling policy value.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_duty_cycling_policy(enum gnss_duty_cycling_policy policy);

/**
 * @brief Sets what stored data (if any) is deleted whenever the GNSS is started.
 *
 * @param value Value indicating which data is deleted.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_data_delete(enum gnss_data_delete value);

/**
 * @brief Sets satellite elevation threshold angle.
 *
 * Satellites with elevation angle less than the threshold are excluded from
 * the PVT estimation.
 *
 * @param elevation Satellite elevation in degrees.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_elevation_threshold(uint8_t elevation);

/**
 * @brief Sets whether low accuracy fixes are allowed.
 *
 * @param value True if low accuracy fixes are allowed, false if not.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_low_accuracy(bool value);

/**
 * @brief Sets the NMEA mask.
 *
 * Bit 0: GGA
 * Bit 1: GLL
 * Bit 2: GSA
 * Bit 3: GSV
 * Bit 4: RMC
 * Bit 5..15: reserved
 *
 * @param nmea_mask NMEA bitmask.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_nmea_mask(uint16_t nmea_mask);

/**
 * @brief Enables/disables priority time window requests.
 *
 * When priority time windows are enabled, GNSS requests longer RF time windows
 * from the modem. This may interfere with LTE operations.
 *
 * Once a single valid PVT estimate has been produced, priority time window
 * requests are automatically disabled.
 *
 * @param value True to enable priority time windows, false the disable.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_priority_time_windows(bool value);

/**
 * @brief Sets which AGPS data is allowed to be injected to the modem (either
 *        automatically or manually). By default all types are enabled.
 *
 * @param ephe True if ephemerides are enabled, false if not.
 * @param alm True if almanacs are enabled, false if not.
 * @param utc True if UTC assistance is enabled, false if not.
 * @param klob True if Klobuchar is enabled, false if not.
 * @param neq True if NeQuick is enabled, false if not.
 * @param time True if system time is enabled, false if not.
 * @param pos True if position is enabled, false if not.
 * @param integrity True if integrity data is enabled, false if not.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_agps_data_enabled(bool ephe, bool alm, bool utc, bool klob,
			       bool neq, bool time, bool pos, bool integrity);

/**
 * @brief Sets whether AGPS data is automatically fetched whenever requested
 *        by GNSS.
 *
 * @param value True if AGPS data is fetched automatically, false if not.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_agps_automatic(bool value);

/**
 * @brief Fetches and injects AGPS data to GNSS.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_inject_agps_data(void);

/**
 * @brief Configures how much PVT information is printed out.
 *
 * @param level 0 = PVT output disabled
 *              1 = fix information enabled
 *              2 = fix and SV information enabled
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_pvt_output_level(uint8_t level);

/**
 * @brief Configures whether NMEA strings are printed out.
 *
 * @param level 0 = NMEA output disabled
 *              1 = NMEA output enabled
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_nmea_output_level(uint8_t level);

/**
 * @brief Configures whether GPS driver event information is printed out.
 *
 * @param level 0 = event output disabled
 *              1 = event output enabled
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_event_output_level(uint8_t level);

#endif /* MOSH_GNSS_H */
