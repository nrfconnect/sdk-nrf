/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/spinlock.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/i2c.h>

#include "bsec_integration.h"
#include "ext_sensors_bsec.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ext_sensors_bsec, CONFIG_EXTERNAL_SENSORS_LOG_LEVEL);

/* Sample rate for the BSEC library
 *
 * BSEC_SAMPLE_RATE_ULP = 0.0033333 Hz = 300 second interval
 * BSEC_SAMPLE_RATE_LP = 0.33333 Hz = 3 second interval
 * BSEC_SAMPLE_RATE_HP = 1 Hz = 1 second interval
 */
#if defined(CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_ULTRA_LOW_POWER)
#define BSEC_SAMPLE_RATE BSEC_SAMPLE_RATE_ULP
#elif defined(CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_LOW_POWER)
#define BSEC_SAMPLE_RATE BSEC_SAMPLE_RATE_LP
#elif defined(CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_CONTINUOUS)
#define BSEC_SAMPLE_RATE BSEC_SAMPLE_RATE_HP
#endif

/* Temperature offset due to external heat sources. */
float temp_offset = (CONFIG_EXTERNAL_SENSORS_BSEC_TEMPERATURE_OFFSET / (float)100);

/* @breif Interval for saving BSEC state to flash
 *
 * Interval = BSEC_SAVE_INTERVAL * 1/BSEC_SAMPLE_RATE
 * Example:  600 * 1/0.33333 Hz = 1800 seconds = 0.5 hour
 */
#define BSEC_SAVE_INTERVAL 600

/* Definitions used to store and retrieve BSEC state from the settings API */
#define SETTINGS_NAME_BSEC "bsec"
#define SETTINGS_KEY_STATE "state"
#define SETTINGS_BSEC_STATE SETTINGS_NAME_BSEC "/" SETTINGS_KEY_STATE

/* Stack size of internal BSEC thread. */
#define BSEC_STACK_SIZE CONFIG_EXTERNAL_SENSORS_BSEC_THREAD_STACK_SIZE
K_THREAD_STACK_DEFINE(thread_stack, BSEC_STACK_SIZE);

struct config {
	/* Variable used to reference the I2C device where BME680 is connected to. */
	const struct device *i2c_master;
};

/* Structure used to maintain internal variables used by the library. */
struct ctx {
	/* Spinlock used to safely read out sensor readings from the BSEC library driver. */
	struct k_spinlock sensor_read_lock;

	/* Intermediate libraries used to hold the latest sampled sensor readings. */
	double temperature_latest;
	double humidity_latest;
	double pressure_latest;
	uint16_t air_quality_latest;

	/* Internal BSEC thread metadata value. */
	struct k_thread thread;

	/* Buffer used to maintain the BSEC library state. */
	uint8_t state_buffer[BSEC_MAX_STATE_BLOB_SIZE];
	int32_t state_buffer_len;

};

static const struct config config = {
	.i2c_master = DEVICE_DT_GET(DT_BUS(DT_NODELABEL(bme680))),
};

static struct ctx ctx;

/* Forward declarations */
static int settings_set(const char *key, size_t len_rd, settings_read_cb read_cb, void *cb_arg);
SETTINGS_STATIC_HANDLER_DEFINE(bsec, SETTINGS_NAME_BSEC, NULL, settings_set, NULL, NULL);

/* Private API */

static int settings_set(const char *key, size_t len_rd, settings_read_cb read_cb, void *cb_arg)
{
	if (!key) {
		LOG_ERR("Invalid key");
		return -EINVAL;
	}

	LOG_DBG("Settings key: %s, size: %d", log_strdup(key), len_rd);

	if (!strncmp(key, SETTINGS_KEY_STATE, strlen(SETTINGS_KEY_STATE))) {
		if (len_rd > sizeof(ctx.state_buffer)) {
			LOG_ERR("Setting too big: %d", len_rd);
			return -EMSGSIZE;
		}

		ctx.state_buffer_len = read_cb(cb_arg, ctx.state_buffer, len_rd);
		if (ctx.state_buffer_len > 0) {
			return 0;
		}

		LOG_ERR("No settings data read");
		return -ENODATA;
	}

	return -ENOTSUP;
}

static int enable_settings(void)
{
	int err;

	err = settings_subsys_init();
	if (err) {
		LOG_ERR("settings_subsys_init, error: %d", err);
		return err;
	}

	err = settings_load_subtree(settings_handler_bsec.name);
	if (err) {
		LOG_ERR("settings_load_subtree, error: %d", err);
	}

	return err;
}

static int8_t bus_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data_ptr, uint16_t len)
{
	uint8_t buf[len + 1];

	buf[0] = reg_addr;
	memcpy(&buf[1], reg_data_ptr, len);

	return i2c_write(config.i2c_master, buf, len + 1, dev_addr);
}

