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
	int rc;

	rc = gpio_pin_toggle_dt(&led);
	__ASSERT_NO_MSG(rc == 0);
}

void thread_definition(void)
{
	int rc;

	rc = gpio_is_ready_dt(&input);
	__ASSERT_NO_MSG(rc == 0);

	rc = gpio_is_ready_dt(&output);
	__ASSERT_NO_MSG(rc == 0);

	rc = gpio_pin_configure_dt(&input, GPIO_INPUT);
	__ASSERT_NO_MSG(rc == 0);

	rc = gpio_pin_configure_dt(&output, GPIO_OUTPUT);
	__ASSERT_NO_MSG(rc == 0);

	rc = gpio_pin_interrupt_configure_dt(&input, GPIO_INT_LEVEL_ACTIVE);
	__ASSERT_NO_MSG(rc == 0);

	gpio_init_callback(&input_active_cb_data, input_active, BIT(input.pin));
	gpio_add_callback_dt(&input, &input_active_cb_data);

	while (1) {
		rc = gpio_pin_set_dt(&output, 0);
		__ASSERT_NO_MSG(rc == 0);
		rc = gpio_pin_set_dt(&output, 1);
		__ASSERT_NO_MSG(rc == 0);
		rc = gpio_pin_set_dt(&output, 0);

		k_msleep(10);
	}
}
