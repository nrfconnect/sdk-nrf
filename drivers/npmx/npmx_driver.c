/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <npmx_core.h>
#include <npmx_driver.h>

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(NPMX, CONFIG_NPMX_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_npm1300_eng_b

struct npmx_data {
	const struct device *dev;
	npmx_instance_t npmx_instance;
	struct k_work work;
	struct gpio_callback gpio_cb;
	struct gpio_callback pof_gpio_cb;
	struct k_work pof_work;
	void (*pof_cb)(npmx_instance_t *p_pm);
};

struct npmx_config {
	const struct i2c_dt_spec i2c;
	const struct gpio_dt_spec host_int_gpio;
	const int pmic_int_pin;
	const struct gpio_dt_spec host_pof_gpio;
	const int pmic_pof_pin;
	const int pmic_reset_pin;
};

static void pof_gpio_callback(const struct device *gpio_dev, struct gpio_callback *cb,
			      uint32_t pins)
{
	struct npmx_data *data = CONTAINER_OF(cb, struct npmx_data, pof_gpio_cb);
	const struct device *npmx_dev = data->dev;
	const struct npmx_config *config = npmx_dev->config;

	gpio_pin_interrupt_configure_dt(&config->host_pof_gpio, GPIO_INT_DISABLE);

	/* The interrupt pin also goes high when POF happens so it needs to be disabled. */
	gpio_pin_interrupt_configure_dt(&config->host_int_gpio, GPIO_INT_DISABLE);

	if (data->pof_cb) {
		data->pof_cb(&data->npmx_instance);
	}
}

static int pof_gpio_interrupt_init(const struct device *dev, gpio_flags_t pof_gpio_flags)
{
	struct npmx_data *data = dev->data;
	const struct npmx_config *config = dev->config;
	const struct gpio_dt_spec *host_pof_gpio = &config->host_pof_gpio;
	int err;

	/* Setup HOST GPIO input interrupt */
	if (!device_is_ready(host_pof_gpio->port)) {
		LOG_ERR("GPIO device %s is not ready", host_pof_gpio->port->name);
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->host_pof_gpio, GPIO_INPUT);
	if (err != 0) {
		LOG_ERR("Failed to configure interrupt GPIO: %d", err);
		return err;
	}

	gpio_init_callback(&data->pof_gpio_cb, pof_gpio_callback, BIT(host_pof_gpio->pin));

	err = gpio_add_callback(host_pof_gpio->port, &data->pof_gpio_cb);
	if (err != 0) {
		LOG_ERR("Failed to set GPIO callback: %d", err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->host_pof_gpio, pof_gpio_flags);
	if (err != 0) {
		LOG_ERR("Failed to configure interrupt: %d", err);
	}

	return err;
}

/* Callback for active sense pin from NPMX device */
static void int_gpio_callback(const struct device *gpio_dev, struct gpio_callback *cb,
			      uint32_t pins)
{
	struct npmx_data *data = CONTAINER_OF(cb, struct npmx_data, gpio_cb);
	const struct device *npmx_dev = data->dev;
	const struct npmx_config *config = npmx_dev->config;

	gpio_pin_interrupt_configure_dt(&config->host_int_gpio, GPIO_INT_DISABLE);
	k_work_submit(&data->work);
}

static void work_cb(struct k_work *work)
{
	struct npmx_data *data = CONTAINER_OF(work, struct npmx_data, work);
	const struct device *npmx_dev = data->dev;
	const struct npmx_config *config = npmx_dev->config;

	npmx_instance_t *npmx_instance = &data->npmx_instance;

	npmx_core_interrupt(npmx_instance);

	npmx_core_proc(npmx_instance);

	gpio_pin_interrupt_configure_dt(&config->host_int_gpio, GPIO_INT_LEVEL_HIGH);
}

static int int_gpio_interrupt_init(const struct device *dev)
{
	struct npmx_data *data = dev->data;
	const struct npmx_config *config = dev->config;
	const struct gpio_dt_spec *host_int_gpio = &config->host_int_gpio;
	int err;

	npmx_instance_t *npmx_instance = &data->npmx_instance;

	/* Use nPM GPIO as interrupt output */
	npmx_gpio_mode_set(npmx_gpio_get(npmx_instance, config->pmic_int_pin),
			   NPMX_GPIO_MODE_OUTPUT_IRQ);

	/* Setup HOST GPIO input interrupt */
	if (!device_is_ready(host_int_gpio->port)) {
		LOG_ERR("GPIO device %s is not ready", host_int_gpio->port->name);
		return -ENODEV;
	}

	k_work_init(&data->work, work_cb);

	err = gpio_pin_configure_dt(&config->host_int_gpio, GPIO_INPUT);
	if (err != 0) {
		LOG_ERR("Failed to configure interrupt GPIO: %d", err);
		return err;
	}

	gpio_init_callback(&data->gpio_cb, int_gpio_callback, BIT(host_int_gpio->pin));

	err = gpio_add_callback(host_int_gpio->port, &data->gpio_cb);
	if (err != 0) {
		LOG_ERR("Failed to set GPIO callback: %d", err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->host_int_gpio, GPIO_INT_LEVEL_HIGH);
	if (err != 0) {
		LOG_ERR("Failed to configure interrupt: %d", err);
	}

	return err;
}

static void generic_callback(npmx_instance_t *pm, npmx_callback_type_t type, uint8_t mask)
{
	LOG_DBG("%s:", npmx_callback_to_str(type));
	for (uint8_t i = 0; i < 8; i++) {
		if (BIT(i) & mask) {
			LOG_DBG("\t%s", npmx_callback_bit_to_str(type, i));
		}
	}
}

static int npmx_driver_init(const struct device *dev)
{
	struct npmx_data *data = dev->data;
	const struct npmx_config *config = dev->config;
	const struct device *bus = config->i2c.bus;
	npmx_backend_instance_t *const backend = &data->npmx_instance.backend_inst;

	npmx_backend_init(backend, (void *)bus, config->i2c.addr);

	data->dev = dev;

	data->npmx_instance.generic_cb = generic_callback;

	npmx_error_t ret = npmx_core_init(&data->npmx_instance);

	if (!device_is_ready(bus)) {
		LOG_ERR("%s: bus device %s is not ready", dev->name, bus->name);
		return -ENODEV;
	}

	if (ret != NPMX_SUCCESS) {
		LOG_ERR("Unable to init npmx device");
		return -EIO;
	}

	/* Clear all events before enabling interrupts. */
	for (uint32_t i = 0; i < NPMX_EVENT_GROUP_COUNT; i++) {
		if (npmx_core_event_interrupt_disable(&data->npmx_instance, (npmx_event_group_t)i,
						      NPMX_EVENT_GROUP_ALL_EVENTS_MASK) !=
		    NPMX_SUCCESS) {
			LOG_ERR("Failed to disable interrupts");
			return -EIO;
		}
	}

	if (int_gpio_interrupt_init(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt");
		return -EIO;
	}

	if (config->pmic_reset_pin != -1) {
		/* Configure pmic's gpio to work as output reset. */
		npmx_gpio_mode_set(npmx_gpio_get(&data->npmx_instance, config->pmic_reset_pin),
				   NPMX_GPIO_MODE_OUTPUT_RESET);
	}

	return 0;
};

npmx_instance_t *npmx_driver_instance_get(const struct device *p_dev)
{
	return (npmx_instance_t *)(&((struct npmx_data *)p_dev->data)->npmx_instance);
}

int npmx_driver_register_pof_cb(const struct device *dev, npmx_pof_config_t const *p_config,
				void (*p_cb)(npmx_instance_t *p_pm))
{
	struct npmx_data *data = dev->data;
	const struct npmx_config *config = dev->config;
	npmx_instance_t *npmx_instance = &data->npmx_instance;

	if (config->pmic_pof_pin == -1) {
		LOG_ERR("PMIC POF pin not configured");
		return -EINVAL;
	}

	if (config->host_pof_gpio.port == NULL) {
		LOG_ERR("HOST POF pin not configured");
		return -EINVAL;
	}

	if (p_cb == NULL) {
		LOG_ERR("Callback not set");
		return -EINVAL;
	}

	data->pof_cb = p_cb;

	npmx_pof_config_set(npmx_pof_get(npmx_instance, 0), p_config);

	npmx_gpio_mode_set(npmx_gpio_get(npmx_instance, config->pmic_pof_pin),
			   NPMX_GPIO_MODE_OUTPUT_PLW);

	gpio_flags_t pof_gpio_flags = (p_config->polarity == NPMX_POF_POLARITY_HIGH) ?
					      GPIO_INT_LEVEL_HIGH :
					      GPIO_INT_LEVEL_LOW;

	int err = pof_gpio_interrupt_init(dev, pof_gpio_flags);

	if (err != 0) {
		LOG_ERR("Failed to configure interrupt: %d", err);
	}

	return err;
}

#define NPMX_DEFINE(inst)                                                                          \
	static struct npmx_data npmx_data_##inst;                                                  \
	static const struct npmx_config npmx_config_##inst = {                                     \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.host_int_gpio = GPIO_DT_SPEC_INST_GET(inst, host_int_gpios),                      \
		.pmic_int_pin = DT_INST_PROP(inst, pmic_int_pin),                                  \
		.host_pof_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, host_pof_gpios, { 0 }),            \
		.pmic_pof_pin = DT_INST_PROP_OR(inst, pmic_pof_pin, -1),                           \
		.pmic_reset_pin = DT_INST_PROP_OR(inst, pmic_reset_pin, -1),                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, npmx_driver_init, NULL, &npmx_data_##inst,                     \
			      &npmx_config_##inst, POST_KERNEL, CONFIG_NPMX_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NPMX_DEFINE)

/*
 * Make sure that this driver is not initialized before the i2c bus is available.
 */
BUILD_ASSERT(CONFIG_NPMX_INIT_PRIORITY > CONFIG_I2C_INIT_PRIORITY);

/*
 * Make sure that this driver is not initialized before the gpio is available.
 */
BUILD_ASSERT(CONFIG_NPMX_INIT_PRIORITY > CONFIG_GPIO_INIT_PRIORITY);
