/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_loops, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

#if !DT_NODE_EXISTS(DT_NODELABEL(out_gpios)) || !DT_NODE_EXISTS(DT_NODELABEL(in_gpios))
#error "Unsupported board: test_gpio node is not defined"
#endif

/* Delay after setting GPIO ouputs. It allows signals to settle. */
#define PROPAGATION_DELAY_MS K_MSEC(1U)

/* Delay between GPIO toggle. */
#define TOGGLE_DELAY_MS 500U

const struct gpio_dt_spec out_pins[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_NODELABEL(out_gpios), gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))
};
const struct gpio_dt_spec in_pins[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_NODELABEL(in_gpios), gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))
};
BUILD_ASSERT(ARRAY_SIZE(in_pins) == ARRAY_SIZE(out_pins), "mismatched in/out pairs");
const uint8_t npairs = ARRAY_SIZE(in_pins);


/* Check that input GPIOs state match with input parameter 'value'. */
static void _check_inputs(uint32_t value)
{
	bool current;
	bool expected;

	LOG_DBG("_check_inputs(%d)", value);

	/* Wait a bit to stabilize logic level. */
	k_sleep(PROPAGATION_DELAY_MS);

	for (uint8_t i = 0; i < npairs; i++) {
		current = gpio_pin_get_dt(&in_pins[i]);
		expected = value & BIT(i);
		LOG_DBG("_check_inputs L[%d]: current: %d, expected: %d", i, current, expected);
		zassert_equal(current, expected,
			"IN[%d] = %d, while expected %d", i, current, expected);
	}
}

/* Set output GPIOs with gpio_pin_set_dt()
 * and confirm that input GPIOs have expected state.
 */
static void _gpio_pin_set_dt_and_check(uint32_t value)
{
	int out_state;
	int rc;

	LOG_DBG("_gpio_pin_set_dt_and_check(%d)", value);

	for (uint8_t i = 0; i < npairs; i++) {
		out_state = (value & BIT(i)) >> i;
		rc = gpio_pin_set_dt(&out_pins[i], out_state);
		LOG_DBG("_gpio_pin_set_dt_and_check: setting OUT[%d] to %d", i, out_state);
		zassert_equal(rc, 0, "gpio_pin_set_dt(OUT[%d], %d) failed", i, out_state);
	}

	/* Check inputs. */
	_check_inputs(value);
}

/**
 * @brief Test GPIOs are working for CONFIG_TEST_DURATION ms.
 */
ZTEST(gpio_more_loops, test_gpios_are_working)
{
	for (int total_delay = 0; total_delay <= CONFIG_TEST_DURATION;
		total_delay += (2 * TOGGLE_DELAY_MS)) {

		_gpio_pin_set_dt_and_check(0b01010101010101010101010101010101);
		k_msleep(TOGGLE_DELAY_MS);
		_gpio_pin_set_dt_and_check(0b10101010101010101010101010101010);
		k_msleep(TOGGLE_DELAY_MS);
	}
}

static void *suite_setup(void)
{
	uint8_t i;
	int rc;

	TC_PRINT("Test executed on %s\n", CONFIG_BOARD_TARGET);
	TC_PRINT("GPIO Loops count: %d\n", npairs);
	TC_PRINT("===================================================================\n");

	for (i = 0; i < npairs; i++) {
		zassert_true(gpio_is_ready_dt(&in_pins[i]), "IN[%d] is not ready", i);
		zassert_true(gpio_is_ready_dt(&out_pins[i]), "OUT[%d] is not ready", i);

	}

	for (i = 0; i < npairs; i++) {
		rc = gpio_pin_configure_dt(&in_pins[i], GPIO_INPUT | GPIO_PULL_UP);
		zassert_equal(rc, 0, "IN[%d] config failed", i);
	}
	_check_inputs(0b11111111111111111111111111111111);
	TC_PRINT("Input pull up works.\n");

	for (i = 0; i < npairs; i++) {
		rc = gpio_pin_configure_dt(&in_pins[i], GPIO_INPUT | GPIO_PULL_DOWN);
		zassert_equal(rc, 0, "IN[%d] config failed", i);
	}
	_check_inputs(0b00000000000000000000000000000000);
	TC_PRINT("Input pull down works.\n");

	for (i = 0; i < npairs; i++) {
		rc = gpio_pin_configure_dt(&in_pins[i], GPIO_INPUT);
		zassert_equal(rc, 0, "IN[%d] config failed", i);
		rc = gpio_pin_configure_dt(&out_pins[i], GPIO_OUTPUT);
		zassert_equal(rc, 0, "OUT[%d] config failed", i);
		LOG_DBG("IN[%d] - OUT[%d] were configured", i, i);
	}

	return NULL;
}

ZTEST_SUITE(gpio_more_loops, NULL, suite_setup, NULL, NULL, NULL);
