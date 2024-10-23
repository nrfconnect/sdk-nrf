/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "main.h"

/* Assumption: All input GPIOs are on the same device */
/* Assumption: All output GPIOs are on the same device */
static const struct device *const dev_out = DEVICE_DT_GET(DEV_OUT);
static const struct device *const dev_in = DEVICE_DT_GET(DEV_IN);

/* Delay after setting GPIO ouputs. It allows signals to settle.
 */
#define PULL_DELAY_US 1000U

/* Check that input GPIOs state match with input parameter 'value'. */
static void _check_inputs(uint32_t value)
{
	int rc;
	gpio_port_value_t v1;
	bool current, expected;

	/* stabilize logic level */
	k_busy_wait(PULL_DELAY_US);

	/* get input port state */
	rc = gpio_port_get_raw(dev_in, &v1);
	zassert_equal(rc, 0, "gpio_port_get_raw(dev_in) failed");

	/* Check each bit has expected state
	 * MSB [IN_5, IN_4, IN_3, IN_2, IN_1] LSB
	 */
	current = (v1 & BIT(PIN_IN_1)) ? true : false;
	expected = (value & BIT(0)) ? true : false;
	zassert_equal(current, expected, "IN_1 = %d, while expected %d", current, expected);
	current = (v1 & BIT(PIN_IN_2)) ? true : false;
	expected = (value & BIT(1)) ? true : false;
	zassert_equal(current, expected, "IN_2 = %d, while expected %d", current, expected);
	current = (v1 & BIT(PIN_IN_3)) ? true : false;
	expected = (value & BIT(2)) ? true : false;
	zassert_equal(current, expected, "IN_3 = %d, while expected %d", current, expected);
	current = (v1 & BIT(PIN_IN_4)) ? true : false;
	expected = (value & BIT(3)) ? true : false;
	zassert_equal(current, expected, "IN_4 = %d, while expected %d", current, expected);
	current = (v1 & BIT(PIN_IN_5)) ? true : false;
	expected = (value & BIT(4)) ? true : false;
	zassert_equal(current, expected, "IN_5 = %d, while expected %d", current, expected);
}

