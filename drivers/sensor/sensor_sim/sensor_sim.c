/*
 * Copyright (c) 2018-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT nordic_sensor_sim

#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <drivers/sensor_sim.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensor_sim, CONFIG_SENSOR_LOG_LEVEL);

#define ACCEL_CHAN_COUNT 3

enum acc_signal {
	ACC_SIGNAL_TOGGLE,
	ACC_SIGNAL_WAVE,
};

struct sensor_sim_data {
	const struct device *dev;
	double temp_sample;
	double humidity_sample;
	double pressure_sample;
	double accel_samples[ACCEL_CHAN_COUNT];
	struct wave_gen_param accel_param[ACCEL_CHAN_COUNT];
	struct k_mutex accel_param_mutex;
	double val_sign;
#if defined(CONFIG_SENSOR_SIM_TRIGGER)
	sensor_trigger_handler_t drdy_handler;
	struct sensor_trigger drdy_trigger;

	K_THREAD_STACK_MEMBER(thread_stack,
			      CONFIG_SENSOR_SIM_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct gpio_callback gpio_cb;
	struct k_sem gpio_sem;
#endif /* CONFIG_SENSOR_SIM_TRIGGER */
};

struct sensor_sim_config {
	uint32_t base_temperature;
	uint8_t base_humidity;
	uint32_t base_pressure;
	enum acc_signal acc_signal;
	struct wave_gen_param acc_param;
	double acc_toggle_amplitude;
#if defined(CONFIG_SENSOR_SIM_TRIGGER)
	struct gpio_dt_spec trigger_gpio;
	uint32_t trigger_timeout;
#endif
};

/**
 * @typedef generator_function
 * @brief Function used to generate sensor value for given channel.
 *
 * @param[in]	dev	Sensor device instance.
 * @param[in]	chan	Selected sensor channel.
 * @param[in]	val_cnt	Number of generated values.
 * @param[out]	out_val	Pointer to the variable that is used to store result.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
typedef int (*generator_function)(const struct device *dev,
				  enum sensor_channel chan, size_t val_cnt,
				  double *out_val);

/**
 * @brief Function used to get wave parameters for given sensor channel.
 *
 * @param[in]	dev	Sensor device instance.
 * @param[in]	chan	Selected sensor channel.
 *
 * @return Pointer to the structure describing parameters of generated wave.
 */
static struct wave_gen_param *get_wave_params(const struct device *dev,
					      enum sensor_channel chan)
{
	struct sensor_sim_data *data = dev->data;
	struct wave_gen_param *dest = NULL;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_XYZ:
		dest = &data->accel_param[0];
		break;
	case SENSOR_CHAN_ACCEL_Y:
		dest = &data->accel_param[1];
		break;
	case SENSOR_CHAN_ACCEL_Z:
		dest = &data->accel_param[2];
		break;
	default:
		break;
	}

	return dest;
}

int sensor_sim_set_wave_param(const struct device *dev,
			      enum sensor_channel chan,
			      const struct wave_gen_param *set_params)
{
	struct sensor_sim_data *data = dev->data;
	const struct sensor_sim_config *config = dev->config;

	if (config->acc_signal != ACC_SIGNAL_WAVE) {
		return -ENOTSUP;
	}

	struct wave_gen_param *dest = get_wave_params(dev, chan);

	if (!dest) {
		return -ENOTSUP;
	}

	if (set_params->type >= WAVE_GEN_TYPE_COUNT) {
		return -EINVAL;
	}

	if ((set_params->type != WAVE_GEN_TYPE_NONE) && (set_params->period_ms == 0)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->accel_param_mutex, K_FOREVER);

	memcpy(dest, set_params, sizeof(*dest));

	if (chan == SENSOR_CHAN_ACCEL_XYZ) {
		memcpy(get_wave_params(dev, SENSOR_CHAN_ACCEL_Y), set_params,
				       sizeof(*dest));
		memcpy(get_wave_params(dev, SENSOR_CHAN_ACCEL_Z), set_params,
				       sizeof(*dest));
	}

	k_mutex_unlock(&data->accel_param_mutex);

	return 0;
}

