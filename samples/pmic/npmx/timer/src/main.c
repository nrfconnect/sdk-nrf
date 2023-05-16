/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <npmx_core.h>
#include <npmx_driver.h>
#include <npmx_timer.h>

#define LOG_MODULE_NAME pmic_timer
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* Timeout in milliseconds within npmx device should call a timer interrupt. */
#define APP_TIMEOUT_MILLISECONDS 500

/* Structure used to submit @ref timeout_callback after an @ref APP_TIMEOUT_MILLISECONDS delay. */
static struct k_work_delayable timeout_timer;

/**
 * @brief Timeout callback is called if npmx device does not call timer interrupt.
 *
 * @param[in] work Structure used to submit work.
 */
static void timeout_callback(struct k_work *work)
{
	LOG_ERR("Timeout, interrupt not received within %d ms", APP_TIMEOUT_MILLISECONDS);
}

/* Variable filled in @ref main() when timer starts working. */
static volatile uint32_t start_time;

/**
 * @brief Callback function to be used when SHIPHOLD WATCHDOG event occurs.
 *        SHIPHOLD WATCHDOG is used by the timer in general-purpose mode.
 *
 * @param[in] p_pm Pointer to the instance of nPM device.
 * @param[in] type Callback type. Should be @ref NPMX_CALLBACK_TYPE_EVENT_SHIPHOLD.
 * @param[in] mask Received event interrupt mask.
 */
static void timer_callback(npmx_instance_t *p_pm, npmx_callback_type_t type, uint8_t mask)
{
	/* Cancel timeout. */
	k_work_cancel_delayable(&timeout_timer);

	/* Calculate elapsed time and print log info. */
	LOG_INF("Elapsed time: %d ms", k_uptime_get_32() - start_time);

	/* Call generic callback to print interrupt data. */
	p_pm->generic_cb(p_pm, type, mask);
}

void main(void)
{
	const struct device *pmic_dev = DEVICE_DT_GET(DT_NODELABEL(npm_0));

	if (!device_is_ready(pmic_dev)) {
		LOG_INF("PMIC device is not ready");
		return;
	}

	LOG_INF("PMIC device ok");

	/* Initialize a delayable work structure used for timeout. */
	k_work_init_delayable(&timeout_timer, timeout_callback);

	/* Get the pointer to npmx device. */
	npmx_instance_t *npmx_instance = npmx_driver_instance_get(pmic_dev);

	/* Get the pointer to TIMER instance. */
	npmx_timer_t *timer_instance = npmx_timer_get(npmx_instance, 0);

	/* Register SHIPHOLD callback. */
	npmx_core_register_cb(npmx_instance, timer_callback, NPMX_CALLBACK_TYPE_EVENT_SHIPHOLD);

	/* Timer general-purpose mode use SHIPHOLD WATCHDOG bit to indicate interrupt. */
	npmx_core_event_interrupt_enable(npmx_instance, NPMX_EVENT_GROUP_SHIPHOLD,
					 NPMX_EVENT_GROUP_SHIPHOLD_WATCHDOG_MASK);

	/* Fill timer's configuration structure to work with general-purpose mode. */
	npmx_timer_config_t timer_config = {
		.mode = NPMX_TIMER_MODE_GENERAL_PURPOSE, /* General purpose mode. */
		.prescaler = NPMX_TIMER_PRESCALER_FAST, /* Timer clock set to 512 Hz. */
		.compare_value = 200 /* Timer counts to compare_value ticks, */
				     /* expected total time is ((1/512) * compare_value). */
	};

	/* Set timer configuration. */
	npmx_timer_config_set(timer_instance, &timer_config);

	/* Save compare value data to internal timer's registers. */
	npmx_timer_task_trigger(timer_instance, NPMX_TIMER_TASK_STROBE);

	/* Enable timer to execute the interrupt. */
	npmx_timer_task_trigger(timer_instance, NPMX_TIMER_TASK_ENABLE);

	/* Save current time to calculate the time difference in the callback. */
	start_time = k_uptime_get_32();

	/* Start timeout timer. */
	k_work_reschedule(&timeout_timer, K_MSEC(APP_TIMEOUT_MILLISECONDS));

	while (1) {
		k_sleep(K_FOREVER);
	}
}
