/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hpf_gpio_loops, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

#if !DT_NODE_EXISTS(DT_NODELABEL(out_gpios)) || !DT_NODE_EXISTS(DT_NODELABEL(in_gpios))
#error "Unsupported board: test_gpio node is not defined"
#endif

/* Delay after setting GPIO ouputs. It allows signals to settle. */
#define PROPAGATION_DELAY_MS K_MSEC(1U)

/* ASSUMPTION: All output GPIOs are on the same device */
const struct gpio_dt_spec out_pins[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_NODELABEL(out_gpios), gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))
};
const struct gpio_dt_spec in_pins[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_NODELABEL(in_gpios), gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))
};
BUILD_ASSERT(ARRAY_SIZE(in_pins) == ARRAY_SIZE(out_pins), "mismatched in/out pairs");
const uint8_t npairs = ARRAY_SIZE(in_pins);


/* Check that input GPIOs state match with input parameter 'value'. */
static void check_inputs(uint32_t value)
{
	bool current;
	bool expected;

	LOG_DBG("check_inputs(%u)", value);

	/* Wait a bit to stabilize logic level. */
	k_sleep(PROPAGATION_DELAY_MS);

	for (uint8_t i = 0; i < npairs; i++) {
		current = gpio_pin_get_dt(&in_pins[i]);
		expected = value & BIT(i);
		LOG_DBG("check_inputs L[%u]: current: %u, expected: %u", i, current, expected);
		zassert_equal(current, expected,
			"IN[%u] = %u, while expected %u", i, current, expected);
	}
}

/* Set output GPIOs with gpio_pin_configure()
 * and confirm that input GPIOs have expected state.
 */
static void gpio_pin_configure_and_check(uint32_t value)
{
	int rc;

	LOG_DBG("gpio_pin_configure_and_check(%u)", value);

	for (uint8_t i = 0; i < npairs; i++) {
		/* Disconnect output */
		rc = gpio_pin_configure_dt(&out_pins[i], GPIO_DISCONNECTED);
		if (rc == -ENOTSUP) {
			TC_PRINT("GPIO_DISCONNECTED not supported, trying with GPIO_INPUT\n");
			rc = gpio_pin_configure_dt(&out_pins[i], GPIO_INPUT);
		}
		zassert_equal(rc, 0,
			"gpio_pin_configure_dt(&out_pins[%u], GPIO_INPUT) failed", i);

		/* Connect output and set logic level */
		if (value & BIT(i)) {
			rc = gpio_pin_configure_dt(&out_pins[i], GPIO_OUTPUT_HIGH);
			LOG_DBG("gpio_pin_configure_dt(&out_pins[%u], GPIO_OUTPUT_HIGH)", i);
		} else {
			rc = gpio_pin_configure_dt(&out_pins[i], GPIO_OUTPUT_LOW);
			LOG_DBG("gpio_pin_configure_dt(&out_pins[%u], GPIO_OUTPUT_LOW)", i);
		}
		zassert_equal(rc, 0, "PIN_OUT_1 config as output failed");
	}

	/* Check inputs */
	check_inputs(value);
}

/* Helper function for gpio_port_set_masked_raw() / gpio_port_set_masked() test. */
static void gpio_port_set_masked_generic_and_check(uint32_t value, bool raw_variant)
{
	int rc;
	gpio_port_pins_t port_pins = 0;
	gpio_port_value_t port_value = 0;

	LOG_DBG("gpio_port_set_masked_generic_and_check(%u, %u)", value, raw_variant);

	for (uint8_t i = 0; i < npairs; i++) {
		port_pins |= (gpio_port_pins_t)BIT(out_pins[i].pin);
		port_value |= (value & BIT(i)) ? (gpio_port_value_t)BIT(out_pins[i].pin) : 0;
	}

	if (raw_variant) {
		rc = gpio_port_set_masked_raw(out_pins[0].port, port_pins, port_value);
		zassert_equal(rc, 0, "gpio_port_set_masked_raw(port, %u, %u) failed",
			port_pins, port_value);
		LOG_DBG("gpio_port_set_masked_raw(port, %u, %u)", port_pins, port_value);
	} else {
		rc = gpio_port_set_masked(out_pins[0].port, port_pins, port_value);
		zassert_equal(rc, 0, "gpio_port_set_masked(port, %u, %u) failed",
			port_pins, port_value);
		LOG_DBG("gpio_port_set_masked(port, %u, %u)", port_pins, port_value);
	}

	/* Check inputs */
	check_inputs(value);
}