#if defined(CONFIG_SENSOR_SIM_TRIGGER)
/**
 * @brief Callback for GPIO when using button as trigger.
 *
 * @param port Pointer to device structure.
 * @param cb Pointer to GPIO callback structure.
 * @param pins Pin mask for callback.
 */
static void sensor_sim_gpio_callback(const struct device *port,
				     struct gpio_callback *cb, uint32_t pins)
{
	struct sensor_sim_data *data = CONTAINER_OF(cb, struct sensor_sim_data,
						    gpio_cb);
	const struct device *dev = data->dev;
	const struct sensor_sim_config *config = dev->config;

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&config->trigger_gpio, GPIO_INT_DISABLE);
	k_sem_give(&data->gpio_sem);
}

/**
 * @brief Function that runs in the sensor simulator thread when using trigger.
 *
 * @param p1 Thread parameter 1 (device instance).
 * @param p2 Thread parameter 2 (unused).
 * @param p3 Thread parameter 3 (unused).
 */
static void sensor_sim_thread(void *p1, void *p2, void *p3)
{
	struct device *dev = p1;
	struct sensor_sim_data *data = dev->data;
	const struct sensor_sim_config *config = dev->config;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		if (config->trigger_gpio.port != NULL) {
			k_sem_take(&data->gpio_sem, K_FOREVER);
		} else {
			k_sleep(K_MSEC(config->trigger_timeout));
		}

		if (data->drdy_handler != NULL) {
			data->drdy_handler(dev, &data->drdy_trigger);
		}

		if (config->trigger_gpio.port != NULL) {
			gpio_pin_interrupt_configure_dt(&config->trigger_gpio,
							GPIO_INT_EDGE_FALLING);
		}
	}
}

/**
 * @brief Initializing thread when simulator uses trigger
 *
 * @param dev Pointer to device instance.
 */
static int sensor_sim_init_thread(const struct device *dev)
{
	struct sensor_sim_data *data = dev->data;
	const struct sensor_sim_config *config = dev->config;
	int ret;

	if (config->trigger_gpio.port != NULL) {
		if (!device_is_ready(config->trigger_gpio.port)) {
			LOG_ERR("GPIO port device not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->trigger_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure GPIO pin");
			return ret;
		}

		gpio_init_callback(&data->gpio_cb, sensor_sim_gpio_callback,
				BIT(config->trigger_gpio.pin));

		ret = gpio_add_callback(config->trigger_gpio.port,
					&data->gpio_cb);
		if (ret < 0) {
			LOG_ERR("Failed to set GPIO callback");
			return ret;
		}

		k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);
	}

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_SENSOR_SIM_THREAD_STACK_SIZE,
			sensor_sim_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_SENSOR_SIM_THREAD_PRIORITY),
			0, K_NO_WAIT);

	return 0;
}

static int sensor_sim_trigger_set(const struct device *dev,
			   const struct sensor_trigger *trig,
			   sensor_trigger_handler_t handler)
{
	struct sensor_sim_data *data = dev->data;
	const struct sensor_sim_config *config = dev->config;

	if (config->trigger_gpio.port != NULL) {
		int ret;

		ret = gpio_pin_interrupt_configure_dt(&config->trigger_gpio,
						      GPIO_INT_DISABLE);
		if (ret < 0) {
			return ret;
		}
	}

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		data->drdy_handler = handler;
		data->drdy_trigger = *trig;
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	if (config->trigger_gpio.port != NULL) {
		return gpio_pin_interrupt_configure_dt(&config->trigger_gpio,
						       GPIO_INT_EDGE_FALLING);
	}

	return 0;
}
#endif /* CONFIG_SENSOR_SIM_TRIGGER */

/**
 * @brief Generates a pseudo-random number between -1 and 1.
 */
static double generate_pseudo_random(void)
{
	return rand() / (RAND_MAX / 2.0) - 1.0;
}

