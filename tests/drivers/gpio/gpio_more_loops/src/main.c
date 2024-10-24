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

static const struct gpio_dt_spec out1_spec = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(out_gpios),
											gpios, 0);
static const struct gpio_dt_spec in1_spec = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(in_gpios),
											gpios, 0);

#if (CONFIG_NUMBER_OF_LOOPBACKS >= 2)
#define LOOP_2_EXISTS
static const struct gpio_dt_spec out2_spec = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(out_gpios),
											gpios, 1);
static const struct gpio_dt_spec in2_spec = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(in_gpios),
											gpios, 1);
#endif

#if (CONFIG_NUMBER_OF_LOOPBACKS >= 3)
#define LOOP_3_EXISTS
static const struct gpio_dt_spec out3_spec = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(out_gpios),
											gpios, 2);
static const struct gpio_dt_spec in3_spec = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(in_gpios),
											gpios, 2);
#endif

#if (CONFIG_NUMBER_OF_LOOPBACKS >= 4)
#define LOOP_4_EXISTS
static const struct gpio_dt_spec out4_spec = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(out_gpios),
											gpios, 3);
static const struct gpio_dt_spec in4_spec = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(in_gpios),
											gpios, 3);
#endif

#if (CONFIG_NUMBER_OF_LOOPBACKS >= 5)
#define LOOP_5_EXISTS
static const struct gpio_dt_spec out5_spec = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(out_gpios),
											gpios, 4);
static const struct gpio_dt_spec in5_spec = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(in_gpios),
											gpios, 4);
#endif

#define TOGGLE_DELAY_MS 500U

/* Delay after setting GPIO ouputs. It allows signals to settle.
 */
#define PULL_DELAY_US 5000U

/* Check that input GPIOs state match with input parameter 'value'. */
static void _check_inputs(uint32_t value)
{
	bool current, expected;

	LOG_DBG("_check_inputs(%d)", value);
	/* stabilize logic level */
	k_busy_wait(PULL_DELAY_US);

	current = (gpio_pin_get_dt(&in1_spec) == 1) ? true : false;
	expected = (value & BIT(0)) ? true : false;
	LOG_DBG("_check_inputs L1: current: %d, expected: %d", current, expected);
	zassert_equal(current, expected, "IN_1 = %d, while expected %d", current, expected);

#if defined(LOOP_2_EXISTS)
	current = (gpio_pin_get_dt(&in2_spec) == 1) ? true : false;
	expected = (value & BIT(1)) ? true : false;
	LOG_DBG("_check_inputs L2: current: %d, expected: %d", current, expected);
	zassert_equal(current, expected, "IN_2 = %d, while expected %d", current, expected);
#endif

#if defined(LOOP_3_EXISTS)
	current = (gpio_pin_get_dt(&in3_spec) == 1) ? true : false;
	expected = (value & BIT(2)) ? true : false;
	LOG_DBG("_check_inputs L3: current: %d, expected: %d", current, expected);
	zassert_equal(current, expected, "IN_3 = %d, while expected %d", current, expected);
#endif

#if defined(LOOP_4_EXISTS)
	current = (gpio_pin_get_dt(&in4_spec) == 1) ? true : false;
	expected = (value & BIT(3)) ? true : false;
	LOG_DBG("_check_inputs L4: current: %d, expected: %d", current, expected);
	zassert_equal(current, expected, "IN_4 = %d, while expected %d", current, expected);
#endif

#if defined(LOOP_5_EXISTS)
	current = (gpio_pin_get_dt(&in5_spec) == 1) ? true : false;
	expected = (value & BIT(4)) ? true : false;
	LOG_DBG("_check_inputs L5: current: %d, expected: %d", current, expected);
	zassert_equal(current, expected, "IN_5 = %d, while expected %d", current, expected);
#endif
}

/* Set output GPIOs with gpio_pin_set_dt()
 * and confirm that input GPIOs have expected state.
 */
static void _gpio_pin_set_dt_and_check(uint32_t value)
{
	int rc;

	LOG_DBG("_gpio_pin_set_dt_and_check(%d)", value);
	/* OUT_1 */
	if (value & BIT(0)) {
		rc = gpio_pin_set_dt(&out1_spec, 1);
		LOG_DBG("_gpio_pin_set_dt_and_check L1: setting pin high");
	} else {
		rc = gpio_pin_set_dt(&out1_spec, 0);
		LOG_DBG("_gpio_pin_set_dt_and_check L1: setting pin low");
	}
	zassert_equal(rc, 0, "gpio_pin_set_dt(OUT_1) failed");

#if defined(LOOP_2_EXISTS)
	if (value & BIT(1)) {
		rc = gpio_pin_set_dt(&out2_spec, 1);
		LOG_DBG("_gpio_pin_set_dt_and_check L2: setting pin high");
	} else {
		rc = gpio_pin_set_dt(&out2_spec, 0);
		LOG_DBG("_gpio_pin_set_dt_and_check L2: setting pin low");
	}
	zassert_equal(rc, 0, "gpio_pin_set_dt(OUT_2) failed");
#endif

#if defined(LOOP_3_EXISTS)
	if (value & BIT(2)) {
		rc = gpio_pin_set_dt(&out3_spec, 1);
		LOG_DBG("_gpio_pin_set_dt_and_check L3: setting pin high");
	} else {
		rc = gpio_pin_set_dt(&out3_spec, 0);
		LOG_DBG("_gpio_pin_set_dt_and_check L3: setting pin low");
	}
	zassert_equal(rc, 0, "gpio_pin_set_dt(OUT_3) failed");
#endif

#if defined(LOOP_4_EXISTS)
	if (value & BIT(3)) {
		rc = gpio_pin_set_dt(&out4_spec, 1);
		LOG_DBG("_gpio_pin_set_dt_and_check L4: setting pin high");
	} else {
		rc = gpio_pin_set_dt(&out4_spec, 0);
		LOG_DBG("_gpio_pin_set_dt_and_check L4: setting pin low");
	}
	zassert_equal(rc, 0, "gpio_pin_set_dt(OUT_4) failed");
#endif

#if defined(LOOP_5_EXISTS)
	if (value & BIT(4)) {
		rc = gpio_pin_set_dt(&out5_spec, 1);
		LOG_DBG("_gpio_pin_set_dt_and_check L5: setting pin high");
	} else {
		rc = gpio_pin_set_dt(&out5_spec, 0);
		LOG_DBG("_gpio_pin_set_dt_and_check L5: setting pin low");
	}
	zassert_equal(rc, 0, "gpio_pin_set_dt(OUT_5) failed");
#endif

	/* Check inputs */
	_check_inputs(value);
}

