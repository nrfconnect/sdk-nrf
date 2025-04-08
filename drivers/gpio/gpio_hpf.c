/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT nordic_hpf_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#include "gpio_hpf.h"

struct gpio_hpf_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
};

struct gpio_hpf_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uint8_t port_num;
};

static inline const struct gpio_hpf_cfg *get_port_cfg(const struct device *port)
{
	return port->config;
}

static int gpio_hpf_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	hpf_gpio_data_packet_t msg = {.opcode = HPF_GPIO_PIN_CONFIGURE,
				       .pin = pin,
				       .port = get_port_cfg(port)->port_num,
				       .flags = flags};

	return gpio_send(&msg);
}

static int gpio_hpf_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					 gpio_port_value_t value)
{

	const uint32_t set_mask = value & mask;
	const uint32_t clear_mask = (~set_mask) & mask;

	hpf_gpio_data_packet_t msg = {
		.opcode = HPF_GPIO_PIN_SET, .pin = set_mask, .port = get_port_cfg(port)->port_num};

	int ret_val = gpio_send(&msg);

	if (ret_val < 0) {
		return ret_val;
	}

	msg.opcode = HPF_GPIO_PIN_CLEAR;
	msg.pin = clear_mask;

	return gpio_send(&msg);
}

static int gpio_hpf_port_set_bits_raw(const struct device *port, gpio_port_pins_t mask)
{
	hpf_gpio_data_packet_t msg = {
		.opcode = HPF_GPIO_PIN_SET, .pin = mask, .port = get_port_cfg(port)->port_num};

	return gpio_send(&msg);
}

static int gpio_hpf_port_clear_bits_raw(const struct device *port, gpio_port_pins_t mask)
{
	hpf_gpio_data_packet_t msg = {
		.opcode = HPF_GPIO_PIN_CLEAR, .pin = mask, .port = get_port_cfg(port)->port_num};

	return gpio_send(&msg);
}

static int gpio_hpf_port_toggle_bits(const struct device *port, gpio_port_pins_t mask)
{
	hpf_gpio_data_packet_t msg = {
		.opcode = HPF_GPIO_PIN_TOGGLE, .pin = mask, .port = get_port_cfg(port)->port_num};

	return gpio_send(&msg);
}

static const struct gpio_driver_api gpio_hpf_drv_api_funcs = {
	.pin_configure = gpio_hpf_pin_configure,
	.port_set_masked_raw = gpio_hpf_port_set_masked_raw,
	.port_set_bits_raw = gpio_hpf_port_set_bits_raw,
	.port_clear_bits_raw = gpio_hpf_port_clear_bits_raw,
	.port_toggle_bits = gpio_hpf_port_toggle_bits,
};

#define GPIO_HPF_DEVICE(id)                                                                       \
	static const struct gpio_hpf_cfg gpio_hpf_p##id##_cfg = {                                \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(id),              \
			},                                                                         \
		.port_num = DT_INST_PROP(id, port),                                                \
	};                                                                                         \
                                                                                                   \
	static struct gpio_hpf_data gpio_hpf_p##id##_data;                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, gpio_hpf_init, NULL, &gpio_hpf_p##id##_data,                   \
			      &gpio_hpf_p##id##_cfg, POST_KERNEL, CONFIG_GPIO_HPF_INIT_PRIORITY, \
			      &gpio_hpf_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(GPIO_HPF_DEVICE)
