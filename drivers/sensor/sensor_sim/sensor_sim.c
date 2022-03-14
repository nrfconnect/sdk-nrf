/*
 * Copyright (c) 2018-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT nordic_sensor_sim

#include <drivers/gpio.h>
#include <kernel.h>
#include <init.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <logging/log.h>

#include <drivers/sensor_sim.h>

#define ACCEL_CHAN_COUNT	3

LOG_MODULE_REGISTER(sensor_sim, CONFIG_SENSOR_SIM_LOG_LEVEL);

enum acc_signal {
	ACC_SIGNAL_TOGGLE,
	ACC_SIGNAL_WAVE,
};

struct sensor_sim_data {
	double temp_sample;
	double humidity_sample;
	double pressure_sample;
	double accel_samples[ACCEL_CHAN_COUNT];
	struct wave_gen_param accel_param[ACCEL_CHAN_COUNT];
	struct k_mutex accel_param_mutex;
	const double amplitude;
	double val_sign;
#if defined(CONFIG_SENSOR_SIM_TRIGGER)
	const struct device *gpio;
	const char *gpio_port;
	uint8_t gpio_pin;
	struct gpio_callback gpio_cb;
	struct k_sem gpio_sem;

	sensor_trigger_handler_t drdy_handler;
	struct sensor_trigger drdy_trigger;

	K_THREAD_STACK_MEMBER(thread_stack,
			      CONFIG_SENSOR_SIM_THREAD_STACK_SIZE);
	struct k_thread thread;
#endif  /* CONFIG_SENSOR_SIM_TRIGGER */
};

struct sensor_sim_config {
	uint32_t base_temperature;
	uint8_t base_humidity;
	uint32_t base_pressure;
	enum acc_signal acc_signal;
	struct wave_gen_param acc_param;
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

/**
 * @brief Helper function to convert from double to sensor_value struct
 *
 * @param[in]	val		Sensor value to convert.
 * @param[out]	sense_val	Pointer to sensor_value to store the converted data.
 */
static void double_to_sensor_value(double val,
				struct sensor_value *sense_val)
{
	sense_val->val1 = (int)val;
	sense_val->val2 = (val - (int)val) * 1000000;
}

#if defined(CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON)
/**
 * @brief Callback for GPIO when using button as trigger.
 *
 * @param dev Pointer to device structure.
 * @param cb Pointer to GPIO callback structure.
 * @param pins Pin mask for callback.
 */
static void sensor_sim_gpio_callback(const struct device *dev,
				struct gpio_callback *cb,
				uint32_t pins)
{
	ARG_UNUSED(pins);
	struct sensor_sim_data *data =
		CONTAINER_OF(cb, struct sensor_sim_data, gpio_cb);

	gpio_pin_interrupt_configure(dev, data->gpio_pin, GPIO_INT_DISABLE);
	k_sem_give(&data->gpio_sem);
}
#endif /* CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON */

#if defined(CONFIG_SENSOR_SIM_TRIGGER)
/**
 * @brief Function that runs in the sensor simulator thread when using trigger.
 *
 * @param dev_ptr Pointer to sensor simulator device.
 */
static void sensor_sim_thread(int dev_ptr)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct sensor_sim_data *data = dev->data;

	while (true) {
		if (IS_ENABLED(CONFIG_SENSOR_SIM_TRIGGER_USE_TIMEOUT)) {
			k_sleep(K_MSEC(CONFIG_SENSOR_SIM_TRIGGER_TIMEOUT_MSEC));
		} else if (IS_ENABLED(CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON)) {
			k_sem_take(&data->gpio_sem, K_FOREVER);
		} else {
			/* Should not happen. */
			__ASSERT_NO_MSG(false);
		}

		if (data->drdy_handler != NULL) {
			data->drdy_handler(dev, &data->drdy_trigger);
		}

#if defined(CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON)
		gpio_pin_interrupt_configure(data->gpio, data->gpio_pin,
					     GPIO_INT_EDGE_FALLING);
#endif
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

#if defined(CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON)
	data->gpio = device_get_binding(data->gpio_port);
	if (data->gpio == NULL) {
		LOG_ERR("Failed to get pointer to %s device",
			data->gpio_port);
		return -EINVAL;
	}

	gpio_pin_configure(data->gpio, data->gpio_pin,
			   GPIO_INPUT | GPIO_PULL_UP | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&data->gpio_cb,
			   sensor_sim_gpio_callback,
			   BIT(data->gpio_pin));

	if (gpio_add_callback(data->gpio, &data->gpio_cb) < 0) {
		LOG_ERR("Failed to set GPIO callback");
		return -EIO;
	}

	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);

#endif   /* CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON */

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_SENSOR_SIM_THREAD_STACK_SIZE,
			// TODO TORA: upmerge confirmation from Jan Tore needed.
			(k_thread_entry_t)sensor_sim_thread, (void *)dev,
			NULL, NULL,
			K_PRIO_COOP(CONFIG_SENSOR_SIM_THREAD_PRIORITY),
			0, K_NO_WAIT);

	return 0;
}

