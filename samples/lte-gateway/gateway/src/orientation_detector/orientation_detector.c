/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "orientation_detector.h"
#include <sensor.h>

#define FLIP_ACCELERATION_THRESHOLD		7.0

static struct device *dev;

int orientation_detector_poll(
	struct orientation_detector_sensor_data *sensor_data)
{
	int err;
	struct sensor_value accel_data[3];
	enum orientation_state current_orientation;

	err = sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);

	if (err) {
		printk("sensor_sample_fetch failed\n");
		return err;
	}

	err = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, &accel_data[0]);

	if (err) {
		printk("sensor_channel_get failed\n");
		return err;
	}

	sensor_data->z = sensor_value_to_double(&accel_data[2]);

	if (sensor_data->z >= FLIP_ACCELERATION_THRESHOLD) {
		current_orientation = ORIENTATION_NORMAL;
	} else if (sensor_data->z <= -FLIP_ACCELERATION_THRESHOLD) {
		current_orientation = ORIENTATION_UPSIDE_DOWN;
	} else {
		current_orientation = ORIENTATION_ON_SIDE;
	}

	sensor_data->orientation = current_orientation;

	return 0;
}


void orientation_detector_init(struct device *accel_device)
{
	dev = accel_device;
}
