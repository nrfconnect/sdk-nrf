/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_event, LOG_LEVEL_INF);

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

static struct k_sem gpio_event;
static struct gpio_callback cb_data;

/* ISR for Port event. */
static void gpio_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	uint8_t i;

	for (i = 0; i < npins; i++) {
		if (pins & BIT(out_pins[i].pin)) {
			k_sem_give(&gpio_event);
			TC_PRINT("Event at %s, pin %d\n", dev->name, out_pins[i].pin);
		}
	}
}

static bool generic_edge_test(int initial_value)
{
	uint8_t i;
	int rc;
	int opposite_value;
	bool test_failed = false;

	for (i = 0; i < npins; i++) {
		/* Reset semaphore */
		k_sem_reset(&gpio_event);

		/* Set GPIO to physical 'initial_value' */
		rc = gpio_pin_set_raw(out_pins[i].port, out_pins[i].pin, initial_value);
		zassert_equal(rc, 0, "[%d]: Port %s, pin %d, set %d failed",
			i, out_pins[i].port->name, out_pins[i].pin, initial_value);

		/* Configure event from edge */
		if (initial_value == 0) {
			rc = gpio_pin_interrupt_configure_dt(&out_pins[i], GPIO_INT_EDGE_RISING);
		} else {
			rc = gpio_pin_interrupt_configure_dt(&out_pins[i], GPIO_INT_EDGE_FALLING);
		}
		zassert_equal(rc, 0, "[%d]: failed to configure interrupt (err %d)", i, rc);

		gpio_init_callback(&cb_data, gpio_isr, BIT(out_pins[i].pin));
		rc = gpio_add_callback(out_pins[i].port, &cb_data);
		zassert_equal(rc, 0, "[%d]: failed to add calback (err %d)", i, rc);

		/* Change GPIO to physical opposite state */
		if (initial_value == 0) {
			opposite_value = 1;
		} else {
			opposite_value = 0;
		}
		rc = gpio_pin_set_raw(out_pins[i].port, out_pins[i].pin, opposite_value);
		zassert_equal(rc, 0, "[%d]: Port %s, pin %d, set %d failed",
			i, out_pins[i].port->name, out_pins[i].pin, opposite_value);

		/* Check that interrupt was generated */
		rc = k_sem_take(&gpio_event, Z_TIMEOUT_MS(200));
		if (rc < 0) {
			test_failed = true;
			TC_PRINT("[%d]: No event from Port %s, pin %d\n",
				i, out_pins[i].port->name, out_pins[i].pin);
		}

		/* Remove callback */
		gpio_remove_callback(out_pins[i].port, &cb_data);
	}

	return test_failed;
}

/**
 * @brief Test if event from rising edge is detected on GPIOs.
 */
ZTEST(gpio_event, test_rising_edge)
{
	bool test_failed = false;
	const int initial_gpio_state = 0;

	test_failed = generic_edge_test(initial_gpio_state);
	zassert_equal(test_failed, false, "At least one interrupt is missing");
}

/**
 * @brief Test if event from falling edge is detected on GPIOs.
 */
ZTEST(gpio_event, test_falling_edge)
{
	bool test_failed = false;
	const int initial_gpio_state = 1;

	test_failed = generic_edge_test(initial_gpio_state);
	zassert_equal(test_failed, false, "At least one interrupt is missing");
}

static void *suite_setup(void)
{
	uint8_t i;
	int rc;

	TC_PRINT("Test executed on %s\n", CONFIG_BOARD_TARGET);
	TC_PRINT("GPIO count: %d\n", npins);
	for (i = 0; i < npins; i++) {
		TC_PRINT("%d: Port %s, pin %d\n", i, out_pins[i].port->name, out_pins[i].pin);
	}
	TC_PRINT("===================================================================\n");

	k_sem_init(&gpio_event, 0, 1);

	for (i = 0; i < npins; i++) {
		zassert_true(gpio_is_ready_dt(&out_pins[i]), "OUT[%d] is not ready", i);

		/* Configure GPIO as output */
		rc = gpio_pin_configure_dt(&out_pins[i], GPIO_OUTPUT);
		zassert_equal(rc, 0, "OUT[%d] config failed", i);
	}

	return NULL;
}

ZTEST_SUITE(gpio_event, NULL, suite_setup, NULL, NULL, NULL);
