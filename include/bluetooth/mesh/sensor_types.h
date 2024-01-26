/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @defgroup bt_mesh_sensor_types Sensor types
 *  @{
 *  @brief All available sensor types from the mesh device properties
 *  specification.
 */

#ifndef BT_MESH_SENSOR_TYPES_H__
#define BT_MESH_SENSOR_TYPES_H__

#include <bluetooth/mesh/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @def BT_MESH_SENSOR_TYPE_FORCE
 *
 *  @brief Force the given sensor type to be included in the build.
 *
 *  If @em CONFIG_BT_MESH_SENSOR_FORCE_ALL is disabled, only referenced sensor
 *  types will be included in the build, and the node will be unable to
 *  interpret the rest. Use this macro inside a function to force the type to be
 *  included in the build:
 *
 *  @code{.c}
 *  void some_function(void)
 *  {
 *      BT_MESH_SENSOR_TYPE_FORCE(bt_mesh_sensor_time_since_motion_sensed);
 *      BT_MESH_SENSOR_TYPE_FORCE(bt_mesh_sensor_motion_threshold);
 *  }
 *  @endcode
 *
 *  @param[in] _type Sensor type to force into the build.
 */
#define BT_MESH_SENSOR_TYPE_FORCE(_type)                                       \
	{                                                                      \
		const struct bt_mesh_sensor_type *volatile __unused val =      \
			&_type;                                                \
	}

/** @defgroup bt_mesh_sensor_formats_percentage Percentage sensor formats
 *  @{
 */

/** Percentage 8
 *  - Unit: Percent
 *  - Encoding: 8 bit unsigned scalar (Resolution: 0.5)
 *  - Range: 0 to 100.0
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_percentage_8;

/** Percentage 16
 *  - Unit: Percent
 *  - Encoding: 16 bit unsigned scalar (Resolution: 0.01)
 *  - Range: 0 to 100.0
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_percentage_16;

/** Percentage delta trigger
 *  - Unit: Percent
 *  - Encoding: 16 bit unsigned scalar (Resolution: 0.01)
 *  - Range: 0 to 655.35
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_percentage_delta_trigger;
/** @} */

/** @defgroup bt_mesh_sensor_formats_environmental Environmental sensor formats
 *  @{
 */

/** Temp 8
 *  - Unit: Celsius
 *  - Encoding: 8 bit signed scalar (Resolution: 0.5)
 *  - Range: -64.0 to 63.0
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_temp_8;

/** Temp
 *  - Unit: Celsius
 *  - Encoding: 16 bit signed scalar (Resolution: 0.01)
 *  - Range: -327.67 to 327.67
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_temp;

/** Co2 concentration
 *  - Unit: Parts per million
 *  - Encoding: 16 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 0 to 65534
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_co2_concentration;

/** Noise
 *  - Unit: Decibel
 *  - Encoding: 8 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 0 to 254
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_noise;

/** Voc concentration
 *  - Unit: Parts per billion
 *  - Encoding: 16 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 0 to 65534
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_voc_concentration;

/** Wind speed
 *  - Unit: Meters per second
 *  - Encoding: 16 bit unsigned scalar (Resolution: 0.01)
 *  - Range: 0 to 655.35
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_wind_speed;

/** Temp 8 wide
 *  - Unit: Celsius
 *  - Encoding: 8 bit signed scalar (Resolution: 1.0)
 *  - Range: -128 to 127
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_temp_8_wide;

/** Gust factor
 *  - Unit: Unitless
 *  - Encoding: 8 bit unsigned scalar (Resolution: 0.1)
 *  - Range: 0 to 25.5
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_gust_factor;

/** Magnetic flux density
 *  - Unit: Microtesla
 *  - Encoding: 16 bit signed scalar (Resolution: 0.1)
 *  - Range: -3276.8 to 3276.7
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_magnetic_flux_density;

/** Pollen concentration
 *  - Unit: Concentration per m3
 *  - Encoding: 24 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 0 to 16777215
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_pollen_concentration;

/** Pressure
 *  - Unit: Pascal
 *  - Encoding: 32 bit unsigned scalar (Resolution: 0.1)
 *  - Range: 0 to 429496729.5
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_pressure;

/** Rainfall
 *  - Unit: Meter
 *  - Encoding: 16 bit unsigned scalar (Resolution: 0.001)
 *  - Range: 0 to 65.535
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_rainfall;

/** Uv index
 *  - Unit: Unitless
 *  - Encoding: 8 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 0 to 255
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_uv_index;
/** @} */

/** @defgroup bt_mesh_sensor_formats_time Time sensor formats
 *  @{
 */

/** Time decihour 8
 *  - Unit: Hours
 *  - Encoding: 8 bit unsigned scalar (Resolution: 0.1)
 *  - Range: 0 to 24.0
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_time_decihour_8;

/** Time hour 24
 *  - Unit: Hours
 *  - Encoding: 24 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 0 to 16777214
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_time_hour_24;

/** Time second 16
 *  - Unit: Seconds
 *  - Encoding: 16 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 0 to 65534
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_time_second_16;

/** Time millisecond 24
 *  - Unit: Seconds
 *  - Encoding: 24 bit unsigned scalar (Resolution: 0.001)
 *  - Range: 0 to 16777.214
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_time_millisecond_24;

/** Time exponential 8
 *  - Unit: Seconds
 *  - Encoding: 8 bit unsigned exponential time (pow(1.1, N - 64) seconds)
 *  - Range: 0 to 73216705
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_time_exp_8;
/** @} */

/** @defgroup bt_mesh_sensor_formats_electrical Electrical sensor formats
 *  @{
 */

/** Electric current
 *  - Unit: Ampere
 *  - Encoding: 16 bit unsigned scalar (Resolution: 0.01)
 *  - Range: 0 to 655.34
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_electric_current;

/** Voltage
 *  - Unit: Volt
 *  - Encoding: 16 bit unsigned scalar (Resolution: 1/64)
 *  - Range: 0 to 1022.0
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_voltage;

/** Energy32
 *  - Unit: kWh
 *  - Encoding: 32 bit unsigned scalar (Resolution: 0.001)
 *  - Range: 0 to 4294967.293
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_energy32;

/** Apparent energy32
 *  - Unit: Kilovolt-ampere-hours
 *  - Encoding: 32 bit unsigned scalar (Resolution: 0.001)
 *  - Range: 0 to 4294967.293
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_apparent_energy32;

/** Apparent power
 *  - Unit: Volt-ampere
 *  - Encoding: 24 bit unsigned scalar (Resolution: 0.1)
 *  - Range: 0 to 1677721.3
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_apparent_power;

/** Power
 *  - Unit: Watt
 *  - Encoding: 24 bit unsigned scalar (Resolution: 0.1)
 *  - Range: 0 to 1677721.4
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_power;

/** Energy
 *  - Unit: kWh
 *  - Encoding: 24 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 0 to 16777214
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_energy;

/** @} */

/** @defgroup bt_mesh_sensor_formats_lighting Lighting sensor formats
 *  @{
 */
/** Chromatic distance
 *  - Unit: Unitless
 *  - Encoding: 16 bit signed scalar (Resolution: 0.00001)
 *  - Range: -0.05 to 0.05
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_chromatic_distance;

/** Chromaticity coordinate
 *  - Unit: Unitless
 *  - Encoding: 16 bit unsigned scalar (Resolution: 1/65536)
 *  - Range: 0 to 0.99998
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_chromaticity_coordinate;

/** Correlated color temp
 *  - Unit: Kelvin
 *  - Encoding: 16 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 800 to 65534
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_correlated_color_temp;

/** Illuminance
 *  - Unit: Lux
 *  - Encoding: 24 bit unsigned scalar (Resolution: 0.01)
 *  - Range: 0 to 167772.14
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_illuminance;

/** Luminous efficacy
 *  - Unit: Lumen per Watt
 *  - Encoding: 16 bit unsigned scalar (Resolution: 0.1)
 *  - Range: 0 to 1800.0
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_luminous_efficacy;

/** Luminous energy
 *  - Unit: Kilolumen-hours
 *  - Encoding: 24 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 0 to 16777214
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_luminous_energy;

/** Luminous exposure
 *  - Unit: Kilolux-hours
 *  - Encoding: 24 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 0 to 16777214
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_luminous_exposure;

/** Luminous flux
 *  - Unit: Lumen
 *  - Encoding: 16 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 0 to 65534
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_luminous_flux;

/** Perceived lightness
 *  - Unit: Unitless
 *  - Encoding: 16 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 0 to 65535
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_perceived_lightness;
/** @} */

/** @defgroup bt_mesh_sensor_formats_miscellaneous Miscellaneous sensor formats
 *  @{
 */

/** Direction 16
 *  - Unit: Degrees
 *  - Encoding: 16 bit unsigned scalar (Resolution: 0.01)
 *  - Range: 0 to 359.99
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_direction_16;

/** Count 16
 *  - Unit: Unitless
 *  - Encoding: 16 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 0 to 65534
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_count_16;

/** Gen lvl
 *  - Unit: Unitless
 *  - Encoding: 16 bit unsigned scalar (Resolution: 1.0)
 *  - Range: 0 to 65535
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_gen_lvl;

/** Cos of the angle
 *  - Unit: Unitless
 *  - Encoding: 8 bit signed scalar (Resolution: 1.0)
 *  - Range: -100 to 100
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_cos_of_the_angle;

/** Boolean
 *  - Unit: Unitless
 *  - Encoding: boolean
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_boolean;

/** Coefficient
 *  - Unit: Unitless
 *  - Encoding: 32 bit float
 */
extern const struct bt_mesh_sensor_format bt_mesh_sensor_format_coefficient;
/** @} */

/** @defgroup bt_mesh_sensor_types_occupancy Occupancy sensor types
 *  @{
 */

/** Motion sensed
 *
 *  Channels:
 *  - Motion sensed
 *    - Unit: Percent
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.5 %)
 *    - Range: 0 to 100.0
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_motion_sensed;

/** Motion threshold
 *
 *  This sensor type should be used as a sensor setting.
 *
 *  Channels:
 *  - Motion threshold
 *    - Unit: Percent
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.5 %)
 *    - Range: 0 to 100.0
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_motion_threshold;

/** People count
 *
 *  Channels:
 *  - People count
 *    - Unit: _unitless_
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1)
 *    - Range: 0 to 65533
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_people_count;

/** Presence detected
 *
 *  Channels:
 *  - Presence detected
 *    - Unit: _unitless_
 *    - Encoding: boolean
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_presence_detected;

/** Time since motion sensed
 *
 *  Channels:
 *  - Time since motion detected
 *    - Unit: Seconds
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1 second)
 *    - Range: 0 to 65533
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_time_since_motion_sensed;

/** Time since presence detected
 *
 *  Channels:
 *  - Time since presence detected
 *    - Unit: Seconds
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1 second)
 *    - Range: 0 to 65533
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_time_since_presence_detected;

/** @} */

/** @defgroup bt_mesh_sensor_types_ambient_temperature Ambient temperature
 *  sensor types
 *  @{
 */

/** Average ambient temperature in a period of day
 *
 *  The series X-axis is measured in Hours.
 *
 *  Channels:
 *  - Temperature
 *    - Unit: Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 0.5 C)
 *    - Range: -64.0 to 63.5
 *  - Start time
 *    - Unit: Hours
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.1 hours)
 *    - Range: 0 to 24.0
 *  - End time
 *    - Unit: Hours
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.1 hours)
 *    - Range: 0 to 24.0
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_avg_amb_temp_in_day;

/** Indoor ambient temperature statistical values
 *
 *  Channels:
 *  - Average
 *    - Unit: Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 0.5 C)
 *    - Range: -64.0 to 63.5
 *  - Standard deviation value
 *    - Unit: Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 0.5 C)
 *    - Range: -64.0 to 63.5
 *  - Minimum value
 *    - Unit: Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 0.5 C)
 *    - Range: -64.0 to 63.5
 *  - Maximum value
 *    - Unit: Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 0.5 C)
 *    - Range: -64.0 to 63.5
 *  - Sensing duration
 *    - Unit: Seconds
 *    - Encoding: 8 bit unsigned exponential time (pow(1.1, N - 64) seconds)
 *    - Range: 0 to 73216705
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_indoor_amb_temp_stat_values;

/** Outdoor statistical values
 *
 *  Channels:
 *  - Average
 *    - Unit: Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 0.5 C)
 *    - Range: -64.0 to 63.5
 *  - Standard deviation value
 *    - Unit: Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 0.5 C)
 *    - Range: -64.0 to 63.5
 *  - Minimum value
 *    - Unit: Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 0.5 C)
 *    - Range: -64.0 to 63.5
 *  - Maximum value
 *    - Unit: Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 0.5 C)
 *    - Range: -64.0 to 63.5
 *  - Sensing duration
 *    - Unit: Seconds
 *    - Encoding: 8 bit unsigned exponential time (pow(1.1, N - 64) seconds)
 *    - Range: 0 to 73216705
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_outdoor_stat_values;

/** Present ambient temperature
 *
 *  Channels:
 *  - Present ambient temperature
 *    - Unit: Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 0.5 C)
 *    - Range: -64.0 to 63.5
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_amb_temp;

/** Present indoor ambient temperature
 *
 *  Channels:
 *  - Present indoor ambient temperature
 *    - Unit: Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 0.5 C)
 *    - Range: -64.0 to 63.5
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_indoor_amb_temp;

/** Present outdoor ambient temperature
 *
 *  Channels:
 *  - Present outdoor ambient temperature
 *    - Unit: Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 0.5 C)
 *    - Range: -64.0 to 63.5
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_outdoor_amb_temp;

/** Desired ambient temperature
 *
 *  Channels:
 *  - Desired ambient temperature
 *    - Unit: Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 0.5 C)
 *    - Range: -64.0 to 63.5
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_desired_amb_temp;

/** Lumen maintenance factor
 *
 *  Channels:
 *  - Lumen Maintenance Factor
 *    - Unit: Percent
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.5 %)
 *    - Range: 0 to 100.0
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_lumen_maintenance_factor;

/** Luminous efficacy
 *
 *  Channels:
 *  - Luminous Efficacy
 *    - Unit: Lumen per Watt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.1 lm/W)
 *    - Range: 0 to 6553.3
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_luminous_efficacy;

/** Luminous energy since turn on
 *
 *  Channels:
 *  - Luminous Energy Since Turn On
 *    - Unit: Kilo Lumen hours
 *    - Encoding: 24 bit unsigned scalar (Resolution: 1 klmh)
 *    - Range: 0 to 16777213
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_luminous_energy_since_turn_on;

/** Luminous exposure
 *
 *  Channels:
 *  - Luminous Exposure
 *    - Unit: Kilo Lux hours
 *    - Encoding: 24 bit unsigned scalar (Resolution: 1 klxh)
 *    - Range: 0 to 16777213
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_luminous_exposure;

/** Luminous flux range
 *
 *  Channels:
 *  - Minimum Luminous Flux
 *    - Unit: Lumen
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1 lm)
 *    - Range: 0 to 65533
 *  - Maximum Luminous Flux
 *    - Unit: Lumen
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1 lm)
 *    - Range: 0 to 65533
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_luminous_flux_range;

/** @} */

/** @defgroup bt_mesh_sensor_types_environmental Environmental sensor types
 *  @{
 */

/** Present Ambient Relative Humidity
 *
 *  Channels:
 *  - Present Ambient Relative Humidity
 *    - Unit: Percent
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 %)
 *    - Range: 0 to 100.0
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_amb_rel_humidity;

/** Present Ambient Carbon Dioxide Concentration
 *
 *  Channels:
 *  - Present Ambient Carbon Dioxide Concentration
 *    - Unit: Parts per million
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1 ppm)
 *    - Range: 0 to 65533
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_present_amb_co2_concentration;

/** Present Ambient Volatile Organic Compounds Concentration
 *
 *  Channels:
 *  - Present Ambient Volatile Organic Compounds Concentration
 *    - Unit: Parts per billion
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1 ppb)
 *    - Range: 0 to 65533
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_present_amb_voc_concentration;

/** Present Ambient Noise
 *
 *  Channels:
 *  - Present Ambient Noise
 *    - Unit: Decibel
 *    - Encoding: 8 bit unsigned scalar (Resolution: 1 dB)
 *    - Range: 0 to 253
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_amb_noise;

/** Apparent Wind Direction
 *
 *  The apparent wind direction is the relative clockwise direction of the wind
 *  in relation to the observer.
 *
 *  For example, the apparent wind direction aboard a boat is given in degrees
 *  relative to the heading of the boat.
 *
 *  Channels:
 *  - Apparent Wind Direction
 *    - Unit: Degrees
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 degrees)
 *    - Range: 0 to 359.99
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_apparent_wind_direction;

/** Apparent Wind Speed
 *
 *  Apparent Wind Speed is the relative speed of the wind in relation to the
 *  observer.
 *
 *  Channels:
 *  - Apparent Wind Speed
 *    - Unit: Metres per second
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 m/s)
 *    - Range: 0 to 655.35
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_apparent_wind_speed;

/** Dew Point
 *
 *  Channels:
 *  - Dew Point
 *    - Unit: Degrees Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 1 C)
 *    - Range: -128 to 127
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_dew_point;

/** Gust Factor
 *
 *  Channels:
 *  - Gust Factor
 *    - Unit: _unitless_
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.1)
 *    - Range: 0 to 25.5
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_gust_factor;

/** Heat Index
 *
 *  Channels:
 *  - Heat Index
 *    - Unit: Degrees Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 1 C)
 *    - Range: -128 to 127
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_heat_index;

/** Present Indoor Relative Humidity
 *
 *  Channels:
 *  - Humidity
 *    - Unit: Percentage
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 %)
 *    - Range: 0 to 100
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_present_indoor_relative_humidity;

/** Present Outdoor Relative Humidity
 *
 *  Channels:
 *  - Humidity
 *    - Unit: Percentage
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 %)
 *    - Range: 0 to 100
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_present_outdoor_relative_humidity;

/** Magnetic Declination
 *
 *  The magnetic declination is the angle on the horizontal plane between the
 *  direction of True North (geographic) and the direction of Magnetic North,
 *  measured clockwise from True North to Magnetic North.
 *
 *  Channels:
 *  - Magnetic Declination
 *    - Unit: Degrees
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 degrees)
 *    - Range: 0 to 359.99
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_magnetic_declination;

/** Magnetic Flux Density - 2D
 *
 * @note The Magnetic Flux Density unit is measured in tesla in the
 *       specification. In the API, it is measured in microtesla to accommodate
 *       the 6 digit format of the sensor values.
 *
 *  Channels:
 *  - X-axis
 *    - Unit: Microtesla
 *    - Encoding: 16 bit signed scalar (Resolution: 0.1 uT)
 *    - Range: -6553.6 to 6553.5
 *  - Y-axis
 *    - Unit: Microtesla
 *    - Encoding: 16 bit signed scalar (Resolution: 0.1 uT)
 *    - Range: -6553.6 to 6553.5
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_magnetic_flux_density_2d;

/** Magnetic Flux Density - 3D
 *
 * @note The Magnetic Flux Density unit is measured in tesla in the
 *       specification. In the API, it is measured in microtesla to accommodate
 *       the 6 digit format of the sensor values.
 *
 *  Channels:
 *  - X-axis
 *    - Unit: Microtesla
 *    - Encoding: 16 bit signed scalar (Resolution: 0.1 uT)
 *    - Range: -6553.6 to 6553.5
 *  - Y-axis
 *    - Unit: Microtesla
 *    - Encoding: 16 bit signed scalar (Resolution: 0.1 uT)
 *    - Range: -6553.6 to 6553.5
 *  - Z-axis
 *    - Unit: Microtesla
 *    - Encoding: 16 bit signed scalar (Resolution: 0.1 uT)
 *    - Range: -6553.6 to 6553.5
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_magnetic_flux_density_3d;

/** Pollen Concentration
 *
 *  Channels:
 *  - Pollen Concentration
 *    - Unit: Concentration
 *    - Encoding: 24 bit unsigned scalar (Resolution: 1 per m3)
 *    - Range: 0 to 16777215
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_pollen_concentration;

/** Air Pressure
 *
 *  Channels:
 *  - Pressure
 *    - Unit: Pascal
 *    - Encoding: 32 bit unsigned scalar (Resolution: 0.1 Pa)
 *    - Range: 0 to 429496729.5
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_air_pressure;

/** Pressure
 *
 *  A sensor reporting pressure other than air pressure.
 *
 *  Channels:
 *  - Pressure
 *    - Unit: Pascal
 *    - Encoding: 32 bit unsigned scalar (Resolution: 0.1 Pa)
 *    - Range: 0 to 429496729.5
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_pressure;

/** Rainfall
 *
 *  Channels:
 *  - Rainfall
 *    - Unit: Meter
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.001 m)
 *    - Range: 0 to 655.35
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_rainfall;

/** True Wind Direction
 *
 *  Wind direction is reported by the direction from which it originates and is
 *  an angle measured clockwise relative to Geographic North.
 *
 *  Channels:
 *  - True Wind Direction
 *    - Unit: Degrees
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 degrees)
 *    - Range: 0 to 359.99
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_true_wind_direction;

/** True Wind Speed
 *
 *  Channels:
 *  - True Wind Speed
 *    - Unit: Metres per second
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 m/s)
 *    - Range: 0 to 655.35
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_true_wind_speed;

/** UV Index
 *
 *  Channels:
 *  - UV Index
 *    - Unit: _unitless_
 *    - Encoding: 8 bit unsigned scalar (Resolution: 1)
 *    - Range: 0 to 255
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_uv_index;

/** Wind Chill
 *
 *  Channels:
 *  - Wind Chill
 *    - Unit: Degrees Celsius
 *    - Encoding: 8 bit signed scalar (Resolution: 1 C)
 *    - Range: -128 to 127
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_wind_chill;

/** @} */

/** @defgroup bt_mesh_sensor_types_device_operating_temperature Device operating
 *  temperature sensor types
 *  @{
 */

/** Device operating temperature range specification
 *
 *  Channels:
 *  - Minimum Temperature
 *    - Unit: Celsius
 *    - Encoding: 16 bit signed scalar (Resolution: 0.01 C)
 *    - Range: -327.68 to 327.67
 *  - Maximum Temperature
 *    - Unit: Celsius
 *    - Encoding: 16 bit signed scalar (Resolution: 0.01 C)
 *    - Range: -327.68 to 327.67
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_dev_op_temp_range_spec;

/** Device operating temperature statistical values
 *
 *  Channels:
 *  - Average Temperature
 *    - Unit: Celsius
 *    - Encoding: 16 bit signed scalar (Resolution: 0.01 C)
 *    - Range: -327.68 to 327.67
 *  - Standard Deviation Temperature
 *    - Unit: Celsius
 *    - Encoding: 16 bit signed scalar (Resolution: 0.01 C)
 *    - Range: -327.68 to 327.67
 *  - Minimum Temperature
 *    - Unit: Celsius
 *    - Encoding: 16 bit signed scalar (Resolution: 0.01 C)
 *    - Range: -327.68 to 327.67
 *  - Maximum Temperature
 *    - Unit: Celsius
 *    - Encoding: 16 bit signed scalar (Resolution: 0.01 C)
 *    - Range: -327.68 to 327.67
 *  - Sensing duration
 *    - Unit: Seconds
 *    - Encoding: 8 bit unsigned exponential time (pow(1.1, N - 64) seconds)
 *    - Range: 0 to 73216705
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_dev_op_temp_stat_values;

/** Present device operating temperature
 *
 *  Channels:
 *  - Temperature
 *    - Unit: Celsius
 *    - Encoding: 16 bit signed scalar (Resolution: 0.01 C)
 *    - Range: -327.68 to 327.67
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_dev_op_temp;

/** Relative runtime in a device operating temperature range
 *
 *  The series X-axis is measured in Celsius.
 *
 *  Channels:
 *  - Relative Value
 *    - Unit: Percent
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.5 %)
 *    - Range: 0 to 100.0
 *  - Minimum Temperature Value
 *    - Unit: Celsius
 *    - Encoding: 16 bit signed scalar (Resolution: 0.01 C)
 *    - Range: -327.68 to 327.67
 *  - Maximum Temperature Value
 *    - Unit: Celsius
 *    - Encoding: 16 bit signed scalar (Resolution: 0.01 C)
 *    - Range: -327.68 to 327.67
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_rel_runtime_in_a_dev_op_temp_range;

/** @} */

/** @defgroup bt_mesh_sensor_types_electrical_input Electrical input sensor
 *  types
 *  @{
 */

/** Average input current
 *
 *  Channels:
 *  - Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 *  - Sensing duration
 *    - Unit: Seconds
 *    - Encoding: 8 bit unsigned exponential time (pow(1.1, N - 64) seconds)
 *    - Range: 0 to 73216705
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_avg_input_current;

/** Average input voltage
 *
 *  Channels:
 *  - Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 *  - Sensing duration
 *    - Unit: Seconds
 *    - Encoding: 8 bit unsigned exponential time (pow(1.1, N - 64) seconds)
 *    - Range: 0 to 73216705
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_avg_input_voltage;

/** Input current range specification
 *
 *  Channels:
 *  - Minimum Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 *  - Maximum Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 *  - Typical Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_input_current_range_spec;

/** Input current statistics
 *
 *  Channels:
 *  - Average Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 *  - Standard Deviation Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 *  - Minimum Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 *  - Maximum Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 *  - Sensing Duration
 *    - Unit: Seconds
 *    - Encoding: 8 bit unsigned exponential time (pow(1.1, N - 64) seconds)
 *    - Range: 0 to 73216705
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_input_current_stat;

/** Input voltage range specification
 *
 *  Channels:
 *  - Minimum Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 *  - Maximum Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 *  - Typical Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_input_voltage_range_spec;

/** Input voltage statistics
 *
 *  Channels:
 *  - Average Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 *  - Standard Deviation Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 *  - Minimum Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 *  - Maximum Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 *  - Sensing Duration
 *    - Unit: Seconds
 *    - Encoding: 8 bit unsigned exponential time (pow(1.1, N - 64) seconds)
 *    - Range: 0 to 73216705
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_input_voltage_stat;

/** Present input current
 *
 *  Channels:
 *  - Present Input Current
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_input_current;

/** Present input ripple voltage
 *
 *  Channels:
 *  - Present Input Ripple Voltage
 *    - Unit: Percent
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.5 %)
 *    - Range: 0 to 100.0
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_present_input_ripple_voltage;

/** Present input voltage
 *
 *  Channels:
 *  - Present Input Voltage
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_input_voltage;

/** Precise Present Ambient Temperature
 *
 *  Channels:
 *  - Precise Present Ambient Temperature
 *    - Unit: Celsius
 *    - Encoding: 16 bit signed scalar (Resolution: 0.01 C)
 *    - Range: -327.68 to 327.67
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_precise_present_amb_temp;

/** Relative runtime in an input current range
 *
 *  The series X-axis is measured in Ampere.
 *
 *  Channels:
 *  - Relative Runtime Value
 *    - Unit: Percent
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.5 %)
 *    - Range: 0 to 100.0
 *  - Minimum Current
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 *  - Maximum Current
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_rel_runtime_in_an_input_current_range;

/** Relative runtime in an input voltage range
 *
 *  The series X-axis is measured in Volt.
 *
 *  Channels:
 *  - Relative Runtime Value
 *    - Unit: Percent
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.5 %)
 *    - Range: 0 to 100.0
 *  - Minimum Voltage
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 *  - Maximum Voltage
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_rel_runtime_in_an_input_voltage_range;

/** @} */

/** @defgroup bt_mesh_sensor_types_energy_management Energy management sensor
 *  types
 *  @{
 */

/** Device power range specification
 *
 *  Channels:
 *  - Minimum Power Value
 *    - Unit: Watt
 *    - Encoding: 24 bit unsigned scalar (Resolution: 0.1 W)
 *    - Range: 0 to 1677721.4
 *  - Typical Power Value
 *    - Unit: Watt
 *    - Encoding: 24 bit unsigned scalar (Resolution: 0.1 W)
 *    - Range: 0 to 1677721.4
 *  - Maximum Power Value
 *    - Unit: Watt
 *    - Encoding: 24 bit unsigned scalar (Resolution: 0.1 W)
 *    - Range: 0 to 1677721.4
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_dev_power_range_spec;

/** Present device input power
 *
 *  Channels:
 *  - Present Device Input Power
 *    - Unit: Watt
 *    - Encoding: 24 bit unsigned scalar (Resolution: 0.1 W)
 *    - Range: 0 to 1677721.4
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_dev_input_power;

/** Present device operating efficiency
 *
 *  Channels:
 *  - Present Device Operating Efficiency
 *    - Unit: Percent
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.5 %)
 *    - Range: 0 to 100.0
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_present_dev_op_efficiency;

/** Total device energy use
 *
 *  Channels:
 *  - Total Device Energy Use
 *    - Unit: Kwh
 *    - Encoding: 24 bit unsigned scalar (Resolution: 1 kWh)
 *    - Range: 0 to 16777214
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_tot_dev_energy_use;

/** Device energy use since turn on
 *
 *  Channels:
 *  - Device Energy Use Since Turn On
 *    - Unit: Kwh
 *    - Encoding: 24 bit unsigned scalar (Resolution: 1 kWh)
 *    - Range: 0 to 16777214
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_dev_energy_use_since_turn_on;

/** Precise Total Device Energy Use
 *
 *  Channels:
 *  - Total Device Energy Use
 *    - Unit: kWh
 *    - Encoding: 32 bit unsigned scalar (Resolution: 0.001 kWh)
 *    - Range: 0 to 4294967.293
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_precise_tot_dev_energy_use;

/** Power Factor
 *
 *  This sensor type should be used as a sensor setting.
 *
 *  Channels:
 *  - Cosine Of the Angle
 *    - Unit: _unitless_
 *    - Encoding: 8 bit signed scalar (Resolution: 1)
 *    - Range: -100 to 100
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_power_factor;

/** Relative device energy use in a period of day
 *
 *  The series X-axis is measured in Hours.
 *
 *  Channels:
 *  - Energy Value
 *    - Unit: kWh
 *    - Encoding: 24 bit unsigned scalar (Resolution: 1 kWh)
 *    - Range: 0 to 16777214
 *  - Start Time
 *    - Unit: Hours
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.1 hours)
 *    - Range: 0 to 24.0
 *  - End Time
 *    - Unit: Hours
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.1 hours)
 *    - Range: 0 to 24.0
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_rel_dev_energy_use_in_a_period_of_day;

/** Apparent energy
 *
 *  Channels:
 *  - Apparent Energy
 *    - Unit: kVAh
 *    - Encoding: 32 bit unsigned scalar (Resolution: 0.001 kVAh)
 *    - Range: 0 to 4294967.293
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_apparent_energy;

/** Apparent power
 *
 *  Channels:
 *  - Apparent Power
 *    - Unit: VA
 *    - Encoding: 32 bit unsigned scalar (Resolution: 0.1 VA)
 *    - Range: 0 to 1677721.3
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_apparent_power;

/** Active energy loadside
 *
 *  Channels:
 *  - Energy
 *    - Unit: kWh
 *    - Encoding: 32 bit unsigned scalar (Resolution: 0.001 kWh)
 *    - Range: 0 to 4294967.293
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_active_energy_loadside;

/** Active power loadside
 *  Channels:
 *  - Power
 *    - Unit: Watt
 *    - Encoding: 24 bit unsigned scalar (Resolution: 0.1 W)
 *    - Range: 0 to 1677721.4
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_active_power_loadside;

/** @} */

/** @defgroup bt_mesh_sensor_types_photometry Photometry sensor types
 *  @{
 */

/** Present ambient light level
 *
 *  Channels:
 *  - Present Ambient Light Level
 *    - Unit: Lux
 *    - Encoding: 24 bit unsigned scalar (Resolution: 0.01 lx)
 *    - Range: 0 to 167772.13
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_amb_light_level;

/** Initial CIE-1931 chromaticity coordinates
 *
 *  Channels:
 *  - Chromaticity x-coordinate
 *    - Unit: _unitless_
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/65536)
 *    - Range: 0 to 0.99998
 *  - Chromaticity y-coordinate
 *    - Unit: _unitless_
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/65536)
 *    - Range: 0 to 0.99998
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_initial_cie_1931_chromaticity_coords;

/** Present CIE-1931 chromaticity coordinates
 *
 *  Channels:
 *  - Chromaticity x-coordinate
 *    - Unit: _unitless_
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/65536)
 *    - Range: 0 to 0.99998
 *  - Chromaticity y-coordinate
 *    - Unit: _unitless_
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/65536)
 *    - Range: 0 to 0.99998
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_present_cie_1931_chromaticity_coords;

/** Initial correlated color temperature
 *
 *  Channels:
 *  - Initial Correlated Color Temperature
 *    - Unit: Kelvin
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1 K)
 *    - Range: 0 to 65533
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_initial_correlated_col_temp;

/** Present correlated color temperature
 *
 *  Channels:
 *  - Present Correlated Color Temperature
 *    - Unit: Kelvin
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1 K)
 *    - Range: 0 to 65533
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_present_correlated_col_temp;

/** Present illuminance
 *
 *  Channels:
 *  - Present Illuminance
 *    - Unit: Lux
 *    - Encoding: 24 bit unsigned scalar (Resolution: 0.01 lx)
 *    - Range: 0 to 167772.13
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_illuminance;

/** Initial luminous flux
 *
 *  Channels:
 *  - Initial Luminous Flux
 *    - Unit: Lumen
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1 lm)
 *    - Range: 0 to 65533
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_initial_luminous_flux;

/** Present luminous flux
 *
 *  Channels:
 *  - Present Luminous Flux
 *    - Unit: Lumen
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1 lm)
 *    - Range: 0 to 65533
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_luminous_flux;

/** Initial planckian distance
 *
 *  Channels:
 *  - Initial Planckian Distance
 *    - Unit: _unitless_
 *    - Encoding: 16 bit signed scalar (Resolution: 1/100000)
 *    - Range: -0.05 to 0.05
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_initial_planckian_distance;

/** Present planckian distance
 *
 *  Channels:
 *  - Present Planckian Distance
 *    - Unit: _unitless_
 *    - Encoding: 16 bit signed scalar (Resolution: 1/100000)
 *    - Range: -0.05 to 0.05
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_present_planckian_distance;

/** Relative exposure time in an illuminance range
 *
 *  The series X-axis is measured in Lux.
 *
 *  Channels:
 *  - Relative value
 *    - Unit: Percent
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.5 %)
 *    - Range: 0 to 100.0
 *  - Minimum Illuminance
 *    - Unit: Lux
 *    - Encoding: 24 bit unsigned scalar (Resolution: 0.01 lx)
 *    - Range: 0 to 167772.13
 *  - Maximum Illuminance
 *    - Unit: Lux
 *    - Encoding: 24 bit unsigned scalar (Resolution: 0.01 lx)
 *    - Range: 0 to 167772.13
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_rel_exposure_time_in_an_illuminance_range;

/** Total light exposure time
 *
 *  Channels:
 *  - Total Light Exposure Time
 *    - Unit: Hours
 *    - Encoding: 24 bit unsigned scalar (Resolution: 0.1 hours)
 *    - Range: 0 to 1677721.3
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_tot_light_exposure_time;

/** @} */

/** @defgroup bt_mesh_sensor_types_power_supply_output Power supply output
 *  sensor types
 *  @{
 */

/** Average output current
 *
 *  Channels:
 *  - Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 *  - Sensing duration
 *    - Unit: Seconds
 *    - Encoding: 8 bit unsigned exponential time (pow(1.1, N - 64) seconds)
 *    - Range: 0 to 73216705
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_avg_output_current;

/** Average output voltage
 *
 *  Channels:
 *  - Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 *  - Sensing duration
 *    - Unit: Seconds
 *    - Encoding: 8 bit unsigned exponential time (pow(1.1, N - 64) seconds)
 *    - Range: 0 to 73216705
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_avg_output_voltage;

/** Output current range
 *
 *  This sensor type should be used as a sensor setting.
 *
 *  Channels:
 *  - Minimum Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 *  - Maximum Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_output_current_range;

/** Output current statistics
 *
 *  Channels:
 *  - Average Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 *  - Standard Deviation Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 *  - Minimum Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 *  - Maximum Electric Current Value
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 *  - Sensing Duration
 *    - Unit: Seconds
 *    - Encoding: 8 bit unsigned exponential time (pow(1.1, N - 64) seconds)
 *    - Range: 0 to 73216705
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_output_current_stat;

/** Output ripple voltage specification
 *
 *  Channels:
 *  - Output Ripple Voltage
 *    - Unit: Percent
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.5 %)
 *    - Range: 0 to 100.0
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_output_ripple_voltage_spec;

/** Output voltage range
 *
 *  This sensor type should be used as a sensor setting.
 *
 *  Channels:
 *  - Minimum Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 *  - Maximum Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_output_voltage_range;

/** Output voltage statistics
 *
 *  Channels:
 *  - Average Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 *  - Standard Deviation Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 *  - Minimum Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 *  - Maximum Voltage Value
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 *  - Sensing Duration
 *    - Unit: Seconds
 *    - Encoding: 8 bit unsigned exponential time (pow(1.1, N - 64) seconds)
 *    - Range: 0 to 73216705
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_output_voltage_stat;

/** Present output current
 *
 *  Channels:
 *  - Present Output Current
 *    - Unit: Ampere
 *    - Encoding: 16 bit unsigned scalar (Resolution: 0.01 A)
 *    - Range: 0 to 655.33
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_output_current;

/** Present output voltage
 *
 *  Channels:
 *  - Present Output Voltage
 *    - Unit: Volt
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1/64 V)
 *    - Range: 0 to 1023.95
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_output_voltage;

/** Present relative output ripple voltage
 *
 *  Channels:
 *  - Output Ripple Voltage
 *    - Unit: Percent
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.5 %)
 *    - Range: 0 to 100.0
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_present_rel_output_ripple_voltage;

/** @} */

/** @defgroup bt_mesh_sensor_types_warranty_and_service Warranty and service
 *  sensor types
 *  @{
 */

/** Sensor Gain
 *
 *  This sensor type should be used as a sensor setting.
 *
 *  Channels:
 *  - Sensor Gain
 *    - Unit: _unitless_
 *    - Encoding: 32 bit float
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_gain;

/** Relative device runtime in a generic level range
 *
 *  The series X-axis is unitless.
 *
 *  Channels:
 *  - Relative Value
 *    - Unit: Percent
 *    - Encoding: 8 bit unsigned scalar (Resolution: 0.5 %)
 *    - Range: 0 to 100.0
 *  - Minimum Generic Level
 *    - Unit: _unitless_
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1)
 *    - Range: 0 to 65535
 *  - Maximum Generic Level
 *    - Unit: _unitless_
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1)
 *    - Range: 0 to 65535
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_rel_dev_runtime_in_a_generic_level_range;

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SENSOR_TYPES_H__ */

/** @} */
