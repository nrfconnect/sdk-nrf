/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <drivers/gpio.h>
#include <init.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <stdlib.h>
#include <logging/log.h>

#include "sensor_sim.h"

#if defined(CONFIG_SENSOR_SIM_DYNAMIC_VALUES)
	#include <math.h>
#endif

LOG_MODULE_REGISTER(sensor_sim, CONFIG_SENSOR_SIM_LOG_LEVEL);

static const double base_accel_samples[3] = {0.0, 0.0, 0.0};
static double accel_samples[3];

/* TODO: Make base sensor data configurable from Kconfig, along with more
 * detailed control over the sensor data data generation.
 */
static const double base_temp_sample = 21.0;
static const double base_humidity_sample = 52.0;
static const double base_pressure_sample = 98.2;
static double temp_sample;
static double humidity_sample;
static double pressure_sample;

/*
 * @brief Helper function to convert from double to sensor_value struct
 *
 * @param val Sensor value to convert.
 * @param sense_val Pointer to sensor_value to store the converted data.
 */
static void double_to_sensor_value(double val,
				struct sensor_value *sense_val)
{
	sense_val->val1 = (int)val;
	sense_val->val2 = (val - (int)val) * 1000000;
}

#if defined(CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON)
/*
 * @brief Callback for GPIO when using button as trigger.
 *
 * @param dev Pointer to device structure.
 * @param cb Pointer to GPIO callback structure.
 * @param pins Pin mask for callback.
 */
static void sensor_sim_gpio_callback(struct device *dev,
				struct gpio_callback *cb,
				u32_t pins)
{
	ARG_UNUSED(pins);
	struct sensor_sim_data *drv_data =
		CONTAINER_OF(cb, struct sensor_sim_data, gpio_cb);

	gpio_pin_disable_callback(dev, drv_data->gpio_pin);
	k_sem_give(&drv_data->gpio_sem);
}
#endif /* CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON */

/*
 * @brief Function that runs in the sensor simulator thread when using trigger.
 *
 * @param dev_ptr Pointer to sensor simulator device.
 */
static void sensor_sim_thread(int dev_ptr)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct sensor_sim_data *drv_data = dev->driver_data;

	while (true) {
		if (IS_ENABLED(CONFIG_SENSOR_SIM_TRIGGER_USE_TIMER)) {
			k_sleep(CONFIG_SENSOR_SIM_TRIGGER_TIMER_MSEC);
		} else if (IS_ENABLED(CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON)) {
			k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		}

		if (drv_data->drdy_handler != NULL) {
			drv_data->drdy_handler(dev, &drv_data->drdy_trigger);
		}

#if defined(CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON)
		gpio_pin_enable_callback(drv_data->gpio, drv_data->gpio_pin);
#endif
	}
}

/*
 * @brief Initializing thread when simulator uses trigger
 *
 * @param dev Pointer to device instance.
 */
static int sensor_sim_init_thread(struct device *dev)
{
	struct sensor_sim_data *drv_data = dev->driver_data;

#if defined(CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON)
	drv_data->gpio = device_get_binding(drv_data->gpio_port);
	if (drv_data->gpio == NULL) {
		LOG_ERR("Failed to get pointer to %s device",
			drv_data->gpio_port);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, drv_data->gpio_pin,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE |
			   GPIO_PUD_PULL_UP);

	gpio_init_callback(&drv_data->gpio_cb,
			   sensor_sim_gpio_callback,
			   BIT(drv_data->gpio_pin));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		LOG_ERR("Failed to set GPIO callback");
		return -EIO;
	}

	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

#endif   /* CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON */

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_SENSOR_SIM_THREAD_STACK_SIZE,
			(k_thread_entry_t)sensor_sim_thread, dev,
			NULL, NULL,
			K_PRIO_COOP(CONFIG_SENSOR_SIM_THREAD_PRIORITY),
			0, 0);

	return 0;
}

static int sensor_sim_trigger_set(struct device *dev,
			   const struct sensor_trigger *trig,
			   sensor_trigger_handler_t handler)
{
	int ret = 0;
	struct sensor_sim_data *drv_data = dev->driver_data;

#if defined(CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON)
	gpio_pin_disable_callback(drv_data->gpio, drv_data->gpio_pin);
#endif

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		drv_data->drdy_handler = handler;
		drv_data->drdy_trigger = *trig;
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		ret = -ENOTSUP;
		break;
	}

#if defined(CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON)
	gpio_pin_enable_callback(drv_data->gpio, drv_data->gpio_pin);
#endif
	return ret;
}

/*
 * @brief Initializes sensor simulator
 *
 * @param dev Pointer to device instance.
 *
 * @return 0 when successful or negative error code
 */
static int sensor_sim_init(struct device *dev)
{
#if defined(CONFIG_SENSOR_SIM_TRIGGER)
#if defined(CONFIG_SENSOR_SIM_TRIGGER_USE_BUTTON)
	struct sensor_sim_data *drv_data = dev->driver_data;

	drv_data->gpio_port = SW0_GPIO_CONTROLLER;
	drv_data->gpio_pin = SW0_GPIO_PIN;
#endif
	if (sensor_sim_init_thread(dev) < 0) {
		LOG_ERR("Failed to initialize trigger interrupt");
		return -EIO;
	}
#endif
	srand(k_cycle_get_32());

	return 0;
}

/**
 * @brief Generates a pseudo-random number between -1 and 1.
 */
