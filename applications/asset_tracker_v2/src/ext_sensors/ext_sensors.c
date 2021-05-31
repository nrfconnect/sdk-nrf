/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <drivers/sensor.h>
#include "ext_sensors.h"
#include <stdlib.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ext_sensors, CONFIG_EXTERNAL_SENSORS_LOG_LEVEL);

/* Convert to s/m2 depending on the maximum measured range used for adxl362. */
#if defined(CONFIG_ADXL362_ACCEL_RANGE_2G)
#define ADXL362_RANGE_MAX_M_S2 19.6133
#elif defined(CONFIG_ADXL362_ACCEL_RANGE_4G)
#define ADXL362_RANGE_MAX_M_S2 39.2266
#elif defined(CONFIG_ADXL362_ACCEL_RANGE_8G)
#define ADXL362_RANGE_MAX_M_S2 78.4532
#endif

#define ADXL362_THRESHOLD_RESOLUTION_DECIMAL_MAX 2048

/* Local accelerometer threshold value. Used to filter out unwanted values in
 * the callback from the accelerometer.
 */
double threshold = ADXL362_RANGE_MAX_M_S2;

struct env_sensor {
	enum sensor_channel channel;
	const struct device *dev;
	struct k_spinlock lock;
};

static struct env_sensor temp_sensor = {
	.channel = SENSOR_CHAN_AMBIENT_TEMP,
	.dev = DEVICE_DT_GET(DT_ALIAS(temp_sensor)),

};

static struct env_sensor humid_sensor = {
	.channel = SENSOR_CHAN_HUMIDITY,
	.dev = DEVICE_DT_GET(DT_ALIAS(humidity_sensor)),
};

static struct env_sensor accel_sensor = {
	.channel = SENSOR_CHAN_ACCEL_XYZ,
	.dev = DEVICE_DT_GET(DT_ALIAS(accelerometer)),
};

static ext_sensor_handler_t evt_handler;
static bool initial_trigger;

static void accelerometer_trigger_handler(const struct device *dev,
					  struct sensor_trigger *trig)
{
	int err = 0;
	struct sensor_value data[ACCELEROMETER_CHANNELS];
	struct ext_sensor_evt evt = {0};

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:

		/* Ignore the initial trigger after initialization of the
		 * accelerometer which always carries jibberish xyz values.
		 */
		if (!initial_trigger) {
			initial_trigger = true;
			break;
		}

		if (sensor_sample_fetch(dev) < 0) {
			LOG_ERR("Sample fetch error");
			return;
		}

		err = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &data[0]);
		err += sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &data[1]);
		err += sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &data[2]);

		if (err) {
			LOG_ERR("sensor_channel_get, error: %d", err);
			return;
		}

		evt.value_array[0] = sensor_value_to_double(&data[0]);
		evt.value_array[1] = sensor_value_to_double(&data[1]);
		evt.value_array[2] = sensor_value_to_double(&data[2]);

		/* Do a soft filter here to avoid sending data triggered by
		 * the inactivity threshold.
		 */
		if ((abs(evt.value_array[0]) > threshold ||
		     (abs(evt.value_array[1]) > threshold) ||
		     (abs(evt.value_array[2]) > threshold))) {

			evt.type = EXT_SENSOR_EVT_ACCELEROMETER_TRIGGER;
			evt_handler(&evt);
		}

		break;
	default:
		LOG_ERR("Unknown trigger");
	}
}

int ext_sensors_init(ext_sensor_handler_t handler)
{
	struct ext_sensor_evt evt = {0};

	if (handler == NULL) {
		LOG_ERR("External sensor handler NULL!");
		return -EINVAL;
	}

	evt_handler = handler;

	if (!device_is_ready(temp_sensor.dev)) {
		LOG_ERR("Temperature sensor device is not ready");
		evt.type = EXT_SENSOR_EVT_TEMPERATURE_ERROR;
		evt_handler(&evt);
	}

	if (!device_is_ready(humid_sensor.dev)) {
		LOG_ERR("Humidity sensor device is not ready");
		evt.type = EXT_SENSOR_EVT_HUMIDITY_ERROR;
		evt_handler(&evt);
	}

	if (!device_is_ready(accel_sensor.dev)) {
		LOG_ERR("Accelerometer device is not ready");
		evt.type = EXT_SENSOR_EVT_ACCELEROMETER_ERROR;
		evt_handler(&evt);
	} else {
#if defined(CONFIG_EXTERNAL_SENSORS_ACTIVITY_DETECTION_AUTO)
		struct sensor_trigger trig = {
			.chan = SENSOR_CHAN_ACCEL_XYZ,
			.type = SENSOR_TRIG_THRESHOLD
		};

		int err = sensor_trigger_set(accel_sensor.dev,
					     &trig, accelerometer_trigger_handler);

		if (err) {
			LOG_ERR("Could not set trigger for device %s, error: %d",
				accel_sensor.dev->name, err);
			return err;
		}
#endif
	}

	return 0;
}

