/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define DEVICE_DT_GET_AND_COMMA(node_id) DEVICE_DT_GET(node_id),
/* Generate a list of devices for all instances of the "compat" */
#define DEVS_FOR_DT_COMPAT(compat) \
	DT_FOREACH_STATUS_OKAY(compat, DEVICE_DT_GET_AND_COMMA)

static const struct device *const devices[] = {
#ifdef CONFIG_UART_NRFX
	DEVS_FOR_DT_COMPAT(nordic_nrf_uarte)
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
 * Test validates if instance can send data.
 */
static void test_send_any_data_instance(const struct device *dev)
{
	struct uart_config test_uart_config;
	const uint8_t tx_buf[1] = {0x23};
	int ret = 0;

	ret = uart_config_get(dev, &test_uart_config);
	zassert_equal(ret, 0, "uart_config_get() failed %d", ret);

	test_uart_config.parity = UART_CFG_PARITY_NONE;
	test_uart_config.stop_bits = UART_CFG_STOP_BITS_1;
	test_uart_config.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
	test_uart_config.baudrate = 115200;

	ret = uart_configure(dev, &test_uart_config);
	zassert_equal(0, ret, "%s: uart_configure() failed %d", dev->name, ret);

	ret = uart_tx(dev, tx_buf, sizeof(tx_buf), 0);
	zassert_equal(0, ret, "%s: uart_tx() failed %d", dev->name, ret);

	/* Data reception is not tested. */
}

static bool test_send_any_data_capable(const struct device *dev)
{
	return true;
}

ZTEST(uart_instances, test_send_any_data)
{
	test_all_instances(test_send_any_data_instance, test_send_any_data_capable);
}

static void *suite_setup(void)
{
	int i;

	TC_PRINT("uart_instances test executed on %s\n", CONFIG_BOARD_TARGET);
	TC_PRINT("===================================================================\n");

	for (i = 0; i < ARRAY_SIZE(devices); i++) {
		zassert_true(device_is_ready(devices[i]),
			     "Device %s is not ready", devices[i]->name);
		k_object_access_grant(devices[i], k_current_get());
	}

	return NULL;
}

ZTEST_SUITE(uart_instances, NULL, suite_setup, NULL, NULL, NULL);
