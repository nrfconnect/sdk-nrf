/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#define NUMBER_OF_READOUT 1000

static const struct device *temp_dev = DEVICE_DT_GET(DT_NODELABEL(temp_sensor));

ZTEST(multicore_temp, test_driver_stress)
{
	int rc;
	struct sensor_value val;
	int32_t temp_val;

	TC_PRINT("Running on %s\n", CONFIG_BOARD_TARGET);

	zassert_true(device_is_ready(temp_dev), "Device %s is not ready.", temp_dev->name);

	for (uint32_t i = 1; i <= NUMBER_OF_READOUT; i++) {
		rc = sensor_sample_fetch_chan(temp_dev, SENSOR_CHAN_DIE_TEMP);
		zassert_ok(rc, "Cannot fetch chan sample: %d.", rc);

		rc = sensor_channel_get(temp_dev, SENSOR_CHAN_DIE_TEMP, &val);
		zassert_ok(rc, "Cannot read from channel %d: %d.", SENSOR_CHAN_DIE_TEMP, rc);

		temp_val = (val.val1 * 100) + (val.val2 / 10000);
		TC_PRINT("%d: Temperature: %d.%02u\n", i, temp_val / 100, abs(temp_val) % 100);
	}
	TC_PRINT("Temperature was successfully read %d times.\n", NUMBER_OF_READOUT);
}

ZTEST_SUITE(multicore_temp, NULL, NULL, NULL, NULL, NULL);
