/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>
#include <zephyr/pm/device_runtime.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define DEVICE_DT_GET_AND_COMMA(node_id) DEVICE_DT_GET(node_id),
/* Generate a list of devices for all instances of the "compat" */
#define DEVS_FOR_DT_COMPAT(compat) \
	DT_FOREACH_STATUS_OKAY(compat, DEVICE_DT_GET_AND_COMMA)

static const struct device *const devices[] = {
#ifdef CONFIG_NRFX_QDEC
	DEVS_FOR_DT_COMPAT(nordic_nrf_qdec)
#endif
};

typedef void (*test_func_t)(const struct device *dev);
typedef bool (*capability_func_t)(const struct device *dev);

static void setup_instance(const struct device *dev)
{
	pm_device_runtime_get(dev);
}

static void tear_down_instance(const struct device *dev)
{
	pm_device_runtime_put(dev);
}

static void test_all_instances(test_func_t func, capability_func_t capability_check)
{
	int devices_skipped = 0;

	zassert_true(ARRAY_SIZE(devices) > 0, "No device found");
	for (int i = 0; i < ARRAY_SIZE(devices); i++) {
		setup_instance(devices[i]);
		TC_PRINT("\nInstance %u: ", i + 1);
		if ((capability_check == NULL) ||
		     capability_check(devices[i])) {
			TC_PRINT("Testing %s\n", devices[i]->name);
			func(devices[i]);
		} else {
			TC_PRINT("Skipped for %s\n", devices[i]->name);
			devices_skipped++;
		}
		tear_down_instance(devices[i]);
		/* Allow logs to be printed. */
		k_sleep(K_MSEC(100));
	}
	if (devices_skipped == ARRAY_SIZE(devices)) {
		ztest_test_skip();
	}
}


/**
 * Test validates if instance can fetch sample.
 */
static void test_sensor_sample_fetch_instance(const struct device *dev)
{
	struct sensor_value val;
	int ret = 0;

	ret = sensor_sample_fetch(dev);
	zassert_equal(0, ret, "%s: sensor_sample_fetch() failed", dev->name);

	ret = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &val);
	zassert_equal(0, ret, "%s: sensor_channel_get() failed", dev->name);

	TC_PRINT("Position = %d degrees\n", val.val1);
}

static bool test_sensor_sample_fetch_capable(const struct device *dev)
{
	return true;
}

ZTEST(qdec_instances, test_sensor_sample_fetch)
{
	test_all_instances(test_sensor_sample_fetch_instance, test_sensor_sample_fetch_capable);
}

static void *suite_setup(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(devices); i++) {
		zassert_true(device_is_ready(devices[i]),
			     "Device %s is not ready", devices[i]->name);
		k_object_access_grant(devices[i], k_current_get());
	}

	return NULL;
}

ZTEST_SUITE(qdec_instances, NULL, suite_setup, NULL, NULL, NULL);
