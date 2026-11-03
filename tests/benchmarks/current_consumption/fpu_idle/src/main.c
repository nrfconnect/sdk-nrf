/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>
#include <math.h>

#define CALC_ROUNDS		 500
#define ACTIVE_AND_SLEEP_TIME_MS 1000
#define THREAD_STACK_SIZE	 4096
#define THREAD_PRIORITY		 0

LOG_MODULE_REGISTER(fpu_idle);

static K_SEM_DEFINE(floating_point_calc_done, 0, 1);
static K_SEM_DEFINE(thread_suspend_request, 0, 1);
static K_SEM_DEFINE(thread_resume_request, 0, 1);

static float floating_point_calculations(void)
{

	float result = 0.0f;

	for (int i = 0; i < CALC_ROUNDS; i++) {
		result += sqrtf((float)i * 1.2345f + 0.1927f) * 1.11056f;
		result /= 2.5678f;
		result *= sqrtf(2.0f);
	}

	k_yield();
	return result;
}

static void thread_worker(void *param1, void *param2, void *param3)
{
	float result;

	while (1) {
		while (k_sem_take(&thread_resume_request, K_FOREVER) != 0) {

		}
		while (k_sem_take(&thread_suspend_request, K_NO_WAIT) != 0) {
			result = floating_point_calculations();
			LOG_INF("[THREAD] result = %f", (double)result);
		}
	}
}

K_THREAD_DEFINE(thread_a, THREAD_STACK_SIZE, thread_worker, NULL, NULL, NULL, THREAD_PRIORITY, 0,
		0);

void timer_handler(struct k_timer *dummy)
{
	k_sem_give(&floating_point_calc_done);
}

K_TIMER_DEFINE(timer, timer_handler, NULL);

int main(void)
{

	float result;

	if (dk_leds_init() != 0) {
		LOG_ERR("DK leds init failed");

		return -1;
	}

	while (1) {
		k_timer_start(&timer, K_MSEC(ACTIVE_AND_SLEEP_TIME_MS), K_NO_WAIT);
		dk_set_led_on(DK_LED1);
		LOG_INF("FPU CALC");
		k_sem_give(&thread_resume_request);
		while (k_sem_take(&floating_point_calc_done, K_NO_WAIT) != 0) {

			result = floating_point_calculations();
			LOG_INF("[MAIN] result = %f", (double)result);
		}
		k_sem_give(&thread_suspend_request);
		dk_set_led_off(DK_LED1);
		LOG_INF("SLEEP");
		k_msleep(ACTIVE_AND_SLEEP_TIME_MS);
	}

	return 0;
}
