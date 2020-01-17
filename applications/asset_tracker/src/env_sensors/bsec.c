/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <string.h>
#include <sys/atomic.h>
#include <spinlock.h>
#include <settings/settings.h>
#include <sys/byteorder.h>
#include <i2c.h>
#include "bsec_integration.h"
#include "env_sensors.h"

#define ENV_INIT_DELAY_S (5) /* Polling delay upon initialization */
#define MAX_INTERVAL_S   (INT_MAX/MSEC_PER_SEC)

#include <logging/log.h>
LOG_MODULE_REGISTER(bsec, CONFIG_ASSET_TRACKER_LOG_LEVEL);

/* @brief Sample rate for the BSEC library
 *
 * BSEC_SAMPLE_RATE_ULP = 0.0033333 Hz = 300 second interval
 * BSEC_SAMPLE_RATE_LP = 0.33333 Hz = 3 second interval
 */
#define BSEC_SAMPLE_RATE	BSEC_SAMPLE_RATE_LP

/* @breif Interval for saving BSEC state to flash
 *
 * Interval = BSEC_STATE_INTERVAL * 1/BSEC_SAMPLE_RATE
 * Example:  600 * 1/0.33333 Hz = 1800 seconds = 0.5 hour
 */
#define BSEC_STATE_SAVE_INTERVAL	600

struct device *i2c_master;

struct env_sensor {
	env_sensor_data_t sensor;
	struct k_spinlock lock;
};

static struct env_sensor temp_sensor = {
	.sensor =  {
		.type = ENV_SENSOR_TEMPERATURE,
	},
};

static struct env_sensor humid_sensor = {
	.sensor =  {
		.type = ENV_SENSOR_HUMIDITY,
	},
};

static struct env_sensor pressure_sensor = {
	.sensor =  {
		.type = ENV_SENSOR_AIR_PRESSURE,
	},
};

static struct env_sensor air_quality_sensor = {
	.sensor =  {
		.type = ENV_SENSOR_AIR_QUALITY,
	},
};

/* size of stack area used by bsec thread */
#define STACKSIZE 4096

static K_THREAD_STACK_DEFINE(thread_stack, STACKSIZE);
static struct k_thread thread;

static u8_t s_state_buffer[BSEC_MAX_STATE_BLOB_SIZE];
static s32_t s_state_buffer_len;

static struct k_delayed_work env_sensors_poller;
static env_sensors_data_ready_cb data_ready_cb;
static u32_t data_send_interval_s = CONFIG_ENVIRONMENT_DATA_SEND_INTERVAL;
static bool backoff_enabled;
static bool initialized;

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	if (!strcmp(key, "state")) {
		s_state_buffer_len = len_rd;
		if (read_cb(cb_arg, s_state_buffer, len_rd) > 0) {
			return 0;
		}
	}
	s_state_buffer_len = 0;
	return -1;
}

static int enable_settings(void)
{
	int err;

	settings_subsys_init();
	struct settings_handler my_conf = {
		.name = "bsec",
		.h_set = settings_set
	};

	err = settings_register(&my_conf);
	if (err) {
		LOG_ERR("Cannot register settings handler");
		return err;
	}

	/* This module loads settings for all application modules */
	err = settings_load();
	if (err) {
		LOG_ERR("Cannot load settings");
		return err;
	}

	return err;
}

static int8_t bus_write(u8_t dev_addr, u8_t reg_addr,
			u8_t *reg_data_ptr, u16_t data_len)
{
	u8_t buf[data_len+1];

	buf[0] = reg_addr;
	memcpy(&buf[1], reg_data_ptr, data_len);
	return i2c_write(i2c_master, buf, data_len+1, dev_addr);
}

static s8_t bus_read(u8_t dev_addr, u8_t reg_addr,
		     u8_t *reg_data_ptr, u16_t data_len)
{
	return i2c_write_read(i2c_master, dev_addr, &reg_addr,
			      1, reg_data_ptr, data_len);
}

static s64_t get_timestamp_us(void)
{
	return k_uptime_get()*1000;
}

