/*
 * Copyright 2020-2023 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sensors_config.h"
#include "peripherals.h"

#define KPA_TO_PA_FACTOR 1e3
#define GAUSS_TO_TESLA_FACTOR 1e-4

static struct anjay_zephyr_ipso_sensor_context illuminance_sensor_def[] = {
#if ILLUMINANCE_AVAILABLE
	{ .name = "Illuminance",
	  .unit = "lx",
	  .device = DEVICE_DT_GET(ILLUMINANCE_NODE),
	  .channel = SENSOR_CHAN_LIGHT,
	  .min_range_value = NAN,
	  .max_range_value = NAN }
#endif // ILLUMINANCE_AVAILABLE
};

static struct anjay_zephyr_ipso_sensor_context temperature_sensor_def[] = {
#if TEMPERATURE_AVAILABLE
	{ .name = "Temperature",
	  .unit = "Cel",
	  .device = DEVICE_DT_GET(TEMPERATURE_NODE),
	  .channel = SENSOR_CHAN_AMBIENT_TEMP,
	  .min_range_value = NAN,
	  .max_range_value = NAN }
#endif // TEMPERATURE_AVAILABLE
};

static struct anjay_zephyr_ipso_sensor_context humidity_sensor_def[] = {
#if HUMIDITY_AVAILABLE
	{ .name = "Humidity",
	  .unit = "%RH",
	  .device = DEVICE_DT_GET(HUMIDITY_NODE),
	  .channel = SENSOR_CHAN_HUMIDITY,
	  .min_range_value = NAN,
	  .max_range_value = NAN }
#endif // HUMIDITY_AVAILABLE
};

static struct anjay_zephyr_ipso_sensor_context acceleration_sensor_def[] = {
#if ACCELEROMETER_AVAILABLE
	{ .name = "Accelerometer",
	  .unit = "m/s2",
	  .device = DEVICE_DT_GET(ACCELEROMETER_NODE),
	  .channel = SENSOR_CHAN_ACCEL_XYZ,
	  .use_y_value = true,
	  .use_z_value = true,
	  .min_range_value = NAN,
	  .max_range_value = NAN }
#endif // ACCELEROMETER_AVAILABLE
};

static struct anjay_zephyr_ipso_sensor_context magnetic_field_sensor_def[] = {
#if MAGNETOMETER_AVAILABLE
	{ .name = "Magnetometer",
	  .unit = "T",
	  .device = DEVICE_DT_GET(MAGNETOMETER_NODE),
	  .channel = SENSOR_CHAN_MAGN_XYZ,
	  .scale_factor = GAUSS_TO_TESLA_FACTOR,
	  .use_y_value = true,
	  .use_z_value = true,
	  .min_range_value = NAN,
	  .max_range_value = NAN }
#endif // MAGNETOMETER_AVAILABLE
};

static struct anjay_zephyr_ipso_sensor_context pressure_sensor_def[] = {
#if BAROMETER_AVAILABLE
	{ .name = "Barometer",
	  .unit = "Pa",
	  .device = DEVICE_DT_GET(BAROMETER_NODE),
	  .channel = SENSOR_CHAN_PRESS,
	  .scale_factor = KPA_TO_PA_FACTOR,
	  .min_range_value = NAN,
	  .max_range_value = NAN }
#endif // BAROMETER_AVAILABLE
};

static struct anjay_zephyr_ipso_sensor_context distance_sensor_def[] = {
#if DISTANCE_AVAILABLE
	{ .name = "Distance",
	  .unit = "m",
	  .device = DEVICE_DT_GET(DISTANCE_NODE),
	  .channel = SENSOR_CHAN_DISTANCE,
	  .min_range_value = NAN,
	  .max_range_value = NAN }
#endif // DISTANCE_AVAILABLE
};

static struct anjay_zephyr_ipso_sensor_context angular_rate_sensor_def[] = {
#if GYROMETER_AVAILABLE
	{ .name = "Gyrometer",
	  .unit = "deg/s",
	  .device = DEVICE_DT_GET(GYROMETER_NODE),
	  .channel = SENSOR_CHAN_GYRO_XYZ,
	  .use_y_value = true,
	  .use_z_value = true,
	  .min_range_value = NAN,
	  .max_range_value = NAN }
#endif // GYROMETER_AVAILABLE
};

static struct anjay_zephyr_ipso_sensor_oid_set sensors_basic_oid_def[] = {
	{ .user_sensors = illuminance_sensor_def,
	  .oid = 3301,
	  .user_sensors_array_length = AVS_ARRAY_SIZE(illuminance_sensor_def) },
	{ .user_sensors = temperature_sensor_def,
	  .oid = 3303,
	  .user_sensors_array_length = AVS_ARRAY_SIZE(temperature_sensor_def) },
	{ .user_sensors = humidity_sensor_def,
	  .oid = 3304,
	  .user_sensors_array_length = AVS_ARRAY_SIZE(humidity_sensor_def) },
	{ .user_sensors = pressure_sensor_def,
	  .oid = 3315,
	  .user_sensors_array_length = AVS_ARRAY_SIZE(pressure_sensor_def) },
	{ .user_sensors = distance_sensor_def,
	  .oid = 3330,
	  .user_sensors_array_length = AVS_ARRAY_SIZE(distance_sensor_def) }
};

static struct anjay_zephyr_ipso_sensor_oid_set sensors_3d_oid_def[] = {
	{ .user_sensors = acceleration_sensor_def,
	  .oid = 3313,
	  .user_sensors_array_length = AVS_ARRAY_SIZE(acceleration_sensor_def) },
	{ .user_sensors = magnetic_field_sensor_def,
	  .oid = 3314,
	  .user_sensors_array_length = AVS_ARRAY_SIZE(magnetic_field_sensor_def) },
	{ .user_sensors = angular_rate_sensor_def,
	  .oid = 3334,
	  .user_sensors_array_length = AVS_ARRAY_SIZE(angular_rate_sensor_def) }
};

void sensors_install(anjay_t *anjay)
{
	anjay_zephyr_ipso_basic_sensors_install(anjay, sensors_basic_oid_def,
						AVS_ARRAY_SIZE(sensors_basic_oid_def));
	anjay_zephyr_ipso_three_axis_sensors_install(anjay, sensors_3d_oid_def,
						     AVS_ARRAY_SIZE(sensors_3d_oid_def));
}
