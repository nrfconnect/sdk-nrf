/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 *  @defgroup bt_mesh_sensor_types Sensor types
 *  @{
 *  @brief All available sensor types from the Mesh Device Properties
 *        Specification.
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
 *  If @ref CONFIG_BT_MESH_SENSOR_FORCE_ALL is disabled, only referenced sensor
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
 *  This sensor type supports series access.
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
 *    - Unit: Lumen hours
 *    - Encoding: 24 bit unsigned scalar (Resolution: 1000 lmh)
 *    - Range: 0 to 16777213000
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_luminous_energy_since_turn_on;

/** Luminous exposure
 *
 *  Channels:
 *  - Luminous Exposure
 *    - Unit: Lux hours
 *    - Encoding: 24 bit unsigned scalar (Resolution: 1000 lxh)
 *    - Range: 0 to 16777213000
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
 *  This sensor type supports series access.
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
 *  This sensor type supports series access.
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
 *  This sensor type supports series access.
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

/** Present device input power
 *
 *  Channels:
 *  - Present Device Input Power
 *    - Unit: Watt
 *    - Encoding: 24 bit unsigned scalar (Resolution: 0.1 W)
 *    - Range: 0 to 1677721.3
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
 *    - Range: 0 to 16777213
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_tot_dev_energy_use;

/** Device energy use since turn on
 *
 *  Channels:
 *  - Device Energy Use Since Turn On
 *    - Unit: Kwh
 *    - Encoding: 24 bit unsigned scalar (Resolution: 1 kWh)
 *    - Range: 0 to 16777213
 */
extern const struct bt_mesh_sensor_type
	bt_mesh_sensor_dev_energy_use_since_turn_on;

/** Precise Total Device Energy Use
 *
 *  Channels:
 *  - Total Device Energy Use
 *    - Unit: Kwh
 *    - Encoding: 32 bit unsigned scalar (Resolution: 0.01 kWh)
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
 *    - Range: -256 to 100
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_power_factor;

/** Relative device energy use in a period of day
 *
 *  This sensor type supports series access.
 *  The series X-axis is measured in Hours.
 *
 *  Channels:
 *  - Energy Value
 *    - Unit: Kwh
 *    - Encoding: 24 bit unsigned scalar (Resolution: 1 kWh)
 *    - Range: 0 to 16777213
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

/** Relative device runtime in a generic level range
 *
 *  This sensor type supports series access.
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

/** Present luminous flux
 *
 *  Channels:
 *  - Present Luminous Flux
 *    - Unit: Lumen
 *    - Encoding: 16 bit unsigned scalar (Resolution: 1 lm)
 *    - Range: 0 to 65533
 */
extern const struct bt_mesh_sensor_type bt_mesh_sensor_present_luminous_flux;

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
 *  This sensor type supports series access.
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

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SENSOR_TYPES_H__ */
