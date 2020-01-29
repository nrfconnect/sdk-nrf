/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <drivers/sensor.h>
#include <spinlock.h>
#include "env_sensors.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(env_sensors, CONFIG_ASSET_TRACKER_LOG_LEVEL);

#define ENV_INIT_DELAY_S (5) /* Polling delay upon initialization */
#define MAX_INTERVAL_S   (INT_MAX/MSEC_PER_SEC)

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
static env_sensors_data_ready_cb data_ready_cb;
static u32_t data_send_interval_s = CONFIG_ENVIRONMENT_DATA_SEND_INTERVAL;
static bool backoff_enabled;
static bool initialized;

static inline int submit_poll_work(const u32_t delay_s)
{
	return k_delayed_work_submit(&env_sensors_poller,
				     K_SECONDS((u32_t)delay_s));
}

int env_sensors_poll(void)
{
	return initialized ? submit_poll_work(K_NO_WAIT) : -ENXIO;
}

static void env_sensors_poll_fn(struct k_work *work)
{
	int num_sensors = ARRAY_SIZE(env_sensors);
	struct sensor_value data[num_sensors];

	int err;

	if (data_send_interval_s == 0) {
		return;
	}

	if (IS_ENABLED(CONFIG_BME680)) {
		err = sensor_sample_fetch_chan(env_sensors[0]->dev,
					SENSOR_CHAN_ALL);
		if (err) {
			LOG_ERR("Failed to fetch data from %s, error: %d",
				log_strdup(env_sensors[0]->dev_name), err);
		}
	}
	for (int i = 0; i < num_sensors; i++) {
		if (!(IS_ENABLED(CONFIG_BME680))) {
			err = sensor_sample_fetch_chan(env_sensors[i]->dev,
				env_sensors[i]->channel);
			if (err) {
				LOG_ERR("Failed to fetch data from %s, error: %d",
					log_strdup(env_sensors[i]->dev_name), err);
			}
		}
		err = sensor_channel_get(env_sensors[i]->dev,
			env_sensors[i]->channel, &data[i]);
		if (err) {
			LOG_ERR("Failed to fetch data from %s, error: %d",
				log_strdup(env_sensors[i]->dev_name), err);
		} else {
			k_spinlock_key_t key = k_spin_lock(&(env_sensors[i]->lock));

			env_sensors[i]->sensor.value = sensor_value_to_double(&data[i]);
			k_spin_unlock(&(env_sensors[i]->lock), key);
		}
	}

	if (data_ready_cb) {
		data_ready_cb();
	}

	submit_poll_work(backoff_enabled ?
		CONFIG_ENVIRONMENT_DATA_BACKOFF_TIME : data_send_interval_s);
}

/**@brief Initialize environment sensors. */
int env_sensors_init_and_start(const env_sensors_data_ready_cb cb)
{
	for (int i = 0; i < ARRAY_SIZE(env_sensors); i++) {
		env_sensors[i]->dev =
			device_get_binding(env_sensors[i]->dev_name);
		__ASSERT(env_sensors[i]->dev, "Could not get device %s\n",
			env_sensors[i]->dev_name);
	}

	data_ready_cb = cb;

	k_delayed_work_init(&env_sensors_poller, env_sensors_poll_fn);

	initialized = true;

	return (data_send_interval_s > 0) ?
		submit_poll_work(ENV_INIT_DELAY_S) : 0;
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

void env_sensors_set_send_interval(const u32_t interval_s)
{
	if (interval_s == data_send_interval_s) {
		return;
	}

	data_send_interval_s = MIN(interval_s, MAX_INTERVAL_S);

	if (!initialized) {
		return;
	}

	if (data_send_interval_s) {
		/* restart work for new interval to take effect */
		env_sensors_poll();
	} else if (k_delayed_work_remaining_get(&env_sensors_poller) > 0) {
		k_delayed_work_cancel(&env_sensors_poller);
	}
}

u32_t env_sensors_get_send_interval(void)
{
	return data_send_interval_s;
}

void env_sensors_set_backoff_enable(const bool enable)
{
	backoff_enabled = enable;
}