/* Set output GPIOs with gpio_pin_configure() and confirm that input GPIOs have expected state. */
static void _gpio_pin_configure_and_check(uint32_t value)
{
	int rc;

	/* OUT_1 */
	/* Disconnect output */
	rc = gpio_pin_configure(dev_out, PIN_OUT_1, GPIO_DISCONNECTED);
	if (rc == -ENOTSUP) {
		TC_PRINT(
			"gpio_pin_configure(PIN_OUT_1, GPIO_DISCONNECTED) failed; trying with GPIO_INPUT\n");
		rc = gpio_pin_configure(dev_out, PIN_OUT_1, GPIO_INPUT);
	}
	zassert_equal(rc, 0, "gpio_pin_configure(dev, PIN_OUT_1, GPIO_INPUT) failed");
	/* Set output logic level */
	if (value & BIT(0)) {
		rc = gpio_pin_configure(dev_out, PIN_OUT_1, GPIO_OUTPUT_HIGH);
	} else {
		rc = gpio_pin_configure(dev_out, PIN_OUT_1, GPIO_OUTPUT_LOW);
	}
	zassert_equal(rc, 0, "PIN_OUT_1 config as output failed");

	/* OUT_2 */
	/* Disconnect output */
	rc = gpio_pin_configure(dev_out, PIN_OUT_2, GPIO_DISCONNECTED);
	if (rc == -ENOTSUP) {
		TC_PRINT(
			"gpio_pin_configure(PIN_OUT_2, GPIO_DISCONNECTED) failed; trying with GPIO_INPUT\n");
		rc = gpio_pin_configure(dev_out, PIN_OUT_2, GPIO_INPUT);
	}
	zassert_equal(rc, 0, "gpio_pin_configure(dev, PIN_OUT_2, GPIO_INPUT) failed");
	/* Set output logic level */
	if (value & BIT(1)) {
		rc = gpio_pin_configure(dev_out, PIN_OUT_2, GPIO_OUTPUT_HIGH);
	} else {
		rc = gpio_pin_configure(dev_out, PIN_OUT_2, GPIO_OUTPUT_LOW);
	}
	zassert_equal(rc, 0, "PIN_OUT_2 config as output failed");

	/* OUT_3 */
	/* Disconnect output */
	rc = gpio_pin_configure(dev_out, PIN_OUT_3, GPIO_DISCONNECTED);
	if (rc == -ENOTSUP) {
		TC_PRINT(
			"gpio_pin_configure(PIN_OUT_3, GPIO_DISCONNECTED) failed; trying with GPIO_INPUT\n");
		rc = gpio_pin_configure(dev_out, PIN_OUT_3, GPIO_INPUT);
	}
	zassert_equal(rc, 0, "gpio_pin_configure(dev, PIN_OUT_3, GPIO_INPUT) failed");
	/* Set output logic level */
	if (value & BIT(2)) {
		rc = gpio_pin_configure(dev_out, PIN_OUT_3, GPIO_OUTPUT_HIGH);
	} else {
		rc = gpio_pin_configure(dev_out, PIN_OUT_3, GPIO_OUTPUT_LOW);
	}
	zassert_equal(rc, 0, "PIN_OUT_3 config as output failed");

	/* OUT_4 */
	/* Disconnect output */
	rc = gpio_pin_configure(dev_out, PIN_OUT_4, GPIO_DISCONNECTED);
	if (rc == -ENOTSUP) {
		TC_PRINT(
			"gpio_pin_configure(PIN_OUT_4, GPIO_DISCONNECTED) failed; trying with GPIO_INPUT\n");
		rc = gpio_pin_configure(dev_out, PIN_OUT_4, GPIO_INPUT);
	}
	zassert_equal(rc, 0, "gpio_pin_configure(dev, PIN_OUT_4, GPIO_INPUT) failed");
	/* Set output logic level */
	if (value & BIT(3)) {
		rc = gpio_pin_configure(dev_out, PIN_OUT_4, GPIO_OUTPUT_HIGH);
	} else {
		rc = gpio_pin_configure(dev_out, PIN_OUT_4, GPIO_OUTPUT_LOW);
	}
	zassert_equal(rc, 0, "PIN_OUT_4 config as output failed");

	/* OUT_5 */
	/* Disconnect output */
	rc = gpio_pin_configure(dev_out, PIN_OUT_5, GPIO_DISCONNECTED);
	if (rc == -ENOTSUP) {
		TC_PRINT(
			"gpio_pin_configure(PIN_OUT_5, GPIO_DISCONNECTED) failed; trying with GPIO_INPUT\n");
		rc = gpio_pin_configure(dev_out, PIN_OUT_5, GPIO_INPUT);
	}
	zassert_equal(rc, 0, "gpio_pin_configure(dev, PIN_OUT_5, GPIO_INPUT) failed");
	/* Set output logic level */
	if (value & BIT(4)) {
		rc = gpio_pin_configure(dev_out, PIN_OUT_5, GPIO_OUTPUT_HIGH);
	} else {
		rc = gpio_pin_configure(dev_out, PIN_OUT_5, GPIO_OUTPUT_LOW);
	}
	zassert_equal(rc, 0, "PIN_OUT_5 config as output failed");

	/* Check inputs */
	_check_inputs(value);
}

/* Set output GPIOs with gpio_port_[set|clear]_bits_raw()
 * and confirm that input GPIOs have expected state.
 */
