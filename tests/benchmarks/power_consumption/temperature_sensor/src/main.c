/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

static const struct device *temp_dev = DEVICE_DT_GET(DT_NODELABEL(temp_sensor));

void set_trigger(void);

static void trigger_handler(const struct device *temp_dev, const struct sensor_trigger *trig)
{
	ARG_UNUSED(temp_dev);
	ARG_UNUSED(trig);

	k_busy_wait(1000000);
	set_trigger();
}

void set_trigger(void)
{
	int rc;
	struct sensor_value val = {0};
	struct sensor_trigger trig = {.type = SENSOR_TRIG_THRESHOLD, .chan = SENSOR_CHAN_DIE_TEMP};

	/* Set sampling frequency to 1 Hz, to expect a trigger after 1 s. */
	val.val1 = 1;
	rc = sensor_attr_set(temp_dev, SENSOR_CHAN_DIE_TEMP, SENSOR_ATTR_SAMPLING_FREQUENCY, &val);
	__ASSERT_NO_MSG(rc == 0);

	rc = sensor_sample_fetch_chan(temp_dev, SENSOR_CHAN_DIE_TEMP);
	__ASSERT_NO_MSG(rc == 0);

	rc = sensor_channel_get(temp_dev, SENSOR_CHAN_DIE_TEMP, &val);
	__ASSERT_NO_MSG(rc == 0);

	printk("Temperature: %d.%d\n", val.val1, val.val2 / 10000);

	/* Verify sensor reading - should be within room temperature limits.*/
	__ASSERT_NO_MSG(val.val1 > 10);
	__ASSERT_NO_MSG(val.val1 < 35);

	/*
	 * Set the upper threshold 5* below the current temperature to ensure that the callback will
	 * be triggered
	 */
	val.val1 -= 5;
	rc = sensor_attr_set(temp_dev, SENSOR_CHAN_DIE_TEMP, SENSOR_ATTR_UPPER_THRESH, &val);
	__ASSERT_NO_MSG(rc == 0);
	printk("Upper threshold: %d.%d\n", val.val1, val.val2 / 10000);

	rc = sensor_trigger_set(temp_dev, &trig, trigger_handler);
	__ASSERT_NO_MSG(rc == 0);
}

int main(void)
{
	int rc;

	rc = device_is_ready(temp_dev);
	__ASSERT_NO_MSG(rc);

	set_trigger();

	return 0;
}