/* Set output GPIOs with gpio_port_set_masked_raw()
 * and confirm that input GPIOs have expected state.
 */
static void gpio_port_set_masked_raw_and_check(uint32_t value)
{
	gpio_port_set_masked_generic_and_check(value, true);
}

/* Set output GPIOs with gpio_port_set_masked()
 * and confirm that input GPIOs have expected state.
 */
static void gpio_port_set_masked_and_check(uint32_t value)
{
	gpio_port_set_masked_generic_and_check(value, false);
}

/* Helper function for gpio_port_set_bits_raw() / gpio_port_set_bits() test. */
static void gpio_port_set_clear_bits_generic_and_check(uint32_t value, bool raw_variant)
{
	int rc;

	LOG_DBG("gpio_port_set_clear_bits_generic_and_check(%u)", value);

	for (uint8_t i = 0; i < npairs; i++) {
		if (raw_variant) {
			if (value & BIT(i)) {
				rc = gpio_port_set_bits_raw(out_pins[i].port,
					(gpio_port_pins_t)BIT(out_pins[i].pin));
				zassert_equal(rc, 0,
					"gpio_port_set_bits_raw(PIN_OUT_%u) failed", i);
				LOG_DBG("gpio_port_set_bits_raw(PIN_OUT_%u)", i);
			} else {
				rc = gpio_port_clear_bits_raw(out_pins[i].port,
					(gpio_port_pins_t)BIT(out_pins[i].pin));
				zassert_equal(rc, 0,
					"gpio_port_clear_bits_raw(PIN_OUT_%u) failed", i);
				LOG_DBG("gpio_port_clear_bits_raw(PIN_OUT_%u)", i);
			}
		} else {
			if (value & BIT(i)) {
				rc = gpio_port_set_bits(out_pins[i].port,
					(gpio_port_pins_t)BIT(out_pins[i].pin));
				zassert_equal(rc, 0, "gpio_port_set_bits(PIN_OUT_%u) failed", i);
				LOG_DBG("gpio_port_set_bits(PIN_OUT_%u)", i);
			} else {
				rc = gpio_port_clear_bits(out_pins[i].port,
					(gpio_port_pins_t)BIT(out_pins[i].pin));
				zassert_equal(rc, 0, "gpio_port_clear_bits(PIN_OUT_%u) failed", i);
				LOG_DBG("gpio_port_clear_bits(PIN_OUT_%u)", i);
			}
		}
	}

	/* Check inputs */
	check_inputs(value);
}

/* Set output GPIOs with gpio_port_[set|clear]_bits_raw()
 * and confirm that input GPIOs have expected state.
 */
static void gpio_port_set_clear_bits_raw_and_check(uint32_t value)
{
	gpio_port_set_clear_bits_generic_and_check(value, true);
}

/* Set output GPIOs with gpio_port_[set|clear]_bits()
 * and confirm that input GPIOs have expected state.
 */
static void gpio_port_set_clear_bits_and_check(uint32_t value)
{
	gpio_port_set_clear_bits_generic_and_check(value, false);
}

/* Set output GPIOs with gpio_port_toggle_bits()
 * and confirm that input GPIOs have expected state.
 */