static void _gpio_port_set_clear_bits_raw_and_check(uint32_t value)
{
	int rc;

	/* OUT_1 */
	if (value & BIT(0)) {
		rc = gpio_port_set_bits_raw(dev_out, BIT(PIN_OUT_1));
		zassert_equal(rc, 0, "gpio_port_set_bits_raw(dev, PIN_OUT_1) failed");
	} else {
		rc = gpio_port_clear_bits_raw(dev_out, BIT(PIN_OUT_1));
		zassert_equal(rc, 0, "gpio_port_clear_bits_raw(dev, PIN_OUT_1) failed");
	}

	/* OUT_2 */
	if (value & BIT(1)) {
		rc = gpio_port_set_bits_raw(dev_out, BIT(PIN_OUT_2));
		zassert_equal(rc, 0, "gpio_port_set_bits_raw(dev, PIN_OUT_2) failed");
	} else {
		rc = gpio_port_clear_bits_raw(dev_out, BIT(PIN_OUT_2));
		zassert_equal(rc, 0, "gpio_port_clear_bits_raw(dev, PIN_OUT_2) failed");
	}

	/* OUT_3 */
	if (value & BIT(2)) {
		rc = gpio_port_set_bits_raw(dev_out, BIT(PIN_OUT_3));
		zassert_equal(rc, 0, "gpio_port_set_bits_raw(dev, PIN_OUT_3) failed");
	} else {
		rc = gpio_port_clear_bits_raw(dev_out, BIT(PIN_OUT_3));
		zassert_equal(rc, 0, "gpio_port_clear_bits_raw(dev, PIN_OUT_3) failed");
	}

	/* OUT_4 */
	if (value & BIT(3)) {
		rc = gpio_port_set_bits_raw(dev_out, BIT(PIN_OUT_4));
		zassert_equal(rc, 0, "gpio_port_set_bits_raw(dev, PIN_OUT_4) failed");
	} else {
		rc = gpio_port_clear_bits_raw(dev_out, BIT(PIN_OUT_4));
		zassert_equal(rc, 0, "gpio_port_clear_bits_raw(dev, PIN_OUT_4) failed");
	}

	/* OUT_5 */
	if (value & BIT(4)) {
		rc = gpio_port_set_bits_raw(dev_out, BIT(PIN_OUT_5));
		zassert_equal(rc, 0, "gpio_port_set_bits_raw(dev, PIN_OUT_5) failed");
	} else {
		rc = gpio_port_clear_bits_raw(dev_out, BIT(PIN_OUT_5));
		zassert_equal(rc, 0, "gpio_port_clear_bits_raw(dev, PIN_OUT_5) failed");
	}

	/* Check inputs */
	_check_inputs(value);
}

/* Set output GPIOs with gpio_port_[set|clear]_bits()
 * and confirm that input GPIOs have expected state.
 */