static void output_ready(s64_t timestamp, float iaq, u8_t iaq_accuracy,
			float temperature, float humidity, float pressure,
			float raw_temperature, float raw_humidity, float gas,
			bsec_library_return_t bsec_status, float static_iaq,
			float co2_equivalent, float breath_voc_equivalent)
{
	k_spinlock_key_t key = k_spin_lock(&(temp_sensor.lock));

	temp_sensor.sensor.value = temperature;
	k_spin_unlock(&(temp_sensor.lock), key);
	key = k_spin_lock(&(humid_sensor.lock));
	humid_sensor.sensor.value = humidity;
	k_spin_unlock(&(humid_sensor.lock), key);
	key = k_spin_lock(&(pressure_sensor.lock));
	pressure_sensor.sensor.value = pressure / 1000;
	k_spin_unlock(&(pressure_sensor.lock), key);
	key = k_spin_lock(&(air_quality_sensor.lock));
	air_quality_sensor.sensor.value = iaq;
	k_spin_unlock(&(air_quality_sensor.lock), key);
}

static u32_t state_load(u8_t *state_buffer, u32_t n_buffer)
{
	if ((s_state_buffer_len > 0) && (s_state_buffer_len <= n_buffer)) {
		memcpy(state_buffer, s_state_buffer, s_state_buffer_len);
		return s_state_buffer_len;
	} else {
		return 0;
	}
}

static void state_save(const u8_t *state_buffer, u32_t length)
{
	settings_save_one("bsec/state", state_buffer, length);
}

static u32_t config_load(u8_t *config_buffer, u32_t n_buffer)
{
	/* Not implemented */
	return 0;
}

static void bsec_thread(void)
{
	bsec_iot_loop((void *)k_sleep, get_timestamp_us, output_ready,
			state_save, BSEC_STATE_SAVE_INTERVAL);
}

int env_sensors_get_temperature(env_sensor_data_t *sensor_data)
{
	if (sensor_data == NULL) {
		return -1;
	}
	k_spinlock_key_t key = k_spin_lock(&temp_sensor.lock);

	memcpy(sensor_data, &temp_sensor.sensor, sizeof(temp_sensor.sensor));
	k_spin_unlock(&temp_sensor.lock, key);
	return 0;
}

int env_sensors_get_humidity(env_sensor_data_t *sensor_data)
{
	if (sensor_data == NULL) {
		return -1;
	}
	k_spinlock_key_t key = k_spin_lock(&humid_sensor.lock);

	memcpy(sensor_data, &humid_sensor.sensor,
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

	memcpy(sensor_data, &pressure_sensor.sensor,
		sizeof(pressure_sensor.sensor));
	k_spin_unlock(&pressure_sensor.lock, key);
	return 0;
}

int env_sensors_get_air_quality(env_sensor_data_t *sensor_data)
{
	if (sensor_data == NULL) {
		return -1;
	}
	k_spinlock_key_t key = k_spin_lock(&air_quality_sensor.lock);

	memcpy(sensor_data, &air_quality_sensor.sensor,
		sizeof(air_quality_sensor.sensor));
	k_spin_unlock(&air_quality_sensor.lock, key);
	return 0;
}

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

	if (data_send_interval_s == 0) {
		return;
	}

	if (data_ready_cb) {
		data_ready_cb();
	}

	submit_poll_work(backoff_enabled ?
		CONFIG_ENVIRONMENT_DATA_BACKOFF_TIME : data_send_interval_s);
}

int env_sensors_init_and_start(const env_sensors_data_ready_cb cb)
{
	return_values_init bsec_ret;
	int ret;

	i2c_master = device_get_binding("I2C_2");
	if (!i2c_master) {
		LOG_ERR("cannot bind to BME680");
		return -EINVAL;
	}
	ret = enable_settings();
	if (ret) {
		LOG_ERR("Cannot enable settings err: %d", ret);
		return ret;
	}
	bsec_ret = bsec_iot_init(BSEC_SAMPLE_RATE, 1.2f, bus_write,
				bus_read, (void *)k_sleep, state_load,
				config_load);
	if (bsec_ret.bme680_status) {
		/* Could not initialize BME680 */
		return (int)bsec_ret.bme680_status;
	} else if (bsec_ret.bsec_status) {
		/* Could not initialize BSEC library */
		return (int)bsec_ret.bsec_status;
	}

	k_thread_create(&thread, thread_stack, STACKSIZE,
			(k_thread_entry_t)bsec_thread, NULL, NULL, NULL,
			CONFIG_SYSTEM_WORKQUEUE_PRIORITY, 0, K_NO_WAIT);

	data_ready_cb = cb;

	k_delayed_work_init(&env_sensors_poller, env_sensors_poll_fn);

	initialized = true;

	return (data_send_interval_s > 0) ?
		submit_poll_work(ENV_INIT_DELAY_S) : 0;
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
