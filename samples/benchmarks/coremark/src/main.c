/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/drivers/gpio.h>
#include <system_nrf.h>

#include "coremark_zephyr.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

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

static void flush_log(void)
{
	if (IS_ENABLED(CONFIG_LOG_PROCESS_THREAD)) {
		while (log_data_pending()) {
			k_sleep(K_MSEC(100));
		}
		k_sleep(K_MSEC(100));
	} else {
		while (LOG_PROCESS()) {
		}
	}
}

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

int main(void)
{
	LOG_INF("CoreMark sample for %s", CONFIG_BOARD_TARGET);
	flush_log();

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

		LOG_INF("Press %s to start the test ...",  BUTTON_LABEL);
	}

	while (true) {
		k_sem_take(&start_coremark, K_FOREVER);
		LOG_INF("CoreMark started! "
			"CPU FREQ: %d Hz, threads: %d, data size: %d; iterations: %d\n",
			SystemCoreClock,
			CONFIG_COREMARK_THREADS_NUMBER,
			CONFIG_COREMARK_DATA_SIZE,
			CONFIG_COREMARK_ITERATIONS);
		flush_log();

		LED_ON();
		coremark_run();
		LED_OFF();
		flush_log();

		if (!IS_ENABLED(CONFIG_APP_MODE_FLASH_AND_RUN)) {
			LOG_INF("CoreMark finished! Push %s to restart ...\n", BUTTON_LABEL);
		}

		(void)atomic_set(&coremark_in_progress, false);
	};

	return 0;
}