static void _gpio_port_set_clear_bits_and_check(uint32_t value)
{
	int rc;

	/* OUT_1 */
	if (value & BIT(0)) {
		rc = gpio_port_set_bits(dev_out, BIT(PIN_OUT_1));
		zassert_equal(rc, 0, "gpio_port_set_bits(dev, PIN_OUT_1) failed");
	} else {
		rc = gpio_port_clear_bits(dev_out, BIT(PIN_OUT_1));
		zassert_equal(rc, 0, "gpio_port_clear_bits(dev, PIN_OUT_1) failed");
	}

	/* OUT_2 */
	if (value & BIT(1)) {
		rc = gpio_port_set_bits(dev_out, BIT(PIN_OUT_2));
		zassert_equal(rc, 0, "gpio_port_set_bits(dev, PIN_OUT_2) failed");
	} else {
		rc = gpio_port_clear_bits(dev_out, BIT(PIN_OUT_2));
		zassert_equal(rc, 0, "gpio_port_clear_bits(dev, PIN_OUT_2) failed");
	}

	/* OUT_3 */
	if (value & BIT(2)) {
		rc = gpio_port_set_bits(dev_out, BIT(PIN_OUT_3));
		zassert_equal(rc, 0, "gpio_port_set_bits(dev, PIN_OUT_3) failed");
	} else {
		rc = gpio_port_clear_bits(dev_out, BIT(PIN_OUT_3));
		zassert_equal(rc, 0, "gpio_port_clear_bits(dev, PIN_OUT_3) failed");
	}

	/* OUT_4 */
	if (value & BIT(3)) {
		rc = gpio_port_set_bits(dev_out, BIT(PIN_OUT_4));
		zassert_equal(rc, 0, "gpio_port_set_bits(dev, PIN_OUT_4) failed");
	} else {
		rc = gpio_port_clear_bits(dev_out, BIT(PIN_OUT_4));
		zassert_equal(rc, 0, "gpio_port_clear_bits(dev, PIN_OUT_4) failed");
	}

	/* OUT_5 */
	if (value & BIT(4)) {
		rc = gpio_port_set_bits(dev_out, BIT(PIN_OUT_5));
		zassert_equal(rc, 0, "gpio_port_set_bits(dev, PIN_OUT_5) failed");
	} else {
		rc = gpio_port_clear_bits(dev_out, BIT(PIN_OUT_5));
		zassert_equal(rc, 0, "gpio_port_clear_bits(dev, PIN_OUT_5) failed");
	}

	/* Check inputs */
	_check_inputs(value);
}

/* Set output GPIOs with gpio_port_set_masked_raw()
 * and confirm that input GPIOs have expected state.
 */
static void _gpio_port_set_masked_raw_and_check(uint32_t value)
{
	int rc;
	gpio_port_pins_t port_pins;
	gpio_port_value_t port_value = 0;

	port_pins = BIT(PIN_OUT_5) | BIT(PIN_OUT_4) | BIT(PIN_OUT_3)
				| BIT(PIN_OUT_2) | BIT(PIN_OUT_1);

	port_value += (value & BIT(0)) ? BIT(PIN_OUT_1) : 0;
	port_value += (value & BIT(1)) ? BIT(PIN_OUT_2) : 0;
	port_value += (value & BIT(2)) ? BIT(PIN_OUT_3) : 0;
	port_value += (value & BIT(3)) ? BIT(PIN_OUT_4) : 0;
	port_value += (value & BIT(4)) ? BIT(PIN_OUT_5) : 0;

	rc = gpio_port_set_masked_raw(dev_out, port_pins, port_value);
	zassert_equal(rc, 0, "gpio_port_set_masked_raw(port, %d, %d) failed",
		port_pins, port_value);

	/* Check inputs */
	_check_inputs(value);
}

/* Set output GPIOs with gpio_port_set_masked()
 * and confirm that input GPIOs have expected state.
 */
static void _gpio_port_set_masked_and_check(uint32_t value)
{
	int rc;
	gpio_port_pins_t port_pins;
	gpio_port_value_t port_value = 0;

	port_pins = BIT(PIN_OUT_5) | BIT(PIN_OUT_4) | BIT(PIN_OUT_3)
				| BIT(PIN_OUT_2) | BIT(PIN_OUT_1);

	port_value += (value & BIT(0)) ? BIT(PIN_OUT_1) : 0;
	port_value += (value & BIT(1)) ? BIT(PIN_OUT_2) : 0;
	port_value += (value & BIT(2)) ? BIT(PIN_OUT_3) : 0;
	port_value += (value & BIT(3)) ? BIT(PIN_OUT_4) : 0;
	port_value += (value & BIT(4)) ? BIT(PIN_OUT_5) : 0;

	rc = gpio_port_set_masked(dev_out, port_pins, port_value);
	zassert_equal(rc, 0, "gpio_port_set_masked(port, %d, %d) failed",
		port_pins, port_value);

	/* Check inputs */
	_check_inputs(value);
}

/* Set output GPIOs with gpio_port_toggle_bits()
 * and confirm that input GPIOs have expected state.
 */
