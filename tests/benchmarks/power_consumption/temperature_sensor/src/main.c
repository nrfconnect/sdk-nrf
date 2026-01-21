/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

static const struct device *temp_dev = DEVICE_DT_GET(DT_NODELABEL(temp_sensor));

static bool suspend_req;

bool self_suspend_req(void)
{
	suspend_req = true;
	return true;
}

void thread_definition(void)
{
	int rc;
	int32_t temp_val;
	struct sensor_value val;

	while (1) {
		if (suspend_req) {
			suspend_req = false;
			k_thread_suspend(k_current_get());
		}

		rc = sensor_sample_fetch_chan(temp_dev, SENSOR_CHAN_DIE_TEMP);
		__ASSERT(rc == 0, "Cannot fetch chan sample: %d.", rc);

		rc = sensor_channel_get(temp_dev, SENSOR_CHAN_DIE_TEMP, &val);
		__ASSERT(rc == 0, "Cannot read from channel %d: %d.",
			   SENSOR_CHAN_DIE_TEMP, rc);

		temp_val = (val.val1 * 100) + (val.val2 / 10000);
		printk("Temperature: %d.%02u\n",
			temp_val/100, abs(temp_val) % 100);

		__ASSERT(val.val1 > 10, "Temperature to low (laboratory conditions)");
		__ASSERT(val.val1 < 35, "Temperature too high (laboratory conditions)");

		k_sleep(K_MSEC(20));
	};
};