static void gpio_port_toggle_bits_and_check(uint32_t value)
{
	int rc;
	gpio_port_pins_t port_pins = 0;

	LOG_DBG("gpio_port_toggle_bits_and_check(%u)", value);

	/* Set initial state 00000 */
	for (uint8_t i = 0; i < npairs; i++) {
		port_pins |= (gpio_port_pins_t)BIT(out_pins[i].pin);
	}
	rc = gpio_port_set_masked_raw(out_pins[0].port, port_pins, 0);
	zassert_equal(rc, 0, "gpio_port_set_masked_raw(port, %u, 0) failed", port_pins);

	/* All outputs are now 0, check which pins shall be toggled */
	port_pins = 0;
	for (uint8_t i = 0; i < npairs; i++) {
		port_pins |= (value & BIT(i)) ? (gpio_port_pins_t)BIT(out_pins[i].pin) : 0;
	}

	rc = gpio_port_toggle_bits(out_pins[0].port, port_pins);
	zassert_equal(rc, 0, "gpio_port_toggle_bits(port, %u) failed", port_pins);
	LOG_DBG("gpio_port_toggle_bits(port, %u)", port_pins);

	/* Check inputs */
	check_inputs(value);
}

/* Helper function for gpio_port_set_clr_bits_raw() / gpio_port_set_clr_bits() test. */
static void gpio_port_set_clr_bits_generic_and_check(uint32_t value, bool raw_variant)
{
	int rc;
	gpio_port_pins_t set_pins = 0;
	gpio_port_pins_t clear_pins = 0;

	LOG_DBG("gpio_port_set_clr_bits_generic_and_check(%u)", value);

	for (uint8_t i = 0; i < npairs; i++) {
		if (value & BIT(i)) {
			set_pins |= (gpio_port_pins_t)BIT(out_pins[i].pin);
		} else {
			clear_pins |= (gpio_port_pins_t)BIT(out_pins[i].pin);
		}
	}

	if (raw_variant) {
		rc = gpio_port_set_clr_bits_raw(out_pins[0].port, set_pins, clear_pins);
		zassert_equal(rc, 0, "gpio_port_set_clr_bits_raw(port, %u, %u) failed",
			set_pins, clear_pins);
		LOG_DBG("gpio_port_set_clr_bits_raw(port, %u, %u)", set_pins, clear_pins);
	} else {
		rc = gpio_port_set_clr_bits(out_pins[0].port, set_pins, clear_pins);
		zassert_equal(rc, 0, "gpio_port_set_clr_bits(port, %u, %u) failed",
			set_pins, clear_pins);
		LOG_DBG("gpio_port_set_clr_bits(port, %u, %u)", set_pins, clear_pins);
	}

	/* Check inputs */
	check_inputs(value);
}

/* Set output GPIOs with gpio_port_set_clr_bits_raw()
 * and confirm that input GPIOs have expected state.
 */
static void gpio_port_set_clr_bits_raw_and_check(uint32_t value)
{
	gpio_port_set_clr_bits_generic_and_check(value, true);
}

/* Set output GPIOs with gpio_port_set_clr_bits()
 * and confirm that input GPIOs have expected state.
 */
static void gpio_port_set_clr_bits_and_check(uint32_t value)
{
	gpio_port_set_clr_bits_generic_and_check(value, false);
}

/* Helper function for gpio_pin_set_raw() / gpio_pin_set() test. */
static void gpio_pin_set_generic_and_check(uint32_t value, bool raw_variant)
{
	int rc;
	int pin_value;

	LOG_DBG("gpio_pin_set_generic_and_check(%u)", value);

	for (uint8_t i = 0; i < npairs; i++) {
		pin_value = (value & BIT(i)) >> i;
		if (raw_variant) {
			rc = gpio_pin_set_raw(out_pins[i].port, out_pins[i].pin, pin_value);
			zassert_equal(rc, 0, "gpio_pin_set_raw(port, PIN_OUT_%u, %u) failed",
				i, pin_value);
			LOG_DBG("gpio_pin_set_raw(port, PIN_OUT_%u, %u)", i, pin_value);
		} else {
			rc = gpio_pin_set(out_pins[i].port, out_pins[i].pin, pin_value);
			zassert_equal(rc, 0, "gpio_pin_set(port, PIN_OUT_%u, %u) failed",
				i, pin_value);
			LOG_DBG("gpio_pin_set(port, PIN_OUT_%u, %u)", i, pin_value);
		}
	}

	/* Check inputs */
	check_inputs(value);
}

