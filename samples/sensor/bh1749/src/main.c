/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/__assert.h>

#define THRESHOLD_UPPER                 50
#define THRESHOLD_LOWER                 0
#define TRIGGER_ON_DATA_READY           0

static K_SEM_DEFINE(sem, 0, 1);

static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trigger)
{
	ARG_UNUSED(dev);
	switch (trigger->type) {
	case SENSOR_TRIG_THRESHOLD:
		printk("Threshold trigger\r\n");
		break;
	case SENSOR_TRIG_DATA_READY:
		printk("Data ready trigger\r\n");
		break;
	default:
		printk("Unknown trigger event %d\r\n", trigger->type);
		break;
	}
	k_sem_give(&sem);
}

static int bh1479_set_attribute(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr, int value)
{
	int ret;
	struct sensor_value sensor_val;

	sensor_val.val1 = (value);

	ret = sensor_attr_set(dev, chan, attr, &sensor_val);
	if (ret) {
		printk("sensor_attr_set failed ret %d\n", ret);
	}

	return ret;
}

/* This sets up  the sensor to signal valid data when a threshold
 * value is reached.
 */
static void process(const struct device *dev)
{
	int ret;
	struct sensor_value temp_val;
	struct sensor_trigger sensor_trig_conf = {
		#if (TRIGGER_ON_DATA_READY)
		.type = SENSOR_TRIG_DATA_READY,
		#else
		.type = SENSOR_TRIG_THRESHOLD,
		#endif
		.chan = SENSOR_CHAN_RED,
	};

	if (IS_ENABLED(CONFIG_BH1749_TRIGGER)) {
		bh1479_set_attribute(dev, SENSOR_CHAN_ALL,
				     SENSOR_ATTR_LOWER_THRESH,
				     THRESHOLD_LOWER);
		bh1479_set_attribute(dev, SENSOR_CHAN_ALL,
				     SENSOR_ATTR_UPPER_THRESH,
				     THRESHOLD_UPPER);

		if (sensor_trigger_set(dev, &sensor_trig_conf,
				trigger_handler)) {
			printk("Could not set trigger\n");
			return;
		}
	}

	while (1) {
		if (IS_ENABLED(CONFIG_BH1749_TRIGGER)) {
			/* Waiting for a trigger event */
			k_sem_take(&sem, K_FOREVER);
		} else {
			/* Waiting some time for first fetch if trigger disabled*/
			k_sleep(K_MSEC(1000));
		}

		ret = sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
		/* The sensor does only support fetching SENSOR_CHAN_ALL */
		if (ret) {
			printk("sensor_sample_fetch failed ret %d\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_RED, &temp_val);
		if (ret) {
			printk("sensor_channel_get failed ret %d\n", ret);
			return;
		}
		printk("BH1749 RED: %d\n", temp_val.val1);

		ret = sensor_channel_get(dev, SENSOR_CHAN_GREEN, &temp_val);
		if (ret) {
			printk("sensor_channel_get failed ret %d\n", ret);
			return;
		}
		printk("BH1749 GREEN: %d\n", temp_val.val1);

		ret = sensor_channel_get(dev, SENSOR_CHAN_BLUE, &temp_val);
		if (ret) {
			printk("sensor_channel_get failed ret %d\n", ret);
			return;
		}
		printk("BH1749 BLUE: %d\n", temp_val.val1);

		ret = sensor_channel_get(dev, SENSOR_CHAN_IR, &temp_val);
		if (ret) {
			printk("sensor_channel_get failed ret %d\n", ret);
			return;
		}
		printk("BH1749 IR: %d\n", temp_val.val1);
		k_sleep(K_MSEC(2000));
	}
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET_ONE(rohm_bh1749);

	if (IS_ENABLED(CONFIG_LOG_BACKEND_RTT)) {
		/* Give RTT log time to be flushed before executing tests */
		k_sleep(K_MSEC(500));
	}

	if (!device_is_ready(dev)) {
		printk("Sensor device not ready\n");
		return 0;
	}

	printk("device is %p, name is %s\n", dev, dev->name);
	process(dev);

	return 0;
}
