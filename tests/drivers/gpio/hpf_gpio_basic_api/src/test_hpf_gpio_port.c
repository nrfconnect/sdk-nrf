/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include "test_hpf_gpio.h"

#define ALL_BITS ((gpio_port_value_t)-1)

static const struct device *const dev_out = DEVICE_DT_GET(DEV_OUT);
static const struct device *const dev_in = DEVICE_DT_GET(DEV_IN);

/* Delay after pull input config to allow signal to settle.  The value
 * selected is conservative (higher than may be necessary).
 */
#define PULL_DELAY_MS K_MSEC(1U)

/* Short-hand for a checked read of PIN_IN raw state */
static bool raw_in(void)
{
	k_sleep(PULL_DELAY_MS);
	gpio_port_value_t v;
	int rc = gpio_port_get_raw(dev_in, &v);

	zassert_equal(rc, 0,
		      "raw_in failed");
	return (v & BIT(PIN_IN)) ? true : false;
}

/* Short-hand for a checked write of PIN_OUT raw state */
static void raw_out(bool set)
{
	int rc;

	if (set) {
		rc = gpio_port_set_bits_raw(dev_out, BIT(PIN_OUT));
	} else {
		rc = gpio_port_clear_bits_raw(dev_out, BIT(PIN_OUT));
	}
	zassert_equal(rc, 0,
		      "raw_out failed");
	k_sleep(PULL_DELAY_MS);
}

/* Short-hand for a checked write of PIN_OUT logic state */
static void logic_out(bool set)
{
	int rc;

	if (set) {
		rc = gpio_port_set_bits(dev_out, BIT(PIN_OUT));
	} else {
		rc = gpio_port_clear_bits(dev_out, BIT(PIN_OUT));
	}
	zassert_equal(rc, 0,
		      "raw_out failed");
	k_sleep(PULL_DELAY_MS);
}

/* Verify device, configure for physical in and out, verify
 * connection, verify raw_in().
 */
static int setup(void)
{
	int rc;
	gpio_port_value_t v1;

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

	TC_PRINT("Validate device %s\n", dev_out->name);
	zassert_true(device_is_ready(dev_out), "GPIO dev_out is not ready");

	TC_PRINT("Validate device %s\n", dev_in->name);
	zassert_true(device_is_ready(dev_in), "GPIO dev_out is not ready");

	TC_PRINT("Check e%s output %d connected to %s input %d\n", dev_out->name,
		 PIN_OUT, dev_in->name, PIN_IN);

	rc = gpio_pin_configure(dev_in, PIN_IN, GPIO_INPUT);
	zassert_equal(rc, 0,
		      "pin config input failed");

	/* Test output low */
	rc = gpio_pin_configure(dev_out, PIN_OUT, GPIO_OUTPUT_LOW);
	zassert_equal(rc, 0,
		      "pin config output low failed");

	k_sleep(PULL_DELAY_MS);

	rc = gpio_port_get_raw(dev_in, &v1);
	zassert_equal(rc, 0,
		      "get raw low failed");
	if (raw_in() != false) {
		TC_PRINT("FATAL output pin not wired to input pin? (out low => in high)\n");
		while (true) {
			k_sleep(K_FOREVER);
		}
	}

	zassert_equal(v1 & BIT(PIN_IN), 0,
		      "out low does not read low");

	/* Disconnect output */
	rc = gpio_pin_configure(dev_out, PIN_OUT, GPIO_DISCONNECTED);
	if (rc == -ENOTSUP) {
		TC_PRINT("NOTE: cannot configure pin as disconnected; trying as input\n");
		rc = gpio_pin_configure(dev_out, PIN_OUT, GPIO_INPUT);
	}
	zassert_equal(rc, 0,
		      "output disconnect failed");

	/* Test output high */
	rc = gpio_pin_configure(dev_out, PIN_OUT, GPIO_OUTPUT_HIGH);
	zassert_equal(rc, 0,
		      "pin config output high failed");

	k_sleep(PULL_DELAY_MS);

	rc = gpio_port_get_raw(dev_in, &v1);
	zassert_equal(rc, 0,
		      "get raw high failed");
	if (raw_in() != true) {
		TC_PRINT("FATAL output pin not wired to input pin? (out high => in low)\n");
		while (true) {
			k_sleep(K_FOREVER);
		}
	}
	zassert_not_equal(v1 & BIT(PIN_IN), 0,
			  "out high does not read low");

	TC_PRINT("OUT %d to IN %d linkage works\n", PIN_OUT, PIN_IN);
	return TC_PASS;
}

