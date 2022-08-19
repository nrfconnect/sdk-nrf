/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <npmx.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <npmx_driver.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(NPMX, CONFIG_NPMX_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_npm1300

/**
 * @brief Static pointer to npmx instance.
 * Only one npmx instance can be used.
 */
static npmx_instance_t *p_npmx_instance = NULL;

static void generic_callback(npmx_instance_t *pm, npmx_callback_type_t type, uint32_t arg)
{
	LOG_DBG("%s:", npmx_callback_to_str(type));
	for (uint8_t i = 0; i < 8; i++) {
		if ((1U << i) & arg) {
			LOG_DBG("\t%s", npmx_callback_bit_to_str(type, i));
		}
	}
}

static int npmx_init(const struct device *dev)
{
	if (p_npmx_instance) {
		LOG_ERR("Only one npmx device can be used");
		return -EACCES;
	}

	struct npmx_data *data = dev->data;
	const struct npmx_config *config = dev->config;
	const struct device *bus = config->i2c.bus;
	npmx_backend_instance_t *const backend = &data->npmx_instance.backend_inst;

	if (!device_is_ready(bus)) {
		LOG_ERR("%s: bus device %s is not ready", dev->name, bus->name);
		return -ENODEV;
	}

	npmx_backend_init(backend, (void *)bus, config->i2c.addr);

	data->dev = dev;

	p_npmx_instance = &data->npmx_instance;

	p_npmx_instance->generic_cb = generic_callback;

	if (npmx_core_init(&data->npmx_instance) == false) {
		LOG_ERR("Unable to init npmx device");
		return -EIO;
	}

	/* TODO: clear all events before enabling interrupts */
	if (npmx_gpio_interrupt_init(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt");
		return -EIO;
	}

	return 0;
};

bool npmx_driver_register_cb(npmx_callback_t cb, npmx_callback_type_t type)
{
	if (p_npmx_instance && cb) {
		p_npmx_instance->registered_cb[(uint32_t)type] = cb;
		return true;
	}
	return false;
}

bool npmx_driver_event_interrupt_enable(npmx_event_group_t event, uint8_t flags_mask)
{
	if (p_npmx_instance) {
		return npmx_event_interrupt_enable(p_npmx_instance, event, flags_mask);
	}
	return false;
}

#define NPMX_DEFINE(inst)									   \
	static struct npmx_data npmx_data_##inst;						   \
	static const struct npmx_config npmx_config_##inst = {					   \
		.i2c = I2C_DT_SPEC_INST_GET(inst),						   \
		.int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),				   \
	};											   \
	DEVICE_DT_INST_DEFINE(inst, npmx_init, NULL, &npmx_data_##inst, &npmx_config_##inst,	   \
			      POST_KERNEL, CONFIG_NPMX_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NPMX_DEFINE)