static int8_t bus_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data_ptr, uint16_t len)
{
	return i2c_write_read(config.i2c_master, dev_addr, &reg_addr, 1, reg_data_ptr, len);
}

static int64_t get_timestamp_us(void)
{
	return k_uptime_get() * MSEC_PER_SEC;
}

static void delay_ms(uint32_t period)
{
	k_sleep(K_MSEC(period));
}

static void output_ready(int64_t timestamp, float iaq, uint8_t iaq_accuracy, float temperature,
			 float humidity, float pressure, float raw_temperature, float raw_humidity,
			 float gas, bsec_library_return_t bsec_status, float static_iaq,
			 float co2_equivalent, float breath_voc_equivalent)
{
	k_spinlock_key_t key = k_spin_lock(&(ctx.sensor_read_lock));

	ctx.temperature_latest = (double)temperature;
	ctx.humidity_latest = (double)humidity;
	ctx.pressure_latest = (double)pressure / 1000;
	ctx.air_quality_latest = (uint16_t)iaq;

	k_spin_unlock(&(ctx.sensor_read_lock), key);
}

static uint32_t state_load(uint8_t *state_buffer, uint32_t n_buffer)
{
	if ((ctx.state_buffer_len > 0) && (ctx.state_buffer_len <= n_buffer)) {
		memcpy(state_buffer, ctx.state_buffer, ctx.state_buffer_len);
		return ctx.state_buffer_len;
	}

	LOG_DBG("No stored state to load");
	return 0;
}

static void state_save(const uint8_t *state_buffer, uint32_t length)
{
	LOG_INF("Storing state to flash");
	if (length > sizeof(ctx.state_buffer)) {
		LOG_ERR("State buffer too big to save: %d", length);
		return;
	}

	int err = settings_save_one(SETTINGS_BSEC_STATE, state_buffer, length);

	if (err) {
		LOG_ERR("Storing state to flash failed");
	}
}

static uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer)
{
	/* Not implemented */
	return 0;
}

static void bsec_thread_fn(void)
{
	bsec_iot_loop(delay_ms, get_timestamp_us, output_ready, state_save, BSEC_SAVE_INTERVAL);
}

/* Public API */

int ext_sensors_bsec_temperature_get(double *temperature)
{
	if (temperature == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&ctx.sensor_read_lock);

	memcpy(temperature, &ctx.temperature_latest, sizeof(ctx.temperature_latest));
	k_spin_unlock(&ctx.sensor_read_lock, key);
	return 0;
}

int ext_sensors_bsec_humidity_get(double *humidity)
{
	if (humidity == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&ctx.sensor_read_lock);

	memcpy(humidity, &ctx.humidity_latest, sizeof(ctx.humidity_latest));
	k_spin_unlock(&ctx.sensor_read_lock, key);
	return 0;
}

int ext_sensors_bsec_pressure_get(double *pressure)
{
	if (pressure == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&ctx.sensor_read_lock);

	memcpy(pressure, &ctx.pressure_latest, sizeof(ctx.pressure_latest));
	k_spin_unlock(&ctx.sensor_read_lock, key);
	return 0;
}

int ext_sensors_bsec_air_quality_get(uint16_t *air_quality)
{
	if (air_quality == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&ctx.sensor_read_lock);

	memcpy(air_quality, &ctx.air_quality_latest, sizeof(ctx.air_quality_latest));
	k_spin_unlock(&ctx.sensor_read_lock, key);
	return 0;
}

int ext_sensors_bsec_init(void)
{
	int err;
	return_values_init bsec_ret;

	err = enable_settings();
	if (err) {
		LOG_ERR("enable_settings, error: %d", err);
		return err;
	}

	if (!device_is_ready(config.i2c_master)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	bsec_ret = bsec_iot_init(BSEC_SAMPLE_RATE, temp_offset, bus_write, bus_read, delay_ms,
				 state_load, config_load);

	if (bsec_ret.bme680_status) {
		LOG_ERR("Could not initialize BME680: %d", bsec_ret.bme680_status);
		return -EIO;
	} else if (bsec_ret.bsec_status) {
		LOG_ERR("Could not initialize BSEC library: %d", bsec_ret.bsec_status);

		if (bsec_ret.bsec_status == BSEC_E_CONFIG_VERSIONMISMATCH) {
			LOG_ERR("Deleting state from flash");
			settings_delete(SETTINGS_BSEC_STATE);
		}

		return -EIO;
	}

	k_thread_create(&ctx.thread,
			thread_stack,
			BSEC_STACK_SIZE,
			(k_thread_entry_t)bsec_thread_fn,
			NULL, NULL, NULL, K_HIGHEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	return 0;
}
