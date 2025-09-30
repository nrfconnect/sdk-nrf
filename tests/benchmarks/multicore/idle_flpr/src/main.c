/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(idle_flpr, LOG_LEVEL_INF);

#if !defined(CONFIG_SOC_NRF54H20_CPUFLPR)
/* Unable to use GPIO Zephyr driver on FLPR core due to lack of GPIOTE interrupt. */
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
#endif

/* Variables used to make CPU active for ~1 second */
static struct k_timer my_timer;
static bool timer_expired;

void my_timer_handler(struct k_timer *dummy)
{
	timer_expired = true;
}

int main(void)
{
	int counter = 0;

	LOG_INF("Multicore idle_flpr test on %s", CONFIG_BOARD_TARGET);
	LOG_INF("Main sleeps for %d ms", CONFIG_TEST_SLEEP_DURATION_MS);

#if !defined(CONFIG_SOC_NRF54H20_CPUFLPR)
	int ret;

	ret = gpio_is_ready_dt(&led);
	if (!ret) {
		LOG_ERR("LED is not ready");
	}
	__ASSERT(ret, "LED is not ready\n");

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Unable to configure GPIO as output");
	}
	__ASSERT(ret == 0, "Unable to configure GPIO as output\n");
#endif

	k_timer_init(&my_timer, my_timer_handler, NULL);

	/* Run test forever */
	while (1) {
		timer_expired = false;

#if !defined(CONFIG_SOC_NRF54H20_CPUFLPR)
		/* Turn ON LED */
		ret = gpio_pin_set_dt(&led, 1);
#endif

		/* start a one-shot timer that expires after 1 second */
		k_timer_start(&my_timer, K_MSEC(1000), K_NO_WAIT);

#if !defined(CONFIG_SOC_NRF54H20_CPUFLPR)
		if (ret < 0) {
			LOG_ERR("Unable to turn on LED");
		}
		__ASSERT(ret == 0, "Unable to turn on LED\n");
#endif

		/* Keep CPU active for ~ 1 second */
		while (!timer_expired) {
			k_busy_wait(10000);
			k_yield();
		}

		LOG_INF("Run %d", counter);
		counter++;

#if !defined(CONFIG_SOC_NRF54H20_CPUFLPR)
		/* Turn OFF LED */
		ret = gpio_pin_set_dt(&led, 0);
		if (ret < 0) {
			LOG_ERR("Unable to turn off LED");
		}
		__ASSERT(ret == 0, "Unable to turn off LED\n");
#endif

		/* Sleep / enter low power state */
		k_msleep(CONFIG_TEST_SLEEP_DURATION_MS);
	}

	return 0;
}
