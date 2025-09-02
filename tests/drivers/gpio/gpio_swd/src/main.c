/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_swd, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>


#if !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), out_gpios)
#error "Unsupported board: out_gpios are not defined"
#endif

const struct gpio_dt_spec out_pins[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_PATH(zephyr_user), out_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))
};
BUILD_ASSERT(ARRAY_SIZE(out_pins) > 0, "missing out pins");
const uint8_t npins = ARRAY_SIZE(out_pins);


/**
 * @brief Test if GPIOs can be configured as inputs.
 */
ZTEST(gpio_swd, test_configure_input)
{
	uint8_t i;
	int rc;

	for (i = 0; i < npins; i++) {
		rc = gpio_pin_configure_dt(&out_pins[i], GPIO_INPUT);
		zassert_equal(rc, 0, "[%d]: Port %s, pin %d, config GPIO_INPUT failed",
			i, out_pins[i].port->name, out_pins[i].pin);
	}
}

/**
 * @brief Test if GPIOs can be configured as outputs.
 */
ZTEST(gpio_swd, test_configure_output)
{
	uint8_t i;
	int rc;

	for (i = 0; i < npins; i++) {
		rc = gpio_pin_configure_dt(&out_pins[i], GPIO_OUTPUT);
		zassert_equal(rc, 0, "[%d]: Port %s, pin %d, config GPIO_OUTPUT failed",
			i, out_pins[i].port->name, out_pins[i].pin);
	}
}

/**
 * @brief Test if GPIOs configured as output can have its value set.
 */
ZTEST(gpio_swd, test_set_output)
{
	uint8_t i;
	int rc;

	for (i = 0; i < npins; i++) {
		rc = gpio_pin_configure_dt(&out_pins[i], GPIO_OUTPUT_HIGH);
		zassert_equal(rc, 0, "[%d]: Port %s, pin %d, config high failed",
			i, out_pins[i].port->name, out_pins[i].pin);
		rc = gpio_pin_get_dt(&out_pins[i]);
		zassert_equal(rc, 1, "[%d]: Port %s, pin %d is not high",
			i, out_pins[i].port->name, out_pins[i].pin);

		rc = gpio_pin_configure_dt(&out_pins[i], GPIO_OUTPUT_LOW);
		zassert_equal(rc, 0, "[%d]: Port %s, pin %d, config low failed",
			i, out_pins[i].port->name, out_pins[i].pin);
		rc = gpio_pin_get_dt(&out_pins[i]);
		zassert_equal(rc, 0, "[%d]: Port %s, pin %d is not low",
			i, out_pins[i].port->name, out_pins[i].pin);
	}
}

/**
 * @brief Test if GPIOs configured as output can have its value set.
 */
ZTEST(gpio_swd, test_set_output_after_5_seconds)
{
	uint8_t i;
	int rc;

	k_msleep(5000);
	for (i = 0; i < npins; i++) {
		rc = gpio_pin_configure_dt(&out_pins[i], GPIO_OUTPUT_HIGH);
		zassert_equal(rc, 0, "[%d]: Port %s, pin %d, config high failed",
			i, out_pins[i].port->name, out_pins[i].pin);
		rc = gpio_pin_get_dt(&out_pins[i]);
		zassert_equal(rc, 1, "[%d]: Port %s, pin %d is not high",
			i, out_pins[i].port->name, out_pins[i].pin);

		rc = gpio_pin_configure_dt(&out_pins[i], GPIO_OUTPUT_LOW);
		zassert_equal(rc, 0, "[%d]: Port %s, pin %d, config low failed",
			i, out_pins[i].port->name, out_pins[i].pin);
		rc = gpio_pin_get_dt(&out_pins[i]);
		zassert_equal(rc, 0, "[%d]: Port %s, pin %d is not low",
			i, out_pins[i].port->name, out_pins[i].pin);
	}
}

static void *suite_setup(void)
{
	uint8_t i;

	TC_PRINT("Test executed on %s\n", CONFIG_BOARD_TARGET);
	TC_PRINT("GPIO count: %d\n", npins);
	for (i = 0; i < npins; i++) {
		TC_PRINT("%d: Port %s, pin %d\n", i, out_pins[i].port->name, out_pins[i].pin);
	}
	TC_PRINT("===================================================================\n");

	for (i = 0; i < npins; i++) {
		zassert_true(gpio_is_ready_dt(&out_pins[i]), "OUT[%d] is not ready", i);
	}

	return NULL;
}

ZTEST_SUITE(gpio_swd, NULL, suite_setup, NULL, NULL, NULL);