int ext_sensors_temperature_get(double *ext_temp)
{
	int err;
	struct sensor_value data = {0};
	struct ext_sensor_evt evt = {0};

	err = sensor_sample_fetch_chan(temp_sensor.dev, SENSOR_CHAN_ALL);
	if (err) {
		LOG_ERR("Failed to fetch data from %s, error: %d",
			temp_sensor.dev->name, err);
		evt.type = EXT_SENSOR_EVT_TEMPERATURE_ERROR;
		evt_handler(&evt);
		return -ENODATA;
	}

	err = sensor_channel_get(temp_sensor.dev, temp_sensor.channel, &data);
	if (err) {
		LOG_ERR("Failed to fetch data from %s, error: %d",
			temp_sensor.dev->name, err);
		evt.type = EXT_SENSOR_EVT_TEMPERATURE_ERROR;
		evt_handler(&evt);
		return -ENODATA;
	}

	k_spinlock_key_t key = k_spin_lock(&(temp_sensor.lock));
	*ext_temp = sensor_value_to_double(&data);
	k_spin_unlock(&(temp_sensor.lock), key);

	return 0;
}

int ext_sensors_humidity_get(double *ext_hum)
{
	int err;
	struct sensor_value data = {0};
	struct ext_sensor_evt evt = {0};

	err = sensor_sample_fetch_chan(humid_sensor.dev, SENSOR_CHAN_ALL);
	if (err) {
		LOG_ERR("Failed to fetch data from %s, error: %d",
			humid_sensor.dev->name, err);
		evt.type = EXT_SENSOR_EVT_HUMIDITY_ERROR;
		evt_handler(&evt);
		return -ENODATA;
	}

	err = sensor_channel_get(humid_sensor.dev, humid_sensor.channel, &data);
	if (err) {
		LOG_ERR("Failed to fetch data from %s, error: %d",
			humid_sensor.dev->name, err);
		evt.type = EXT_SENSOR_EVT_HUMIDITY_ERROR;
		evt_handler(&evt);
		return -ENODATA;
	}

	k_spinlock_key_t key = k_spin_lock(&(humid_sensor.lock));
	*ext_hum = sensor_value_to_double(&data);
	k_spin_unlock(&(humid_sensor.lock), key);

	return 0;
}

int ext_sensors_mov_thres_set(double threshold_new)
{
	int err, input_value;
	double range_max_m_s2 = ADXL362_RANGE_MAX_M_S2;
	double threshold_new_copy;
	struct ext_sensor_evt evt = {0};

	if (threshold_new > range_max_m_s2) {
		LOG_ERR("Invalid threshold value");
		return -ENOTSUP;
	}

	threshold_new_copy = threshold_new;

	/* Convert threshold value into 11-bit decimal value relative
	 * to the configured measuring range of the accelerometer.
	 */
	threshold_new = (threshold_new *
		(ADXL362_THRESHOLD_RESOLUTION_DECIMAL_MAX / range_max_m_s2));

	/* Add 0.5 to ensure proper conversion from double to int. */
	threshold_new = threshold_new + 0.5;
	input_value = (int)threshold_new;

	if (input_value >= ADXL362_THRESHOLD_RESOLUTION_DECIMAL_MAX) {
		input_value = ADXL362_THRESHOLD_RESOLUTION_DECIMAL_MAX - 1;
	} else if (input_value < 0) {
		input_value = 0;
	}

	const struct sensor_value data = {
		.val1 = input_value
	};

	err = sensor_attr_set(accel_sensor.dev, SENSOR_CHAN_ACCEL_X,
			      SENSOR_ATTR_UPPER_THRESH, &data);
	if (err) {
		LOG_ERR("Failed to set accelerometer x-axis threshold value");
		LOG_ERR("Device: %s, error: %d",
			accel_sensor.dev->name, err);
		evt.type = EXT_SENSOR_EVT_ACCELEROMETER_ERROR;
		evt_handler(&evt);
		return err;
	}

	err = sensor_attr_set(accel_sensor.dev, SENSOR_CHAN_ACCEL_Y,
			      SENSOR_ATTR_UPPER_THRESH, &data);
	if (err) {
		LOG_ERR("Failed to set accelerometer y-axis threshold value");
		LOG_ERR("Device: %s, error: %d",
			accel_sensor.dev->name, err);
		evt.type = EXT_SENSOR_EVT_ACCELEROMETER_ERROR;
		evt_handler(&evt);
		return err;
	}

	err = sensor_attr_set(accel_sensor.dev, SENSOR_CHAN_ACCEL_Z,
			      SENSOR_ATTR_UPPER_THRESH, &data);
	if (err) {
		LOG_ERR("Failed to set accelerometer z-axis threshold value");
		LOG_ERR("Device: %s, error: %d",
			accel_sensor.dev->name, err);
		evt.type = EXT_SENSOR_EVT_ACCELEROMETER_ERROR;
		evt_handler(&evt);
		return err;
	}

	threshold = threshold_new_copy;

	return 0;
}

int ext_sensors_accelerometer_trigger_callback_set(bool enable)
{
	int err;
	struct sensor_trigger trig = {
		.chan = SENSOR_CHAN_ACCEL_XYZ,
		.type = SENSOR_TRIG_THRESHOLD
	};
	struct ext_sensor_evt evt = {0};

	sensor_trigger_handler_t handler = enable ? accelerometer_trigger_handler : NULL;

	err = sensor_trigger_set(accel_sensor.dev, &trig, handler);
	if (err) {
		LOG_ERR("Could not set trigger for device %s, error: %d",
			accel_sensor.dev->name, err);
		evt.type = EXT_SENSOR_EVT_ACCELEROMETER_ERROR;
		evt_handler(&evt);
		return err;
	}

	initial_trigger = false;

	return 0;
}