/* gpio_port_set_bits_raw()
 * gpio_port_clear_bits_raw()
 * gpio_port_set_masked_raw()
 * gpio_port_toggle_bits()
 */
static int bits_physical(void)
{
	int rc;

	TC_PRINT("- %s\n", __func__);

	/* port_set_bits_raw */
	rc = gpio_port_set_bits_raw(dev_out, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "port set raw out failed");
	zassert_equal(raw_in(), true,
		      "raw set mismatch");

	/* port_clear_bits_raw */
	rc = gpio_port_clear_bits_raw(dev_out, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "port clear raw out failed");
	zassert_equal(raw_in(), false,
		      "raw clear mismatch");

	/* set after clear changes */
	rc = gpio_port_set_bits_raw(dev_out, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "port set raw out failed");
	zassert_equal(raw_in(), true,
		      "raw set mismatch");

	/* raw_out() after set works */
	raw_out(false);
	zassert_equal(raw_in(), false,
		      "raw_out() false mismatch");

	/* raw_out() set after raw_out() clear works */
	raw_out(true);
	zassert_equal(raw_in(), true,
		      "raw_out() true mismatch");

	rc = gpio_port_set_masked_raw(dev_out, BIT(PIN_OUT), 0);
	zassert_equal(rc, 0,
		      "set_masked_raw low failed");
	zassert_equal(raw_in(), false,
		      "set_masked_raw low mismatch");

	rc = gpio_port_set_masked_raw(dev_out, BIT(PIN_OUT), ALL_BITS);
	zassert_equal(rc, 0,
		      "set_masked_raw high failed");
	zassert_equal(raw_in(), true,
		      "set_masked_raw high mismatch");

	rc = gpio_port_set_clr_bits_raw(dev_out, 0, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "clear out failed");
	zassert_equal(raw_in(), false,
		      "clear out mismatch");

	rc = gpio_port_set_clr_bits_raw(dev_out, BIT(PIN_OUT), 0);
	zassert_equal(rc, 0,
		      "set out failed");
	zassert_equal(raw_in(), true,
		      "set out mismatch");

	/* Conditionally verify that behavior with __ASSERT disabled
	 * is to set the bit.
	 */
	if (false) {
		/* preserve set */
		rc = gpio_port_set_clr_bits_raw(dev_out, BIT(PIN_OUT), BIT(PIN_OUT));
		zassert_equal(rc, 0,
			      "s/c dup set failed");
		zassert_equal(raw_in(), true,
			      "s/c dup set mismatch");

		/* do set */
		raw_out(false);
		rc = gpio_port_set_clr_bits_raw(dev_out, BIT(PIN_OUT), BIT(PIN_OUT));
		zassert_equal(rc, 0,
			      "s/c dup2 set failed");
		zassert_equal(raw_in(), true,
			      "s/c dup2 set mismatch");
	}

	rc = gpio_port_toggle_bits(dev_out, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "toggle_bits high-to-low failed");
	zassert_equal(raw_in(), false,
		      "toggle_bits high-to-low mismatch");

	rc = gpio_port_toggle_bits(dev_out, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "toggle_bits low-to-high failed");
	zassert_equal(raw_in(), true,
		      "toggle_bits low-to-high mismatch");

	return TC_PASS;
}

/* gpio_pin_get_raw()
 * gpio_pin_set_raw()
 */
static int pin_physical(void)
{
	int rc;

	TC_PRINT("- %s\n", __func__);

	raw_out(true);
	zassert_equal(gpio_pin_get_raw(dev_in, PIN_IN), raw_in(),
		      "pin_get_raw high failed");

	raw_out(false);
	zassert_equal(gpio_pin_get_raw(dev_in, PIN_IN), raw_in(),
		      "pin_get_raw low failed");

	rc = gpio_pin_set_raw(dev_out, PIN_OUT, 32);
	zassert_equal(rc, 0,
		      "pin_set_raw high failed");
	zassert_equal(raw_in(), true,
		      "pin_set_raw high failed");

	rc = gpio_pin_set_raw(dev_out, PIN_OUT, 0);
	zassert_equal(rc, 0,
		      "pin_set_raw low failed");
	zassert_equal(raw_in(), false,
		      "pin_set_raw low failed");

	rc = gpio_pin_toggle(dev_out, PIN_OUT);
	zassert_equal(rc, 0,
		      "pin_toggle low-to-high failed");
	zassert_equal(raw_in(), true,
		      "pin_toggle low-to-high mismatch");

	rc = gpio_pin_toggle(dev_out, PIN_OUT);
	zassert_equal(rc, 0,
		      "pin_toggle high-to-low failed");
	zassert_equal(raw_in(), false,
		      "pin_toggle high-to-low mismatch");

	return TC_PASS;
}

