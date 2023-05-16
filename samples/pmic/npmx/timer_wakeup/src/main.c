/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <npmx_core.h>
#include <npmx_driver.h>
#include <npmx_led.h>
#include <npmx_ship.h>
#include <npmx_timer.h>

#define LOG_MODULE_NAME pmic_timer_wakeup
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/** @brief Time of hibernate mode (in milliseconds). */
#define HIBERNATE_TIME_MS 8000

/** @brief Scheduling priority used by thread responsible for turning on LED. */
#define PRIORITY 7

/** @brief Size of stack area used by thread responsible for turning on LED. */
#define STACKSIZE 1024

/** @brief Index of the LED used to indicate when the PMIC wakes-up. */
#define LED_IDX 0

/** @brief Task used by thread_led. */
K_THREAD_STACK_DEFINE(thread_led_stack_area, STACKSIZE);

/** @brief Declaration of thread used to turn on the LED. */
static struct k_thread thread_led_data;

/** @brief Declaration of the pointer to the SHIP driver instance. */
npmx_ship_t *m_ship_inst;

/** @brief Declaration of the pointer to the TIMER driver instance. */
npmx_timer_t *m_timer_inst;

/** @brief Variable filled in @ref button_callback() when the TIMER starts working. */
static volatile uint32_t start_time;

/**
 * @brief Function used to turn the LED on.
 *
 * @param[in] p_pm The pointer to the instance of nPM device.
 * @param     p2   Unused.
 * @param     p3   Unused.
 */
void thread_led(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	npmx_error_t err_mode = NPMX_ERROR_IO;
	npmx_error_t err_state = NPMX_ERROR_IO;
	npmx_instance_t *p_pm = (npmx_instance_t *)p1;

	npmx_led_mode_t led_mode;

	while (1) {
		err_mode = npmx_led_mode_get(npmx_led_get(p_pm, LED_IDX), &led_mode);

		/* Successful reading of the LED mode means that the PMIC has woken up and */
		/* it is possible to turn on the LED. */
		if (err_mode == NPMX_SUCCESS) {
			err_mode =
				npmx_led_mode_set(npmx_led_get(p_pm, LED_IDX), NPMX_LED_MODE_HOST);
			err_state = npmx_led_state_set(npmx_led_get(p_pm, LED_IDX), true);

			if ((err_mode == NPMX_SUCCESS) && (err_state == NPMX_SUCCESS)) {
				break;
			}
			LOG_INF("Attempt to turn on the LED was not successful");
		}
	}

	LOG_INF("LED turned on, elapsed time: %d ms", k_uptime_get_32() - start_time);
}

/**
 * @brief Callback function used when SHIPHOLD PRESSED event occurs.
 *
 * @param[in] p_pm Pointer to the instance of nPM device.
 * @param[in] type Callback type. Should be @ref NPMX_CALLBACK_TYPE_EVENT_SHIPHOLD.
 * @param[in] mask Received event interrupt mask.
 */
static void button_callback(npmx_instance_t *p_pm, npmx_callback_type_t type, uint8_t mask)
{
	/* Turn off the LED. */
	npmx_led_state_set(npmx_led_get(p_pm, LED_IDX), false);

	/* Create a new thread. */
	k_thread_create(&thread_led_data, thread_led_stack_area,
			K_THREAD_STACK_SIZEOF(thread_led_stack_area), thread_led, p_pm, NULL, NULL,
			PRIORITY, 0, K_MSEC(HIBERNATE_TIME_MS));

	/* Save current time to calculate the time spent in hibernate mode. */
	start_time = k_uptime_get_32();

	LOG_INF("Enter hibernate mode");

	/* Enter hibernate mode. */
	npmx_ship_task_trigger(m_ship_inst, NPMX_SHIP_TASK_HIBERNATE);
}

void main(void)
{
	const struct device *pmic_dev = DEVICE_DT_GET(DT_NODELABEL(npm_0));

	if (!device_is_ready(pmic_dev)) {
		LOG_INF("PMIC device is not ready");
		return;
	}

	LOG_INF("PMIC device ok");

	/* Get the pointer to npmx device. */
	npmx_instance_t *npmx_inst = npmx_driver_instance_get(pmic_dev);

	/* Turn off the LED. */
	npmx_led_state_set(npmx_led_get(npmx_inst, LED_IDX), false);

	/* Get the pointer to the SHIP instance */
	m_ship_inst = npmx_ship_get(npmx_inst, 0);

	/* Fill ship hold configuration structure. */
	npmx_ship_config_t ship_config = {
#if defined(SHPHLD_SHPHLDCONFIG_SHPHLDTIM_1000ms)
		.time = NPMX_SHIP_TIME_1000_MS, /* SHPHLD must be held low for 1000 ms to exit */
		/* from the ship or hibernate mode. */
#elif defined(SHPHLD_SHPHLDCONFIG_SHPHLDTIM_1008ms)
		.time = NPMX_SHIP_TIME_1008_MS, /* SHPHLD must be held low for 1008 ms to exit */
		/* from the ship or hibernate mode. */
#endif
#if defined(SHPHLD_SHPHLDCONFIG_SHPHLDDISPULLDOWN_Msk)
		.disable_active_pd = true, /* Active pull-downs on VSYS, VOUT(1/2) are disabled. */
#endif
		.inverted_polarity = false /* Button is active in the LOW state. */
	};

	/* Set ship hold configuration. */
	npmx_ship_config_set(m_ship_inst, &ship_config);

	/* Register @ref button_callback to NPMX_CALLBACK_TYPE_EVENT_SHIPHOLD event. */
	npmx_core_register_cb(npmx_inst, button_callback, NPMX_CALLBACK_TYPE_EVENT_SHIPHOLD);

	/* @ref button_callback is triggered when the SHPHLD button is pressed. */
	npmx_core_event_interrupt_enable(npmx_inst, NPMX_EVENT_GROUP_SHIPHOLD,
					 NPMX_EVENT_GROUP_SHIPHOLD_PRESSED_MASK);

	/* Get the pointer to the TIMER instance. */
	m_timer_inst = npmx_timer_get(npmx_inst, 0);

	/* Fill TIMER configuration structure to work with ship mode. */
	npmx_timer_config_t timer_config = {
		.mode = NPMX_TIMER_MODE_WAKEUP, /* Wake-up mode. */
		.prescaler = NPMX_TIMER_PRESCALER_FAST, /* Timer clock set to 512 Hz. */
		.compare_value =
			(HIBERNATE_TIME_MS / 1000) * 512 /* Timer counts to compare_value ticks, */
		/* expected total time is ((1/512) * compare_value). */
	};

	/* Set the TIMER configuration. */
	npmx_timer_config_set(m_timer_inst, &timer_config);

	/* Set LED to be controlled by the host and eventually turn it off. */
	npmx_led_mode_set(npmx_led_get(npmx_inst, LED_IDX), NPMX_LED_MODE_HOST);
	npmx_led_state_set(npmx_led_get(npmx_inst, LED_IDX), false);

	/* Save compare value data to internal timer's registers. */
	npmx_timer_task_trigger(m_timer_inst, NPMX_TIMER_TASK_STROBE);

#if defined(TIMER_WAKEUPACT_TASKWAKEUPACT_Msk)
	/* Activate TIMER wake-up mode. */
	npmx_timer_task_trigger(m_timer_inst, NPMX_TIMER_TASK_WAKEUP);
#endif

	while (1) {
		k_sleep(K_FOREVER);
	}
}
