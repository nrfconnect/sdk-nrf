/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>
#include <math.h>

#if defined(CONFIG_EXTERNAL_SENSORS_BME680_BSEC)
#include "ext_sensors_bsec.h"
#endif

#include "ext_sensors.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ext_sensors, CONFIG_EXTERNAL_SENSORS_LOG_LEVEL);

/* Convert to s/m2 depending on the maximum measured range used for adxl362. */
#if IS_ENABLED(CONFIG_ADXL362_ACCEL_RANGE_2G)
#define ADXL362_RANGE_MAX_M_S2 19.6133
#elif IS_ENABLED(CONFIG_ADXL362_ACCEL_RANGE_4G)
#define ADXL362_RANGE_MAX_M_S2 39.2266
#elif IS_ENABLED(CONFIG_ADXL362_ACCEL_RANGE_8G)
#define ADXL362_RANGE_MAX_M_S2 78.4532
#endif

/* This is derived from the sensitivity values in the datasheet. */
#define ADXL362_THRESHOLD_RESOLUTION_DECIMAL_MAX 2000

#if IS_ENABLED(CONFIG_ADXL362_ACCEL_ODR_12_5)
#define ADXL362_TIMEOUT_MAX_S 5242.88
#elif IS_ENABLED(CONFIG_ADXL362_ACCEL_ODR_25)
#define ADXL362_TIMEOUT_MAX_S 2621.44
#elif IS_ENABLED(CONFIG_ADXL362_ACCEL_ODR_50)
#define ADXL362_TIMEOUT_MAX_S 1310.72
#elif IS_ENABLED(CONFIG_ADXL362_ACCEL_ODR_100)
#define ADXL362_TIMEOUT_MAX_S 655.36
#elif IS_ENABLED(CONFIG_ADXL362_ACCEL_ODR_200)
#define ADXL362_TIMEOUT_MAX_S 327.68
#elif IS_ENABLED(CONFIG_ADXL362_ACCEL_ODR_400)
#define ADXL362_TIMEOUT_MAX_S 163.84
#endif

#define ADXL362_TIMEOUT_RESOLUTION_MAX 65536

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

static struct env_sensor press_sensor = {
	.channel = SENSOR_CHAN_PRESS,
	.dev = DEVICE_DT_GET(DT_ALIAS(pressure_sensor)),
};

/** Sensor struct for the low-power accelerometer */
static struct env_sensor accel_sensor_lp = {
	.channel = SENSOR_CHAN_ACCEL_XYZ,
	.dev = DEVICE_DT_GET(DT_ALIAS(accelerometer)),
};

#if defined(CONFIG_EXTERNAL_SENSORS_IMPACT_DETECTION)
/** Sensor struct for the high-G accelerometer */
static struct env_sensor accel_sensor_hg = {
	.channel = SENSOR_CHAN_ACCEL_XYZ,
	.dev = DEVICE_DT_GET(DT_ALIAS(impact_sensor)),
};
#endif

static ext_sensor_handler_t evt_handler;

static void accelerometer_trigger_handler(const struct device *dev,
					  const struct sensor_trigger *trig)
{
	int err = 0;
	struct sensor_value data[ACCELEROMETER_CHANNELS];
	struct ext_sensor_evt evt = {0};

	switch (trig->type) {
	case SENSOR_TRIG_MOTION:
	case SENSOR_TRIG_STATIONARY:

		if (sensor_sample_fetch(dev) < 0) {
			LOG_ERR("Sample fetch error");
			return;
		}

		err = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, &data[0]);
		if (err) {
			LOG_ERR("sensor_channel_get, error: %d", err);
			return;
		}

		evt.value_array[0] = sensor_value_to_double(&data[0]);
		evt.value_array[1] = sensor_value_to_double(&data[1]);
		evt.value_array[2] = sensor_value_to_double(&data[2]);

		if (trig->type == SENSOR_TRIG_MOTION) {
			evt.type = EXT_SENSOR_EVT_ACCELEROMETER_ACT_TRIGGER;
			LOG_DBG("Activity detected");
		} else {
			evt.type = EXT_SENSOR_EVT_ACCELEROMETER_INACT_TRIGGER;
			LOG_DBG("Inactivity detected");
		}
		evt_handler(&evt);

		break;
	default:
		LOG_ERR("Unknown trigger");
	}
}

