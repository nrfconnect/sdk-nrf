/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/misc/coresight/nrf_etr.h>
#include <system_nrf.h>

#include "coremark_zephyr.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define COOP_PRIO -1

#ifndef CONFIG_APP_MODE_FLASH_AND_RUN
/*
 * Get button configuration from the devicetree. This is mandatory.
 */
#define BUTTON_NODE             DT_ALIAS(button)
#define LED_NODE                DT_ALIAS(led)

#if !DT_NODE_HAS_STATUS(BUTTON_NODE, okay)
	#error "Unsupported board: /"button/" alias is not defined."
#endif

#if !DT_NODE_HAS_STATUS(LED_NODE, okay)
	#error "Unsupported board: /"led/" alias is not defined."
#endif

#define BUTTON_LABEL            DT_PROP(DT_ALIAS(button), label)
#define LED_ON()                (void)gpio_pin_set_dt(&status_led, 1)
#define LED_OFF()               (void)gpio_pin_set_dt(&status_led, 0)

static const struct gpio_dt_spec start_button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static const struct gpio_dt_spec status_led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

#else

#define BUTTON_LABEL            "Reset"
#define LED_ON()
#define LED_OFF()

static const struct gpio_dt_spec start_button;
static const struct gpio_dt_spec status_led;

#endif

static K_SEM_DEFINE(start_coremark, 0, 1);
static atomic_t coremark_in_progress;

static void logging_mode_multi_domain_blocked_state_indicate(void)
{
	if (!IS_ENABLED(CONFIG_NRF_ETR)) {
		return;
	}

	LOG_INF("Logging is blocked for all cores until this core finishes the CoreMark "
		"benchmark\n");

	/* The wait time is necessary to include the log in the ETR flush operation. The logging
	 * frontend submits the log to the STM port (STMESP), which STM processes and directs the
	 * generated logging stream to the STM sink - ETR. The log should be available in the ETR
	 * peripheral after the sleep operation.
	 */
	k_sleep(K_MSEC(100));

	/* Flush logs from the ETR and direct them to UART. */
	nrf_etr_flush();
}

static int logging_mode_indicate(void)
{
	/* Enforce synchronous logging as the sample doesn't flush logs. */
	BUILD_ASSERT(IS_ENABLED(CONFIG_LOG_MODE_MINIMAL) ||
		     IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ||
		     IS_ENABLED(CONFIG_LOG_FRONTEND_ONLY),
		     "Logs should be processed synchronously to avoid negative impact on the "
		     "benchamrk performance");

	/* Standard logging mode */
	if (IS_ENABLED(CONFIG_LOG_MODE_MINIMAL) || IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		LOG_INF("Standard logging mode\n");

		return 0;
	}

	/* Multi-domain logging mode */
	if (IS_ENABLED(CONFIG_LOG_FRONTEND_ONLY) || IS_ENABLED(CONFIG_LOG_FRONTEND_STMESP)) {
		/* Only the core responsible for moving the logs from the STM sink to UART
		 * indicates the logging mode.
		 */
		if (IS_ENABLED(CONFIG_NRF_ETR)) {
			LOG_INF("Multi-domain logging mode");
			LOG_INF("This core is used to output logs from all cores to terminal "
				"over UART\n");
		}

		return 0;
	}

	/* This part should never be executed. */
	k_panic();

	return 0;
}

SYS_INIT(logging_mode_indicate, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (gpio_pin_get_dt(&start_button) && atomic_cas(&coremark_in_progress, false, true)) {
		LOG_INF("%s pressed!", BUTTON_LABEL);
		k_sem_give(&start_coremark);
	}
}

static int led_init(void)
{
	int ret;

	if (!device_is_ready(status_led.port)) {
		LOG_ERR("Error: led device %s is not ready",
			status_led.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&status_led, GPIO_OUTPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d",
			ret, status_led.port->name, status_led.pin);
		return ret;
	}

	LED_OFF();

	return ret;
}

static int button_init(void)
{
	int ret;
	static struct gpio_callback button_cb_data;

	if (!device_is_ready(start_button.port)) {
		LOG_ERR("Error: button1 device %s is not ready",
			start_button.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&start_button, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d",
			ret, start_button.port->name, start_button.pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&start_button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d",
			ret, start_button.port->name, start_button.pin);
		return ret;
	}

	(void) gpio_init_callback(&button_cb_data, button_pressed, BIT(start_button.pin));
	(void) gpio_add_callback(start_button.port, &button_cb_data);

	return ret;
}

static void main_thread_priority_cooperative_set(void)
{
	BUILD_ASSERT(CONFIG_MAIN_THREAD_PRIORITY >= 0);
	k_thread_priority_set(k_current_get(), COOP_PRIO);
}

int main(void)
{
	/* Drivers need to be run from a non-blocking thread.
	 * We need preemptive priority during init.
	 * Later we prefer cooperative priority to ensure no interference with the benchmark.
	 */
	main_thread_priority_cooperative_set();

	LOG_INF("CoreMark sample for %s", CONFIG_BOARD_TARGET);

	if (IS_ENABLED(CONFIG_APP_MODE_FLASH_AND_RUN)) {
		(void)atomic_set(&coremark_in_progress, true);
		k_sem_give(&start_coremark);
	} else if (!IS_ENABLED(CONFIG_APP_MODE_FLASH_AND_RUN)) {
		if (led_init()) {
			k_panic();
		}
		if (button_init()) {
			k_panic();
		}

		LOG_INF("Press %s to start the test ...\n",  BUTTON_LABEL);
	}

	while (true) {
		k_sem_take(&start_coremark, K_FOREVER);
		LOG_INF("CoreMark started! "
			"CPU FREQ: %d Hz, threads: %d, data size: %d; iterations: %d\n",
			SystemCoreClock,
			CONFIG_COREMARK_THREADS_NUMBER,
			CONFIG_COREMARK_DATA_SIZE,
			CONFIG_COREMARK_ITERATIONS);

		logging_mode_multi_domain_blocked_state_indicate();

		LED_ON();
		coremark_run();
		LED_OFF();

		if (IS_ENABLED(CONFIG_APP_MODE_FLASH_AND_RUN)) {
			LOG_INF("CoreMark finished! Press the reset button to restart...\n");
		} else {
			LOG_INF("CoreMark finished! Press %s to restart ...\n", BUTTON_LABEL);
		}

		(void)atomic_set(&coremark_in_progress, false);
	};

	return 0;
}