/* Verify configure output level is independent of active level, and
 * raw output is independent of active level.
 */
static int check_raw_output_levels(void)
{
	int rc;

	TC_PRINT("- %s\n", __func__);

	rc = gpio_pin_configure(dev_out, PIN_OUT,
				GPIO_ACTIVE_HIGH | GPIO_OUTPUT_LOW);
	zassert_equal(rc, 0,
		      "active high output low failed");
	zassert_equal(raw_in(), false,
		      "active high output low raw mismatch");
	raw_out(true);
	zassert_equal(raw_in(), true,
		      "set high mismatch");

	rc = gpio_pin_configure(dev_out, PIN_OUT,
				GPIO_ACTIVE_HIGH | GPIO_OUTPUT_HIGH);
	zassert_equal(rc, 0,
		      "active high output high failed");
	zassert_equal(raw_in(), true,
		      "active high output high raw mismatch");
	raw_out(false);
	zassert_equal(raw_in(), false,
		      "set low mismatch");

	rc = gpio_pin_configure(dev_out, PIN_OUT,
				GPIO_ACTIVE_LOW | GPIO_OUTPUT_LOW);
	zassert_equal(rc, 0,
		      "active low output low failed");
	zassert_equal(raw_in(), false,
		      "active low output low raw mismatch");
	raw_out(true);
	zassert_equal(raw_in(), true,
		      "set high mismatch");

	rc = gpio_pin_configure(dev_out, PIN_OUT,
				GPIO_ACTIVE_LOW | GPIO_OUTPUT_HIGH);
	zassert_equal(rc, 0,
		      "active low output high failed");
	zassert_equal(raw_in(), true,
		      "active low output high raw mismatch");
	raw_out(false);
	zassert_equal(raw_in(), false,
		      "set low mismatch");

	return TC_PASS;
}

/* Verify configure output level is dependent on active level, and
 * logic output is dependent on active level.
 */
static int check_logic_output_levels(void)
{
	int rc;

	TC_PRINT("- %s\n", __func__);

	rc = gpio_pin_configure(dev_out, PIN_OUT,
				GPIO_ACTIVE_HIGH | GPIO_OUTPUT_INACTIVE);
	zassert_equal(rc, 0,
		      "active true output false failed: %d", rc);
	zassert_equal(raw_in(), false,
		      "active true output false logic mismatch");
	logic_out(true);
	zassert_equal(raw_in(), true,
		      "set true mismatch");

	rc = gpio_pin_configure(dev_out, PIN_OUT,
				GPIO_ACTIVE_HIGH | GPIO_OUTPUT_ACTIVE);
	zassert_equal(rc, 0,
		      "active true output true failed");
	zassert_equal(raw_in(), true,
		      "active true output true logic mismatch");
	logic_out(false);
	zassert_equal(raw_in(), false,
		      "set false mismatch");

	rc = gpio_pin_configure(dev_out, PIN_OUT,
				GPIO_ACTIVE_LOW | GPIO_OUTPUT_ACTIVE);
	zassert_equal(rc, 0,
		      "active low output true failed");

	zassert_equal(raw_in(), false,
		      "active low output true raw mismatch");
	logic_out(false);
	zassert_equal(raw_in(), true,
		      "set false mismatch");

	rc = gpio_pin_configure(dev_out, PIN_OUT,
				GPIO_ACTIVE_LOW | GPIO_OUTPUT_INACTIVE);
	zassert_equal(rc, 0,
		      "active low output false failed");
	zassert_equal(raw_in(), true,
		      "active low output false logic mismatch");
	logic_out(true);
	zassert_equal(raw_in(), false,
		      "set true mismatch");

	return TC_PASS;
}