static void _gpio_port_toggle_bits_and_check(uint32_t value)
{
	int rc;
	gpio_port_pins_t port_pins;

	/* Set initial state 00000 */
	port_pins = BIT(PIN_OUT_5) | BIT(PIN_OUT_4) | BIT(PIN_OUT_3)
				| BIT(PIN_OUT_2) | BIT(PIN_OUT_1);
	rc = gpio_port_set_masked_raw(dev_out, port_pins, 0);
	zassert_equal(rc, 0, "gpio_port_set_masked_raw(port, %d, %d) failed",
		port_pins, 0);

	/* All outputs are now 0, check which pins shall be toggled */
	port_pins = 0;
	port_pins += (value & BIT(0)) ? BIT(PIN_OUT_1) : 0;
	port_pins += (value & BIT(1)) ? BIT(PIN_OUT_2) : 0;
	port_pins += (value & BIT(2)) ? BIT(PIN_OUT_3) : 0;
	port_pins += (value & BIT(3)) ? BIT(PIN_OUT_4) : 0;
	port_pins += (value & BIT(4)) ? BIT(PIN_OUT_5) : 0;

	rc = gpio_port_toggle_bits(dev_out, port_pins);
	zassert_equal(rc, 0, "gpio_port_toggle_bits(port, %d) failed", port_pins);

	/* Check inputs */
	_check_inputs(value);
}

/* Set output GPIOs with gpio_port_set_clr_bits_raw()
 * and confirm that input GPIOs have expected state.
 */
static void _gpio_port_set_clr_bits_raw_and_check(uint32_t value)
{
	int rc;
	gpio_port_pins_t set_pins = 0;
	gpio_port_pins_t clear_pins = 0;

	/* OUT_1 */
	if (value & BIT(0)) {
		set_pins += BIT(PIN_OUT_1);
	} else {
		clear_pins += BIT(PIN_OUT_1);
	}

	/* OUT_2 */
	if (value & BIT(1)) {
		set_pins += BIT(PIN_OUT_2);
	} else {
		clear_pins += BIT(PIN_OUT_2);
	}

	/* OUT_3 */
	if (value & BIT(2)) {
		set_pins += BIT(PIN_OUT_3);
	} else {
		clear_pins += BIT(PIN_OUT_3);
	}

	/* OUT_4 */
	if (value & BIT(3)) {
		set_pins += BIT(PIN_OUT_4);
	} else {
		clear_pins += BIT(PIN_OUT_4);
	}

	/* OUT_5 */
	if (value & BIT(4)) {
		set_pins += BIT(PIN_OUT_5);
	} else {
		clear_pins += BIT(PIN_OUT_5);
	}

	rc = gpio_port_set_clr_bits_raw(dev_out, set_pins, clear_pins);
	zassert_equal(rc, 0, "gpio_port_set_clr_bits_raw(port, %d, %d) failed",
		set_pins, clear_pins);

	/* Check inputs */
	_check_inputs(value);
}

/* Set output GPIOs with gpio_port_set_clr_bits()
 * and confirm that input GPIOs have expected state.
 */
static void _gpio_port_set_clr_bits_and_check(uint32_t value)
{
	int rc;
	gpio_port_pins_t set_pins = 0;
	gpio_port_pins_t clear_pins = 0;

	/* OUT_1 */
	if (value & BIT(0)) {
		set_pins += BIT(PIN_OUT_1);
	} else {
		clear_pins += BIT(PIN_OUT_1);
	}

	/* OUT_2 */
	if (value & BIT(1)) {
		set_pins += BIT(PIN_OUT_2);
	} else {
		clear_pins += BIT(PIN_OUT_2);
	}

	/* OUT_3 */
	if (value & BIT(2)) {
		set_pins += BIT(PIN_OUT_3);
	} else {
		clear_pins += BIT(PIN_OUT_3);
	}

	/* OUT_4 */
	if (value & BIT(3)) {
		set_pins += BIT(PIN_OUT_4);
	} else {
		clear_pins += BIT(PIN_OUT_4);
	}

	/* OUT_5 */
	if (value & BIT(4)) {
		set_pins += BIT(PIN_OUT_5);
	} else {
		clear_pins += BIT(PIN_OUT_5);
	}

	rc = gpio_port_set_clr_bits(dev_out, set_pins, clear_pins);
	zassert_equal(rc, 0, "gpio_port_set_clr_bits(port, %d, %d) failed",
		set_pins, clear_pins);

	/* Check inputs */
	_check_inputs(value);
}

