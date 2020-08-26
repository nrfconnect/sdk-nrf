/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <stdlib.h>
#include <device.h>
#include <drivers/sensor.h>
#include <orientation_handler.h>

static const struct device *dev;

uint8_t orientation_get(void)
{
	if (!dev) {
		printk("Device %s is not initiated\n", dev->name);
		return THINGY_ORIENT_NONE;
	}

	int rc = sensor_sample_fetch(dev);

	if (rc) {
		printk("Error fetching sample\n");
		return THINGY_ORIENT_NONE;
	}

	struct sensor_value accel[3];

	rc = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);

	if (rc) {
		printk("Error getting channel\n");
		return THINGY_ORIENT_NONE;
	}

	uint8_t orr = THINGY_ORIENT_NONE;
	int32_t top_val = 0;

	for (size_t i = 0; i < 3; i++) {
		if (abs(accel[i].val1) > abs(top_val)) {
			top_val = accel[i].val1;
			if (top_val < 0) {
				orr = i + 3;
			} else {
				orr = i;
			}
		}
	}

	return orr;
}

int orientation_init(void)
{
	dev = device_get_binding(DT_LABEL(DT_NODELABEL(lis2dh12)));

	if (dev == NULL) {
		return -ENODEV;
	}

	return 0;
}
