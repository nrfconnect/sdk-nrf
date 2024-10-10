/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT nordic_nrfe_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#include "gpio_nrfe.h"

struct gpio_nrfe_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
};

struct gpio_nrfe_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uint8_t port_num;
};

static inline const struct gpio_nrfe_cfg *get_port_cfg(const struct device *port)
{
	return port->config;
}

static int gpio_nrfe_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	nrfe_gpio_data_packet_t msg = {.opcode = NRFE_GPIO_PIN_CONFIGURE,
				       .pin = pin,
				       .port = get_port_cfg(port)->port_num,
				       .flags = flags};

	return gpio_send(&msg);
}

static int gpio_nrfe_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					 gpio_port_value_t value)
{

	const uint32_t set_mask = value & mask;
	const uint32_t clear_mask = (~set_mask) & mask;

	nrfe_gpio_data_packet_t msg = {
		.opcode = NRFE_GPIO_PIN_SET, .pin = set_mask, .port = get_port_cfg(port)->port_num};

	int ret_val = gpio_send(&msg);

	if (ret_val < 0) {
		return ret_val;
	}

	msg.opcode = NRFE_GPIO_PIN_CLEAR;
	msg.pin = clear_mask;

	return gpio_send(&msg);
}

static int gpio_nrfe_port_set_bits_raw(const struct device *port, gpio_port_pins_t mask)
{
	nrfe_gpio_data_packet_t msg = {
		.opcode = NRFE_GPIO_PIN_SET, .pin = mask, .port = get_port_cfg(port)->port_num};

	return gpio_send(&msg);
}

static int gpio_nrfe_port_clear_bits_raw(const struct device *port, gpio_port_pins_t mask)
{
	nrfe_gpio_data_packet_t msg = {
		.opcode = NRFE_GPIO_PIN_CLEAR, .pin = mask, .port = get_port_cfg(port)->port_num};

	return gpio_send(&msg);
}

static int gpio_nrfe_port_toggle_bits(const struct device *port, gpio_port_pins_t mask)
{
	nrfe_gpio_data_packet_t msg = {
		.opcode = NRFE_GPIO_PIN_TOGGLE, .pin = mask, .port = get_port_cfg(port)->port_num};

	return gpio_send(&msg);
}

static const struct gpio_driver_api gpio_nrfe_drv_api_funcs = {
	.pin_configure = gpio_nrfe_pin_configure,
	.port_set_masked_raw = gpio_nrfe_port_set_masked_raw,
	.port_set_bits_raw = gpio_nrfe_port_set_bits_raw,
	.port_clear_bits_raw = gpio_nrfe_port_clear_bits_raw,
	.port_toggle_bits = gpio_nrfe_port_toggle_bits,
};

#define GPIO_NRFE_DEVICE(id)                                                                       \
	static const struct gpio_nrfe_cfg gpio_nrfe_p##id##_cfg = {                                \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(id),              \
			},                                                                         \
		.port_num = DT_INST_PROP(id, port),                                                \
	};                                                                                         \
                                                                                                   \
	static struct gpio_nrfe_data gpio_nrfe_p##id##_data;                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, gpio_nrfe_init, NULL, &gpio_nrfe_p##id##_data,                   \
			      &gpio_nrfe_p##id##_cfg, POST_KERNEL, CONFIG_GPIO_NRFE_INIT_PRIORITY, \
			      &gpio_nrfe_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NRFE_DEVICE)
