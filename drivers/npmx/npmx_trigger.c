/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <npmx_driver.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(NPMX_TRIGGER, CONFIG_NPMX_LOG_LEVEL);

/* Callback for active sense pin from NPMX device */
static void npmx_gpio_callback(const struct device *gpio_dev, struct gpio_callback *cb,
			       uint32_t pins)
{
	struct npmx_data *data = CONTAINER_OF(cb, struct npmx_data, gpio_cb);
	const struct device *npmx_dev = data->dev;
	const struct npmx_config *config = npmx_dev->config;

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);
	k_work_submit(&data->work);
}

static void npmx_work_cb(struct k_work *work)
{
	struct npmx_data *data = CONTAINER_OF(work, struct npmx_data, work);
	const struct device *npmx_dev = data->dev;
	const struct npmx_config *config = npmx_dev->config;

	npmx_instance_t *npmx_instance = &data->npmx_instance;

	npmx_interrupt(npmx_instance);

	npmx_proc(npmx_instance);

	gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_LEVEL_HIGH);
}

int npmx_gpio_interrupt_init(const struct device *dev)
{
	struct npmx_data *data = dev->data;
	const struct npmx_config *config = dev->config;
	const struct gpio_dt_spec *int_gpio = &config->int_gpio;
	int err;

	/* Setup gpio interrupt */
	if (!device_is_ready(int_gpio->port)) {
		LOG_ERR("GPIO device %s is not ready", int_gpio->port->name);
		return -ENODEV;
	}

	k_work_init(&data->work, npmx_work_cb);

	err = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (err != 0) {
		LOG_ERR("Failed to configure interrupt GPIO: %d", err);
		return err;
	}

	gpio_init_callback(&data->gpio_cb, npmx_gpio_callback, BIT(int_gpio->pin));

	err = gpio_add_callback(int_gpio->port, &data->gpio_cb);
	if (err != 0) {
		LOG_ERR("Failed to set GPIO callback: %d", err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_LEVEL_HIGH);
	if (err != 0) {
		LOG_ERR("Failed to configure interrupt: %d", err);
	}

	return err;
}
