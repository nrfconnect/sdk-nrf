/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(idle_stm, LOG_LEVEL_INF);

#ifdef CONFIG_LOG_FRONTEND_STMESP
#include <zephyr/logging/log_frontend_stmesp.h>
#endif

#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/uart.h>

/* Variables used to make CPU active for ~1 second */
static struct k_timer my_timer;
static volatile bool timer_expired;

void my_timer_handler(struct k_timer *dummy)
{
	timer_expired = true;
}

int main(void)
{
	int counter = 0;

	LOG_INF("Multicore idle_stm test on %s", CONFIG_BOARD_TARGET);
	LOG_INF("Main sleeps for %d ms", CONFIG_TEST_SLEEP_DURATION_MS);

	k_timer_init(&my_timer, my_timer_handler, NULL);

	/* Run test forever */
	while (1) {
		timer_expired = false;

		/* start a one-shot timer that expires after 1 second */
		k_timer_start(&my_timer, K_MSEC(1000), K_NO_WAIT);

		/* Keep CPU active for ~ 1 second */
		while (!timer_expired) {
			k_busy_wait(10000);
			k_yield();
		}

		LOG_INF("Run %d", counter);
		counter++;

		/* Sleep / enter low power state */
		k_msleep(CONFIG_TEST_SLEEP_DURATION_MS);
	}

	return 0;
}
