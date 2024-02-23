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
	GNSS_DATA_DELETE_EPHEMERIDES,
	GNSS_DATA_DELETE_EKF,
	GNSS_DATA_DELETE_ALL,
	GNSS_DATA_DELETE_TCXO
};

enum gnss_dynamics_mode {
	GNSS_DYNAMICS_MODE_GENERAL,
	GNSS_DYNAMICS_MODE_STATIONARY,
	GNSS_DYNAMICS_MODE_PEDESTRIAN,
	GNSS_DYNAMICS_MODE_AUTOMOTIVE
};

enum gnss_timing_source {
	GNSS_TIMING_SOURCE_RTC,
	GNSS_TIMING_SOURCE_TCXO
};

enum gnss_qzss_nmea_mode {
	/** Standard NMEA PRN reporting. */
	GNSS_QZSS_NMEA_MODE_STANDARD,
	/** Custom NMEA PRN reporting (PRN IDs 193...202 used for QZSS satellites). */
	GNSS_QZSS_NMEA_MODE_CUSTOM
};

struct gnss_1pps_mode {
	/** True if 1PPS is enabled, false if disabled. */
	bool enable;
	/** Pulse interval (s) 0...1800s. 0 denotes one-time pulse mode. */
	uint16_t pulse_interval;
	/** Pulse width (ms) 1...500ms. */
	uint16_t pulse_width;
	/** True if start time is applied, false if not. */
	bool apply_start_time;
	/** Gregorian year. */
	uint16_t year;
	/** Month of the year. */
	uint8_t month;
	/** Day of the month. */
	uint8_t day;
	/** Hour of the day. */
	uint8_t hour;
	/** Minute of the hour. */
	uint8_t minute;
	/** Second of the minute. */
	uint8_t second;
};

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
 * @brief Deletes GNSS data from NV memory.
 *
 * @param data Value indicating which data is deleted.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_delete_data(enum gnss_data_delete data);

/**
 * @brief Deletes GNSS data from NV memory using custom bitmask.
 *
 * @param mask Bitmask indicating which is deleted.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_delete_data_custom(uint32_t mask);

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
 * @brief Sets the system mask.
 *
 * Bit 0: GPS
 * Bit 1: reserved
 * Bit 2: QZSS
 * Bit 3..7: reserved
 *
 * @param system_mask System bitmask.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_system_mask(uint8_t system_mask);

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
 * @brief Enables/disabled A-GNSS filtered ephemerides.
 *
 * When enabled, the nRF Cloud A-GNSS service will be requested to reduce
 * the set of ephemerides returned to only include those satellites which
 * are at or above the most recently set elevation threshold angle.
 *
 * @param enable True to enable, false to disable.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_agnss_filtered_ephemerides(bool enable);

/**
 * @brief Sets the GNSS use case configuration.
 *
 * @param low_accuracy_enabled True if low accuracy fixes are allowed, false if not.
 * @param scheduled_downloads_disabled True if scheduled downloads are disabled, false if not.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_use_case(bool low_accuracy_enabled, bool scheduled_downloads_disabled);

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
 * @brief Sets the dynamics mode.
 *
 * @param mode Dynamics mode.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_dynamics_mode(enum gnss_dynamics_mode mode);

/**
 * @brief Sets the QZSS NMEA mode. In standard NMEA mode QZSS satellites are not reported.
 *        In custom NMEA mode QZSS satellites are reported using PRN IDs 193...202.
 *
 * @param nmea_mode QZSS NMEA mode.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_qzss_nmea_mode(enum gnss_qzss_nmea_mode nmea_mode);

/**
 * @brief Sets the QZSS PRN mask.
 *
 * @param mask Bits 0 to 9 when set correspond to PRNs 193...202 being enabled.
 *             Bits 10...15 are reserved.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_qzss_mask(uint16_t mask);

/**
 * @brief Sets the 1PPS mode configuration.
 *
 * @param config 1PPS mode configuration.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_1pps_mode(const struct gnss_1pps_mode *config);

/**
 * @brief Sets the timing source during sleep.
 *
 * @param source Timing source.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_timing_source(enum gnss_timing_source source);

/**
 * @brief Sets which A-GNSS data is allowed to be injected to the modem (either
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
int gnss_set_agnss_data_enabled(bool ephe, bool alm, bool utc, bool klob,
				bool neq, bool time, bool pos, bool integrity);

/**
 * @brief Sets whether A-GNSS data is automatically fetched whenever requested
 *        by GNSS.
 *
 * @param value True if A-GNSS data is fetched automatically, false if not.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_agnss_automatic(bool value);

/**
 * @brief Fetches and injects A-GNSS data to GNSS.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_inject_agnss_data(void);

/**
 * @brief Enables P-GPS. Once enabled, P-GPS remains enabled until reboot.
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_enable_pgps(void);

/**
 * @brief Queries A-GNSS data expiry information from GNSS.
 *
 * @return 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_get_agnss_expiry(void);

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
 * @brief Configures whether GNSS API event information is printed out.
 *
 * @param level 0 = event output disabled
 *              1 = event output enabled
 *
 * @retval 0 if the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int gnss_set_event_output_level(uint8_t level);

#endif /* MOSH_GNSS_H */
