/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define OUTPUT_NODE DT_ALIAS(out0)
#define INPUT_NODE  DT_ALIAS(in0)

static const struct gpio_dt_spec output = GPIO_DT_SPEC_GET(OUTPUT_NODE, gpios);
static const struct gpio_dt_spec input = GPIO_DT_SPEC_GET(INPUT_NODE, gpios);
extern const struct gpio_dt_spec led;

static struct gpio_callback input_active_cb_data;

void input_active(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	gpio_pin_toggle_dt(&led);
}

void thread_definition(void)
{
	gpio_pin_configure_dt(&output, GPIO_OUTPUT);
	gpio_pin_configure_dt(&input, GPIO_INPUT);

	gpio_pin_interrupt_configure_dt(&input, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&input_active_cb_data, input_active, BIT(input.pin));
	gpio_add_callback(input.port, &input_active_cb_data);

	while (1) {
		gpio_pin_toggle_dt(&output);
		k_msleep(10);
	}
}
