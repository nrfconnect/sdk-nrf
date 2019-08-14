/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <sensor.h>
#include <spinlock.h>
#include "env_sensors.h"

struct env_sensor {
	env_sensor_data_t sensor;
	enum sensor_channel channel;
	u8_t *dev_name;
	struct device *dev;
	struct k_spinlock lock;
};

static struct env_sensor temp_sensor = {
	.sensor =  {
		.type = ENV_SENSOR_TEMPERATURE,
		.value = 0,
	},
	.channel = SENSOR_CHAN_AMBIENT_TEMP,
	.dev_name = CONFIG_TEMP_DEV_NAME
};

static struct env_sensor humid_sensor = {
	.sensor =  {
		.type = ENV_SENSOR_HUMIDITY,
		.value = 0,
	},
	.channel = SENSOR_CHAN_HUMIDITY,
	.dev_name = CONFIG_TEMP_DEV_NAME
};

static struct env_sensor pressure_sensor = {
	.sensor =  {
		.type = ENV_SENSOR_AIR_PRESSURE,
		.value = 0,
	},
	.channel = SENSOR_CHAN_PRESS,
	.dev_name = CONFIG_TEMP_DEV_NAME
};

/* Array containg environment sensors available on the board. */
static struct env_sensor *env_sensors[] = {
	&temp_sensor,
	&humid_sensor,
	&pressure_sensor
};

static struct k_delayed_work env_sensors_poller;

static void env_sensors_poll_fn(struct k_work *work)
{
	int num_sensors = ARRAY_SIZE(env_sensors);
	struct sensor_value data[num_sensors];

	int err;

	if (IS_ENABLED(CONFIG_BME680)) {
		err = sensor_sample_fetch_chan(env_sensors[0]->dev,
					SENSOR_CHAN_ALL);
		if (err) {
			printk("Failed to fetch data from %s, error: %d\n",
				env_sensors[0]->dev_name, err);
		}
	}
	for (int i = 0; i < num_sensors; i++) {
		if (!(IS_ENABLED(CONFIG_BME680))) {
			err = sensor_sample_fetch_chan(env_sensors[i]->dev,
				env_sensors[i]->channel);
			if (err) {
				printk("Failed to fetch data from %s, error: %d\n",
					env_sensors[i]->dev_name, err);
			}
		}
		err = sensor_channel_get(env_sensors[i]->dev,
			env_sensors[i]->channel, &data[i]);
		if (err) {
			printk("Failed to fetch data from %s, error: %d\n",
				env_sensors[i]->dev_name, err);
		} else {
			k_spinlock_key_t key = k_spin_lock(&(env_sensors[i]->lock));

			env_sensors[i]->sensor.value = sensor_value_to_double(&data[i]);
			k_spin_unlock(&(env_sensors[i]->lock), key);
		}
	}

	 k_delayed_work_submit(&env_sensors_poller, K_SECONDS(10));
}
/**@brief Initialize environment sensors. */
int env_sensors_init_and_start(void)
{
	for (int i = 0; i < ARRAY_SIZE(env_sensors); i++) {
		env_sensors[i]->dev =
			device_get_binding(env_sensors[i]->dev_name);
		__ASSERT(env_sensors[i]->dev, "Could not get device %s\n",
			env_sensors[i]->dev_name);
	}

	k_delayed_work_init(&env_sensors_poller, env_sensors_poll_fn);
	env_sensors_poll_fn(NULL);

	return k_delayed_work_submit(&env_sensors_poller, K_SECONDS(10));
}

int env_sensors_get_temperature(env_sensor_data_t *sensor_data)
{
	if (sensor_data == NULL) {
		return -1;
	}
	k_spinlock_key_t key = k_spin_lock(&temp_sensor.lock);

	memcpy(sensor_data, &(temp_sensor.sensor), sizeof(temp_sensor.sensor));
	k_spin_unlock(&temp_sensor.lock, key);
	return 0;
}

int env_sensors_get_humidity(env_sensor_data_t *sensor_data)
{
	if (sensor_data == NULL) {
		return -1;
	}
	k_spinlock_key_t key = k_spin_lock(&humid_sensor.lock);

	memcpy(sensor_data, &(humid_sensor.sensor),
		sizeof(humid_sensor.sensor));
	k_spin_unlock(&humid_sensor.lock, key);
	return 0;
}

int env_sensors_get_pressure(env_sensor_data_t *sensor_data)
{
	if (sensor_data == NULL) {
		return -1;
	}
	k_spinlock_key_t key = k_spin_lock(&pressure_sensor.lock);

	memcpy(sensor_data, &(pressure_sensor.sensor),
		sizeof(pressure_sensor.sensor));
	k_spin_unlock(&pressure_sensor.lock, key);
	return 0;
}

int env_sensors_get_air_quality(env_sensor_data_t *sensor_data)
{
	return -1;
}
