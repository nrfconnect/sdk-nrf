/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <sensor.h>
#include "orientation_detector.h"

#define FLIP_ACCELERATION_THRESHOLD	5.0
#define CALIBRATION_ITERATIONS		CONFIG_ACCEL_CALIBRATION_ITERATIONS
#define MEASUREMENT_ITERATIONS		CONFIG_ACCEL_ITERATIONS
#define ACCEL_INVERTED			CONFIG_ACCEL_INVERTED

static struct device *dev;
static double accel_offset[3];

int orientation_detector_poll(
	struct orientation_detector_sensor_data *sensor_data)
{
	int err;
	u8_t i;
	double aggregated_data[3] = {0};
	struct sensor_value accel_data[3];
	enum orientation_state current_orientation;

	for (i = 0; i < MEASUREMENT_ITERATIONS; i++) {
		err = sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_Z);
		if (err) {
			printk("sensor_sample_fetch failed\n");
			return err;
		}

		err = sensor_channel_get(dev,
				SENSOR_CHAN_ACCEL_Z, &accel_data[2]);

		if (err) {
			printk("sensor_channel_get failed\n");
			return err;
		}

		aggregated_data[2] += sensor_value_to_double(&accel_data[2]);
	}

	sensor_data->z = (aggregated_data[2] / (double)MEASUREMENT_ITERATIONS) -
				accel_offset[2];

	if (sensor_data->z >= FLIP_ACCELERATION_THRESHOLD) {
		if (IS_ENABLED(ACCEL_INVERTED)) {
			current_orientation = ORIENTATION_UPSIDE_DOWN;
		} else {
			current_orientation = ORIENTATION_NORMAL;
		}
	} else if (sensor_data->z <= -FLIP_ACCELERATION_THRESHOLD) {
		if (IS_ENABLED(ACCEL_INVERTED)) {
			current_orientation = ORIENTATION_NORMAL;
		} else {
			current_orientation = ORIENTATION_UPSIDE_DOWN;
		}
	} else {
		current_orientation = ORIENTATION_ON_SIDE;
	}

	sensor_data->orientation = current_orientation;

	return 0;
}

int orientation_detector_calibrate(void)
{
	u8_t i;
	int err;
	struct sensor_value accel_data[3];
	double aggregated_data[3] = {0};

	for (i = 0; i < CALIBRATION_ITERATIONS; i++) {
		err = sensor_sample_fetch(dev);
		if (err) {
			printk("sensor_sample_fetch failed\n");
			return err;
		}

		err = sensor_channel_get(dev,
				SENSOR_CHAN_ACCEL_X, &accel_data[0]);
		err += sensor_channel_get(dev,
				SENSOR_CHAN_ACCEL_Y, &accel_data[1]);
		err += sensor_channel_get(dev,
				SENSOR_CHAN_ACCEL_Z, &accel_data[2]);
		if (err) {
			printk("sensor_channel_get failed\n");
			return err;
		}

		aggregated_data[0] += sensor_value_to_double(&accel_data[0]);
		aggregated_data[1] += sensor_value_to_double(&accel_data[1]);
		aggregated_data[2] +=
			(sensor_value_to_double(&accel_data[2])
			+ ((double)SENSOR_G) / 1000000.0);
	}

	accel_offset[0] = aggregated_data[0] / (double)CALIBRATION_ITERATIONS;
	accel_offset[1] = aggregated_data[1] / (double)CALIBRATION_ITERATIONS;
	accel_offset[2] = aggregated_data[2] / (double)CALIBRATION_ITERATIONS;

	return 0;
}

void orientation_detector_init(struct device *accel_device)
{
	dev = accel_device;
}
