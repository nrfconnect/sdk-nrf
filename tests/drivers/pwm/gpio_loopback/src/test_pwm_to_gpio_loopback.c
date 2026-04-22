/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/ztest.h>

#include "test_pwm_to_gpio_loopback.h"

#define NUMBER_OF_CYCLE_TO_CAPTURE 5

#if !defined(CONFIG_SOC_NRF54H20_CPUFLPR)
static struct gpio_callback pwm_input_cb_data;
static volatile uint32_t high, low;

void pwm_input_captured_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	uint32_t pin = 0;

	while (pins >>= 1) {
		pin++;
	}

	if (gpio_pin_get(dev, pin)) {
		high++;
	} else {
		low++;
	}
}
#endif /* !CONFIG_SOC_NRF54H20_CPUFLPR */

void get_test_devices(struct pwm_dt_spec *out, struct gpio_dt_spec *in)
{
	/* PWM generator device */
	out->dev = DEVICE_DT_GET(PWM_LOOPBACK_OUT_CTLR);
	out->channel = PWM_LOOPBACK_OUT_CHANNEL;
	out->period = PWM_LOOPBACK_OUT_PERIOD;
	out->flags = PWM_LOOPBACK_OUT_FLAGS;
	zassert_true(pwm_is_ready_dt(out), "pwm loopback output device is not ready");

	in->port = DEVICE_DT_GET(GPIO_LOOPBACK_IN_CTRL);
	in->pin = GPIO_LOOPBACK_IN_PIN;
	in->dt_flags = GPIO_LOOPBACK_IN_FLAGS;
	zassert_true(gpio_is_ready_dt(in), "pwm loopback in device is not ready");
}

#if defined(CONFIG_SOC_NRF54H20_CPUFLPR)
/*
 * nRF54H20 FLPR has no GPIO pin interrupts; sample the loopback pin in a timed
 * busy loop and count rising/falling edges (same thresholds as the GPIOTE path).
 */
static void test_capture(uint32_t period, uint32_t pulse, pwm_flags_t flags)
{
	struct pwm_dt_spec out;
	struct gpio_dt_spec in;
	int err;
	uint32_t high_cnt = 0;
	uint32_t low_cnt = 0;
	const uint32_t duration_us =
		(NUMBER_OF_CYCLE_TO_CAPTURE * period) + (period >> 2);

	TC_PRINT("Pulse/period: %u/%u usec\n", pulse, period);

	get_test_devices(&out, &in);

	err = pwm_set(out.dev, out.channel, PWM_USEC(period), PWM_USEC(pulse),
		      out.flags ^= (flags & PWM_POLARITY_MASK));
	zassert_equal(err, 0, "failed to set pwm output (err %d)", err);

	err = gpio_pin_configure_dt(&in, GPIO_INPUT);
	zassert_equal(err, 0, "failed to configure input pin (err %d)", err);

	int last = gpio_pin_get_dt(&in);

	for (uint32_t us = 0; us < duration_us; us++) {
		k_busy_wait(1);
		int now = gpio_pin_get_dt(&in);

		if (now != last) {
			if (now) {
				high_cnt++;
			} else {
				low_cnt++;
			}
			last = now;
		}
	}

	TC_PRINT("PWM output -high state counter: %u -low state counter: %u\n", high_cnt, low_cnt);
	zassert((high_cnt >= NUMBER_OF_CYCLE_TO_CAPTURE) && (low_cnt >= NUMBER_OF_CYCLE_TO_CAPTURE),
		"PWM not captured");
}
#else
static void test_capture(uint32_t period, uint32_t pulse, pwm_flags_t flags)
{
	struct pwm_dt_spec out;
	struct gpio_dt_spec in;

	int err = 0;

	TC_PRINT("Pulse/period: %u/%u usec\n", pulse, period);

	get_test_devices(&out, &in);

	/* clear edge counters */
	high = 0;
	low = 0;

	/* configure and enable PWM */
	err = pwm_set(out.dev, out.channel, PWM_USEC(period), PWM_USEC(pulse),
		      out.flags ^= (flags & PWM_POLARITY_MASK));
	zassert_equal(err, 0, "failed to set pwm output (err %d)", err);

	/* configure and enable GPIOTE */
	err = gpio_pin_configure_dt(&in, GPIO_INPUT);
	zassert_equal(err, 0, "failed to configure input pin (err %d)", err);

	err = gpio_pin_interrupt_configure_dt(&in, GPIO_INT_EDGE_BOTH);
	zassert_equal(err, 0, "failed to configure input pin interrupt (err %d)", err);

	gpio_init_callback(&pwm_input_cb_data, pwm_input_captured_callback, BIT(in.pin));
	gpio_add_callback(in.port, &pwm_input_cb_data);

	/* NUMBER_OF_CYCLE_TO_CAPTURE periods plus 1/4 of period to catch the last edge */
	k_usleep((NUMBER_OF_CYCLE_TO_CAPTURE * period) + (period >> 2));

	TC_PRINT("PWM output -high state counter: %d -low state counter: %d\n", high, low);
	zassert((high >= NUMBER_OF_CYCLE_TO_CAPTURE) && (low >= NUMBER_OF_CYCLE_TO_CAPTURE),
		"PWM not captured");
}
#endif /* CONFIG_SOC_NRF54H20_CPUFLPR */

ZTEST(pwm_loopback, test_pwm_polarity_normal)
{
	test_capture(CONFIG_TEST_PWM_PERIOD_USEC, (CONFIG_TEST_PWM_PERIOD_USEC >> 1),
		PWM_POLARITY_NORMAL);
}

ZTEST(pwm_loopback, test_pwm_polarity_inverted)
{
	test_capture(CONFIG_TEST_PWM_PERIOD_USEC, (CONFIG_TEST_PWM_PERIOD_USEC >> 1),
		PWM_POLARITY_INVERTED);
}