#if defined(CONFIG_EXTERNAL_SENSORS_IMPACT_DETECTION)
static void impact_trigger_handler(const struct device *dev,
				   const struct sensor_trigger *trig)
{
	struct sensor_value data[ACCELEROMETER_CHANNELS];
	struct ext_sensor_evt evt = {0};
	int err;

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		if (sensor_sample_fetch(dev) < 0) {
			LOG_ERR("Sample fetch error");
			return;
		}

		err = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, data);
		if (err) {
			LOG_ERR("sensor_channel_get, error: %d", err);
			return;
		}

		evt.value = sqrt(pow(sensor_ms2_to_g(&data[0]), 2.0) +
				 pow(sensor_ms2_to_g(&data[1]), 2.0) +
				 pow(sensor_ms2_to_g(&data[2]), 2.0));

		LOG_DBG("Detected impact of %6.2f g\n", evt.value);

		if (evt.value > 0.0) {
			evt.type = EXT_SENSOR_EVT_ACCELEROMETER_IMPACT_TRIGGER;
			evt_handler(&evt);
		}
		break;
	default:
		LOG_ERR("Unknown trigger");
	}
}
#endif

int ext_sensors_init(ext_sensor_handler_t handler)
{
	struct ext_sensor_evt evt = {0};

	if (handler == NULL) {
		LOG_ERR("External sensor handler NULL!");
		return -EINVAL;
	}

	evt_handler = handler;

#if defined(CONFIG_EXTERNAL_SENSORS_BME680_BSEC)
	int err = ext_sensors_bsec_init();

	if (err) {
		LOG_ERR("ext_sensors_bsec_init, error: %d", err);
		evt.type = EXT_SENSOR_EVT_BME680_BSEC_ERROR;
		evt_handler(&evt);
	}
#else
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

	if (!device_is_ready(press_sensor.dev)) {
		LOG_ERR("Pressure sensor device is not ready");
		evt.type = EXT_SENSOR_EVT_PRESSURE_ERROR;
		evt_handler(&evt);
	}
#endif /* if defined(CONFIG_EXTERNAL_SENSORS_BME680_BSEC) */

	if (!device_is_ready(accel_sensor_lp.dev)) {
		LOG_ERR("Low-power accelerometer device is not ready");
		evt.type = EXT_SENSOR_EVT_ACCELEROMETER_ERROR;
		evt_handler(&evt);
	}

#if defined(CONFIG_EXTERNAL_SENSORS_IMPACT_DETECTION)
	if (!device_is_ready(accel_sensor_hg.dev)) {
		LOG_ERR("High-G accelerometer device is not ready");
		evt.type = EXT_SENSOR_EVT_ACCELEROMETER_ERROR;
		evt_handler(&evt);
	} else {
		struct sensor_trigger trig = {
			.chan = SENSOR_CHAN_ACCEL_XYZ,
			.type = SENSOR_TRIG_THRESHOLD
		};

		int err = sensor_trigger_set(accel_sensor_hg.dev,
					     &trig, impact_trigger_handler);
		if (err) {
			LOG_ERR("Could not set trigger for device %s, error: %d",
				accel_sensor_hg.dev->name, err);
			return err;
		}
	}
#endif
	return 0;
}