/* Set output GPIOs with gpio_pin_set_raw()
 * and confirm that input GPIOs have expected state.
 */
static void _gpio_pin_set_raw_and_check(uint32_t value)
{
	int rc;

	/* OUT_1 */
	rc = gpio_pin_set_raw(dev_out, PIN_OUT_1, value & BIT(0));
	zassert_equal(rc, 0, "gpio_pin_set_raw(dev, PIN_OUT_1, %d) failed", value & BIT(0));

	/* OUT_2 */
	rc = gpio_pin_set_raw(dev_out, PIN_OUT_2, value & BIT(1));
	zassert_equal(rc, 0, "gpio_pin_set_raw(dev, PIN_OUT_2, %d) failed", value & BIT(1));

	/* OUT_3 */
	rc = gpio_pin_set_raw(dev_out, PIN_OUT_3, value & BIT(2));
	zassert_equal(rc, 0, "gpio_pin_set_raw(dev, PIN_OUT_3, %d) failed", value & BIT(2));

	/* OUT_4 */
	rc = gpio_pin_set_raw(dev_out, PIN_OUT_4, value & BIT(3));
	zassert_equal(rc, 0, "gpio_pin_set_raw(dev, PIN_OUT_4, %d) failed", value & BIT(3));

	/* OUT_5 */
	rc = gpio_pin_set_raw(dev_out, PIN_OUT_5, value & BIT(4));
	zassert_equal(rc, 0, "gpio_pin_set_raw(dev, PIN_OUT_5, %d) failed", value & BIT(4));

	/* Check inputs */
	_check_inputs(value);
}

/* Set output GPIOs with gpio_pin_set()
 * and confirm that input GPIOs have expected state.
 */
static void _gpio_pin_set_and_check(uint32_t value)
{
	int rc;

	/* OUT_1 */
	rc = gpio_pin_set(dev_out, PIN_OUT_1, value & BIT(0));
	zassert_equal(rc, 0, "gpio_pin_set(dev, PIN_OUT_1, %d) failed", value & BIT(0));

	/* OUT_2 */
	rc = gpio_pin_set(dev_out, PIN_OUT_2, value & BIT(1));
	zassert_equal(rc, 0, "gpio_pin_set(dev, PIN_OUT_2, %d) failed", value & BIT(1));

	/* OUT_3 */
	rc = gpio_pin_set(dev_out, PIN_OUT_3, value & BIT(2));
	zassert_equal(rc, 0, "gpio_pin_set(dev, PIN_OUT_3, %d) failed", value & BIT(2));

	/* OUT_4 */
	rc = gpio_pin_set(dev_out, PIN_OUT_4, value & BIT(3));
	zassert_equal(rc, 0, "gpio_pin_set(dev, PIN_OUT_4, %d) failed", value & BIT(3));

	/* OUT_5 */
	rc = gpio_pin_set(dev_out, PIN_OUT_5, value & BIT(4));
	zassert_equal(rc, 0, "gpio_pin_set(dev, PIN_OUT_5, %d) failed", value & BIT(4));

	/* Check inputs */
	_check_inputs(value);
}

/* Set output GPIOs with gpio_pin_toggle()
 *and confirm that input GPIOs have expected state.
 */