/* Set output GPIOs with gpio_pin_set_raw()
 * and confirm that input GPIOs have expected state.
 */
static void gpio_pin_set_raw_and_check(uint32_t value)
{
	gpio_pin_set_generic_and_check(value, true);
}

/* Set output GPIOs with gpio_pin_set()
 * and confirm that input GPIOs have expected state.
 */
static void gpio_pin_set_and_check(uint32_t value)
{
	gpio_pin_set_generic_and_check(value, false);
}

/* Set output GPIOs with gpio_pin_toggle()
 *and confirm that input GPIOs have expected state.
 */
static void gpio_pin_toggle_and_check(uint32_t value)
{
	int rc;
	gpio_port_pins_t port_pins = 0;

	LOG_DBG("gpio_pin_toggle_and_check(%u)", value);

	/* Set initial state 00000 */
	for (uint8_t i = 0; i < npairs; i++) {
		port_pins |= (gpio_port_pins_t)BIT(out_pins[i].pin);
	}
	rc = gpio_port_set_masked_raw(out_pins[0].port, port_pins, 0);
	zassert_equal(rc, 0, "gpio_port_set_masked_raw(port, %u, 0) failed", port_pins);

	for (uint8_t i = 0; i < npairs; i++) {
		if (value & BIT(i)) {
			rc = gpio_pin_toggle(out_pins[i].port, out_pins[i].pin);
			zassert_equal(rc, 0, "gpio_pin_toggle(port, PIN_OUT_%u) failed", i);
			LOG_DBG("gpio_pin_toggle(port, PIN_OUT_%u)", i);
		}
	}

	/* Check inputs */
	check_inputs(value);
}

/* Generate "moving 1" pattern on outputs.
 * Set outputs (and check inputs) using function provided in the fn parameter.
 */
