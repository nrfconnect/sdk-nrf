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
	int rc;

	rc = pm_device_runtime_put(dev);
	__ASSERT_NO_MSG(rc == 0);
	counter++;
}

void thread_definition(void)
{
	int rc;

	gpio_pin_configure_dt(&test_pin, GPIO_OUTPUT_INACTIVE);

	rc = comparator_set_trigger_callback(test_dev, test_callback, NULL);
	__ASSERT_NO_MSG(rc == 0);

	rc = comparator_set_trigger(test_dev, COMPARATOR_TRIGGER_BOTH_EDGES);
	__ASSERT_NO_MSG(rc == 0);
	k_msleep(10);

	while (1) {
		rc = pm_device_runtime_get(test_dev);
		__ASSERT_NO_MSG(rc == 0);
		k_busy_wait(100);
		counter = 0;
		gpio_pin_set_dt(&test_pin, 1);
		k_msleep(10);
		__ASSERT_NO_MSG(counter == 1);

		rc = pm_device_runtime_get(test_dev);
		__ASSERT_NO_MSG(rc == 0);
		k_busy_wait(100);
		gpio_pin_set_dt(&test_pin, 0);
		k_msleep(10);
		__ASSERT_NO_MSG(counter == 2);
	}
};
