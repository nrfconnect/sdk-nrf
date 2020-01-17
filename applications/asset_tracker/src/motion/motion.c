/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <sensor.h>
#include "motion.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(motion, CONFIG_ASSET_TRACKER_LOG_LEVEL);

motion_handler_t handler;
static struct device *accel_dev;

#define FLIP_ACCELERATION_THRESHOLD	5.0

static int accelerometer_poll(motion_acceleration_data_t *sensor_data)
{
	if (sensor_data == NULL) {
		return -EINVAL;
	}

	int err;

	struct sensor_value accel_data[3];

	/* If using the ADXL362 driver, all channels must be fetched */
	if (IS_ENABLED(CONFIG_ADXL362)) {
		err = sensor_sample_fetch_chan(accel_dev,
						SENSOR_CHAN_ALL);
	} else {
		err = sensor_sample_fetch_chan(accel_dev,
						SENSOR_CHAN_ACCEL_XYZ);
	}

	if (err) {
		LOG_ERR("sensor_sample_fetch failed");
		return err;
	}

	err = sensor_channel_get(accel_dev,
			SENSOR_CHAN_ACCEL_X, &accel_data[0]);

	if (err) {
		LOG_ERR("sensor_channel_get failed");
		return err;
	}

	err = sensor_channel_get(accel_dev,
			SENSOR_CHAN_ACCEL_Y, &accel_data[1]);

	if (err) {
		LOG_ERR("sensor_channel_get failed");
		return err;
	}
	err = sensor_channel_get(accel_dev,
			SENSOR_CHAN_ACCEL_Z, &accel_data[2]);

	if (err) {
		LOG_ERR("sensor_channel_get failed");
		return err;
	}

	sensor_data->x = sensor_value_to_double(&accel_data[0]);
	sensor_data->y = sensor_value_to_double(&accel_data[1]);
	sensor_data->z = sensor_value_to_double(&accel_data[2]);

	return 0;
}

static int get_orientation(motion_orientation_state_t *orientation,
				motion_acceleration_data_t *acceleration_data)
{
	if (orientation == NULL || acceleration_data == NULL) {
		return -EINVAL;
	}

	if (acceleration_data->z >= FLIP_ACCELERATION_THRESHOLD) {
		*orientation = IS_ENABLED(CONFIG_ACCEL_INVERTED) ?
		MOTION_ORIENTATION_UPSIDE_DOWN : MOTION_ORIENTATION_NORMAL;
	} else if (acceleration_data->z <= -FLIP_ACCELERATION_THRESHOLD) {
		*orientation = IS_ENABLED(CONFIG_ACCEL_INVERTED) ?
		MOTION_ORIENTATION_NORMAL : MOTION_ORIENTATION_UPSIDE_DOWN;
	} else {
		*orientation = MOTION_ORIENTATION_ON_SIDE;
	}

	return 0;
}

/**@brief Callback for sensor trigger events */
static void sensor_trigger_handler(struct device *dev,
			struct sensor_trigger *trigger)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trigger);

	motion_data_t motion_data;

	if (accelerometer_poll(&motion_data.acceleration) == 0) {
		if (get_orientation(&motion_data.orientation,
					&motion_data.acceleration) == 0) {
			handler(motion_data);
		}
	}
}

/**@brief Initializes the accelerometer device and
 * configures trigger if set.
 */
static int accelerometer_init(void)
{
	int err = 0;

	accel_dev = device_get_binding(CONFIG_ACCEL_DEV_NAME);

	if (accel_dev == NULL) {
		LOG_ERR("Could not get %s device",
			log_strdup(CONFIG_ACCEL_DEV_NAME));
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_ACCEL_USE_EXTERNAL)) {

		struct sensor_trigger sensor_trig = {
			.type = SENSOR_TRIG_THRESHOLD,
		};

		err = sensor_trigger_set(accel_dev, &sensor_trig,
				sensor_trigger_handler);

		if (err) {
			LOG_ERR("Unable to set accelerometer trigger");
			return err;
		}
	}
	return 0;
}

/**@brief Initialize motion module. */
int motion_init_and_start(motion_handler_t motion_handler)
{
	if (motion_handler == NULL) {
		return -EINVAL;
	}

	int err;

	handler = motion_handler;
	err = accelerometer_init();

	if (err) {
		return err;
	}

	sensor_trigger_handler(NULL, NULL);
	return 0;
}

void motion_simulate_trigger(void)
{
	sensor_trigger_handler(NULL, NULL);
}