static void generic_moving_1(void (*fn)(uint32_t))
{
	int value;

	/* Set/check 111..11 */
	(*fn)(0xFFFFFFFFU);

	/* Set/check 000..00 */
	(*fn)(0U);

	/* Moving 1 */
	for (uint16_t i = 0; i < (2 * npairs - 1); i++) {
		if (i == 0) {
			value = 1;
			/* Set/check 00001 */
			(*fn)(value);
		} else if (i < npairs) {
			/* shift left */
			value = value << 1;
			(*fn)(value);
		} else {
			/* shift right */
			value = value >> 1;
			(*fn)(value);
		}
	}

	/* Set/check 000..00 */
	(*fn)(0U);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_pin_configure()
 */
ZTEST(hpf_gpio_extended, test_moving_1_with_configure_output)
{
	generic_moving_1(&gpio_pin_configure_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_port_set_masked_raw()
 */
ZTEST(hpf_gpio_extended, test_moving_1_with_port_set_masked_raw)
{
	generic_moving_1(&gpio_port_set_masked_raw_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_port_set_masked()
 */
ZTEST(hpf_gpio_extended, test_moving_1_with_port_set_masked)
{
	generic_moving_1(&gpio_port_set_masked_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_port_set_bits_raw() and gpio_port_clear_bits_raw()
 */
ZTEST(hpf_gpio_extended, test_moving_1_with_set_clear_bits_raw)
{
	generic_moving_1(&gpio_port_set_clear_bits_raw_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_port_set_bits() and gpio_port_clear_bits()
 */
ZTEST(hpf_gpio_extended, test_moving_1_with_set_clear_bits)
{
	generic_moving_1(&gpio_port_set_clear_bits_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_port_toggle_bits()
 */
ZTEST(hpf_gpio_extended, test_moving_1_with_gpio_port_toggle_bits)
{
	generic_moving_1(&gpio_port_toggle_bits_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_port_set_clr_bits_raw()
 */
ZTEST(hpf_gpio_extended, test_moving_1_with_gpio_port_set_clr_bits_raw)
{
	generic_moving_1(&gpio_port_set_clr_bits_raw_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_port_set_clr_bits()
 */
ZTEST(hpf_gpio_extended, test_moving_1_with_gpio_port_set_clr_bits)
{
	generic_moving_1(&gpio_port_set_clr_bits_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_pin_set_raw()
 */
ZTEST(hpf_gpio_extended, test_moving_1_with_gpio_pin_set_raw)
{
	generic_moving_1(&gpio_pin_set_raw_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_pin_set()
 */
ZTEST(hpf_gpio_extended, test_moving_1_with_gpio_pin_set)
{
	generic_moving_1(&gpio_pin_set_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_pin_toggle()
 */
ZTEST(hpf_gpio_extended, test_moving_1_with_gpio_pin_toggle)
{
	generic_moving_1(&gpio_pin_toggle_and_check);
}

/**
 * @brief Stress test - send many GPIO requests one by one
 */
ZTEST(hpf_gpio_extended, test_stress_gpio_set)
{
	int rc;
	int rc_acc = 0;
	gpio_port_pins_t port_pins = 0;
	gpio_port_value_t port_value;

	for (int i = 0; i < npairs; i++) {
		port_pins |= (gpio_port_pins_t)BIT(out_pins[i].pin);
	}

	for (int loop = 0; loop < 200000; loop++) {
		port_value = 0;
		for (int pair = 0; pair < npairs; pair++) {
			port_value |= (loop % (pair + 2)) ? BIT(out_pins[pair].pin) : 0;

		}
		rc = gpio_port_set_masked_raw(out_pins[0].port, port_pins, port_value);
		/* If TX buffer is full, wait a bit and retry */
		while (rc == -EIO) {
			k_usleep(100);
			rc = gpio_port_set_masked_raw(out_pins[0].port, port_pins, port_value);
		}

		/* Report any other error */
		if (rc) {
			TC_PRINT("%d: rc = %d\n", loop, rc);
			rc_acc += rc;
		}
	}
	zassert_equal(rc_acc, 0, "at least one set operation failed, rc_acc = %d)", rc_acc);
}

static void *suite_setup(void)
{
	uint8_t i;
	int rc;

	TC_PRINT("Test executed on %s\n", CONFIG_BOARD_TARGET);

	TC_PRINT("-> Test uses following backend: ");
	if (IS_ENABLED(CONFIG_GPIO_HPF_GPIO_BACKEND_ICMSG)) {
		TC_PRINT("ICMsg\n");
	} else if (IS_ENABLED(CONFIG_GPIO_HPF_GPIO_BACKEND_ICBMSG)) {
		TC_PRINT("ICBMsg\n");
	} else if (IS_ENABLED(CONFIG_GPIO_HPF_GPIO_BACKEND_MBOX)) {
		TC_PRINT("MBOX\n");
	} else {
		TC_PRINT("unknown\n");
	}
	TC_PRINT("GPIO Loops count: %u\n", npairs);
	TC_PRINT("===================================================================\n");

	for (i = 0; i < npairs; i++) {
		zassert_true(gpio_is_ready_dt(&in_pins[i]), "IN[%u] is not ready", i);
		zassert_true(gpio_is_ready_dt(&out_pins[i]), "OUT[%u] is not ready", i);
	}

	for (i = 0; i < npairs; i++) {
		rc = gpio_pin_configure_dt(&in_pins[i], GPIO_INPUT);
		zassert_equal(rc, 0, "IN[%u] config failed (%d)", i, rc);
		rc = gpio_pin_configure_dt(&out_pins[i], GPIO_OUTPUT);
		zassert_equal(rc, 0, "OUT[%u] config failed (%d)", i, rc);
		LOG_DBG("IN[%u] - OUT[%u] were configured", i, i);
	}

	return NULL;
}

ZTEST_SUITE(hpf_gpio_extended, NULL, suite_setup, NULL, NULL, NULL);
