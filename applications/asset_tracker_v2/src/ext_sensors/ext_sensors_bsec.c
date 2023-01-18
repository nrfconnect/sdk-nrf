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

#include "bsec_interface.h"
#include "bme68x.h"
#include "ext_sensors_bsec.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ext_sensors_bsec, CONFIG_EXTERNAL_SENSORS_LOG_LEVEL);

#define BSEC_TOTAL_HEAT_DUR		UINT16_C(140)
#define BSEC_INPUT_PRESENT(x, shift)	(x.process_data & (1 << (shift - 1)))

/* Sample rate for the BSEC library
 *
 * BSEC_SAMPLE_RATE_ULP = 0.0033333 Hz = 300 second interval
 * BSEC_SAMPLE_RATE_LP = 0.33333 Hz = 3 second interval
 * BSEC_SAMPLE_RATE_HP = 1 Hz = 1 second interval
 */
#if defined(CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_ULTRA_LOW_POWER)
#define BSEC_SAMPLE_RATE BSEC_SAMPLE_RATE_ULP
#define BSEC_SAMPLE_PERIOD_S 300
#elif defined(CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_LOW_POWER)
#define BSEC_SAMPLE_RATE BSEC_SAMPLE_RATE_LP
#define BSEC_SAMPLE_PERIOD_S 3
#elif defined(CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_CONTINUOUS)
#define BSEC_SAMPLE_RATE BSEC_SAMPLE_RATE_CONT
#define BSEC_SAMPLE_PERIOD_S 1
#endif

/* Temperature offset due to external heat sources. */
float temp_offset = (CONFIG_EXTERNAL_SENSORS_BSEC_TEMPERATURE_OFFSET / (float)100);

/* Define which sensor values to request.
 * The order is not important, but output_ready needs to be updated if different types
 * of sensor values are requested.
 */
static const bsec_sensor_configuration_t requested_virtual_sensors[4] = {
		{
			.sensor_id   = BSEC_OUTPUT_IAQ,
			.sample_rate = BSEC_SAMPLE_RATE,
		},
		{
			.sensor_id   = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
			.sample_rate = BSEC_SAMPLE_RATE,
		},
		{
			.sensor_id   = BSEC_OUTPUT_RAW_PRESSURE,
			.sample_rate = BSEC_SAMPLE_RATE,
		},
		{
			.sensor_id   = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
			.sample_rate = BSEC_SAMPLE_RATE,
		},
	};

/* some RAM space needed by bsec_get_state and bsec_set_state for (de-)serialization. */
static uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE];

/* @breif Interval for saving BSEC state to flash
 *
 * Interval = BSEC_SAVE_INTERVAL * 1/BSEC_SAMPLE_RATE
 * Example:  600 * 1/0.33333 Hz = 1800 seconds = 0.5 hour
 */
#define BSEC_SAVE_INTERVAL 600
#define BSEC_SAVE_INTERVAL_S (BSEC_SAVE_INTERVAL * BSEC_SAMPLE_PERIOD_S)

/* Definitions used to store and retrieve BSEC state from the settings API */
#define SETTINGS_NAME_BSEC "bsec"
#define SETTINGS_KEY_STATE "state"
#define SETTINGS_BSEC_STATE SETTINGS_NAME_BSEC "/" SETTINGS_KEY_STATE

/* Stack size of internal BSEC thread. */
#define BSEC_STACK_SIZE CONFIG_EXTERNAL_SENSORS_BSEC_THREAD_STACK_SIZE
static K_THREAD_STACK_DEFINE(thread_stack, BSEC_STACK_SIZE);

/* Used for a timeout for when BSEC's state should be saved. */
static K_TIMER_DEFINE(bsec_save_state_timer, NULL, NULL);

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

	bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
	uint8_t n_required_sensor_settings;

	struct bme68x_dev dev;
};

const static struct i2c_dt_spec bme680_i2c_dev = I2C_DT_SPEC_GET(DT_NODELABEL(bme680));

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

	LOG_DBG("Settings key: %s, size: %d", key, len_rd);

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