static void _gpio_pin_toggle_and_check(uint32_t value)
{
	int rc;
	gpio_port_pins_t port_pins;

	/* Set initial state 00000 */
	port_pins = BIT(PIN_OUT_5) | BIT(PIN_OUT_4) | BIT(PIN_OUT_3)
				| BIT(PIN_OUT_2) | BIT(PIN_OUT_1);
	rc = gpio_port_set_masked_raw(dev_out, port_pins, 0);
	zassert_equal(rc, 0, "gpio_port_set_masked_raw(port, %d, %d) failed",
		port_pins, 0);

	/* OUT_1 */
	if (value & BIT(0)) {
		rc = gpio_pin_toggle(dev_out, PIN_OUT_1);
		zassert_equal(rc, 0, "gpio_pin_toggle(dev_out, PIN_OUT_1) failed");
	}

	/* OUT_2 */
	if (value & BIT(1)) {
		rc = gpio_pin_toggle(dev_out, PIN_OUT_2);
		zassert_equal(rc, 0, "gpio_pin_toggle(dev_out, PIN_OUT_2) failed");
	}
	/* OUT_3 */
	if (value & BIT(2)) {
		rc = gpio_pin_toggle(dev_out, PIN_OUT_3);
		zassert_equal(rc, 0, "gpio_pin_toggle(dev_out, PIN_OUT_3) failed");
	}
	/* OUT_4 */
	if (value & BIT(3)) {
		rc = gpio_pin_toggle(dev_out, PIN_OUT_4);
		zassert_equal(rc, 0, "gpio_pin_toggle(dev_out, PIN_OUT_4) failed");
	}
	/* OUT_5 */
	if (value & BIT(4)) {
		rc = gpio_pin_toggle(dev_out, PIN_OUT_5);
		zassert_equal(rc, 0, "gpio_pin_toggle(dev_out, PIN_OUT_5) failed");
	}

	/* Check inputs */
	_check_inputs(value);
}

/* Generate "mooving 1" pattern on outputs.
 * Set outputs (and check inputs) using function provided in the fn parameter.
 */
