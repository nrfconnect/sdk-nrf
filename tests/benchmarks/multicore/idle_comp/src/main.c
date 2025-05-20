/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device_runtime.h>

static const struct device *test_dev = DEVICE_DT_GET(DT_ALIAS(test_comp));
static const struct gpio_dt_spec test_pin = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), test_gpios);
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

volatile int counter;
static void test_callback(const struct device *dev, void *user_data)
{
	counter++;
}

int main(void)
{

	int rc;
	int test_repetitions = 3;

	gpio_pin_configure_dt(&test_pin, GPIO_OUTPUT_INACTIVE);

	rc = gpio_is_ready_dt(&led);
	__ASSERT(rc, "Error: GPIO Device not ready");

	rc = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	__ASSERT(rc == 0, "Could not configure led GPIO");

	pm_device_runtime_enable(test_dev);
	pm_device_runtime_get(test_dev);

	rc = comparator_set_trigger_callback(test_dev, test_callback, NULL);
	__ASSERT_NO_MSG(rc == 0);

	rc = comparator_set_trigger(test_dev, COMPARATOR_TRIGGER_BOTH_EDGES);
	__ASSERT_NO_MSG(rc == 0);

#if defined(CONFIG_COVERAGE)
	printk("Coverage analysis enabled\n");
	while (test_repetitions--)
#else
	while (test_repetitions)
#endif
	{
		counter = 0;
		k_busy_wait(200000);
		gpio_pin_set_dt(&test_pin, 1);
		k_busy_wait(400000);
		__ASSERT_NO_MSG(counter == 1);
		gpio_pin_set_dt(&test_pin, 0);
		k_busy_wait(400000);
		__ASSERT_NO_MSG(counter == 2);
		pm_device_runtime_put(test_dev);
		gpio_pin_set_dt(&led, 0);
		k_msleep(1000);
		gpio_pin_set_dt(&led, 1);
		pm_device_runtime_get(test_dev);
		k_msleep(1);
	}

#if defined(CONFIG_COVERAGE)
	printk("Coverage analysis start\n");
#endif
	return 0;
}
