/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(idle_pwm_loop, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device_runtime.h>

#if !DT_NODE_EXISTS(DT_NODELABEL(pwm_to_gpio_loopback))
#error "Unsupported board: pwm_to_gpio_loopback node is not defined"
#endif

#define SHM_START_ADDR		(DT_REG_ADDR(DT_NODELABEL(cpuapp_cpurad_ipc_shm)))
volatile static uint32_t *shared_var = (volatile uint32_t *) SHM_START_ADDR;
#define HOST_IS_READY	(1)
#define REMOTE_IS_READY	(2)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

static const struct gpio_dt_spec pin_in = GPIO_DT_SPEC_GET_BY_IDX(
	DT_NODELABEL(pwm_to_gpio_loopback),	gpios, 0);

static const struct pwm_dt_spec pwm_out = PWM_DT_SPEC_GET(
	DT_NODELABEL(pwm_to_gpio_loopback));

static struct gpio_callback gpio_input_cb_data;
static volatile uint32_t high, low;

/* Variables used to make CPU active for ~1 second */
static struct k_timer my_timer;
static bool timer_expired;

void my_timer_handler(struct k_timer *dummy)
{
	timer_expired = true;
}

void gpio_input_edge_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
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

int main(void)
{
	uint32_t counter = 0;
	uint32_t pulse;
	uint32_t edges;
	uint32_t tolerance;
	int ret;
	int test_repetitions = 3;

	ret = gpio_is_ready_dt(&led);
	__ASSERT(ret, "Error: GPIO Device not ready");

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	__ASSERT(ret == 0, "Could not configure led GPIO");

	/* Wait a bit to solve NRFS request timeout issue. */
	k_msleep(100);

	/* Set PWM fill ratio to 50% */
	pulse = pwm_out.period >> 1;

	/* Calculate how many rising edges occur during 1 second of activity */
	edges = 1000000000 / pwm_out.period;

	/* Due to not perfect synchronization some edges may be lost.
	 * Set tolerance to 1% (or at least 1 edge).
	 */
	tolerance = edges / 100;
	if (tolerance == 0) {
		tolerance = 1;
	}

	LOG_INF("Multicore idle_pwm_loopback test on %s", CONFIG_BOARD_TARGET);
	LOG_INF("Core will sleep for %u ms", CONFIG_TEST_SLEEP_DURATION_MS);
	LOG_INF("Testing PWM device %s, channel %d", pwm_out.dev->name, pwm_out.channel);
	LOG_INF("GPIO loopback at %s, pin %d", pin_in.port->name, pin_in.pin);
	LOG_INF("Pulse/period: %u/%u usec", pulse / 1000, pwm_out.period / 1000);
	LOG_INF("Expected number of edges in 1 second: %u (+/- %u)", edges, tolerance);
	LOG_INF("Shared memory at %p", (void *) shared_var);
	LOG_INF("===================================================================");

	ret = pwm_is_ready_dt(&pwm_out);
	if (!ret) {
		LOG_ERR("Device %s is not ready.", pwm_out.dev->name);
	}
	__ASSERT_NO_MSG(ret);

	ret = gpio_is_ready_dt(&pin_in);
	if (!ret) {
		LOG_ERR("Device %s is not ready.", pin_in.port->name);
	}
	__ASSERT_NO_MSG(ret);

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	pm_device_runtime_enable(pwm_out.dev);
#endif

	/* configure gpio and init GPIOTE callback */
	ret = gpio_pin_configure_dt(&pin_in, GPIO_INPUT);
	if (ret) {
		LOG_ERR("gpio_pin_configure_dt() has failed (%d)", ret);
	}
	__ASSERT_NO_MSG(ret == 0);

	gpio_init_callback(&gpio_input_cb_data, gpio_input_edge_callback, BIT(pin_in.pin));

	k_timer_init(&my_timer, my_timer_handler, NULL);

	/* Synchronize Remote core with Host core */
#if !defined(CONFIG_TEST_ROLE_REMOTE)
	LOG_DBG("HOST starts");
	*shared_var = HOST_IS_READY;
	sys_cache_data_flush_range((void *) shared_var, sizeof(*shared_var));
	LOG_DBG("HOST wrote HOST_IS_READY: %u", *shared_var);
	while (*shared_var != REMOTE_IS_READY) {
		k_msleep(1);
		sys_cache_data_invd_range((void *) shared_var, sizeof(*shared_var));
		LOG_DBG("shared_var is: %u", *shared_var);
	}
	LOG_DBG("HOST continues");
#else
	LOG_DBG("REMOTE starts");
	while (*shared_var != HOST_IS_READY) {
		k_msleep(1);
		sys_cache_data_invd_range((void *) shared_var, sizeof(*shared_var));
		LOG_DBG("shared_var is: %u", *shared_var);
	}
	LOG_DBG("REMOTE found that HOST_IS_READY");
	*shared_var = REMOTE_IS_READY;
	sys_cache_data_flush_range((void *) shared_var, sizeof(*shared_var));
	LOG_DBG("REMOTE wrote REMOTE_IS_READY: %u", *shared_var);
	LOG_DBG("REMOTE continues");
#endif

#if defined(CONFIG_COVERAGE)
	printk("Coverage analysis enabled\n");
	while (test_repetitions--)
#else
	/* Run test forever. */
	while (test_repetitions)
#endif
	{
		timer_expired = false;

		/* clear edge counters */
		high = 0;
		low = 0;

		/* start a one-shot timer that expires after 1 second */
		k_timer_start(&my_timer, K_MSEC(1000), K_NO_WAIT);

		/* configure and enable GPIOTE */
		ret = gpio_pin_interrupt_configure_dt(&pin_in, GPIO_INT_EDGE_BOTH);
		if (ret) {
			LOG_ERR("gpio_pin_interrupt_configure_dt() has failed (%d)", ret);
		}
		__ASSERT_NO_MSG(ret == 0);

		ret = gpio_add_callback(pin_in.port, &gpio_input_cb_data);
		if (ret) {
			LOG_ERR("gpio_add_callback() has failed (%d)", ret);
		}
		__ASSERT_NO_MSG(ret == 0);

		/* configure and enable PWM */
#if defined(CONFIG_PM_DEVICE_RUNTIME)
		pm_device_runtime_get(pwm_out.dev);
#endif
		ret = pwm_set_dt(&pwm_out, pwm_out.period, pulse);
		if (ret) {
			LOG_ERR("pwm_set_dt(%u, %u) returned %d",
				pwm_out.period, pulse, ret);
		}
		__ASSERT_NO_MSG(ret == 0);

		/* Keep PWM active for ~ 1 second */
		while (!timer_expired) {
			/* GPIOTE shall count edges here */
			k_busy_wait(10000);
		}

		/* Disable PWM */
		ret = pwm_set_dt(&pwm_out, 0, 0);
		if (ret) {
			LOG_ERR("pwm_set_dt(%u, %u) returned %d",
				0, 0, ret);
			return ret;
		}
#if defined(CONFIG_PM_DEVICE_RUNTIME)
		pm_device_runtime_put(pwm_out.dev);
#endif

		/* Disable GPIOTE interrupt */
		ret = gpio_remove_callback(pin_in.port, &gpio_input_cb_data);
		if (ret) {
			LOG_ERR("gpio_remove_callback() has failed (%d)", ret);
		}
		__ASSERT_NO_MSG(ret == 0);

		ret = gpio_pin_interrupt_configure_dt(&pin_in, GPIO_INT_DISABLE |
				GPIO_INT_LEVELS_LOGICAL | GPIO_INT_HIGH_1);
		if (ret) {
			LOG_ERR("gpio_pin_interrupt_configure_dt() has failed (%d)", ret);
		}
		__ASSERT_NO_MSG(ret == 0);

		LOG_INF("Iteration %u: rising: %u, falling %u",
			counter, high, low);

		/* Check if PWM is working */
		__ASSERT_NO_MSG(high >= edges - tolerance);
		__ASSERT_NO_MSG(low >= edges - tolerance);

		/* Sleep / enter low power state */
		gpio_pin_set_dt(&led, 0);
		k_msleep(CONFIG_TEST_SLEEP_DURATION_MS);
		gpio_pin_set_dt(&led, 1);
		counter++;
	}

#if defined(CONFIG_COVERAGE)
	printk("Coverage analysis start\n");
#endif
	return 0;
}