/**
 * @brief Generate value for acceleration signal toggling between two values on fetch.
 *
 * @param[in]	dev	Sensor device instance.
 * @param[in]	chan	Selected sensor channel.
 * @param[in]	val_cnt	Number of generated values.
 * @param[out]	out_val	Pointer to the variable that is used to store result.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
static int generate_toggle(const struct device *dev, enum sensor_channel chan,
			   size_t val_cnt, double *out_val)
{
	struct sensor_sim_data *data = dev->data;
	const struct sensor_sim_config *config = dev->config;

	ARG_UNUSED(chan);

	double res_val = config->acc_toggle_amplitude * data->val_sign;

	while (val_cnt > 0) {
		*out_val = res_val;
		out_val++;
		val_cnt--;
	}

	data->val_sign *= -1.0;

	return 0;
}

/**
 * @brief Generate value of acceleration wave signal.
 *
 * @param[in]	dev	Sensor device instance.
 * @param[in]	chan	Selected sensor channel.
 * @param[in]	val_cnt	Number of generated values.
 * @param[out]	out_val	Pointer to the variable that is used to store result.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
static int generate_wave(const struct device *dev, enum sensor_channel chan,
			 size_t val_cnt, double *out_val)
{
	struct sensor_sim_data *data = dev->data;

	__ASSERT_NO_MSG((val_cnt == 1) || (chan == SENSOR_CHAN_ACCEL_XYZ));

	const enum sensor_channel chans[ARRAY_SIZE(data->accel_param)] = {
		(chan == SENSOR_CHAN_ACCEL_XYZ) ? (SENSOR_CHAN_ACCEL_X) : chan,
		(chan == SENSOR_CHAN_ACCEL_XYZ) ? (SENSOR_CHAN_ACCEL_Y) : SENSOR_CHAN_PRIV_START,
		(chan == SENSOR_CHAN_ACCEL_XYZ) ? (SENSOR_CHAN_ACCEL_Z) : SENSOR_CHAN_PRIV_START
	};

	uint32_t time = k_uptime_get_32();
	int err = 0;

	k_mutex_lock(&data->accel_param_mutex, K_FOREVER);

	for (size_t i = 0; (i < ARRAY_SIZE(chans)) && !err; i++) {
		if (chans[i] == SENSOR_CHAN_PRIV_START) {
			break;
		}

		err = wave_gen_generate_value(time, get_wave_params(dev, chans[i]), out_val + i);
	}

	k_mutex_unlock(&data->accel_param_mutex);

	if (err) {
		LOG_ERR("Cannot generate wave value (err %d)", err);
	}

	return err;
}

/**
 * @brief Generates accelerometer data.
 *
 * @param[in]	dev	Sensor device instance.
 * @param[in]	chan	Channel to generate data for.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
static int generate_accel_data(const struct device *dev,
			       enum sensor_channel chan)
{
	struct sensor_sim_data *data = dev->data;
	const struct sensor_sim_config *config = dev->config;
	int retval = 0;
	generator_function gen_fn = NULL;

	if (config->acc_signal == ACC_SIGNAL_WAVE) {
		gen_fn = generate_wave;
	} else if (config->acc_signal == ACC_SIGNAL_TOGGLE) {
		gen_fn = generate_toggle;
	} else {
		return -ENOTSUP;
	}

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		retval = gen_fn(dev, chan, 1, &data->accel_samples[0]);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		retval = gen_fn(dev, chan, 1, &data->accel_samples[1]);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		retval = gen_fn(dev, chan, 1, &data->accel_samples[2]);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		retval = gen_fn(dev, chan, ACCEL_CHAN_COUNT,
				&data->accel_samples[0]);
		break;

	default:
		retval = -ENOTSUP;
	}

	return retval;
}

/**
 * @brief Generates simulated sensor data for a channel.
 *
 * @param[in]	dev	Sensor device instance.
 * @param[in]	chan	Channel to generate data for.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
static int sensor_sim_generate_data(const struct device *dev,
				    enum sensor_channel chan)
{
	struct sensor_sim_data *data = dev->data;
	const struct sensor_sim_config *config = dev->config;
	int err = 0;

	switch (chan) {
	case SENSOR_CHAN_ALL:
		data->temp_sample = (double)config->base_temperature +
				    generate_pseudo_random();
		data->humidity_sample = (double)config->base_humidity +
					generate_pseudo_random();
		data->pressure_sample = (double)config->base_pressure +
					generate_pseudo_random();
		err = generate_accel_data(dev, SENSOR_CHAN_ACCEL_XYZ);
		break;

	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		err = generate_accel_data(dev, chan);
		break;

	case SENSOR_CHAN_AMBIENT_TEMP:
		data->temp_sample = (double)config->base_temperature +
				    generate_pseudo_random();
		break;
	case SENSOR_CHAN_HUMIDITY:
		data->humidity_sample = (double)config->base_humidity +
					generate_pseudo_random();
		break;
	case SENSOR_CHAN_PRESS:
		data->pressure_sample = (double)config->base_pressure +
					generate_pseudo_random();
		break;
	default:
		err = -ENOTSUP;
	}

	return err;
}

static int sensor_sim_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	return sensor_sim_generate_data(dev, chan);
}

static int sensor_sim_channel_get(const struct device *dev,
				  enum sensor_channel chan,
				  struct sensor_value *sample)
{
	struct sensor_sim_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		sensor_value_from_double(sample, data->accel_samples[0]);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		sensor_value_from_double(sample, data->accel_samples[1]);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		sensor_value_from_double(sample, data->accel_samples[2]);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		sensor_value_from_double(sample, data->accel_samples[0]);
		sensor_value_from_double(++sample, data->accel_samples[1]);
		sensor_value_from_double(++sample, data->accel_samples[2]);
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		sensor_value_from_double(sample, data->temp_sample);
		break;
	case SENSOR_CHAN_HUMIDITY:
		sensor_value_from_double(sample, data->humidity_sample);
		break;
	case SENSOR_CHAN_PRESS:
		sensor_value_from_double(sample, data->pressure_sample);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api sensor_sim_api_funcs = {
	.sample_fetch = sensor_sim_sample_fetch,
	.channel_get = sensor_sim_channel_get,
#if defined(CONFIG_SENSOR_SIM_TRIGGER)
	.trigger_set = sensor_sim_trigger_set
#endif
};

static int sensor_sim_init(const struct device *dev)
{
	struct sensor_sim_data *data = dev->data;
	const struct sensor_sim_config *config = dev->config;

	data->dev = dev;

#if defined(CONFIG_SENSOR_SIM_TRIGGER)
	int ret;

	ret = sensor_sim_init_thread(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize trigger interrupt");
		return ret;
	}
#endif
	srand(k_cycle_get_32());

	k_mutex_init(&data->accel_param_mutex);

	if (config->acc_signal == ACC_SIGNAL_WAVE) {
		for (size_t i = 0; i < ARRAY_SIZE(data->accel_param); i++) {
			memcpy(&data->accel_param[i], &config->acc_param,
			       sizeof(config->acc_param));
		}
	}

	return 0;
}

#ifdef CONFIG_SENSOR_SIM_TRIGGER
#define SENSOR_SIM_TRIGGER_INIT(n)					       \
	.trigger_gpio = GPIO_DT_SPEC_INST_GET_OR(n, trigger_gpios, {}),       \
	.trigger_timeout = DT_INST_PROP(n, trigger_timeout),
#else
#define SENSOR_SIM_TRIGGER_INIT(n)
#endif

#define SENSOR_SIM_DEFINE(n)						       \
	static struct sensor_sim_data data##n = {			       \
		.val_sign = 1.0,					       \
	};								       \
									       \
	static const struct sensor_sim_config config##n = {		       \
		.base_temperature = DT_INST_PROP(n, base_temperature),	       \
		.base_humidity = DT_INST_PROP(n, base_humidity),	       \
		.base_pressure = DT_INST_PROP(n, base_pressure),	       \
		.acc_signal = DT_INST_ENUM_IDX(n, acc_signal),		       \
		.acc_param = {						       \
			.type = DT_INST_ENUM_IDX(n, acc_wave_type),	       \
			.amplitude = DT_INST_PROP(n, acc_wave_amplitude),      \
			.period_ms = DT_INST_PROP(n, acc_wave_period),	       \
		},							       \
		.acc_toggle_amplitude = DT_INST_PROP(n, acc_toggle_amplitude), \
		SENSOR_SIM_TRIGGER_INIT(n)				       \
	};								       \
									       \
	DEVICE_DT_INST_DEFINE(n, sensor_sim_init, NULL, &data##n, &config##n,  \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	       \
			      &sensor_sim_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_SIM_DEFINE)