/* gpio_port_set_bits()
 * gpio_port_clear_bits()
 * gpio_port_set_masked()
 * gpio_port_toggle_bits()
 */
static int bits_logical(void)
{
	int rc;

	TC_PRINT("- %s\n", __func__);

	rc = gpio_pin_configure(dev_out, PIN_OUT,
				GPIO_OUTPUT_HIGH | GPIO_ACTIVE_LOW);
	zassert_equal(rc, 0,
		      "output configure failed");
	zassert_equal(raw_in(), true,
		      "raw out high mismatch");

	/* port_set_bits */
	rc = gpio_port_set_bits(dev_out, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "port set raw out failed");
	zassert_equal(raw_in(), false,
		      "raw low set mismatch");

	/* port_clear_bits */
	rc = gpio_port_clear_bits(dev_out, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "port clear raw out failed");
	zassert_equal(raw_in(), true,
		      "raw low clear mismatch");

	/* set after clear changes */
	rc = gpio_port_set_bits_raw(dev_out, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "port set raw out failed");
	zassert_equal(raw_in(), true,
		      "raw set mismatch");

	/* Conditionally verify that behavior with __ASSERT disabled
	 * is to set the bit.
	 */
	if (false) {
		/* preserve set */
		rc = gpio_port_set_clr_bits(dev_out, BIT(PIN_OUT), BIT(PIN_OUT));
		zassert_equal(rc, 0,
			      "s/c set toggle failed");
		zassert_equal(raw_in(), false,
			      "s/c set toggle mismatch");

		/* force set */
		raw_out(true);
		rc = gpio_port_set_clr_bits(dev_out, BIT(PIN_OUT), BIT(PIN_OUT));
		zassert_equal(rc, 0,
			      "s/c dup set failed");
		zassert_equal(raw_in(), false,
			      "s/c dup set mismatch");
	}

	rc = gpio_port_toggle_bits(dev_out, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "toggle_bits active-to-inactive failed");
	zassert_equal(raw_in(), false,
		      "toggle_bits active-to-inactive mismatch");

	rc = gpio_port_toggle_bits(dev_out, BIT(PIN_OUT));
	zassert_equal(rc, 0,
		      "toggle_bits inactive-to-active failed");
	zassert_equal(raw_in(), true,
		      "toggle_bits inactive-to-active mismatch");

	rc = gpio_pin_toggle(dev_out, PIN_OUT);
	zassert_equal(rc, 0,
		      "pin_toggle low-to-high failed");
	zassert_equal(raw_in(), false,
		      "pin_toggle low-to-high mismatch");

	return TC_PASS;
}

/**
 * @brief Stress test - send many GPIO requests one by one
 */
static int stress_gpio_pin_set_raw(void)
{
	int rc;
	int rc_acc = 0;

	TC_PRINT("- %s\n", __func__);

	for (int i = 0; i < 300000; i++) {
		rc = gpio_pin_set_raw(dev_out, PIN_OUT, (i % 2));
		/* If TX buffer is full, wait a bit and retry */
		while (rc == -EIO) {
			k_usleep(100);
			rc = gpio_pin_set_raw(dev_out, PIN_OUT, (i % 2));
		}

		/* Report any other error */
		if (rc) {
			TC_PRINT("%d: rc = %d\n", i, rc);
			rc_acc += rc;
			break;
		}
	}
	zassert_equal(rc_acc, 0, "at least one set operation failed, rc_acc = %d)", rc_acc);

	return TC_PASS;
}

ZTEST(hpf_gpio_port, test_hpf_gpio_port)
{
	zassert_equal(setup(), TC_PASS,
		      "device setup failed");
	zassert_equal(bits_physical(), TC_PASS,
		      "bits_physical failed");
	zassert_equal(pin_physical(), TC_PASS,
		      "pin_physical failed");
	zassert_equal(check_raw_output_levels(), TC_PASS,
		      "check_raw_output_levels failed");
	zassert_equal(check_logic_output_levels(), TC_PASS,
		      "check_logic_output_levels failed");
	zassert_equal(bits_logical(), TC_PASS,
		      "bits_logical failed");
	zassert_equal(stress_gpio_pin_set_raw(), TC_PASS,
		      "stress_gpio_pin_set_raw failed");
}

/* Test GPIO port configuration */
ZTEST_SUITE(hpf_gpio_port, NULL, NULL, NULL, NULL, NULL);