static void _generic_mooving_1(void (*fn)(uint32_t))
{
	/* Set/check 11111 */
	(*fn)(0x11U);

	/* Set/check 00000 */
	(*fn)(0x0U);

	/* Set/check 00001 */
	(*fn)(0x1U);

	/* Set/check 00010 */
	(*fn)(0x2U);

	/* Set/check 00100 */
	(*fn)(0x4U);

	/* Set/check 01000 */
	(*fn)(0x8U);

	/* Set/check 10000 */
	(*fn)(0x10U);

	/* Set/check 01000 */
	(*fn)(0x8U);

	/* Set/check 00100 */
	(*fn)(0x4U);

	/* Set/check 00010 */
	(*fn)(0x2U);

	/* Set/check 00001 */
	(*fn)(0x1U);

	/* Set/check 00000 */
	(*fn)(0x0U);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_pin_configure()
 */
ZTEST(egpio_extended, test_mooving_1_with_configure_output)
{
	_generic_mooving_1(&_gpio_pin_configure_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_port_set_bits_raw() and gpio_port_clear_bits_raw()
 */
ZTEST(egpio_extended, test_mooving_1_with_set_clear_bits_raw)
{
	_generic_mooving_1(&_gpio_port_set_clear_bits_raw_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_port_set_bits() and gpio_port_clear_bits()
 */
ZTEST(egpio_extended, test_mooving_1_with_set_clear_bits)
{
	_generic_mooving_1(&_gpio_port_set_clear_bits_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_port_set_masked_raw()
 */
ZTEST(egpio_extended, test_mooving_1_with_port_set_masked_raw)
{
	_generic_mooving_1(&_gpio_port_set_masked_raw_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_port_set_masked()
 */
ZTEST(egpio_extended, test_mooving_1_with_port_set_masked)
{
	_generic_mooving_1(&_gpio_port_set_masked_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_port_toggle_bits()
 */
ZTEST(egpio_extended, test_mooving_1_with_gpio_port_toggle_bits)
{
	_generic_mooving_1(&_gpio_port_toggle_bits_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_port_set_clr_bits_raw()
 */
ZTEST(egpio_extended, test_mooving_1_with_gpio_port_set_clr_bits_raw)
{
	_generic_mooving_1(&_gpio_port_set_clr_bits_raw_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_port_set_clr_bits()
 */
ZTEST(egpio_extended, test_mooving_1_with_gpio_port_set_clr_bits)
{
	_generic_mooving_1(&_gpio_port_set_clr_bits_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_pin_set_raw()
 */
ZTEST(egpio_extended, test_mooving_1_with_gpio_pin_set_raw)
{
	_generic_mooving_1(&_gpio_pin_set_raw_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_pin_set()
 */
ZTEST(egpio_extended, test_mooving_1_with_gpio_pin_set)
{
	_generic_mooving_1(&_gpio_pin_set_and_check);
}

/**
 * @brief test setting output GPIO high/low with
 * gpio_pin_toggle()
 */
ZTEST(egpio_extended, test_mooving_1_with_gpio_pin_toggle)
{
	_generic_mooving_1(&_gpio_pin_toggle_and_check);
}

/**
 * @brief Stress test - send many GPIO requests one by one
 */
ZTEST(egpio_extended, test_stress_gpio_set)
{
	int rc = 0;
	gpio_port_pins_t port_pins;
	gpio_port_value_t port_value;

	port_pins = BIT(PIN_OUT_5) | BIT(PIN_OUT_4) | BIT(PIN_OUT_3)
				| BIT(PIN_OUT_2) | BIT(PIN_OUT_1);
	for (int i = 0; i < 200000; i++) {
		port_value = (i % 2) ? BIT(PIN_OUT_1) : 0;
		port_value += (i % 3) ? BIT(PIN_OUT_2) : 0;
		port_value += (i % 4) ? BIT(PIN_OUT_3) : 0;
		port_value += (i % 5) ? BIT(PIN_OUT_4) : 0;
		port_value += (i % 6) ? BIT(PIN_OUT_5) : 0;
		rc += gpio_port_set_masked_raw(dev_out, port_pins, port_value);
	}
	zassert_equal(rc, 0, "at least one set operation failed, rc = %d)", rc);
}

static void *suite_setup(void)
{
	int rc;

	TC_PRINT("Test executed on %s\n", CONFIG_BOARD_TARGET);

	k_object_access_grant(dev_out, k_current_get());
	k_object_access_grant(dev_in, k_current_get());

	TC_PRINT("Out device: %s\n", dev_out->name);
	zassert_true(device_is_ready(dev_out), "GPIO dev_out is not ready");

	TC_PRINT("In device: %s\n", dev_in->name);
	zassert_true(device_is_ready(dev_in), "GPIO dev_out is not ready");

	TC_PRINT("===================================================================\n");

	rc = gpio_pin_configure(dev_in, PIN_IN_1, GPIO_INPUT);
	zassert_equal(rc, 0, "pin config input failed");
	rc = gpio_pin_configure(dev_in, PIN_IN_2, GPIO_INPUT);
	zassert_equal(rc, 0, "pin config input failed");
	rc = gpio_pin_configure(dev_in, PIN_IN_3, GPIO_INPUT);
	zassert_equal(rc, 0, "pin config input failed");
	rc = gpio_pin_configure(dev_in, PIN_IN_4, GPIO_INPUT);
	zassert_equal(rc, 0, "pin config input failed");
	rc = gpio_pin_configure(dev_in, PIN_IN_5, GPIO_INPUT);
	zassert_equal(rc, 0, "pin config input failed");

	return NULL;
}

ZTEST_SUITE(egpio_extended, NULL, suite_setup, NULL, NULL, NULL);
