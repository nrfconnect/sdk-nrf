/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define DEVICE_DT_GET_AND_COMMA(node_id) DEVICE_DT_GET(node_id),
/* Generate a list of devices for all instances of the "compat" */
#define DEVS_FOR_DT_COMPAT(compat) \
	DT_FOREACH_STATUS_OKAY(compat, DEVICE_DT_GET_AND_COMMA)

static const struct device *const devices[] = {
#ifdef CONFIG_I2C_NRFX_TWIM
	DEVS_FOR_DT_COMPAT(nordic_nrf_twim)
#endif
};

typedef void (*test_func_t)(const struct device *dev);
typedef bool (*capability_func_t)(const struct device *dev);

static void setup_instance(const struct device *dev)
{
	/* Left for future test expansion. */
}

static void tear_down_instance(const struct device *dev)
{
	/* Left for future test expansion. */
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
 * Test validates if instance can be configured.
 */
static void test_configure_instance(const struct device *dev)
{
	uint32_t i2c_config = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;
	uint8_t i2c_tx_buffer[1] = {255};
	int ret = 0;

	ret = i2c_configure(dev, i2c_config);
	zassert_equal(0, ret, "%s: i2c_configure() failed (%d)", dev->name, ret);

	/* -EIO as there is no device connnected to the I2C bus. */
	ret = i2c_write(dev, i2c_tx_buffer, sizeof(i2c_tx_buffer), 0xFF);
	zassert_equal(-EIO, ret, "%s: i2c_write() failed (%d)", dev->name, ret);
}


/**
 * Test if any unexpected device can be detected on instance bus.
 * SMBus and i2c scan shell command based.
 */
static void test_scan_instance(const struct device *dev)
{
	uint8_t first_addr = 0x00, last_addr = 0x7F;
	uint8_t dev_cnt = 0;

	struct i2c_msg msg = {
		.buf   = NULL,
		.len   = 0U,
		.flags = I2C_MSG_WRITE | I2C_MSG_STOP
	};

	for (uint8_t addr = first_addr; addr <= last_addr; addr += 1) {
		if (i2c_transfer(dev, &msg, 1, addr) == 0) {
			dev_cnt = dev_cnt + 1;
		}
	}
	zassert_equal(dev_cnt, 0,
		"i2c scan found %d unexpected devices on %s bus\n",
		dev_cnt, dev->name);
}

static bool test_configure_capable(const struct device *dev)
{
	return true;
}

ZTEST(twim_instances, test_configure)
{
	test_all_instances(test_configure_instance, test_configure_capable);
}

ZTEST(twim_instances, test_scan)
{
	test_all_instances(test_scan_instance, test_configure_capable);
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

ZTEST_SUITE(twim_instances, NULL, suite_setup, NULL, NULL, NULL);