static int sensor_sim_trigger_set(const struct device *dev,
			   const struct sensor_trigger *trig,
			   sensor_trigger_handler_t handler)
{
	int ret = 0;
	struct sensor_sim_data *data = dev->data;

#if defined(CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON)
	gpio_pin_interrupt_configure(data->gpio, data->gpio_pin,
				     GPIO_INT_DISABLE);
#endif

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		data->drdy_handler = handler;
		data->drdy_trigger = *trig;
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		ret = -ENOTSUP;
		break;
	}

#if defined(CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON)
	gpio_pin_interrupt_configure(data->gpio, data->gpio_pin,
				     GPIO_INT_EDGE_FALLING);
#endif
	return ret;
}
#endif /* CONFIG_SENSOR_SIM_TRIGGER */

/**
 * @brief Initializes sensor simulator
 *
 * @param dev Pointer to device instance.
 *
 * @return 0 when successful or negative error code
 */
static int sensor_sim_init(const struct device *dev)
{
	struct sensor_sim_data *data = dev->data;
	const struct sensor_sim_config *config = dev->config;

#if defined(CONFIG_SENSOR_SIM_TRIGGER)
#if defined(CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON)
	data->gpio_port = DT_GPIO_LABEL(DT_ALIAS(sw0), gpios);
	data->gpio_pin = DT_GPIO_PIN(DT_ALIAS(sw0), gpios);
#endif
	if (sensor_sim_init_thread(dev) < 0) {
		LOG_ERR("Failed to initialize trigger interrupt");
		return -EIO;
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

	ARG_UNUSED(chan);

	double res_val = data->amplitude * data->val_sign;

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

static int sensor_sim_attr_set(const struct device *dev,
		enum sensor_channel chan,
		enum sensor_attribute attr,
		const struct sensor_value *val)
{
	return 0;
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
		double_to_sensor_value(data->accel_samples[0], sample);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		double_to_sensor_value(data->accel_samples[1], sample);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		double_to_sensor_value(data->accel_samples[2], sample);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		double_to_sensor_value(data->accel_samples[0], sample);
		double_to_sensor_value(data->accel_samples[1], ++sample);
		double_to_sensor_value(data->accel_samples[2], ++sample);
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		double_to_sensor_value(data->temp_sample, sample);
		break;
	case SENSOR_CHAN_HUMIDITY:
		double_to_sensor_value(data->humidity_sample, sample);
		break;
	case SENSOR_CHAN_PRESS:
		double_to_sensor_value(data->pressure_sample, sample);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api sensor_sim_api_funcs = {
	.attr_set = sensor_sim_attr_set,
	.sample_fetch = sensor_sim_sample_fetch,
	.channel_get = sensor_sim_channel_get,
#if defined(CONFIG_SENSOR_SIM_TRIGGER)
	.trigger_set = sensor_sim_trigger_set
#endif
};

#define SENSOR_SIM_DEFINE(n)						       \
	static struct sensor_sim_data data##n = {			       \
		.amplitude = 20.0,					       \
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
	};								       \
									       \
	DEVICE_DT_INST_DEFINE(n, sensor_sim_init, NULL, &data##n, &config##n,  \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	       \
			      &sensor_sim_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_SIM_DEFINE)
