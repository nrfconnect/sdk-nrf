/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hpf_gpio, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#if !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), out_gpios)
#error "Unsupported board: out_gpios are not defined"
#endif

#if !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), in_gpios)
#error "Unsupported board: in_gpios are not defined"
#endif

/* Delay after setting GPIO ouputs. It allows signals to settle. */
#define PROPAGATION_DELAY_MS K_MSEC(1U)
#define SLEEP_TIME_MS 1000
#define ACTIVE_TIME_MS 1000

const struct gpio_dt_spec out_pin = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), out_gpios);
const struct gpio_dt_spec in_pin = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), in_gpios);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

K_SEM_DEFINE(timer_expired_sem, 0, 1);

static void timer_handler(struct k_timer *dummy)
{
	(void)dummy;

	k_sem_give(&timer_expired_sem);
}

K_TIMER_DEFINE(timer, timer_handler, NULL);

/* Check that input GPIO state match with input parameter 'value'. */
static void check_input(uint32_t value)
{
	bool current;
	bool expected;

	LOG_DBG("check_input(%u)", value);

	/* Wait a bit to stabilize logic level. */
	k_sleep(PROPAGATION_DELAY_MS);

	current = gpio_pin_get_dt(&in_pin);
	expected = value & BIT(0);
	LOG_DBG("check_input: current: %u, expected: %u", current, expected);
	__ASSERT(current == expected, "IN = %u, while expected %u", current, expected);
}

int main(void)
{
	int ret;

	LOG_INF("hpf_gpio test on %s", CONFIG_BOARD_TARGET);
#if defined(CONFIG_GPIO_HPF_GPIO_BACKEND_ICMSG)
	LOG_INF("Test uses ICmsg backend");
#elif defined(CONFIG_GPIO_HPF_GPIO_BACKEND_ICBMSG)
	LOG_INF("Test uses ICBmsg backend");
#elif defined(CONFIG_GPIO_HPF_GPIO_BACKEND_MBOX)
	LOG_INF("Test uses MBOX backend");
#else
	LOG_INF("Unknown backend");
#endif
	LOG_INF("===================================================================");

	ret = gpio_is_ready_dt(&in_pin);
	__ASSERT(ret, "IN is not ready (%s)", in_pin.port->name);
	ret = gpio_is_ready_dt(&out_pin);
	__ASSERT(ret, "OUT is not ready");
	ret = gpio_is_ready_dt(&led);
	__ASSERT(ret, "LED is not ready");

	ret = gpio_pin_configure_dt(&in_pin, GPIO_INPUT);
	__ASSERT(ret == 0, "IN config failed (%d)", ret);
	ret = gpio_pin_configure_dt(&out_pin, GPIO_OUTPUT);
	__ASSERT(ret == 0, "OUT config failed (%d)", ret);
	LOG_INF("IN and OUT were configured");
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	__ASSERT(ret == 0, "LED configuration failed (%d)", ret);

	while (1) {
		k_timer_start(&timer, K_MSEC(ACTIVE_TIME_MS), K_NO_WAIT);
		gpio_pin_set_dt(&led, 1);

		/* Active state - use HPF GPIO. */
		while (k_sem_take(&timer_expired_sem, K_NO_WAIT) != 0) {
			ret = gpio_pin_set_dt(&out_pin, 1);
			__ASSERT(ret == 0, "OUT set failed (%d)", ret);
			check_input(1);

			ret = gpio_pin_set_dt(&out_pin, 0);
			__ASSERT(ret == 0, "OUT clear failed (%d)", ret);
			check_input(0);
		}

		gpio_pin_set_dt(&led, 0);
		k_msleep(SLEEP_TIME_MS);
	}
}
