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
		k_msleep(200);
		gpio_pin_set_dt(&test_pin, 1);
		k_msleep(400);
		__ASSERT_NO_MSG(counter == 1);
		gpio_pin_set_dt(&test_pin, 0);
		k_msleep(400);
		__ASSERT_NO_MSG(counter == 2);
		pm_device_runtime_put(test_dev);
		k_msleep(1000);

		pm_device_runtime_get(test_dev);
		k_msleep(1);
	}

#if defined(CONFIG_COVERAGE)
	printk("Coverage analysis start\n");
#endif
	return 0;
}