int ext_sensors_temperature_get(double *ext_temp)
{
	int err;
	struct sensor_value data = {0};
	struct ext_sensor_evt evt = {0};

#if defined(CONFIG_EXTERNAL_SENSORS_BME680_BSEC)
	return ext_sensors_bsec_temperature_get(ext_temp);
#endif

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

#if defined(CONFIG_EXTERNAL_SENSORS_BME680_BSEC)
	return ext_sensors_bsec_humidity_get(ext_hum);
#endif

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

int ext_sensors_pressure_get(double *ext_press)
{
	int err;
	struct sensor_value data = {0};
	struct ext_sensor_evt evt = {0};

#if defined(CONFIG_EXTERNAL_SENSORS_BME680_BSEC)
	return ext_sensors_bsec_pressure_get(ext_press);
#endif

	err = sensor_sample_fetch_chan(press_sensor.dev, SENSOR_CHAN_ALL);
	if (err) {
		LOG_ERR("Failed to fetch data from %s, error: %d",
			press_sensor.dev->name, err);
		evt.type = EXT_SENSOR_EVT_PRESSURE_ERROR;
		evt_handler(&evt);
		return -ENODATA;
	}

	err = sensor_channel_get(press_sensor.dev, press_sensor.channel, &data);
	if (err) {
		LOG_ERR("Failed to fetch data from %s, error: %d",
			press_sensor.dev->name, err);
		evt.type = EXT_SENSOR_EVT_PRESSURE_ERROR;
		evt_handler(&evt);
		return -ENODATA;
	}

	k_spinlock_key_t key = k_spin_lock(&(press_sensor.lock));
	*ext_press = sensor_value_to_double(&data);
	k_spin_unlock(&(press_sensor.lock), key);

	return 0;
}

int ext_sensors_air_quality_get(uint16_t *ext_bsec_air_quality)
{
#if defined(CONFIG_EXTERNAL_SENSORS_BME680_BSEC)
	return ext_sensors_bsec_air_quality_get(ext_bsec_air_quality);
#endif
	return -ENOTSUP;
}

int ext_sensors_accelerometer_threshold_set(double threshold, bool upper)
{
	int err, input_value;
	double range_max_m_s2 = ADXL362_RANGE_MAX_M_S2;
	struct ext_sensor_evt evt = {0};

	if ((threshold > range_max_m_s2) || (threshold <= 0.0)) {
		LOG_ERR("Invalid %s threshold value: %f", upper?"activity":"inactivity", threshold);
		return -ENOTSUP;
	}

	/* Convert threshold value into 11-bit decimal value relative
	 * to the configured measuring range of the accelerometer.
	 */
	threshold = (threshold *
		(ADXL362_THRESHOLD_RESOLUTION_DECIMAL_MAX / range_max_m_s2));

	/* Add 0.5 to ensure proper conversion from double to int. */
	threshold = threshold + 0.5;
	input_value = (int)threshold;

	if (input_value >= ADXL362_THRESHOLD_RESOLUTION_DECIMAL_MAX) {
		input_value = ADXL362_THRESHOLD_RESOLUTION_DECIMAL_MAX - 1;
	} else if (input_value < 0) {
		input_value = 0;
	}

	const struct sensor_value data = {
		.val1 = input_value
	};

	enum sensor_attribute attr = upper ? SENSOR_ATTR_UPPER_THRESH : SENSOR_ATTR_LOWER_THRESH;

	/* SENSOR_CHAN_ACCEL_XYZ is not supported by the driver in this case. */
	err = sensor_attr_set(accel_sensor_lp.dev,
		SENSOR_CHAN_ACCEL_X,
		attr,
		&data);
	if (err) {
		LOG_ERR("Failed to set accelerometer threshold value");
		LOG_ERR("Device: %s, error: %d",
			accel_sensor_lp.dev->name, err);
		evt.type = EXT_SENSOR_EVT_ACCELEROMETER_ERROR;
		evt_handler(&evt);
		return err;
	}
	return 0;
}

int ext_sensors_inactivity_timeout_set(double inact_time)
{
	int err, inact_time_decimal;
	struct ext_sensor_evt evt = {0};

	if (inact_time > ADXL362_TIMEOUT_MAX_S || inact_time < 0) {
		LOG_ERR("Invalid timeout value");
		return -ENOTSUP;
	}

	inact_time = inact_time / ADXL362_TIMEOUT_MAX_S * ADXL362_TIMEOUT_RESOLUTION_MAX;
	inact_time_decimal = (int) (inact_time + 0.5);
	inact_time_decimal = MIN(inact_time_decimal, ADXL362_TIMEOUT_RESOLUTION_MAX);
	inact_time_decimal = MAX(inact_time_decimal, 0);

	const struct sensor_value data = {
		.val1 = inact_time_decimal
	};

	err = sensor_attr_set(accel_sensor_lp.dev,
			      SENSOR_CHAN_ACCEL_XYZ,
			      SENSOR_ATTR_HYSTERESIS,
			      &data);
	if (err) {
		LOG_ERR("Failed to set accelerometer inactivity timeout value");
		LOG_ERR("Device: %s, error: %d", accel_sensor_lp.dev->name, err);
		evt.type = EXT_SENSOR_EVT_ACCELEROMETER_ERROR;
		evt_handler(&evt);
		return err;
	}
	return 0;
}

int ext_sensors_accelerometer_trigger_callback_set(bool enable)
{
	int err;
	struct sensor_trigger trig = {
		.chan = SENSOR_CHAN_ACCEL_XYZ,
		.type = SENSOR_TRIG_MOTION
	};
	struct ext_sensor_evt evt = {0};

	sensor_trigger_handler_t handler = enable ? accelerometer_trigger_handler : NULL;

	err = sensor_trigger_set(accel_sensor_lp.dev, &trig, handler);
	if (err) {
		goto error;
	}
	trig.type = SENSOR_TRIG_STATIONARY;
	err = sensor_trigger_set(accel_sensor_lp.dev, &trig, handler);
	if (err) {
		goto error;
	}
		return 0;
error:
	LOG_ERR("Could not set trigger for device %s, error: %d",
		accel_sensor_lp.dev->name, err);
	evt.type = EXT_SENSOR_EVT_ACCELEROMETER_ERROR;
	evt_handler(&evt);
	return err;
}
