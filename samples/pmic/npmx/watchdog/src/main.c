/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/console/console.h>
#include <zephyr/drivers/gpio.h>
#include <npmx_core.h>
#include <npmx_driver.h>
#include <npmx_gpio.h>
#include <npmx_timer.h>

#define LOG_MODULE_NAME pmic_watchdog
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/** @brief Variable filled in @ref kicking_thread() when watchdog kicking stops. */
static volatile uint32_t start_time;

/** @brief Variable filled in @ref timer_callback() when watchdog warning occurs. */
static volatile uint32_t warning_time;

/** @brief Semaphore used to inform @ref kicking_thread() when to stop kicking the watchdog. */
K_SEM_DEFINE(stop_kick_sem, 0, 1);

/** @brief Stack used by kick_thread. */
K_THREAD_STACK_DEFINE(kick_stack, 1000);

/** @brief Kicking thread definition. */
struct k_thread kick_thread;

#if CONFIG_TESTCASE_WATCHDOG_MODE_WARNING
/** @brief Timer watchdog working in warning mode. */
#define TIMER_WATCHDOG_MODE NPMX_TIMER_MODE_WATCHDOG_WARNING
#elif CONFIG_TESTCASE_WATCHDOG_MODE_RESET
/** @brief Timer watchdog working in reset mode. */
#define TIMER_WATCHDOG_MODE NPMX_TIMER_MODE_WATCHDOG_RESET
#endif

/**
 * @brief Kicking thread function.
 *        In this function, the watchdog will be kicked each 100 ms
 *        until the user presses the irq_res on the console.
 *
 * @param[in] p1 Pointer to the @ref npmx_timer_t instance.
 * @param     p2 Unused.
 * @param     p3 Unused.
 */
static void kicking_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Get timer instance from arguments list. */
	npmx_timer_t *timer_instance = (npmx_timer_t *)p1;

	/* If user pressed button on console, kicking stops. */
	while (k_sem_take(&stop_kick_sem, K_NO_WAIT) != 0) {
		k_msleep(100);
		/* Kick watchdog timer. */
		npmx_timer_task_trigger(timer_instance, NPMX_TIMER_TASK_KICK);
	}

	start_time = k_uptime_get_32();
	LOG_INF("Kicking stopped!");
	LOG_INF("Wait for watchdog warning interrupt.");
}

/**
 * @brief Callback function to be used when watchdog warning event occurs.
 *
 * @param[in] p_pm Pointer to the instance of nPM device.
 * @param[in] type Callback type. Should be @ref NPMX_CALLBACK_TYPE_EVENT_SHIPHOLD.
 * @param[in] mask Received event interrupt mask.
 */
static void timer_callback(npmx_instance_t *p_pm, npmx_callback_type_t type, uint8_t mask)
{
	warning_time = k_uptime_get_32();
	LOG_INF("Watchdog warning callback");

	/* Calculate elapsed time and print log info. */
	LOG_INF("Warning after %d ms", warning_time - start_time);

	/* Call generic callback to print interrupt data. */
	p_pm->generic_cb(p_pm, type, mask);
}

/**
 * @brief Reset callback function.
 *
 * @param[in] dev  Unused.
 * @param[in] cb   Unused.
 * @param[in] pins Unused.
 */
void reset_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	LOG_INF("Reset after %d ms\n", k_uptime_get_32() - warning_time);
}

/**
 * @brief Function for configuring GPIO interrupt to handle pulse falling edge
 *        and run @ref reset_callback().
 */
void configure_reset_interrupt(void)
{
	static const struct gpio_dt_spec irq_res =
		GPIO_DT_SPEC_GET_OR(DT_ALIAS(irqres0), gpios, { 0 });
	static struct gpio_callback irq_res_cb_data;
	int ret;

	if (!device_is_ready(irq_res.port)) {
		LOG_ERR("Error: irq_res device %s is not ready\n", irq_res.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&irq_res, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n", ret, irq_res.port->name,
			irq_res.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&irq_res, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n", ret,
			irq_res.port->name, irq_res.pin);
		return;
	}

	gpio_init_callback(&irq_res_cb_data, reset_callback, BIT(irq_res.pin));
	gpio_add_callback(irq_res.port, &irq_res_cb_data);
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
	npmx_instance_t *npmx_instance = npmx_driver_instance_get(pmic_dev);

	/* Configure host pin to handle falling edge on output reset pin. */
	configure_reset_interrupt();

	/* Get the pointer to TIMER instance. */
	npmx_timer_t *timer_instance = npmx_timer_get(npmx_instance, 0);

	/* Register SHIPHOLD callback. */
	npmx_core_register_cb(npmx_instance, timer_callback, NPMX_CALLBACK_TYPE_EVENT_SHIPHOLD);

	/* Enable watchdog interrupt. */
	npmx_core_event_interrupt_enable(npmx_instance, NPMX_EVENT_GROUP_SHIPHOLD,
					 NPMX_EVENT_GROUP_SHIPHOLD_WATCHDOG_MASK);

	/* Fill TIMER's configuration structure to work with watchdog mode. */
	npmx_timer_config_t timer_config = {
		.mode = TIMER_WATCHDOG_MODE, /* Watchdog mode. */
		.prescaler = NPMX_TIMER_PRESCALER_FAST, /* TIMER clock set to 512 Hz. */
		.compare_value = 244, /* TIMER counts to compare_value ticks, */
		/* expected total time is ((1/512) * compare_value). */
	};

	/* Set TIMER configuration. */
	npmx_timer_config_set(timer_instance, &timer_config);

	/* Save compare value data to internal TIMER's registers. */
	npmx_timer_task_trigger(timer_instance, NPMX_TIMER_TASK_STROBE);

	/* Enable TIMER to execute the interrupt. */
	npmx_timer_task_trigger(timer_instance, NPMX_TIMER_TASK_ENABLE);

	/* Create the new thread. */
	k_thread_create(&kick_thread, kick_stack, K_THREAD_STACK_SIZEOF(kick_stack), kicking_thread,
			timer_instance, NULL, NULL, 7, 0, K_NO_WAIT);

	/* Initialize console required to interact with the user.*/
	console_init();

	LOG_INF("Watchdog is being kicked, press any key to stop it.\n");

	while (1) {
		/* Wait for the user interaction. */
		uint8_t c = console_getchar();

		if (c) {
			/* Inform kicking thread to stop kicking. */
			k_sem_give(&stop_kick_sem);
			break;
		}
	}
}
