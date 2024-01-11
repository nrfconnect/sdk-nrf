/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* NCS Integration for BME68X + BSEC */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/settings/settings.h>
#include <zephyr/drivers/sensor.h>

#include "bme68x_iaq.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bsec, CONFIG_BME68X_IAQ_LOG_LEVEL);


#define BSEC_TOTAL_HEAT_DUR		UINT16_C(140)
#define BSEC_INPUT_PRESENT(x, shift)	(x.process_data & (1 << (shift - 1)))

/* Temperature offset due to external heat sources. */
static const float temp_offset = (CONFIG_BME68X_IAQ_TEMPERATURE_OFFSET / (float)100);

/* Define which sensor values to request.
 * The order is not important, but output_ready needs to be updated if different types
 * of sensor values are requested.
 */
static const bsec_sensor_configuration_t bsec_requested_virtual_sensors[4] = {
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


/* Definitions used to store and retrieve BSEC state from the settings API */
#define SETTINGS_NAME_BSEC "bsec"
#define SETTINGS_KEY_STATE "state"
#define SETTINGS_BSEC_STATE SETTINGS_NAME_BSEC "/" SETTINGS_KEY_STATE

/* Stack size of internal BSEC thread. */
static K_THREAD_STACK_DEFINE(thread_stack, CONFIG_BME68X_IAQ_THREAD_STACK_SIZE);

/* Used for a timeout for when BSEC's state should be saved. */
static K_TIMER_DEFINE(bsec_save_state_timer, NULL, NULL);

/* Bus spec for BME68x sensor */
#if BME68x_BUS_SPI
static struct spi_dt_spec bme68x_spi_spec;
#elif BME68x_BUS_I2C
static struct i2c_dt_spec bme68x_i2c_spec;
#endif

/* Semaphore to make sure output data isn't read while being updated */
static K_SEM_DEFINE(output_sem, 1, 1);

static int settings_load_handler(const char *key, size_t len,
				 settings_read_cb read_cb, void *cb_arg, void *param)
{
	ARG_UNUSED(key);
	struct bme68x_iaq_data *data = param;

	if (len > ARRAY_SIZE(data->state_buffer)) {
		return -EINVAL;
	}

	data->state_len = read_cb(cb_arg, data->state_buffer, len);

	if (data->state_len > 0) {
		return 0;
	}

	LOG_WRN("No settings data read");
	return -ENODATA;
}


/* Export current state of BSEC and save it to flash. */
static void state_save(const struct device *dev)
{
	int ret;
	struct bme68x_iaq_data *data = dev->data;

	LOG_DBG("saving state to flash");

	ret = bsec_get_state(0, data->state_buffer, ARRAY_SIZE(data->state_buffer),
				data->work_buffer, ARRAY_SIZE(data->work_buffer),
				&data->state_len);

	__ASSERT(ret == BSEC_OK, "bsec_get_state failed.");
	__ASSERT(data->state_len <= sizeof(data->state_buffer), "state buffer too big to save.");

	ret = settings_save_one(SETTINGS_BSEC_STATE, data->state_buffer, data->state_len);

	__ASSERT(ret == 0, "storing state to flash failed.");
}

/* I2C bus write forwarder for bme68x driver */
static int8_t bus_write(uint8_t reg_addr, const uint8_t *reg_data_ptr, uint32_t len, void *intf_ptr)
{
#if BME68x_BUS_SPI
	const struct spi_buf tx_buf[2] = {
		{
			.buf = &reg_addr,
			.len = sizeof(reg_addr),
		},
		{
			.buf = (uint8_t *)reg_data_ptr,
			.len = len,
		},
	};

	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	return spi_write_dt(&bme68x_spi_spec, &tx);

#elif BME68x_BUS_I2C
	uint8_t buf[len + 1];

	buf[0] = reg_addr;
	memcpy(&buf[1], reg_data_ptr, len);

	return i2c_write_dt(&bme68x_i2c_spec, buf, ARRAY_SIZE(buf));

#endif
}

/* I2C bus read forwarder for bme68x driver */
static int8_t bus_read(uint8_t reg_addr, uint8_t *reg_data_ptr, uint32_t len, void *intf_ptr)
{
#if BME68x_BUS_SPI
	const struct spi_buf tx_buf = {
		.buf = &reg_addr,
		.len = 1,
	};
	const struct spi_buf rx_buf[2] = {
		{.buf = NULL, .len = 1},
		{.buf = reg_data_ptr, .len = len},
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	return spi_transceive_dt(&bme68x_spi_spec, &tx, &rx);

#elif BME68x_BUS_I2C
	return i2c_write_read_dt(&bme68x_i2c_spec, &reg_addr, 1, reg_data_ptr, len);
#endif
}

/* delay function for bme68x driver */
static void delay_us(uint32_t period, void *intf_ptr)
{
	k_usleep((int32_t) period);
}

/* function to handle output of BSEC */
static void output_ready(const struct device *dev, const bsec_output_t *outputs, uint8_t n_outputs)
{
	struct bme68x_iaq_data *data = dev->data;

	k_sem_take(&output_sem, K_FOREVER);
	for (size_t i = 0; i < n_outputs; ++i) {
		switch (outputs[i].sensor_id) {
		case BSEC_OUTPUT_IAQ:
			data->latest.air_quality = (uint16_t) outputs[i].signal;
			LOG_DBG("IAQ: %d", data->latest.air_quality);
			break;
		case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
			data->latest.temperature = (double) outputs[i].signal;
			LOG_DBG("Temp: %.2f C", data->latest.temperature);
			break;
		case BSEC_OUTPUT_RAW_PRESSURE:
			data->latest.pressure = (double) outputs[i].signal;
			LOG_DBG("Press: %.2f Pa", data->latest.pressure);
			break;
		case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
			data->latest.humidity = (double) outputs[i].signal;
			LOG_DBG("Hum: %.2f %%", data->latest.humidity);
			break;
		default:
			LOG_WRN("unknown bsec output id: %d", outputs[i].sensor_id);
			break;
		}
	}
	k_sem_give(&output_sem);
	if (data->trg_handler != NULL) {
		data->trg_handler(dev, data->trigger);
	}
}

/* convert raw bme68x output to valid input for BSEC */
static size_t sensor_data_to_bsec_inputs(bsec_bme_settings_t sensor_settings,
					 const struct bme68x_data *data,
					 bsec_input_t *inputs, uint64_t timestamp_ns)
{
	size_t i = 0;

	if (BSEC_INPUT_PRESENT(sensor_settings, BSEC_INPUT_TEMPERATURE)) {
		/* append heatsource input */
		inputs[i].sensor_id = BSEC_INPUT_HEATSOURCE;
		inputs[i].signal = temp_offset;
		inputs[i].time_stamp = timestamp_ns;
		LOG_DBG("Temp offset: %.2f", inputs[i].signal);
		i++;

		/* append temperature input */
		inputs[i].sensor_id = BSEC_INPUT_TEMPERATURE;
		inputs[i].signal = data->temperature;

		if (IS_ENABLED(BME68X_DO_NOT_USE_FPU)) {
			/* in this config, temperature is output in centidegrees */
			inputs[i].signal /= 100.0f;
		}

		inputs[i].time_stamp = timestamp_ns;
		LOG_DBG("Temp: %.2f", inputs[i].signal);
		i++;
	}
	if (BSEC_INPUT_PRESENT(sensor_settings, BSEC_INPUT_HUMIDITY)) {
		inputs[i].sensor_id = BSEC_INPUT_HUMIDITY;
		inputs[i].signal =  data->humidity;

		if (IS_ENABLED(BME68X_DO_NOT_USE_FPU)) {
			/* in this config, humidity is output in millipercent */
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

/* convert and apply bme68x settings chosen by BSEC */
static int apply_sensor_settings(const struct device *dev, bsec_bme_settings_t sensor_settings)
{
	int ret;
	struct bme68x_conf config = {0};
	struct bme68x_heatr_conf heater_config = {0};
	struct bme68x_iaq_data *data = dev->data;

	heater_config.enable = BME68X_ENABLE;
	heater_config.heatr_temp = sensor_settings.heater_temperature;
	heater_config.heatr_dur = sensor_settings.heater_duration;
	heater_config.heatr_temp_prof = sensor_settings.heater_temperature_profile;
	heater_config.heatr_dur_prof = sensor_settings.heater_duration_profile;
	heater_config.profile_len = sensor_settings.heater_profile_len;
	heater_config.shared_heatr_dur = 0;

	switch (sensor_settings.op_mode) {
	case BME68X_PARALLEL_MODE:
		/* this block is only executed for BME68X_PARALLEL_MODE */
		/* shared heating duration in milliseconds (converted from microseconds) */
		heater_config.shared_heatr_dur =
			BSEC_TOTAL_HEAT_DUR -
			(bme68x_get_meas_dur(sensor_settings.op_mode, &config, &data->dev)
			/ INT64_C(1000));

		__fallthrough;
	case BME68X_FORCED_MODE:
		/* this block is executed for any measurement mode */
		ret = bme68x_get_conf(&config, &data->dev);
		if (ret) {
			LOG_ERR("bme68x_get_conf err: %d", ret);
			return ret;
		}

		config.os_hum = sensor_settings.humidity_oversampling;
		config.os_temp = sensor_settings.temperature_oversampling;
		config.os_pres = sensor_settings.pressure_oversampling;

		ret = bme68x_set_conf(&config, &data->dev);
		if (ret) {
			LOG_ERR("bme68x_set_conf err: %d", ret);
			return ret;
		}

		bme68x_set_heatr_conf(sensor_settings.op_mode, &heater_config, &data->dev);

		__fallthrough;
	case BME68X_SLEEP_MODE:
		/* this block is executed for all modes */
		ret = bme68x_set_op_mode(sensor_settings.op_mode, &data->dev);
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

static void fetch_and_process_output(const struct device *dev,
				     bsec_bme_settings_t *sensor_settings,
				     uint64_t timestamp_ns)
{
	uint8_t n_fields = 0;
	uint8_t n_outputs = 0;
	uint8_t n_inputs = 0;
	bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR] = {0};
	bsec_output_t outputs[ARRAY_SIZE(bsec_requested_virtual_sensors)] = {0};
	struct bme68x_data sensor_data[3] = {0};
	struct bme68x_iaq_data *data = dev->data;
	int ret = bme68x_get_data(sensor_settings->op_mode, sensor_data, &n_fields, &data->dev);

	if (ret) {
		LOG_DBG("bme68x_get_data err: %d", ret);
		return;
	}

	for (size_t i = 0; i < n_fields; ++i) {
		n_outputs = ARRAY_SIZE(bsec_requested_virtual_sensors);
		n_inputs = sensor_data_to_bsec_inputs(*sensor_settings,
							sensor_data + i,
							inputs, timestamp_ns);

		if (n_inputs == 0) {
			continue;
		}
		ret = bsec_do_steps(inputs, n_inputs, outputs, &n_outputs);
		if (ret != BSEC_OK) {
			LOG_ERR("bsec_do_steps err: %d", ret);
			continue;
		}
		output_ready(dev, outputs, n_outputs);
	}
}

/* Manage all recurrings tasks for the sensor:
 * - update device settings according to BSEC
 * - fetch measurement values
 * - update BSEC state
 * - periodically save BSEC state to flash
 */
static void bsec_thread_fn(const struct device *dev)
{
	int ret;
	bsec_bme_settings_t sensor_settings = {0};

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

		if (apply_sensor_settings(dev, sensor_settings)) {
			continue;
		}

		if (sensor_settings.trigger_measurement &&
		    sensor_settings.op_mode != BME68X_SLEEP_MODE) {
			fetch_and_process_output(dev, &sensor_settings, timestamp_ns);
		}

		/* if save timer is expired, save and restart timer */
		if (k_timer_remaining_get(&bsec_save_state_timer) == 0) {
			state_save(dev);
			k_timer_start(&bsec_save_state_timer,
				      K_MINUTES(CONFIG_BME68X_IAQ_SAVE_INTERVAL_MINUTES),
				      K_NO_WAIT);
		}

		k_sleep(K_SECONDS(BSEC_SAMPLE_PERIOD_S));
	}

}

static int bme68x_bsec_init(const struct device *dev)
{
	int err;
	struct bme68x_iaq_data *data = dev->data;
	const struct bme68x_iaq_config *config = dev->config;

#if BME68x_BUS_SPI
	bme68x_spi_spec = config->spi;
#elif BME68x_BUS_I2C
	bme68x_i2c_spec = config->i2c;
#endif
	err = settings_subsys_init();
	if (err) {
		LOG_ERR("settings_subsys_init, error: %d", err);
		return err;
	}

	err = settings_load_subtree_direct(SETTINGS_BSEC_STATE, settings_load_handler, data);
	if (err) {
		LOG_ERR("settings_load_subtree, error: %d", err);
		return err;
	}

#if BME68x_BUS_SPI
	if (!spi_is_ready_dt(&bme68x_spi_spec)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}
	data->dev.intf = BME68X_SPI_INTF;
#elif BME68x_BUS_I2C
	if (!device_is_ready(bme68x_i2c_spec.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}
	data->dev.intf = BME68X_I2C_INTF;
#endif

	data->dev.intf_ptr = NULL;
	data->dev.read = bus_read;
	data->dev.write = bus_write;
	data->dev.delay_us = delay_us;
	data->dev.amb_temp = CONFIG_BME68X_IAQ_EXPECTED_AMBIENT_TEMP;

	err = bme68x_init(&data->dev);
	if (err) {
		LOG_ERR("Failed to init bme68x: %d", err);
		return err;
	}

	err = bsec_init();
	if (err != BSEC_OK) {
		LOG_ERR("Failed to init BSEC: %d", err);
		return err;
	}

	err = bsec_set_state(data->state_buffer, data->state_len,
			     data->work_buffer, ARRAY_SIZE(data->work_buffer));
	if (err != BSEC_OK && err != BSEC_E_CONFIG_EMPTY) {
		LOG_ERR("Failed to set BSEC state: %d", err);
	} else if (err == BSEC_OK) {
		LOG_DBG("Setting BSEC state successful.");
	}

	bsec_update_subscription(bsec_requested_virtual_sensors,
				 ARRAY_SIZE(bsec_requested_virtual_sensors),
				 data->required_sensor_settings, &data->n_required_sensor_settings);

	k_thread_create(&data->thread,
			thread_stack,
			CONFIG_BME68X_IAQ_THREAD_STACK_SIZE,
			(k_thread_entry_t)bsec_thread_fn,
			(void *)dev, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	k_timer_start(&bsec_save_state_timer,
		      K_MINUTES(CONFIG_BME68X_IAQ_SAVE_INTERVAL_MINUTES),
		      K_NO_WAIT);
	return 0;
}

static int bme68x_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct bme68x_iaq_data *data = dev->data;

	if (trig->type != SENSOR_TRIG_TIMER) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	if ((trig->chan == SENSOR_CHAN_ALL)
	    || (trig->chan == SENSOR_CHAN_HUMIDITY)
	    || (trig->chan == SENSOR_CHAN_AMBIENT_TEMP)
	    || (trig->chan == SENSOR_CHAN_PRESS)
	    || (trig->chan == SENSOR_CHAN_IAQ)) {
		data->trigger = trig;
		data->trg_handler = handler;
	} else {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}
	return 0;
}

static int bme68x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	/* fetching is a requirement for the API */
	return 0;
}

static int bme68x_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bme68x_iaq_data *data = dev->data;
	int result = 0;

	k_sem_take(&output_sem, K_FOREVER);
	if (chan == SENSOR_CHAN_HUMIDITY) {
		sensor_value_from_double(val, data->latest.humidity);
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		sensor_value_from_double(val, data->latest.temperature);
	} else if (chan == SENSOR_CHAN_PRESS) {
		sensor_value_from_double(val, data->latest.pressure);
	} else if (chan == SENSOR_CHAN_IAQ) {
		val->val1 = data->latest.air_quality;
		val->val2 = 0;
	} else {
		LOG_ERR("Unsupported sensor channel");
		result = -ENOTSUP;
	}
	k_sem_give(&output_sem);
	return result;
}

static const struct sensor_driver_api bme68x_driver_api = {
	.sample_fetch = &bme68x_sample_fetch,
	.channel_get = &bme68x_channel_get,
	.trigger_set = bme68x_trigger_set,
};

/* there can be only one device supported here because of BSECs internal state */
static struct bme68x_iaq_config config_0 = {
#if BME68x_BUS_SPI
	.spi = SPI_DT_SPEC_INST_GET(0, BME68x_SPI_OPERATION, 0),
#elif BME68x_BUS_I2C
	.i2c = I2C_DT_SPEC_INST_GET(0),
#endif
};
static struct bme68x_iaq_data data_0;

SENSOR_DEVICE_DT_INST_DEFINE(0, bme68x_bsec_init, NULL,
			      &data_0,
			      &config_0,
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
			      &bme68x_driver_api);