/**
 * @brief Test GPIOs are working during CONFIG_TEST_DURATION.
 */
ZTEST(gpio_loopbacks, test_gpios_are_working)
{
	for (int total_delay = 0; total_delay <= CONFIG_TEST_DURATION;
		total_delay += (2 * TOGGLE_DELAY_MS)) {

		_gpio_pin_set_dt_and_check(0b10101);
		k_msleep(TOGGLE_DELAY_MS);
		_gpio_pin_set_dt_and_check(0b01010);
		k_msleep(TOGGLE_DELAY_MS);
	}
}

static void *suite_setup(void)
{
	int rc;

	TC_PRINT("Test executed on %s\n", CONFIG_BOARD_TARGET);
	TC_PRINT("Exisitng GPIO Loops:\n");
	TC_PRINT("GPIO Loop 1\n");
#if defined(LOOP_2_EXISTS)
	TC_PRINT("GPIO Loop 2\n");
#endif
#if defined(LOOP_3_EXISTS)
	TC_PRINT("GPIO Loop 3\n");
#endif
#if defined(LOOP_4_EXISTS)
	TC_PRINT("GPIO Loop 4\n");
#endif
#if defined(LOOP_5_EXISTS)
	TC_PRINT("GPIO Loop 5\n");
#endif
	TC_PRINT("===================================================================\n");

	zassert_true(gpio_is_ready_dt(&in1_spec), "GPIO IN 1 is not ready");
	zassert_true(gpio_is_ready_dt(&out1_spec), "GPIO OUT 1 is not ready");

	rc = gpio_pin_configure_dt(&in1_spec, GPIO_INPUT);
	zassert_equal(rc, 0, "GPIO IN 1 config failed");
	rc = gpio_pin_configure_dt(&out1_spec, GPIO_OUTPUT);
	zassert_equal(rc, 0, "GPIO OUT 1 config failed");
	LOG_DBG("L1: inputs/outputs were configured");

#if defined(LOOP_2_EXISTS)
	zassert_true(gpio_is_ready_dt(&in2_spec), "GPIO IN 2 is not ready");
	zassert_true(gpio_is_ready_dt(&out2_spec), "GPIO OUT 2 is not ready");

	rc = gpio_pin_configure_dt(&in2_spec, GPIO_INPUT);
	zassert_equal(rc, 0, "GPIO IN 2 config failed");
	rc = gpio_pin_configure_dt(&out2_spec, GPIO_OUTPUT);
	zassert_equal(rc, 0, "GPIO OUT 2 config failed");
	LOG_DBG("L2: inputs/outputs were configured");
#endif

#if defined(LOOP_3_EXISTS)
	zassert_true(gpio_is_ready_dt(&in3_spec), "GPIO IN 3 is not ready");
	zassert_true(gpio_is_ready_dt(&out3_spec), "GPIO OUT 3 is not ready");

	rc = gpio_pin_configure_dt(&in3_spec, GPIO_INPUT);
	zassert_equal(rc, 0, "GPIO IN 3 config failed");
	rc = gpio_pin_configure_dt(&out3_spec, GPIO_OUTPUT);
	zassert_equal(rc, 0, "GPIO OUT 3 config failed");
	LOG_DBG("L3: inputs/outputs were configured");
#endif

#if defined(LOOP_4_EXISTS)
	zassert_true(gpio_is_ready_dt(&in4_spec), "GPIO IN 4 is not ready");
	zassert_true(gpio_is_ready_dt(&out4_spec), "GPIO OUT 4 is not ready");

	rc = gpio_pin_configure_dt(&in4_spec, GPIO_INPUT);
	zassert_equal(rc, 0, "GPIO IN 4 config failed");
	rc = gpio_pin_configure_dt(&out4_spec, GPIO_OUTPUT);
	zassert_equal(rc, 0, "GPIO OUT 4 config failed");
	LOG_DBG("L4: inputs/outputs were configured");
#endif

#if defined(LOOP_5_EXISTS)
	zassert_true(gpio_is_ready_dt(&in5_spec), "GPIO IN 5 is not ready");
	zassert_true(gpio_is_ready_dt(&out5_spec), "GPIO OUT 5 is not ready");

	rc = gpio_pin_configure_dt(&in5_spec, GPIO_INPUT);
	zassert_equal(rc, 0, "GPIO IN 5 config failed");
	rc = gpio_pin_configure_dt(&out5_spec, GPIO_OUTPUT);
	zassert_equal(rc, 0, "GPIO OUT 5 config failed");
	LOG_DBG("L5: inputs/outputs were configured");
#endif

	return NULL;
}

ZTEST_SUITE(gpio_loopbacks, NULL, suite_setup, NULL, NULL, NULL);