static double generate_pseudo_random(void)
{
	return (double)rand() / ((double)RAND_MAX / 2.0) - 1.0;
}

/*
 * @brief Calculates sine from uptime
 * The input to the sin() function is limited to avoid overflow issues
 *
 * @param offset Offset for the sine.
 * @param amplitude Amplitude of sine.
 */
static double generate_sine(double offset, double amplitude)
{
	u32_t time = k_uptime_get_32();

	return offset + amplitude * sin(time % 65535);
}

/*
 * @brief Generates accelerometer data.
 *
 * @param chan Channel to generate data for.
 */
static int generate_accel_data(enum sensor_channel chan)
{
	int retval = 0;
	double max_variation = 20.0;
	static int static_val_coeff = 1.0;

	if (IS_ENABLED(CONFIG_SENSOR_SIM_DYNAMIC_VALUES)) {
		switch (chan) {
		case SENSOR_CHAN_ACCEL_X:
			accel_samples[0] = generate_sine(base_accel_samples[0],
								max_variation);
			break;
		case SENSOR_CHAN_ACCEL_Y:
			accel_samples[1] = generate_sine(base_accel_samples[1],
								max_variation);
			break;
		case SENSOR_CHAN_ACCEL_Z:
			accel_samples[2] = generate_sine(base_accel_samples[2],
								max_variation);
			break;
		case SENSOR_CHAN_ACCEL_XYZ:
			accel_samples[0] = generate_sine(base_accel_samples[0],
								max_variation);
			k_sleep(1);
			accel_samples[1] = generate_sine(base_accel_samples[1],
								max_variation);
			k_sleep(1);
			accel_samples[2] = generate_sine(base_accel_samples[2],
								max_variation);
			break;
		default:
			retval = -ENOTSUP;
		}
	}

	if (IS_ENABLED(CONFIG_SENSOR_SIM_STATIC_VALUES)) {
		switch (chan) {
		case SENSOR_CHAN_ACCEL_X:
			accel_samples[0] = static_val_coeff * max_variation;
			break;
		case SENSOR_CHAN_ACCEL_Y:
			accel_samples[1] = static_val_coeff * max_variation;
			break;
		case SENSOR_CHAN_ACCEL_Z:
			accel_samples[2] = static_val_coeff * max_variation;
			break;
		case SENSOR_CHAN_ACCEL_XYZ:
			accel_samples[0] = static_val_coeff * max_variation;
			accel_samples[1] = static_val_coeff * max_variation;
			accel_samples[2] = static_val_coeff * max_variation;
			break;
		default:
			retval = -ENOTSUP;
		}

		static_val_coeff *= -1.0;
	}

	return retval;
}

/**
 * @brief Generates temperature data.
 */
static void generate_temp_data(void)
{
	temp_sample = base_temp_sample + generate_pseudo_random();
}

/**
 * @brief Generates humidity data.
 */
static void generate_humidity_data(void)
{
	humidity_sample = base_humidity_sample + generate_pseudo_random();
}

/**
 * @brief Generates pressure data.
 */
static void generate_pressure_data(void)
{
	pressure_sample = base_pressure_sample + generate_pseudo_random();
}

/*
 * @brief Generates simulated sensor data for a channel.
 *
 * @param chan Channel to generate data for.
 */
static int sensor_sim_generate_data(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		generate_accel_data(SENSOR_CHAN_ACCEL_X);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		generate_accel_data(SENSOR_CHAN_ACCEL_Y);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		generate_accel_data(SENSOR_CHAN_ACCEL_Z);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		generate_accel_data(SENSOR_CHAN_ACCEL_XYZ);
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		generate_temp_data();
		break;
	case SENSOR_CHAN_HUMIDITY:
		generate_humidity_data();
		break;
	case SENSOR_CHAN_PRESS:
		generate_pressure_data();
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int sensor_sim_attr_set(struct device *dev,
		enum sensor_channel chan,
		enum sensor_attribute attr,
		const struct sensor_value *val)
{
	return 0;
}

static int sensor_sim_sample_fetch(struct device *dev,
				enum sensor_channel chan)
{
	return sensor_sim_generate_data(chan);
}

static int sensor_sim_channel_get(struct device *dev,
				  enum sensor_channel chan,
				  struct sensor_value *sample)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		double_to_sensor_value(accel_samples[0], sample);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		double_to_sensor_value(accel_samples[1], sample);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		double_to_sensor_value(accel_samples[2], sample);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		double_to_sensor_value(accel_samples[0], sample);
		double_to_sensor_value(accel_samples[1], ++sample);
		double_to_sensor_value(accel_samples[2], ++sample);
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		double_to_sensor_value(temp_sample, sample);
		break;
	case SENSOR_CHAN_HUMIDITY:
		double_to_sensor_value(humidity_sample, sample);
		break;
	case SENSOR_CHAN_PRESS:
		double_to_sensor_value(pressure_sample, sample);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static struct sensor_sim_data sensor_sim_data;

static const struct sensor_driver_api sensor_sim_api_funcs = {
	.attr_set = sensor_sim_attr_set,
	.sample_fetch = sensor_sim_sample_fetch,
	.channel_get = sensor_sim_channel_get,
#if defined(CONFIG_SENSOR_SIM_TRIGGER)
	.trigger_set = sensor_sim_trigger_set
#endif
};

DEVICE_AND_API_INIT(sensor_sim, CONFIG_SENSOR_SIM_DEV_NAME, sensor_sim_init,
		    &sensor_sim_data, NULL, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &sensor_sim_api_funcs);
