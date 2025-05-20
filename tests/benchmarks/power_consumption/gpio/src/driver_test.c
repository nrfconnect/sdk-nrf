/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>

#define OUTPUT_NODE DT_ALIAS(out0)
#define INPUT_NODE  DT_ALIAS(in0)

static const struct gpio_dt_spec output = GPIO_DT_SPEC_GET(OUTPUT_NODE, gpios);
static const struct gpio_dt_spec input = GPIO_DT_SPEC_GET(INPUT_NODE, gpios);
volatile int counter;

static struct gpio_callback input_active_cb_data;

void input_active(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	counter++;
}

void thread_definition(void)
{
	int rc;
	unsigned int key;

	rc = gpio_is_ready_dt(&input);
	__ASSERT_NO_MSG(rc);

	rc = gpio_is_ready_dt(&output);
	__ASSERT_NO_MSG(rc);

	rc = gpio_pin_configure_dt(&input, GPIO_INPUT);
	__ASSERT_NO_MSG(rc == 0);

	rc = gpio_pin_configure_dt(&output, GPIO_OUTPUT);
	__ASSERT_NO_MSG(rc == 0);

	rc = gpio_pin_interrupt_configure_dt(&input, GPIO_INT_LEVEL_ACTIVE);
	__ASSERT_NO_MSG(rc == 0);

	gpio_init_callback(&input_active_cb_data, input_active, BIT(input.pin));
	gpio_add_callback_dt(&input, &input_active_cb_data);

	while (1) {
		counter = 0;
		key = irq_lock();
		rc = gpio_pin_set_dt(&output, 0);
		rc = gpio_pin_set_dt(&output, 1);
		rc = gpio_pin_set_dt(&output, 0);
		irq_unlock(key);
		__ASSERT_NO_MSG(counter == 1);

		k_msleep(10);
	}
}