static void state_save(void)
{
	int ret;

	ret = bsec_get_state(0, ctx.state_buffer, ARRAY_SIZE(ctx.state_buffer),
				work_buffer, ARRAY_SIZE(work_buffer),
				&ctx.state_buffer_len);

	__ASSERT(ret == BSEC_OK, "bsec_get_state failed.");
	__ASSERT(ctx.state_buffer_len <= sizeof(ctx.state_buffer), "state buffer too big to save.");

	ret = settings_save_one(SETTINGS_BSEC_STATE, ctx.state_buffer,
					ctx.state_buffer_len);


	__ASSERT(ret == 0, "storing state to flash failed.");
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

static int8_t bus_write(uint8_t reg_addr, const uint8_t *reg_data_ptr, uint32_t len, void *intf_ptr)
{
	uint8_t buf[len + 1];

	buf[0] = reg_addr;
	memcpy(&buf[1], reg_data_ptr, len);

	return i2c_write_dt(&bme680_i2c_dev, buf, len + 1);
}

static int8_t bus_read(uint8_t reg_addr, uint8_t *reg_data_ptr, uint32_t len, void *intf_ptr)
{
	return i2c_write_read_dt(&bme680_i2c_dev, &reg_addr, 1, reg_data_ptr, len);
}

static void delay_us(uint32_t period, void *intf_ptr)
{
	k_usleep((int32_t) period);
}

static void output_ready(const bsec_output_t *outputs, uint8_t n_outputs)
{
	k_spinlock_key_t key = k_spin_lock(&(ctx.sensor_read_lock));

	for (size_t i = 0; i < n_outputs; ++i) {
		switch (outputs[i].sensor_id) {
		case BSEC_OUTPUT_IAQ:
			ctx.air_quality_latest = (uint16_t) outputs[i].signal;
			LOG_DBG("IAQ: %d", ctx.air_quality_latest);
			break;
		case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
			ctx.temperature_latest = (double) outputs[i].signal;
			LOG_DBG("Temp: %.2f", ctx.temperature_latest);
			break;
		case BSEC_OUTPUT_RAW_PRESSURE:
			ctx.pressure_latest = (double) outputs[i].signal;
			LOG_DBG("Press: %.2f", ctx.pressure_latest);
			break;
		case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
			ctx.humidity_latest = (double) outputs[i].signal;
			LOG_DBG("Hum: %.2f", ctx.humidity_latest);
			break;
		default:
			LOG_WRN("unknown bsec output id: %d", outputs[i].sensor_id);
			break;
		}
	}

	k_spin_unlock(&(ctx.sensor_read_lock), key);
}

static size_t sensor_data_to_bsec_inputs(bsec_bme_settings_t sensor_settings,
					 const struct bme68x_data *data,
					 bsec_input_t *inputs, uint64_t timestamp_ns)
{
	size_t i = 0;

	if (BSEC_INPUT_PRESENT(sensor_settings, BSEC_INPUT_TEMPERATURE)) {
		inputs[i].sensor_id = BSEC_INPUT_HEATSOURCE;
		inputs[i].signal = temp_offset;
		inputs[i].time_stamp = timestamp_ns;
		LOG_DBG("Temp offset: %.2f", inputs[i].signal);
		i++;
		inputs[i].sensor_id = BSEC_INPUT_TEMPERATURE;
		inputs[i].signal = data->temperature;

		if (!IS_ENABLED(BME68X_USE_FPU)) {
			inputs[i].signal /= 100.0f;
		}

		inputs[i].time_stamp = timestamp_ns;
		LOG_DBG("Temp: %.2f", inputs[i].signal);
		i++;
	}
	if (BSEC_INPUT_PRESENT(sensor_settings, BSEC_INPUT_HUMIDITY)) {
		inputs[i].sensor_id = BSEC_INPUT_HUMIDITY;
		inputs[i].signal =  data->humidity;

		if (!IS_ENABLED(BME68X_USE_FPU)) {
			inputs[i].signal /= 1000.0f;
		}

		inputs[i].time_stamp = timestamp_ns;
		LOG_DBG("Hum: %.2f", inputs[i].signal);
		i++;
	}
	if (BSEC_INPUT_PRESENT(sensor_settings, BSEC_INPUT_PRESSURE)) {
		inputs[i].sensor_id = BSEC_INPUT_PRESSURE;
		inputs[i].signal =  data->pressure;
		inputs[i].time_stamp = timestamp_ns;
		LOG_DBG("Press: %.2f", inputs[i].signal);
		i++;
	}
	if (BSEC_INPUT_PRESENT(sensor_settings, BSEC_INPUT_GASRESISTOR)) {
		inputs[i].sensor_id = BSEC_INPUT_GASRESISTOR;
		inputs[i].signal =  data->gas_resistance;
		inputs[i].time_stamp = timestamp_ns;
		LOG_DBG("Gas: %.2f", inputs[i].signal);
		i++;
	}
	if (BSEC_INPUT_PRESENT(sensor_settings, BSEC_INPUT_PROFILE_PART)) {
		inputs[i].sensor_id = BSEC_INPUT_PROFILE_PART;
		if (sensor_settings.op_mode == BME68X_FORCED_MODE) {
			inputs[i].signal =  0;
		} else {
			inputs[i].signal =  data->gas_index;
		}
		inputs[i].time_stamp = timestamp_ns;
		LOG_DBG("Profile: %.2f", inputs[i].signal);
		i++;
	}
	return i;
}

static int apply_sensor_settings(bsec_bme_settings_t sensor_settings)
{
	int ret;
	struct bme68x_conf config = {0};
	struct bme68x_heatr_conf heater_config = {0};

	heater_config.enable = BME68X_ENABLE;
	heater_config.heatr_temp = sensor_settings.heater_temperature;
	heater_config.heatr_dur = sensor_settings.heater_duration;
	heater_config.heatr_temp_prof = sensor_settings.heater_temperature_profile;
	heater_config.heatr_dur_prof = sensor_settings.heater_duration_profile;
	heater_config.profile_len = sensor_settings.heater_profile_len;
	heater_config.shared_heatr_dur = 0;

	switch (sensor_settings.op_mode) {
	case BME68X_PARALLEL_MODE:
		heater_config.shared_heatr_dur =
			BSEC_TOTAL_HEAT_DUR -
			(bme68x_get_meas_dur(sensor_settings.op_mode, &config, &ctx.dev)
			/ INT64_C(1000));

		/* fall through */
	case BME68X_FORCED_MODE:
		ret = bme68x_get_conf(&config, &ctx.dev);
		if (ret) {
			LOG_ERR("bme68x_get_conf err: %d", ret);
			return ret;
		}

		config.os_hum = sensor_settings.humidity_oversampling;
		config.os_temp = sensor_settings.temperature_oversampling;
		config.os_pres = sensor_settings.pressure_oversampling;

		ret = bme68x_set_conf(&config, &ctx.dev);
		if (ret) {
			LOG_ERR("bme68x_set_conf err: %d", ret);
			return ret;
		}

		bme68x_set_heatr_conf(sensor_settings.op_mode,
					&heater_config, &ctx.dev);

		/* fall through */
	case BME68X_SLEEP_MODE:
		ret = bme68x_set_op_mode(sensor_settings.op_mode, &ctx.dev);
		if (ret) {
			LOG_ERR("bme68x_set_op_mode err: %d", ret);
			return ret;
		}
		break;
	default:
		LOG_ERR("unknown op mode: %d", sensor_settings.op_mode);
	}
	return 0;
}

static void bsec_thread_fn(void)
{
	int ret;
	struct bme68x_data sensor_data[3] = {0};
	bsec_bme_settings_t sensor_settings = {0};
	bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR] = {0};
	bsec_output_t outputs[ARRAY_SIZE(requested_virtual_sensors)] = {0};
	uint8_t n_fields = 0;
	uint8_t n_outputs = 0;
	uint8_t n_inputs = 0;

	while (true) {
		uint64_t timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());

		if (timestamp_ns < sensor_settings.next_call) {
			LOG_DBG("bsec_sensor_control not ready yet");
			k_sleep(K_NSEC(sensor_settings.next_call - timestamp_ns));
			continue;
		}

		memset(&sensor_settings, 0, sizeof(sensor_settings));
		ret = bsec_sensor_control((int64_t)timestamp_ns, &sensor_settings);
		if (ret != BSEC_OK) {
			LOG_ERR("bsec_sensor_control err: %d", ret);
			continue;
		}

		if (apply_sensor_settings(sensor_settings)) {
			continue;
		}

		if (sensor_settings.trigger_measurement &&
		    sensor_settings.op_mode != BME68X_SLEEP_MODE) {
			ret = bme68x_get_data(sensor_settings.op_mode,
					      sensor_data, &n_fields, &ctx.dev);
			if (ret) {
				LOG_DBG("bme68x_get_data err: %d", ret);
				continue;
			}

			for (size_t i = 0; i < n_fields; ++i) {
				n_outputs = ARRAY_SIZE(requested_virtual_sensors);
				n_inputs = sensor_data_to_bsec_inputs(sensor_settings,
								      &sensor_data[i],
								      inputs, timestamp_ns);

				if (n_inputs > 0) {
					ret = bsec_do_steps(inputs, n_inputs, outputs, &n_outputs);
					if (ret != BSEC_OK) {
						LOG_ERR("bsec_do_steps err: %d", ret);
						continue;
					}
					output_ready(outputs, n_outputs);
				}
			}

		}

		/* if save timer is expired, save and restart timer */
		if (k_timer_remaining_get(&bsec_save_state_timer) == 0) {
			state_save();
			k_timer_start(&bsec_save_state_timer,
				      K_SECONDS(BSEC_SAVE_INTERVAL_S),
				      K_NO_WAIT);
		}

		k_sleep(K_SECONDS(BSEC_SAMPLE_PERIOD_S));
	}

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

	err = enable_settings();
	if (err) {
		LOG_ERR("enable_settings, error: %d", err);
		return err;
	}

	if (!device_is_ready(bme680_i2c_dev.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	ctx.dev.intf = BME68X_I2C_INTF;
	ctx.dev.intf_ptr = NULL;
	ctx.dev.read = bus_read;
	ctx.dev.write = bus_write;
	ctx.dev.delay_us = delay_us;
	ctx.dev.amb_temp = 25;

	err = bme68x_init(&ctx.dev);
	if (err) {
		LOG_ERR("Failed to init bme68x: %d", err);
		return err;
	}

	err = bsec_init();
	if (err != BSEC_OK) {
		LOG_ERR("Failed to init BSEC: %d", err);
		return err;
	}

	err = bsec_set_state(ctx.state_buffer, ctx.state_buffer_len,
			     work_buffer, ARRAY_SIZE(work_buffer));
	if (err != BSEC_OK && err != BSEC_E_CONFIG_EMPTY) {
		LOG_ERR("Failed to set BSEC state: %d", err);
	} else if (err == BSEC_OK) {
		LOG_DBG("Setting BSEC state successful.");
	}

	bsec_update_subscription(requested_virtual_sensors, ARRAY_SIZE(requested_virtual_sensors),
				 ctx.required_sensor_settings, &ctx.n_required_sensor_settings);

	k_thread_create(&ctx.thread,
			thread_stack,
			BSEC_STACK_SIZE,
			(k_thread_entry_t)bsec_thread_fn,
			NULL, NULL, NULL, K_HIGHEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	k_timer_start(&bsec_save_state_timer,
		      K_SECONDS(BSEC_SAVE_INTERVAL_S),
		      K_NO_WAIT);

	return 0;
}
